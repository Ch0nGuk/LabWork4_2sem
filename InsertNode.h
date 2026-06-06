#ifndef INSERT_NODE_H
#define INSERT_NODE_H

#include <cstddef>
#include <limits>
#include <stdexcept>

#include "LazyNode.h"
#include "SharedPtr.h"

template <class T>
class InsertNode : public LazyNode<T>
{
public:
    InsertNode(
        const SharedPtr<LazyNode<T>>& source_node,
        const SharedPtr<LazyNode<T>>& inserted_node,
        const Ordinal& insert_position)
        : source(source_node),
          inserted(inserted_node),
          position(insert_position),
          source_length(Ordinal::Zero()),
          inserted_length(Ordinal::Zero()),
          suffix_length(Ordinal::Zero()),
          result_length(Ordinal::Zero())
    {
        if (!source)
        {
            throw std::invalid_argument("Insert source node is null");
        }

        if (!inserted)
        {
            throw std::invalid_argument("Inserted node is null");
        }

        source_length = source->GetOrdinalLength();
        inserted_length = inserted->GetOrdinalLength();

        if (position > source_length)
        {
            throw std::out_of_range("Insert position is out of range");
        }

        suffix_length = source_length.RemovePrefix(position);
        result_length = position.Add(inserted_length).Add(suffix_length);
    }

    const T& Get(const Ordinal& index) const override
    {
        if (index >= result_length)
        {
            throw std::out_of_range("Index out of range");
        }

        if (index < position)
        {
            return source->Get(index);
        }

        Ordinal after_prefix = index.RemovePrefix(position);
        if (after_prefix < inserted_length)
        {
            return inserted->Get(after_prefix);
        }

        Ordinal suffix_offset = after_prefix.RemovePrefix(inserted_length);
        Ordinal source_index = position.Add(suffix_offset);

        return source->Get(source_index);
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
        size_t source_count = source->GetMaterializedCount();
        size_t inserted_count = inserted->GetMaterializedCount();

        if (inserted_count > std::numeric_limits<size_t>::max() - source_count)
        {
            throw std::overflow_error("Materialized count overflow");
        }

        return source_count + inserted_count;
    }

private:
    SharedPtr<LazyNode<T>> source;
    SharedPtr<LazyNode<T>> inserted;
    Ordinal position;
    Ordinal source_length;
    Ordinal inserted_length;
    Ordinal suffix_length;
    Ordinal result_length;
};

#endif // INSERT_NODE_H
