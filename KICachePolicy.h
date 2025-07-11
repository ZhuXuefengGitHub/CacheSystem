#pragma once

namespace MyCache
{

template <typename Key, typename Value> // 模板类的声明开头
class MyCachePolicy
{
public:
    virtual ~MyCachePolicy() {};

    virtual void put(Key key, Value value) = 0; // 添加缓存接口，纯虚函数->必须由子类实现

    virtual bool get(Key key, Value& value) = 0; // 找到的话直接填充到value，避免拷贝大对象

    virtual Value get(Key key) = 0; // 缓存中找到key，直接返回对应的value
};

}