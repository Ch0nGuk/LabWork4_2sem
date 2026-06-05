#ifndef CONCAT_NODE_H
#define CONCAT_NODE_H

#include <cstddef>
#include <limits>
#include <stdexcept>

#include "LazyNode.h"
#include "SharedPtr.h"



inline Ordinal GetRightResidualIndex(const Ordinal& left_length, const Ordinal& global_index)
{
    size_t left_omega = left_length.GetOmegaCoeff();
    size_t left_finite = left_length.GetFinitePart();
    size_t global_omega = global_index.GetOmegaCoeff();
    size_t global_finite = global_index.GetFinitePart();

    if (left_omega == 0)
    {
        if (global_omega == 0)
        {
            if (global_finite < left_finite)
            {
                throw std::out_of_range("Global index is before right side");
            }

            return Ordinal::Finite(global_finite - left_finite);
        }

        return global_index;
    }

    if (global_omega == left_omega && global_finite >= left_finite)
    {
        return Ordinal::Finite(global_finite - left_finite);
    }

    if (global_omega > left_omega)
    {
        return Ordinal::OmegaTimesPlus(global_omega - left_omega, global_finite);
    }

    throw std::out_of_range("Global index is before right side");
}

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

        Ordinal residual = GetRightResidualIndex(left_length, index);
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
        if (right_count + left_count > std::numeric_limits<size_t>::max())
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
