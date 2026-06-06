#ifndef ORDINAL_CACHE_H
#define ORDINAL_CACHE_H

#include <cstddef>
#include <limits>
#include <stdexcept>

#include "DynamicArray.h"
#include "Ordinal.h"

template <class T>
class OrdinalCache
{
public:
    OrdinalCache() : data(), count(0) {}

    ~OrdinalCache() = default;

    OrdinalCache(const OrdinalCache<T>&) = delete;
    OrdinalCache<T>& operator=(const OrdinalCache<T>&) = delete;

    const T* Find(const Ordinal& index) const
    {
        for (int data_index = 0; data_index < data.GetSize(); data_index++)
        {
            const Entry& entry = data.Get(data_index);
            if (entry.index == index)
            {
                return &entry.value;
            }
        }

        return nullptr;
    }

    const T& Store(const Ordinal& index, const T& value)
    {
        if (count == std::numeric_limits<size_t>::max())
        {
            throw std::overflow_error("Ordinal cache size overflow");
        }

        Entry entry(index, value);
        int new_size = static_cast<int>(count + 1);
        data.Resize(new_size, entry);
        count++;
        return data.Get(new_size - 1).value;
    }

    size_t GetSize() const
    {
        return count;
    }

private:
    struct Entry
    {
        Ordinal index;
        T value;

        Entry() : index(Ordinal::Zero()), value() {}

        Entry(const Ordinal& entry_index, const T& entry_value)
            : index(entry_index), value(entry_value) {}
    };

    DynamicArray<Entry> data;
    size_t count;
};

#endif // ORDINAL_CACHE_H
