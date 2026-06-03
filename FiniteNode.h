#ifndef FINITE_NODE_H
#define FINITE_NODE_H

#include <cstddef>
#include <limits>
#include <stdexcept>

#include "DynamicArray.h"
#include "LazyNode.h"
#include "Sequence.h"

template <class T>
class FiniteNode : public LazyNode<T>
{
public:
    FiniteNode() : data() {}

    explicit FiniteNode(const DynamicArray<T>& items) : data(items) {}

    FiniteNode(const T* items, int count) : data(items, count) {}

    FiniteNode(T* items, int count) : data(items, count) {}

    explicit FiniteNode(const Sequence<T>* sequence) : data()
    {
        if (sequence == nullptr)
        {
            throw std::invalid_argument("Source sequence is null");
        }

        int count = sequence->GetLength();
        if (count == 0)
        {
            return;
        }

        DynamicArray<T> copy(count, sequence->Get(0));
        for (int index = 0; index < count; index++)
        {
            copy.Set(index, sequence->Get(index));
        }

        data = copy;
    }

    const T& Get(const Ordinal& index) const override
    {
        if (!index.IsFinite())
        {
            throw std::out_of_range("Finite node accepts only finite indexes");
        }

        size_t finite_index = index.GetFinitePart();
        if (finite_index >= static_cast<size_t>(data.GetSize()))
        {
            throw std::out_of_range("Index out of range");
        }

        return data.Get(static_cast<int>(finite_index));
    }

    Ordinal GetOrdinalLength() const override
    {
        return Ordinal::Finite(static_cast<size_t>(data.GetSize()));
    }

    Cardinal GetCardinalLength() const override
    {
        return Cardinal::Finite(static_cast<size_t>(data.GetSize()));
    }

    size_t GetMaterializedCount() const override
    {
        return static_cast<size_t>(data.GetSize());
    }

private:
    DynamicArray<T> data;
};

#endif // FINITE_NODE_H
