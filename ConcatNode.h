#ifndef CONCAT_NODE_H
#define CONCAT_NODE_H

#include <cstddef>
#include <limits>
#include <stdexcept>

#include "LazyNode.h"
#include "SharedPtr.h"

template <class T>
class ConcatNode : public LazyNode<T>
{
public:
    ConcatNode(
        const SharedPtr<LazyNode<T>>& left_node,
        const SharedPtr<LazyNode<T>>& right_node)
        : left(left_node), right(right_node)
    {
        if (!left || !right)
        {
            throw std::invalid_argument("Concat node side is null");
        }
    }

    const T& Get(const Ordinal& index) const override
    {
        Ordinal left_length = left->GetOrdinalLength();
        if (index < left_length)
        {
            return left->Get(index);
        }

        Ordinal residual = index.RemovePrefix(left_length);
        if (residual >= right->GetOrdinalLength())
        {
            throw std::out_of_range("Index out of range");
        }

        return right->Get(residual);
    }

    Ordinal GetOrdinalLength() const override
    {
        return left->GetOrdinalLength().Add(right->GetOrdinalLength());
    }

    Cardinal GetCardinalLength() const override
    {
        Cardinal left_cardinal = left->GetCardinalLength();
        Cardinal right_cardinal = right->GetCardinalLength();

        if (left_cardinal.IsInfinite() || right_cardinal.IsInfinite())
        {
            return Cardinal::CountableInfinity();
        }

        size_t left_size = left_cardinal.ToSizeT();
        size_t right_size = right_cardinal.ToSizeT();
        if (right_size > std::numeric_limits<size_t>::max() - left_size)
        {
            throw std::overflow_error("Cardinal addition overflow");
        }

        return Cardinal::Finite(left_size + right_size);
    }

    size_t GetMaterializedCount() const override
    {
        size_t left_count = left->GetMaterializedCount();
        size_t right_count = right->GetMaterializedCount();
        if (right_count > std::numeric_limits<size_t>::max() - left_count)
        {
            throw std::overflow_error("Materialized count overflow");
        }

        return left_count + right_count;
    }

private:
    SharedPtr<LazyNode<T>> left;
    SharedPtr<LazyNode<T>> right;
};

#endif // CONCAT_NODE_H
