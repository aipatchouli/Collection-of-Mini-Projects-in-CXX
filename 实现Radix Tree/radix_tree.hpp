#ifndef RADIX_TREE_HPP
#define RADIX_TREE_HPP

#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "radix_tree_iterator.hpp"
#include "radix_tree_node.hpp"

template <typename K>
K radix_substr(const K& key, int begin, int num);

template <>
inline std::string radix_substr<std::string>(const std::string& key, int begin, int num) {
    return key.substr(begin, num);
}

template <typename K>
K radix_join(const K& key1, const K& key2);

template <>
inline std::string radix_join<std::string>(const std::string& key1, const std::string& key2) {
    return key1 + key2;
}

template <typename K>
int radix_length(const K& key);

template <>
inline int radix_length<std::string>(const std::string& key) {
    return key.size();
}

template <typename K, typename T>
class RadixTree {
public:
    using key_type = K;
    using mapped_type = T;
    using value_type = std::pair<const K, T>;
    using iterator = RadixTreeIterator<K, T>;
    using size_type = std::size_t;

    RadixTree()
        : m_size(0), m_root(NULL) {
    }
    ~RadixTree() {
        delete m_root;
    }

    size_type size() const {
        return m_size;
    }
    bool empty() const {
        return m_size == 0;
    }
    void clear() {
        delete m_root;
        m_root = NULL;
        m_size = 0;
    }

    iterator find(const K& key);
    iterator begin();
    iterator end();

    std::pair<iterator, bool> insert(const value_type& val);
    bool erase(const K& key);
    void erase(iterator it);
    void prefix_match(const K& key, std::vector<iterator>& vec);
    void greedy_match(const K& key, std::vector<iterator>& vec);
    iterator longest_match(const K& key);

    T& operator[](const K& lhs);

private:
    size_type m_size;
    RadixTreeNode<K, T>* m_root;

    RadixTreeNode<K, T>* begin(RadixTreeNode<K, T>* node);
    RadixTreeNode<K, T>* find_node(const K& key, RadixTreeNode<K, T>* node, int depth);
    RadixTreeNode<K, T>* append(RadixTreeNode<K, T>* parent, const value_type& val);
    RadixTreeNode<K, T>* prepend(RadixTreeNode<K, T>* node, const value_type& val);
    void greedy_match(RadixTreeNode<K, T>* node, std::vector<iterator>& vec);

    RadixTree(const RadixTree& other);           // delete
    RadixTree& operator=(const RadixTree other); // delete
};

template <typename K, typename T>
void RadixTree<K, T>::prefix_match(const K& key, std::vector<iterator>& vec) {
    vec.clear();

    if (m_root == NULL)
        return;

    RadixTreeNode<K, T>* node;
    K key_sub1, key_sub2;

    node = find_node(key, m_root, 0);

    if (node->m_is_leaf)
        node = node->m_parent;

    int len = radix_length(key) - node->m_depth;
    key_sub1 = radix_substr(key, node->m_depth, len);
    key_sub2 = radix_substr(node->m_key, 0, len);

    if (key_sub1 != key_sub2)
        return;

    greedy_match(node, vec);
}

template <typename K, typename T>
typename RadixTree<K, T>::iterator RadixTree<K, T>::longest_match(const K& key) {
    if (m_root == NULL)
        return iterator(NULL);

    RadixTreeNode<K, T>* node;
    K key_sub;

    node = find_node(key, m_root, 0);

    if (node->m_is_leaf)
        return iterator(node);

    key_sub = radix_substr(key, node->m_depth, radix_length(node->m_key));

    if (!(key_sub == node->m_key))
        node = node->m_parent;

    K nul = radix_substr(key, 0, 0);

    while (node != NULL) {
        typename RadixTreeNode<K, T>::it_child it;
        it = node->m_children.find(nul);
        if (it != node->m_children.end() && it->second->m_is_leaf)
            return iterator(it->second);

        node = node->m_parent;
    }

    return iterator(NULL);
}

template <typename K, typename T>
typename RadixTree<K, T>::iterator RadixTree<K, T>::end() {
    return iterator(NULL);
}

template <typename K, typename T>
typename RadixTree<K, T>::iterator RadixTree<K, T>::begin() {
    RadixTreeNode<K, T>* node;

    if (m_root == NULL)
        node = NULL;
    else
        node = begin(m_root);

    return iterator(node);
}

template <typename K, typename T>
RadixTreeNode<K, T>* RadixTree<K, T>::begin(RadixTreeNode<K, T>* node) {
    if (node->m_is_leaf)
        return node;

    assert(!node->m_children.empty());

    return begin(node->m_children.begin()->second);
}

template <typename K, typename T>
T& RadixTree<K, T>::operator[](const K& lhs) {
    iterator it = find(lhs);

    if (it == end()) {
        std::pair<K, T> val;
        val.first = lhs;

        std::pair<iterator, bool> ret;
        ret = insert(val);

        assert(ret.second == true);

        it = ret.first;
    }

    return it->second;
}

template <typename K, typename T>
void RadixTree<K, T>::greedy_match(const K& key, std::vector<iterator>& vec) {
    RadixTreeNode<K, T>* node;

    vec.clear();

    if (m_root == NULL)
        return;

    node = find_node(key, m_root, 0);

    if (node->m_is_leaf)
        node = node->m_parent;

    greedy_match(node, vec);
}

template <typename K, typename T>
void RadixTree<K, T>::greedy_match(RadixTreeNode<K, T>* node, std::vector<iterator>& vec) {
    if (node->m_is_leaf) {
        vec.push_back(iterator(node));
        return;
    }

    typename std::map<K, RadixTreeNode<K, T>*>::iterator it;

    for (it = node->m_children.begin(); it != node->m_children.end(); ++it) {
        greedy_match(it->second, vec);
    }
}

template <typename K, typename T>
void RadixTree<K, T>::erase(iterator it) {
    erase(it->first);
}

template <typename K, typename T>
bool RadixTree<K, T>::erase(const K& key) {
    if (m_root == NULL)
        return 0;

    RadixTreeNode<K, T>* child;
    RadixTreeNode<K, T>* parent;
    RadixTreeNode<K, T>* grandparent;
    K nul = radix_substr(key, 0, 0);

    child = find_node(key, m_root, 0);

    if (!child->m_is_leaf)
        return 0;

    parent = child->m_parent;
    parent->m_children.erase(nul);

    delete child;

    m_size--;

    if (parent == m_root)
        return 1;

    if (parent->m_children.size() > 1)
        return 1;

    if (parent->m_children.empty()) {
        grandparent = parent->m_parent;
        grandparent->m_children.erase(parent->m_key);
        delete parent;
    } else {
        grandparent = parent;
    }

    if (grandparent == m_root) {
        return 1;
    }

    if (grandparent->m_children.size() == 1) {
        // merge grandparent with the uncle
        typename std::map<K, RadixTreeNode<K, T>*>::iterator it;
        it = grandparent->m_children.begin();

        RadixTreeNode<K, T>* uncle = it->second;

        if (uncle->m_is_leaf)
            return 1;

        uncle->m_depth = grandparent->m_depth;
        uncle->m_key = radix_join(grandparent->m_key, uncle->m_key);
        uncle->m_parent = grandparent->m_parent;

        grandparent->m_children.erase(it);

        grandparent->m_parent->m_children.erase(grandparent->m_key);
        grandparent->m_parent->m_children[uncle->m_key] = uncle;

        delete grandparent;
    }

    return 1;
}

template <typename K, typename T>
RadixTreeNode<K, T>* RadixTree<K, T>::append(RadixTreeNode<K, T>* parent, const value_type& val) {
    int depth;
    int len;
    K nul = radix_substr(val.first, 0, 0);
    RadixTreeNode<K, T>*node_c, *node_cc;

    depth = parent->m_depth + radix_length(parent->m_key);
    len = radix_length(val.first) - depth;

    if (len == 0) {
        node_c = new RadixTreeNode<K, T>(val);

        node_c->m_depth = depth;
        node_c->m_parent = parent;
        node_c->m_key = nul;
        node_c->m_is_leaf = true;

        parent->m_children[nul] = node_c;

        return node_c;
    } else {
        node_c = new RadixTreeNode<K, T>(val);

        K key_sub = radix_substr(val.first, depth, len);

        parent->m_children[key_sub] = node_c;

        node_c->m_depth = depth;
        node_c->m_parent = parent;
        node_c->m_key = key_sub;

        node_cc = new RadixTreeNode<K, T>(val);
        node_c->m_children[nul] = node_cc;

        node_cc->m_depth = depth + len;
        node_cc->m_parent = node_c;
        node_cc->m_key = nul;
        node_cc->m_is_leaf = true;

        return node_cc;
    }
}

template <typename K, typename T>
RadixTreeNode<K, T>* RadixTree<K, T>::prepend(RadixTreeNode<K, T>* node, const value_type& val) {
    int count;
    int len1, len2;

    len1 = radix_length(node->m_key);
    len2 = radix_length(val.first) - node->m_depth;

    for (count = 0; count < len1 && count < len2; count++) {
        if (!(node->m_key[count] == val.first[count + node->m_depth]))
            break;
    }

    assert(count != 0);

    node->m_parent->m_children.erase(node->m_key);

    RadixTreeNode<K, T>* node_a = new RadixTreeNode<K, T>;

    node_a->m_parent = node->m_parent;
    node_a->m_key = radix_substr(node->m_key, 0, count);
    node_a->m_depth = node->m_depth;
    node_a->m_parent->m_children[node_a->m_key] = node_a;

    node->m_depth += count;
    node->m_parent = node_a;
    node->m_key = radix_substr(node->m_key, count, len1 - count);
    node->m_parent->m_children[node->m_key] = node;

    K nul = radix_substr(val.first, 0, 0);
    if (count == len2) {
        RadixTreeNode<K, T>* node_b;

        node_b = new RadixTreeNode<K, T>(val);

        node_b->m_parent = node_a;
        node_b->m_key = nul;
        node_b->m_depth = node_a->m_depth + count;
        node_b->m_is_leaf = true;
        node_b->m_parent->m_children[nul] = node_b;

        return node_b;
    } else {
        RadixTreeNode<K, T>*node_b, *node_c;

        node_b = new RadixTreeNode<K, T>;

        node_b->m_parent = node_a;
        node_b->m_depth = node->m_depth;
        node_b->m_key = radix_substr(val.first, node_b->m_depth, len2 - count);
        node_b->m_parent->m_children[node_b->m_key] = node_b;

        node_c = new RadixTreeNode<K, T>(val);

        node_c->m_parent = node_b;
        node_c->m_depth = radix_length(val.first);
        node_c->m_key = nul;
        node_c->m_is_leaf = true;
        node_c->m_parent->m_children[nul] = node_c;

        return node_c;
    }
}

template <typename K, typename T>
std::pair<typename RadixTree<K, T>::iterator, bool> RadixTree<K, T>::insert(const value_type& val) {
    if (m_root == NULL) {
        K nul = radix_substr(val.first, 0, 0);

        m_root = new RadixTreeNode<K, T>;
        m_root->m_key = nul;
    }

    RadixTreeNode<K, T>* node = find_node(val.first, m_root, 0);

    if (node->m_is_leaf) {
        return std::pair<iterator, bool>(node, false);
    } else if (node == m_root) {
        m_size++;
        return std::pair<iterator, bool>(append(m_root, val), true);
    } else {
        m_size++;
        int len = radix_length(node->m_key);
        K key_sub = radix_substr(val.first, node->m_depth, len);

        if (key_sub == node->m_key) {
            return std::pair<iterator, bool>(append(node, val), true);
        } else {
            return std::pair<iterator, bool>(prepend(node, val), true);
        }
    }
}

template <typename K, typename T>
typename RadixTree<K, T>::iterator RadixTree<K, T>::find(const K& key) {
    if (m_root == NULL)
        return iterator(NULL);

    RadixTreeNode<K, T>* node = find_node(key, m_root, 0);

    // if the node is a internal node, return NULL
    if (!node->m_is_leaf)
        return iterator(NULL);

    return iterator(node);
}

template <typename K, typename T>
RadixTreeNode<K, T>* RadixTree<K, T>::find_node(const K& key, RadixTreeNode<K, T>* node, int depth) {
    if (node->m_children.empty())
        return node;

    typename RadixTreeNode<K, T>::it_child it;
    int len_key = radix_length(key) - depth;

    for (it = node->m_children.begin(); it != node->m_children.end(); ++it) {
        if (len_key == 0) {
            if (it->second->m_is_leaf)
                return it->second; // 查找叶子节点
            else
                continue;
        }

        if (!it->second->m_is_leaf && key[depth] == it->first[0]) {
            int len_node = radix_length(it->first);
            K key_sub = radix_substr(key, depth, len_node);

            if (key_sub == it->first) {
                return find_node(key, it->second, depth + len_node);
            } else {
                return it->second;
            }
        }
    }

    return node;
}

#endif // RADIX_TREE_HPP
