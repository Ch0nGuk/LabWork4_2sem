#ifndef MAP_NODE_H
#define MAP_NODE_H

#include <cstddef>
#include <stdexcept>

#include "LazyNode.h"
#include "OrdinalCache.h"
#include "SharedPtr.h"

template <class TSource, class TResult>
class MapNode : public LazyNode<TResult>
{
public:
    MapNode(
        const SharedPtr<LazyNode<TSource>>& source_node,
        TResult (*map_function)(TSource))
        : source(source_node), mapper(map_function)
    {
        if (!source)
        {
            throw std::invalid_argument("Map source is null");
        }

        if (mapper == nullptr)
        {
            throw std::invalid_argument("Mapper is null");
        }
    }

    const TResult& Get(const Ordinal& index) const override
    {
        if (index >= source->GetOrdinalLength())
        {
            throw std::out_of_range("Index out of range");
        }

        const TResult* cached = cache.Find(index);
        if (cached != nullptr)
        {
            return *cached;
        }

        TResult mapped_value = mapper(source->Get(index));
        return cache.Insert(index, mapped_value);
    }

    Ordinal GetOrdinalLength() const override
    {
        return source->GetOrdinalLength();
    }

    Cardinal GetCardinalLength() const override
    {
        return source->GetCardinalLength();
    }

    size_t GetMaterializedCount() const override
    {
        return cache.GetSize();
    }

private:
    SharedPtr<LazyNode<TSource>> source;
    TResult (*mapper)(TSource);
    mutable OrdinalCache<TResult> cache;
};

#endif // MAP_NODE_H
