#ifndef RECURRENCE_NODE_H
#define RECURRENCE_NODE_H

#include <functional>
#include <limits>
#include <stdexcept>

#include "LazyNode.h"
#include "MutableArraySequence.h"
#include "Sequence.h"

template <class T>
class RecurrenceNode : public LazyNode<T>
{
public:
    RecurrenceNode(
        std::function<T(Sequence<T>*)> recurrence_rule,
        const Sequence<T>* initial_values)
        : rule(recurrence_rule), cache()
    {
        if (!rule)
        {
            throw std::invalid_argument("Recurrence rule is empty");
        }

        if (initial_values == nullptr)
        {
            throw std::invalid_argument("Initial values sequence is null");
        }

        int count = initial_values->GetLength();
        for (int index = 0; index < count; index++)
        {
            cache.Append(initial_values->Get(index));
        }
    }

    const T& Get(const Ordinal& index) const override
    {
        if (!index.IsFinite())
        {
            throw std::out_of_range("Recurrence node length is omega");
        }

        size_t finite_index = index.GetFinitePart();
        if (finite_index > static_cast<size_t>(std::numeric_limits<int>::max()))
        {
            throw std::overflow_error("Index does not fit into Sequence<int> cache API");
        }

        int target = static_cast<int>(finite_index);
        while (cache.GetLength() <= target)
        {
            T value = rule(&cache);
            cache.Append(value);
        }

        return cache.Get(target);
    }

    Ordinal GetOrdinalLength() const override
    {
        return Ordinal::Omega();
    }

    Cardinal GetCardinalLength() const override
    {
        return Cardinal::CountableInfinity();
    }

    size_t GetMaterializedCount() const override
    {
        return static_cast<size_t>(cache.GetLength());
    }

private:
    std::function<T(Sequence<T>*)> rule;
    mutable MutableArraySequence<T> cache;
};

#endif // RECURRENCE_NODE_H
