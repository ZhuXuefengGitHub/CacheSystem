#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>

#include "KICachePolicy.h"
#include "KLfuCache.h"
#include "KLruCache.h"

void testHotDataAccess() {
    std::cout << "\n===测试场景1：热点数据测试===" << std::endl;

    const int CAPACITY = 20;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    //KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    //KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
}

int main() {
    testHotDataAccess();
    return 0;
}