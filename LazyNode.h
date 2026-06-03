#ifndef LAZY_NODE_H
#define LAZY_NODE_H

#include <cstddef>

#include "Cardinal.h"
#include "Ordinal.h"

template <class T>
class LazyNode
{
public:
    virtual ~LazyNode() {}

    virtual const T& Get(const Ordinal& index) const = 0;

    virtual Ordinal GetOrdinalLength() const = 0;
    virtual Cardinal GetCardinalLength() const = 0;

    virtual size_t GetMaterializedCount() const = 0;
};

#endif // LAZY_NODE_H
