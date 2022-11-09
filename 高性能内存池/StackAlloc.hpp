#include <memory>
#include <stdexcept>

template <typename T>
struct StackNode_ {
    T data;
    StackNode_* prev;
};

// T 为存储的对象类型, Alloc 为使用的分配器, 并默认使用 std::allocator 作为对象的分配器
template <typename T, typename Alloc = std::allocator<T>>
class StackAlloc {
public:
    // 使用 typedef 简化类型名
    using Node = StackNode_<T>;
    using allocator = typename Alloc::template rebind<Node>::other;

    // 默认构造
    StackAlloc() = default;
    // 析构
    ~StackAlloc() {
        clear();
    }

    // 当栈中元素为空时返回 true
    bool empty() {
        return head_ == nullptr;
    }

    // 释放栈中元素的所有内存
    void clear();

    // 压栈
    void push(T element);

    // 出栈
    T pop();

    // 返回栈顶元素
    T top() {
        return (head_->data);
    }

private:
    //
    allocator allocator_;
    // 栈顶
    Node* head_ = nullptr;
};

// 释放栈中元素的所有内存
template <typename T, typename Alloc>
void StackAlloc<T, Alloc>::clear() {
    while (head_ != nullptr) {
        Node* temp = head_;
        head_ = head_->prev;
        allocator_.destroy(temp);
        allocator_.deallocate(temp, 1);
    }
}

// 入栈
template <typename T, typename Alloc>
void StackAlloc<T, Alloc>::push(T element) {
    Node* temp = allocator_.allocate(1);
    allocator_.construct(temp, Node{element, head_});
    head_ = temp;
}

// 出栈
template <typename T, typename Alloc>
T StackAlloc<T, Alloc>::pop() {
    if (head_ == nullptr) {
        throw std::runtime_error("Stack is empty!");
    }
    Node* temp = head_;
    T element = head_->data;
    head_ = head_->prev;
    allocator_.destroy(temp);
    allocator_.deallocate(temp, 1);
    return element;
}