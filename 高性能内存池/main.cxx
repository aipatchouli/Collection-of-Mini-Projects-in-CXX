// 测试性能
#include <cassert>
#include <cstdint>
#include <ctime> // clock()
#include <iostream>
#include <vector>

#include "MemoryPool.hpp" // MemoryPool<T>
#include "StackAlloc.hpp" // StackAlloc<T, Alloc>

// 插入元素个数
#define ELEMS 100000000
// 重复次数
#define REPS 1000

int main() {
    clock_t start = 0;
    // 使用 STL 默认分配器
    StackAlloc<int, std::allocator<int> > stackDefault;
    start = clock();
    for (int j = 0; j < REPS; j++) {
        assert(stackDefault.empty());
        for (int i = 0; i < ELEMS; i++) {
            stackDefault.push(i);
        }
        for (int i = 0; i < ELEMS; i++) {
            stackDefault.pop();
        }
    }
    std::cout << "Default Allocator Time: ";
    std::cout << ((static_cast<int64_t>(clock()) - start) / CLOCKS_PER_SEC) << 's' << "\n";

    // 使用内存池
    StackAlloc<int, MemoryPool<int> > stackPool;
    start = clock();
    for (int j = 0; j < REPS; j++) {
        assert(stackPool.empty());
        for (int i = 0; i < ELEMS; i++) {
            stackPool.push(i);
        }
        for (int i = 0; i < ELEMS; i++) {
            stackPool.pop();
        }
    }
    std::cout << "MemoryPool Allocator Time: ";
    std::cout << ((static_cast<int64_t>(clock()) - start) / CLOCKS_PER_SEC) << 's' << "\n";

    // 比较内存池和 std::vector 之间的性能
    std::vector<int> stackVector;
    start = clock();
    for (int j = 0; j < REPS; j++) {
        assert(stackVector.empty());
        for (int i = 0; i < ELEMS; i++) {
            stackVector.push_back(i);
        }
        for (int i = 0; i < ELEMS; i++) {
            stackVector.pop_back();
        }
    }
    std::cout << "Vector Time: ";
    std::cout << ((static_cast<int64_t>(clock()) - start) / CLOCKS_PER_SEC) << 's' << "\n\n";

    return 0;
}