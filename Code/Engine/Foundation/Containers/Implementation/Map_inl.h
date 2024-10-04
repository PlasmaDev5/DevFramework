#pragma once

#include <Foundation/Math/Math.h>

// ***** Const Iterator *****

#define STACK_SIZE 64

template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
void plMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>::Advance(const plInt32 dir0, const plInt32 dir1)
{
  if (m_pElement == nullptr)
  {
    PL_ASSERT_DEBUG(m_pElement != nullptr, "The Iterator is invalid (end).");
    return;
  }

  // if this element has a right child, go there and then search for the left most child of that
  if (m_pElement->m_pLink[dir1] != m_pElement->m_pLink[dir1]->m_pLink[dir1])
  {
    m_pElement = m_pElement->m_pLink[dir1];

    while (m_pElement->m_pLink[dir0] != m_pElement->m_pLink[dir0]->m_pLink[dir0])
      m_pElement = m_pElement->m_pLink[dir0];

    return;
  }

  // if this element has a parent and this element is that parents left child, go directly to the parent
  if ((m_pElement->m_pParent != m_pElement->m_pParent->m_pParent) && (m_pElement->m_pParent->m_pLink[dir0] == m_pElement))
  {
    m_pElement = m_pElement->m_pParent;
    return;
  }

  // if this element has a parent and this element is that parents right child, search for the next parent, whose left child this is
  if ((m_pElement->m_pParent != m_pElement->m_pParent->m_pParent) && (m_pElement->m_pParent->m_pLink[dir1] == m_pElement))
  {
    while (m_pElement->m_pParent->m_pLink[dir1] == m_pElement)
      m_pElement = m_pElement->m_pParent;

    // if we are at the root node..
    if ((m_pElement->m_pParent == nullptr) || (m_pElement->m_pParent == m_pElement->m_pParent->m_pParent))
    {
      m_pElement = nullptr;
      return;
    }

    m_pElement = m_pElement->m_pParent;
    return;
  }

  m_pElement = nullptr;
  return;
}

template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
void plMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>::Next()
{
  if constexpr (REVERSE)
  {
    Advance(1, 0);
  }
  else
  {
    Advance(0, 1);
  }
}

template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
void plMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>::Prev()
{
  if constexpr (REVERSE)
  {
    Advance(0, 1);
  }
  else
  {
    Advance(1, 0);
  }
}

#if PL_ENABLED(PL_USE_CPP20_OPERATORS)

// These functions are used for structured bindings.
// They describe how many elements can be accessed in the binding and which type they are.
namespace std
{
  template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
  struct tuple_size<plMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>> : integral_constant<size_t, 2>
  {
  };

  template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
  struct tuple_element<0, plMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>>
  {
    using type = const KeyType&;
  };

  template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
  struct tuple_element<1, plMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>>
  {
    using type = const ValueType&;
  };


  template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
  struct tuple_size<plMapBaseIteratorBase<KeyType, ValueType, Comparer, REVERSE>> : integral_constant<size_t, 2>
  {
  };

  template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
  struct tuple_element<0, plMapBaseIteratorBase<KeyType, ValueType, Comparer, REVERSE>>
  {
    using type = const KeyType&;
  };

  template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
  struct tuple_element<1, plMapBaseIteratorBase<KeyType, ValueType, Comparer, REVERSE>>
  {
    using type = ValueType&;
  };
} // namespace std
#endif

// ***** plMapBase *****

template <typename KeyType, typename ValueType, typename Comparer>
void plMapBase<KeyType, ValueType, Comparer>::Constructor()
{
  m_uiCount = 0;

  m_NilNode.m_uiLevel = 0;
  m_NilNode.m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent = reinterpret_cast<Node*>(&m_NilNode);

  m_pFreeElementStack = nullptr;
  m_pRoot = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename KeyType, typename ValueType, typename Comparer>
plMapBase<KeyType, ValueType, Comparer>::plMapBase(const Comparer& comparer, plAllocator* pAllocator)
  : m_Elements(pAllocator)
  , m_Comparer(comparer)
{
  Constructor();
}

template <typename KeyType, typename ValueType, typename Comparer>
plMapBase<KeyType, ValueType, Comparer>::plMapBase(const plMapBase<KeyType, ValueType, Comparer>& cc, plAllocator* pAllocator)
  : m_Elements(pAllocator)
{
  Constructor();

  operator=(cc);
}

template <typename KeyType, typename ValueType, typename Comparer>
plMapBase<KeyType, ValueType, Comparer>::~plMapBase()
{
  Clear();
}

template <typename KeyType, typename ValueType, typename Comparer>
void plMapBase<KeyType, ValueType, Comparer>::operator=(const plMapBase<KeyType, ValueType, Comparer>& rhs)
{
  Clear();

  for (ConstIterator it = rhs.GetIterator(); it.IsValid(); ++it)
    Insert(it.Key(), it.Value());
}

template <typename KeyType, typename ValueType, typename Comparer>
void plMapBase<KeyType, ValueType, Comparer>::Clear()
{
  for (Iterator it = GetIterator(); it.IsValid(); ++it)
    plMemoryUtils::Destruct<Node>(it.m_pElement, 1);

  m_pFreeElementStack = nullptr;
  m_Elements.Clear();

  m_uiCount = 0;

  m_NilNode.m_uiLevel = 0;
  m_NilNode.m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent = reinterpret_cast<Node*>(&m_NilNode);

  m_pRoot = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename KeyType, typename ValueType, typename Comparer>
PL_ALWAYS_INLINE bool plMapBase<KeyType, ValueType, Comparer>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename KeyType, typename ValueType, typename Comparer>
PL_ALWAYS_INLINE plUInt32 plMapBase<KeyType, ValueType, Comparer>::GetCount() const
{
  return m_uiCount;
}


template <typename KeyType, typename ValueType, typename Comparer>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::Iterator plMapBase<KeyType, ValueType, Comparer>::GetIterator()
{
  return Iterator(GetLeftMost());
}

template <typename KeyType, typename ValueType, typename Comparer>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::ConstIterator plMapBase<KeyType, ValueType, Comparer>::GetIterator() const
{
  return ConstIterator(GetLeftMost());
}

template <typename KeyType, typename ValueType, typename Comparer>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::ReverseIterator plMapBase<KeyType, ValueType, Comparer>::GetReverseIterator()
{
  return ReverseIterator(GetRightMost());
}

template <typename KeyType, typename ValueType, typename Comparer>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::ConstReverseIterator plMapBase<KeyType, ValueType, Comparer>::GetReverseIterator() const
{
  return ConstReverseIterator(GetRightMost());
}

template <typename KeyType, typename ValueType, typename Comparer>
typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::GetLeftMost() const
{
  if (IsEmpty())
    return nullptr;

  Node* pNode = m_pRoot;

  while ((const void*)pNode->m_pLink[0] != (const void*)&m_NilNode)
    pNode = pNode->m_pLink[0];

  return pNode;
}

template <typename KeyType, typename ValueType, typename Comparer>
typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::GetRightMost() const
{
  if (IsEmpty())
    return nullptr;

  Node* pNode = m_pRoot;

  while ((const void*)pNode->m_pLink[1] != (const void*)&m_NilNode)
    pNode = pNode->m_pLink[1];

  return pNode;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::Internal_Find(const CompatibleKeyType& key) const
{
  Node* pNode = m_pRoot;

  while ((const void*)pNode != (const void*)&m_NilNode)
  {
    const plInt32 dir = (plInt32)m_Comparer.Less(pNode->m_Key, key);
    const plInt32 dir2 = (plInt32)m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
      break;

    pNode = pNode->m_pLink[dir];
  }

  if ((const void*)pNode == (const void*)&m_NilNode)
    return nullptr;

  return pNode;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE bool plMapBase<KeyType, ValueType, Comparer>::TryGetValue(const CompatibleKeyType& key, ValueType& out_value) const
{
  Node* pNode = Internal_Find<CompatibleKeyType>(key);
  if (pNode != nullptr)
  {
    out_value = pNode->m_Value;
    return true;
  }

  return false;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE bool plMapBase<KeyType, ValueType, Comparer>::TryGetValue(const CompatibleKeyType& key, const ValueType*& out_pValue) const
{
  Node* pNode = Internal_Find<CompatibleKeyType>(key);
  if (pNode != nullptr)
  {
    out_pValue = &pNode->m_Value;
    return true;
  }

  return false;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE bool plMapBase<KeyType, ValueType, Comparer>::TryGetValue(const CompatibleKeyType& key, ValueType*& out_pValue) const
{
  Node* pNode = Internal_Find<CompatibleKeyType>(key);
  if (pNode != nullptr)
  {
    out_pValue = &pNode->m_Value;
    return true;
  }

  return false;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE const ValueType* plMapBase<KeyType, ValueType, Comparer>::GetValue(const CompatibleKeyType& key) const
{
  Node* pNode = Internal_Find<CompatibleKeyType>(key);
  return pNode ? &pNode->m_Value : nullptr;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE ValueType* plMapBase<KeyType, ValueType, Comparer>::GetValue(const CompatibleKeyType& key)
{
  Node* pNode = Internal_Find<CompatibleKeyType>(key);
  return pNode ? &pNode->m_Value : nullptr;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE const ValueType& plMapBase<KeyType, ValueType, Comparer>::GetValueOrDefault(const CompatibleKeyType& key, const ValueType& defaultValue) const
{
  Node* pNode = Internal_Find<CompatibleKeyType>(key);
  return pNode ? pNode->m_Value : defaultValue;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::Iterator plMapBase<KeyType, ValueType, Comparer>::Find(const CompatibleKeyType& key)
{
  return Iterator(Internal_Find<CompatibleKeyType>(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::ConstIterator plMapBase<KeyType, ValueType, Comparer>::Find(const CompatibleKeyType& key) const
{
  return ConstIterator(Internal_Find<CompatibleKeyType>(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE bool plMapBase<KeyType, ValueType, Comparer>::Contains(const CompatibleKeyType& key) const
{
  return Internal_Find(key) != nullptr;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::Internal_LowerBound(const CompatibleKeyType& key) const
{
  Node* pNode = m_pRoot;
  Node* pNodeSmaller = nullptr;

  while ((const void*)pNode != (const void*)&m_NilNode)
  {
    const plInt32 dir = (plInt32)m_Comparer.Less(pNode->m_Key, key);
    const plInt32 dir2 = (plInt32)m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
      return pNode;

    if (dir == 0)
      pNodeSmaller = pNode;

    pNode = pNode->m_pLink[dir];
  }

  return pNodeSmaller;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::Iterator plMapBase<KeyType, ValueType, Comparer>::LowerBound(const CompatibleKeyType& key)
{
  return Iterator(Internal_LowerBound(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::ConstIterator plMapBase<KeyType, ValueType, Comparer>::LowerBound(const CompatibleKeyType& key) const
{
  return ConstIterator(Internal_LowerBound(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::Internal_UpperBound(const CompatibleKeyType& key) const
{
  Node* pNode = m_pRoot;
  Node* pNodeSmaller = nullptr;

  while ((const void*)pNode != (const void*)&m_NilNode)
  {
    const plInt32 dir = (plInt32)m_Comparer.Less(pNode->m_Key, key);
    const plInt32 dir2 = (plInt32)m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
    {
      ConstIterator it(pNode);
      ++it;
      return it.m_pElement;
    }

    if (dir == 0)
      pNodeSmaller = pNode;

    pNode = pNode->m_pLink[dir];
  }

  return pNodeSmaller;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::Iterator plMapBase<KeyType, ValueType, Comparer>::UpperBound(const CompatibleKeyType& key)
{
  return Iterator(Internal_UpperBound(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::ConstIterator plMapBase<KeyType, ValueType, Comparer>::UpperBound(const CompatibleKeyType& key) const
{
  return ConstIterator(Internal_UpperBound(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
ValueType& plMapBase<KeyType, ValueType, Comparer>::operator[](const CompatibleKeyType& key)
{
  return FindOrAdd(key).Value();
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
typename plMapBase<KeyType, ValueType, Comparer>::Iterator plMapBase<KeyType, ValueType, Comparer>::FindOrAdd(CompatibleKeyType&& key, bool* out_pExisted)
{
  Node* pNilNode = reinterpret_cast<Node*>(&m_NilNode);
  Node* pInsertedNode = nullptr;

  {
    Node* root = m_pRoot;

    if (m_pRoot != pNilNode)
    {
      Node* it = m_pRoot;
      Node* up[STACK_SIZE];

      plInt32 top = 0;
      plUInt32 dir = 0;

      while (true)
      {
        if (m_Comparer.Equal(it->m_Key, key))
        {
          if (out_pExisted)
            *out_pExisted = true;

          return Iterator(it);
        }

        dir = m_Comparer.Less(it->m_Key, key) ? 1 : 0;

        PL_ASSERT_DEBUG(top < STACK_SIZE, "plMapBase's internal stack is not large enough to be able to sort {0} elements.", GetCount());
        up[top++] = it;

        if (it->m_pLink[dir] == pNilNode)
          break;

        it = it->m_pLink[dir];
      }

      pInsertedNode = AcquireNode(std::forward<CompatibleKeyType>(key), ValueType(), 1, it);
      it->m_pLink[dir] = pInsertedNode;

      while (--top >= 0)
      {
        if (top != 0)
          dir = (up[top - 1]->m_pLink[1] == up[top]) ? 1 : 0;

        up[top] = SkewNode(up[top]);
        up[top] = SplitNode(up[top]);

        if (top != 0)
        {
          up[top - 1]->m_pLink[dir] = up[top];
          up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
        }
        else
          root = up[top];
      }
    }
    else
    {
      pInsertedNode = AcquireNode(std::forward<CompatibleKeyType>(key), ValueType(), 1, pNilNode);
      root = pInsertedNode;
    }

    m_pRoot = root;
    m_pRoot->m_pParent = pNilNode;
    m_NilNode.m_pParent = pNilNode;
  }

  PL_ASSERT_DEBUG(pInsertedNode != nullptr, "Implementation Error.");

  if (out_pExisted)
    *out_pExisted = false;

  return Iterator(pInsertedNode);
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType, typename CompatibleValueType>
typename plMapBase<KeyType, ValueType, Comparer>::Iterator plMapBase<KeyType, ValueType, Comparer>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value)
{
  auto it = FindOrAdd(std::forward<CompatibleKeyType>(key));
  it.Value() = std::forward<CompatibleValueType>(value);

  return it;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
bool plMapBase<KeyType, ValueType, Comparer>::Remove(const CompatibleKeyType& key)
{
  bool bRemoved = true;
  m_pRoot = Remove(m_pRoot, key, bRemoved);
  m_pRoot->m_pParent = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent = reinterpret_cast<Node*>(&m_NilNode);

  return bRemoved;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::AcquireNode(CompatibleKeyType&& key, ValueType&& value, plUInt8 uiLevel, Node* pParent)
{
  Node* pNode;

  if (m_pFreeElementStack == nullptr)
  {
    m_Elements.PushBack();
    pNode = &m_Elements.PeekBack();
  }
  else
  {
    pNode = m_pFreeElementStack;
    m_pFreeElementStack = m_pFreeElementStack->m_pParent;
  }

  plMemoryUtils::Construct<SkipTrivialTypes>(pNode, 1);

  pNode->m_pParent = pParent;
  pNode->m_Key = std::forward<CompatibleKeyType>(key);
  pNode->m_Value = std::move(value);
  pNode->m_uiLevel = uiLevel;
  pNode->m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  pNode->m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);

  ++m_uiCount;

  return pNode;
}

template <typename KeyType, typename ValueType, typename Comparer>
void plMapBase<KeyType, ValueType, Comparer>::ReleaseNode(Node* pNode)
{
  PL_ASSERT_DEBUG(pNode != nullptr && pNode != &m_NilNode, "pNode is invalid.");

  plMemoryUtils::Destruct<Node>(pNode, 1);

  // try to reduce the element array, if possible
  if (pNode == &m_Elements.PeekBack())
  {
    m_Elements.PopBack();
  }
  else if (pNode == &m_Elements.PeekFront())
  {
    m_Elements.PopFront();
  }
  else
  {
    pNode->m_pParent = m_pFreeElementStack;
    m_pFreeElementStack = pNode;
  }

  --m_uiCount;
}

template <typename KeyType, typename ValueType, typename Comparer>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::SkewNode(Node* root)
{
  if ((root->m_pLink[0]->m_uiLevel == root->m_uiLevel) && (root->m_uiLevel != 0))
  {
    Node* save = root->m_pLink[0];
    root->m_pLink[0] = save->m_pLink[1];
    root->m_pLink[0]->m_pParent = root;
    save->m_pLink[1] = root;
    save->m_pLink[1]->m_pParent = save;
    root = save;
  }

  return root;
}

template <typename KeyType, typename ValueType, typename Comparer>
PL_ALWAYS_INLINE typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::SplitNode(Node* root)
{
  if ((root->m_pLink[1]->m_pLink[1]->m_uiLevel == root->m_uiLevel) && (root->m_uiLevel != 0))
  {
    Node* save = root->m_pLink[1];
    root->m_pLink[1] = save->m_pLink[0];
    root->m_pLink[1]->m_pParent = root;
    save->m_pLink[0] = root;
    save->m_pLink[0]->m_pParent = save;
    root = save;
    ++root->m_uiLevel;
  }

  return root;
}

template <typename KeyType, typename ValueType, typename Comparer>
template <typename CompatibleKeyType>
typename plMapBase<KeyType, ValueType, Comparer>::Node* plMapBase<KeyType, ValueType, Comparer>::Remove(Node* root, const CompatibleKeyType& key, bool& bRemoved)
{
  bRemoved = false;

  Node* ToErase = reinterpret_cast<Node*>(&m_NilNode);
  Node* ToOverride = reinterpret_cast<Node*>(&m_NilNode);

  if (root != &m_NilNode)
  {
    Node* it = root;
    Node* up[STACK_SIZE];
    plInt32 top = 0;
    plInt32 dir = 0;

    while (true)
    {
      PL_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
      up[top++] = it;

      if (it == &m_NilNode)
        return root;

      if (m_Comparer.Equal(it->m_Key, key))
        break;

      dir = m_Comparer.Less(it->m_Key, key) ? 1 : 0;

      it = it->m_pLink[dir];
    }

    ToOverride = it;

    if ((it->m_pLink[0] == &m_NilNode) || (it->m_pLink[1] == &m_NilNode))
    {
      plInt32 dir2 = it->m_pLink[0] == &m_NilNode;

      if (--top != 0)
      {
        PL_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");
        up[top - 1]->m_pLink[dir] = it->m_pLink[dir2];
        up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
      }
      else
        root = it->m_pLink[1];
    }
    else
    {
      Node* heir = it->m_pLink[1];
      Node* prev = it;

      while (heir->m_pLink[0] != &m_NilNode)
      {
        PL_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
        up[top++] = prev = heir;

        heir = heir->m_pLink[0];
      }

      ToErase = heir;
      ToOverride = it;

      prev->m_pLink[prev == it] = heir->m_pLink[1];
      prev->m_pLink[prev == it]->m_pParent = prev;
    }

    while (--top >= 0)
    {
      if (top != 0)
      {
        PL_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");
        dir = up[top - 1]->m_pLink[1] == up[top];
      }

      PL_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");

      if ((up[top]->m_pLink[0]->m_uiLevel < up[top]->m_uiLevel - 1) || (up[top]->m_pLink[1]->m_uiLevel < up[top]->m_uiLevel - 1))
      {
        if (up[top]->m_pLink[1]->m_uiLevel > --up[top]->m_uiLevel)
          up[top]->m_pLink[1]->m_uiLevel = up[top]->m_uiLevel;

        up[top] = SkewNode(up[top]);
        up[top]->m_pLink[1] = SkewNode(up[top]->m_pLink[1]);
        up[top]->m_pLink[1]->m_pParent = up[top];

        up[top]->m_pLink[1]->m_pLink[1] = SkewNode(up[top]->m_pLink[1]->m_pLink[1]);
        up[top] = SplitNode(up[top]);
        up[top]->m_pLink[1] = SplitNode(up[top]->m_pLink[1]);
        up[top]->m_pLink[1]->m_pParent = up[top];
      }

      if (top != 0)
      {
        PL_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");

        up[top - 1]->m_pLink[dir] = up[top];
        up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
      }
      else
      {
        PL_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
        root = up[top];
      }
    }
  }

  root->m_pParent = reinterpret_cast<Node*>(&m_NilNode);


  // if necessary, swap nodes
  if (ToErase != &m_NilNode)
  {
    Node* parent = ToOverride->m_pParent;

    if (parent != &m_NilNode)
    {
      if (parent->m_pLink[0] == ToOverride)
      {
        parent->m_pLink[0] = ToErase;
        parent->m_pLink[0]->m_pParent = parent;
      }
      if (parent->m_pLink[1] == ToOverride)
      {
        parent->m_pLink[1] = ToErase;
        parent->m_pLink[1]->m_pParent = parent;
      }
    }
    else
      root = ToErase;

    ToErase->m_uiLevel = ToOverride->m_uiLevel;
    ToErase->m_pLink[0] = ToOverride->m_pLink[0];
    ToErase->m_pLink[0]->m_pParent = ToErase;
    ToErase->m_pLink[1] = ToOverride->m_pLink[1];
    ToErase->m_pLink[1]->m_pParent = ToErase;
  }

  // remove the erased node
  if (ToOverride != &m_NilNode)
  {
    bRemoved = true;
    ReleaseNode(ToOverride);
  }

  return root;
}

template <typename KeyType, typename ValueType, typename Comparer>
typename plMapBase<KeyType, ValueType, Comparer>::Iterator plMapBase<KeyType, ValueType, Comparer>::Remove(const Iterator& pos)
{
  PL_ASSERT_DEBUG(pos.m_pElement != nullptr, "The Iterator(pos) is invalid.");

  Iterator temp(pos);
  ++temp;
  Remove(pos.Key());
  return temp;
}

template <typename KeyType, typename ValueType, typename Comparer>
bool plMapBase<KeyType, ValueType, Comparer>::operator==(const plMapBase<KeyType, ValueType, Comparer>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  auto itLhs = GetIterator();
  auto itRhs = rhs.GetIterator();

  while (itLhs.IsValid())
  {
    if (!m_Comparer.Equal(itLhs.Key(), itRhs.Key()))
      return false;

    if (itLhs.Value() != itRhs.Value())
      return false;

    ++itLhs;
    ++itRhs;
  }

  return true;
}

#undef STACK_SIZE


template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
plMap<KeyType, ValueType, Comparer, AllocatorWrapper>::plMap()
  : plMapBase<KeyType, ValueType, Comparer>(Comparer(), AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
plMap<KeyType, ValueType, Comparer, AllocatorWrapper>::plMap(plAllocator* pAllocator)
  : plMapBase<KeyType, ValueType, Comparer>(Comparer(), pAllocator)
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
plMap<KeyType, ValueType, Comparer, AllocatorWrapper>::plMap(const Comparer& comparer, plAllocator* pAllocator)
  : plMapBase<KeyType, ValueType, Comparer>(comparer, pAllocator)
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
plMap<KeyType, ValueType, Comparer, AllocatorWrapper>::plMap(const plMap<KeyType, ValueType, Comparer, AllocatorWrapper>& other)
  : plMapBase<KeyType, ValueType, Comparer>(other, AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
plMap<KeyType, ValueType, Comparer, AllocatorWrapper>::plMap(const plMapBase<KeyType, ValueType, Comparer>& other)
  : plMapBase<KeyType, ValueType, Comparer>(other, AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
void plMap<KeyType, ValueType, Comparer, AllocatorWrapper>::operator=(const plMap<KeyType, ValueType, Comparer, AllocatorWrapper>& rhs)
{
  plMapBase<KeyType, ValueType, Comparer>::operator=(rhs);
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
void plMap<KeyType, ValueType, Comparer, AllocatorWrapper>::operator=(const plMapBase<KeyType, ValueType, Comparer>& rhs)
{
  plMapBase<KeyType, ValueType, Comparer>::operator=(rhs);
}

template <typename KeyType, typename ValueType, typename Comparer>
void plMapBase<KeyType, ValueType, Comparer>::Swap(plMapBase<KeyType, ValueType, Comparer>& other)
{
  SwapNilNode(this->m_pRoot, &this->m_NilNode, &other.m_NilNode);
  SwapNilNode(other.m_pRoot, &other.m_NilNode, &this->m_NilNode);

  plMath::Swap(this->m_pRoot, other.m_pRoot);
  plMath::Swap(this->m_uiCount, other.m_uiCount);
  plMath::Swap(this->m_pFreeElementStack, other.m_pFreeElementStack);
  plMath::Swap(this->m_Comparer, other.m_Comparer);

  // after we swapped the root nodes, fix up their parent nodes
  this->m_pRoot->m_pParent = reinterpret_cast<Node*>(&this->m_NilNode);
  other.m_pRoot->m_pParent = reinterpret_cast<Node*>(&other.m_NilNode);

  // the set allocator is stored in this array
  m_Elements.Swap(other.m_Elements);
}

template <typename KeyType, typename ValueType, typename Comparer>
void plMapBase<KeyType, ValueType, Comparer>::SwapNilNode(Node*& pCurNode, NilNode* pOld, NilNode* pNew)
{
  if (pCurNode == pOld)
  {
    pCurNode = reinterpret_cast<Node*>(pNew);
    return;
  }

  SwapNilNode(pCurNode->m_pLink[0], pOld, pNew);
  SwapNilNode(pCurNode->m_pLink[1], pOld, pNew);
}
