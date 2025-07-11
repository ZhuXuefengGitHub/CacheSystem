#pragma once

#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "KICachePolicy.h"

namespace MyCache
{

template<typename Key, typename Value> class KLruCache;

template<typename key, typename Value>
class LruNode
{
private:
    Key key_;
    Value value_;
    size_t accessCount_;
    std::weak_ptr<LruNode<Key, Value>> prev_; // 打破循环引用计数，shared_ptr计数+1,weak_ptr计数不加一
    std::shared_ptr<LruNode<Key, Value>> next_;

public:
    LruNode(Key key, Value value) : key_(key), value_(value), accessCount_(1){}

    // 提供一些必要访问器
    Key getKey() const { return key_; } // 函数不修改任何成员变量
    Value getValue() const { return value_; }
    void saveValue(const Value& value) { value_ = value; }
    size_t getAccessCount() { return accessCount_; }
    void incrementAccessCount() { ++accessCount_; };

    friend class KLruCache<Key, Value>; // 友元类，单向；除public外，KLruCache还可以访问LruNode类的private和protect成员
    // 因为要在这里声明友元，所以在这之前提前声明了类KLruCache
};

template<typename Key, typename Value>
class KLruCache : public KICachePolicy<Key, Value> 
{
public:
    using LruNodeType = LruNode<Key, Value>;
    using NodePtr = std::shared_ptr<LruNodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    KLruCache(int capacity) : capacity_(capacity)
    {
        initializeList();
    }

    // 以下重写父类函数
    ~KLruCache() override = default;

    void put(Key key, Value value) override
    {
        if (capacity_ <= 0)
            return;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            // 如果当前容器中存在，则更新value
            updateExistNode(it->second, value);
            return;
        }
        addNewNode(key, value);
    }

    bool get(Key key, Value& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_); // 锁，保证原子性
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            moveToMostRecent(it->second);
            value = it->second->getValue();
            return ture;
        }
        return false;
    }

    Value get(Key key) override
    {
        Value value{};
        get(key, value);
        return value;
    }

    void remove(Key key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            removeNode(it->second);
            nodeMap_.erase(it);
        }
    }

private:
    void initializeList()
    {
        // 创建虚拟头尾节点
        dummyHead_ = std::make_shared<LruNodeType>(Key(), Value()); // Key(), Value()初始化为默认值
        dummyTail_ = std::make_shared<LruNodeType>(Key(), Value());
        dummyHead_->next = dummyTail_;
        dummyTail_->prev = dummyHead_;
    }

    void updateExistNode(NodePtr node, canst Value& value)
    {
        node->setValue(value);
        moveToMostRecent(node); // 将该节点移到最新位置
    }

    void moveToMostRecent(NodePtr node)
    {
        removeNode(node);
        insertNode(node);
    }

    void removeNode(NodePtr node)
    {
        if (!node->prev_.expired() && node->next) // node->prev_.expired():前驱节点被释放；node->next:节点非空
        {
            auto prev = node->prev_.lock(); // lock的原子性
            // 如果原始对象仍然存在（未被销毁），返回有效的 shared_ptr
            // 如果原始对象已被销毁，返回空的 shared_ptr（相当于 nullptr）
            prev->next = node->next_;
            node->next->prev_ = prev;
            node->next_ = nullptr;
        }
    }

    void insertNode(NodePtr node)
    {
        // 从尾部插入节点
        node->next_ = dummyTail_;
        node->prev_ = dummyTail_->prev_;
        dummyTail_->prev_.lock()->next = node;
        dummyTail_->prev_ = node;
    }

    void addNewNode(const Key& key, const Value& value)
    {
        if (nodeMap_.size() >= capacity_)
        {
            evictLeastRecent();
        }

        NodePtr newNode = std::make_shared<LruNode>(key, value);
        insertNode(newNode);
        nodeMap_[key] = newNode;
    }

    void evictLeastRecent()
    {
        // 驱除最近最少使用
        NodePtr leastRecentNode = dummyHead_->next_;
        removeNode(leastRecentNode);
        nodeMap_.erase(leastRecentNode->getKey());
    }

private:
    int         capacity_;
    NodePtr     dummyHead_;
    NodePtr     dummyTail_;
    std::mutex  mutex_;
    NodeMap     nodeMap_;
};
    
}