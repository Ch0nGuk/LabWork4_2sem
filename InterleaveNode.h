#ifndef INTERLEAVE_NODE_H
#define INTERLEAVE_NODE_H

#include <cstddef>
#include <limits>
#include <stdexcept>

#include "DynamicArray.h"
#include "LazyNode.h"
#include "SharedPtr.h"

template <class T>
class InterleaveNode : public LazyNode<T>
{
public:
    explicit InterleaveNode(
        const DynamicArray<SharedPtr<LazyNode<T>>>& source_nodes)
        : sources(source_nodes),
          source_count(source_nodes.GetSize()),
          rounds(Ordinal::Zero()),
          result_length(Ordinal::Zero())
    {
        if (source_count <= 0)
        {
            throw std::invalid_argument("Interleave source count is zero");
        }

        for (int index = 0; index < source_count; index++)
        {
            if (sources.Get(index).IsNull())
            {
                throw std::invalid_argument("Interleave source is null");
            }
        }

        rounds = CalculateMinLength(sources);
        result_length = RoundsCount(
            rounds,
            static_cast<size_t>(source_count));
    }

    const T& Get(const Ordinal& index) const override
    {
        if (index >= result_length)
        {
            throw std::out_of_range("Index out of range");
        }

        size_t finite_part = index.GetFinitePart();
        size_t count = static_cast<size_t>(source_count);

        size_t source_id = finite_part % count;
        size_t source_finite_part = finite_part / count;

        Ordinal source_index = Ordinal::OmegaTimesPlus(
            index.GetOmegaCoeff(),
            source_finite_part);

        return sources.Get(static_cast<int>(source_id))->Get(source_index);
    }

    Ordinal GetOrdinalLength() const override
    {
        return result_length;
    }

    Cardinal GetCardinalLength() const override
    {
        return result_length.ToCardinal();
    }

    size_t GetMaterializedCount() const override
    {
        size_t total = 0;

        for (int index = 0; index < source_count; index++)
        {
            size_t current = sources.Get(index)->GetMaterializedCount();

            if (current > std::numeric_limits<size_t>::max() - total)
            {
                throw std::overflow_error("Materialized count overflow");
            }

            total += current;
        }

        return total;
    }

private:
    DynamicArray<SharedPtr<LazyNode<T>>> sources;
    int source_count;
    Ordinal rounds;
    Ordinal result_length;

    static Ordinal CalculateMinLength(
        const DynamicArray<SharedPtr<LazyNode<T>>>& source_nodes)
    {
        if (source_nodes.GetSize() <= 0)
        {
            throw std::invalid_argument("Interleave source count is zero");
        }

        Ordinal result = source_nodes.Get(0)->GetOrdinalLength();

        for (int index = 1; index < source_nodes.GetSize(); index++)
        {
            Ordinal current = source_nodes.Get(index)->GetOrdinalLength();

            if (current < result)
            {
                result = current;
            }
        }

        return result;
    }

    static Ordinal RoundsCount(
        const Ordinal& rounds_value,
        size_t factor)
    {
        if (factor == 0 || rounds_value.IsZero())
        {
            return Ordinal::Zero();
        }

        size_t finite_part = rounds_value.GetFinitePart();

        if (factor != 0 &&
            finite_part > std::numeric_limits<size_t>::max() / factor)
        {
            throw std::overflow_error("Ordinal multiplication overflow");
        }

        size_t scaled_finite_part = finite_part * factor;

        if (rounds_value.IsFinite())
        {
            return Ordinal::Finite(scaled_finite_part);
        }

        return Ordinal::OmegaTimesPlus(
            rounds_value.GetOmegaCoeff(),
            scaled_finite_part);
    }
};

#endif // INTERLEAVE_NODE_H
