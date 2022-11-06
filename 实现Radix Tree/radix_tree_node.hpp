#ifndef RadixTreeNode_HPP
#define RadixTreeNode_HPP

#include <map>

template <typename K, typename T>
class RadixTreeNode {
  friend class RadixTree<K, T>;
  friend class RadixTreeIterator<K, T>;

  typedef std::pair<const K, T> value_type;
  typedef typename std::map<K, RadixTreeNode<K, T> *>::iterator it_child;

 private:
  RadixTreeNode()
      : m_children(),
        m_parent(nullptr),
        m_value(nullptr),
        m_depth(0),
        m_is_leaf(false),
        m_key() {}
  RadixTreeNode(const value_type &val);
  RadixTreeNode(const RadixTreeNode &);             // delete
  RadixTreeNode &operator=(const RadixTreeNode &);  // delete

  ~RadixTreeNode();

  std::map<K, RadixTreeNode<K, T> *> m_children;
  RadixTreeNode<K, T> *m_parent;
  value_type *m_value;
  int m_depth;
  bool m_is_leaf;
  K m_key;
};

template <typename K, typename T>
RadixTreeNode<K, T>::RadixTreeNode(const value_type &val) :
    m_children(),
    m_parent(nullptr),
    m_value(nullptr),
    m_depth(0),
    m_is_leaf(false),
    m_key()
{
    m_value = new value_type(val);
}

template <typename K, typename T>
RadixTreeNode<K, T>::~RadixTreeNode()
{
    it_child it;
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        delete it->second;
    }
    delete m_value;
}
      

#endif // RadixTreeNode_HPP
