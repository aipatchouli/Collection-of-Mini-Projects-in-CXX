#ifndef radix_tree_iterator_hpp
#define radix_tree_iterator_hpp

#include <iostream>

// forward declaration
template <typename K, typename T>
class RadixTree;
template <typename K, typename T>
class RadixTreeNode;

template <typename K, typename T>
class RadixTreeIterator {
    friend class RadixTree<K, T>;

public:
    RadixTreeIterator()
        : m_pointee(0) {
    }
    RadixTreeIterator(const RadixTreeIterator& r)
        : m_pointee(r.m_pointee) {
    }
    RadixTreeIterator& operator=(const RadixTreeIterator& r) {
        m_pointee = r.m_pointee;
        return *this;
    }
    ~RadixTreeIterator() {
    }

    std::pair<const K, T>& operator*() const;
    std::pair<const K, T>* operator->() const;
    const RadixTreeIterator<K, T>& operator++();
    RadixTreeIterator<K, T> operator++(int);
    // const RadixTreeIterator<K, T>& operator-- ();
    bool operator!=(const RadixTreeIterator<K, T>& lhs) const;
    bool operator==(const RadixTreeIterator<K, T>& lhs) const;

private:
    RadixTreeNode<K, T>* m_pointee;
    RadixTreeIterator(RadixTreeNode<K, T>* p)
        : m_pointee(p) {
    }

    RadixTreeNode<K, T>* increment(RadixTreeNode<K, T>* node) const;
    RadixTreeNode<K, T>* descend(RadixTreeNode<K, T>* node) const;
};

template <typename K, typename T>
RadixTreeNode<K, T>* RadixTreeIterator<K, T>::increment(RadixTreeNode<K, T>* node) const {
    RadixTreeNode<K, T>* parent = node->m_parent;

    if (parent == NULL)
        return NULL;

    typename RadixTreeNode<K, T>::it_child it = parent->m_children.find(node->m_key);
    assert(it != parent->m_children.end());
    ++it;

    if (it == parent->m_children.end())
        return increment(parent);
    else
        return descend(it->second);
}

template <typename K, typename T>
RadixTreeNode<K, T>* RadixTreeIterator<K, T>::descend(RadixTreeNode<K, T>* node) const {
    if (node->m_is_leaf)
        return node;

    typename RadixTreeNode<K, T>::it_child it = node->m_children.begin();

    assert(it != node->m_children.end());

    return descend(it->second);
}

template <typename K, typename T>
std::pair<const K, T>& RadixTreeIterator<K, T>::operator*() const {
    return *m_pointee->m_value;
}

template <typename K, typename T>
std::pair<const K, T>* RadixTreeIterator<K, T>::operator->() const {
    return m_pointee->m_value;
}

template <typename K, typename T>
bool RadixTreeIterator<K, T>::operator!=(const RadixTreeIterator<K, T>& lhs) const {
    return m_pointee != lhs.m_pointee;
}

template <typename K, typename T>
bool RadixTreeIterator<K, T>::operator==(const RadixTreeIterator<K, T>& lhs) const {
    return m_pointee == lhs.m_pointee;
}

template <typename K, typename T>
const RadixTreeIterator<K, T>& RadixTreeIterator<K, T>::operator++() {
    if (m_pointee != NULL) // it is undefined behaviour to dereference iterator that is out of bounds...
        m_pointee = increment(m_pointee);
    return *this;
}

template <typename K, typename T>
RadixTreeIterator<K, T> RadixTreeIterator<K, T>::operator++(int) {
    RadixTreeIterator<K, T> copy(*this);
    ++(*this);
    return copy;
}

/*
template <typename K, typename T>
const RadixTreeIterator<K, T>& RadixTreeIterator<K, T>::operator-- ()
{
    assert(m_pointee != NULL);

    return *this;
}
*/

#endif // RadixTreeIterator
