#ifndef MEMORY_POOL_HPP
#define MEMORY_POOL_HPP

#include <climits>
#include <cstddef>
#include <utility>

template <typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    template <typename U>
    struct rebind {
        using other = MemoryPool<U>;
    };

    MemoryPool() noexcept = default;
    ~MemoryPool() noexcept;

    // 同一时间只能分配一个对象, n 和 hint 会被忽略
    T* allocate(size_t n = 1, const T* hint = 0);

    // 调用构造函数
    template <typename U, typename... Args>
    void construct(U* p, Args&&... args);

    // 销毁指针 p 指向的内存区块
    void deallocate(T* p, size_t n = 1);

    // 销毁内存池中的对象, 即调用对象的析构函数
    template <typename U>
    void destroy(U* p) {
        p->~U();
    }

private:
    // 用于存储内存池中的对象槽,
    // 要么被实例化为一个存放对象的槽,
    // 要么被实例化为一个指向存放对象槽的槽指针
    union Slot_ {
        T element;
        Slot_* next;
    };

    // 数据指针
    using data_pointer_ = char*;
    // 对象槽
    using slot_type_ = Slot_;
    // 对象槽指针
    using slot_pointer_ = Slot_*;

    // 指向当前内存区块
    slot_pointer_ currentBlock_{nullptr};
    // 指向当前内存区块的一个对象槽
    slot_pointer_ currentSlot_{nullptr};
    // 指向当前内存区块的最后一个对象槽
    slot_pointer_ lastSlot_{nullptr};
    // 指向当前内存区块中的空闲对象槽
    slot_pointer_ freeSlots_{nullptr};

    // 检查定义的内存池大小是否过小
    static_assert(BlockSize >= 2 * sizeof(slot_type_), "BlockSize is too small.");
};

template <typename T, size_t BlockSize>
T* MemoryPool<T, BlockSize>::allocate(size_t /*n*/, const T* /*hint*/) {
    if (freeSlots_ != nullptr) {
        // 从空闲对象槽中分配一个对象
        T* result = (T*)freeSlots_;
        freeSlots_ = freeSlots_->next;
        return result;
    } 
    // 当前内存区块中的对象槽已经用完, 需要分配新的内存区块
    if (currentSlot_ >= lastSlot_) {
        // 分配内存区块
        data_pointer_ newBlock = (data_pointer_)(operator new(BlockSize));
        // 将内存区块的头部作为一个指针, 指向下一个内存区块
        reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
        currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
        // 计算出内存区块中的对象槽的个数
        data_pointer_ body = newBlock + sizeof(slot_pointer_);
        size_t bodySize = BlockSize - sizeof(slot_pointer_);
        size_t slotCount = bodySize / sizeof(slot_type_);
        // 计算出内存区块中的最后一个对象槽
        lastSlot_ = reinterpret_cast<slot_pointer_>(body + slotCount * sizeof(slot_type_));
        // 计算出内存区块中的第一个对象槽
        currentSlot_ = reinterpret_cast<slot_pointer_>(body);
    }
    // 从当前内存区块中分配一个对象
    return reinterpret_cast<T*>(currentSlot_++);
}

// construct()函数的实现
template <typename T, size_t BlockSize>
template <typename U, typename... Args>
void MemoryPool<T, BlockSize>::construct(U* p, Args&&... args) {
    new (p) U(std::forward<Args>(args)...);
}

// deallocate()函数的实现
template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::deallocate(T* p, size_t /*n*/) {
    if (p != nullptr) {
        // 将对象槽插入到空闲对象槽链表中
        reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
        freeSlots_ = (slot_pointer_)(p);
    }
}

// 析构函数的实现
template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool() noexcept {
    slot_pointer_ curr = currentBlock_;
    while (curr != nullptr) {
        slot_pointer_ prev = curr->next;
        ::operator delete(curr);
        curr = prev;
    }
}

#endif // MEMORY_POOL_HPP