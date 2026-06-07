#ifndef PAIR_PROJECTION_NODE_H
#define PAIR_PROJECTION_NODE_H

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "LazyNode.h"
#include "SharedPtr.h"

template <class TFirst, class TSecond>
class FirstProjectionNode : public LazyNode<TFirst>
{
public:
    explicit FirstProjectionNode(
        const SharedPtr<LazyNode<std::pair<TFirst, TSecond>>>& source_node)
        : source(source_node)
    {
        if (source.IsNull())
        {
            throw std::invalid_argument("First projection source is null");
        }
    }

    const TFirst& Get(const Ordinal& index) const override
    {
        return source->Get(index).first;
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
        return source->GetMaterializedCount();
    }

private:
    SharedPtr<LazyNode<std::pair<TFirst, TSecond>>> source;
};

template <class TFirst, class TSecond>
class SecondProjectionNode : public LazyNode<TSecond>
{
public:
    explicit SecondProjectionNode(
        const SharedPtr<LazyNode<std::pair<TFirst, TSecond>>>& source_node)
        : source(source_node)
    {
        if (source.IsNull())
        {
            throw std::invalid_argument("Second projection source is null");
        }
    }

    const TSecond& Get(const Ordinal& index) const override
    {
        return source->Get(index).second;
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
        return source->GetMaterializedCount();
    }

private:
    SharedPtr<LazyNode<std::pair<TFirst, TSecond>>> source;
};

#endif // PAIR_PROJECTION_NODE_H
