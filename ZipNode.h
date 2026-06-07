#ifndef ZIP_NODE_H
#define ZIP_NODE_H

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "LazyNode.h"
#include "OrdinalCache.h"
#include "SharedPtr.h"

template <class TLeft, class TRight>
class ZipNode : public LazyNode<std::pair<TLeft, TRight>>
{
public:
    typedef std::pair<TLeft, TRight> Result;

    ZipNode(
        const SharedPtr<LazyNode<TLeft>>& left_node,
        const SharedPtr<LazyNode<TRight>>& right_node)
        : left(left_node),
          right(right_node),
          length(Ordinal::Zero())
    {
        if (left.IsNull())
        {
            throw std::invalid_argument("Zip left source is null");
        }

        if (right.IsNull())
        {
            throw std::invalid_argument("Zip right source is null");
        }

        Ordinal left_length = left->GetOrdinalLength();
        Ordinal right_length = right->GetOrdinalLength();

        length = (left_length < right_length)
            ? left_length
            : right_length;
    }

    const Result& Get(const Ordinal& index) const override
    {
        if (index >= length)
        {
            throw std::out_of_range("Index out of range");
        }

        const Result* cached = cache.Find(index);
        if (cached != nullptr)
        {
            return *cached;
        }

        Result value(left->Get(index), right->Get(index));
        return cache.Insert(index, value);
    }

    Ordinal GetOrdinalLength() const override
    {
        return length;
    }

    Cardinal GetCardinalLength() const override
    {
        return length.ToCardinal();
    }

    size_t GetMaterializedCount() const override
    {
        return cache.GetSize();
    }

private:
    SharedPtr<LazyNode<TLeft>> left;
    SharedPtr<LazyNode<TRight>> right;

    Ordinal length;
    mutable OrdinalCache<Result> cache;
};

#endif // ZIP_NODE_H
