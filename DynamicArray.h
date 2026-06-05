#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <algorithm>
#include <stdexcept>

template <typename T>
class DynamicArray
{
private:
    T* data;
    int size;

    bool CheckIndex(int index) const
    {
        if (index < 0 || index >= size)
        {
            throw std::out_of_range("Index out of range");
        }

        return true;
    }

public:
    DynamicArray() : data(nullptr), size(0) {}

    DynamicArray(int size, const T& default_item) : data(nullptr), size(size)
    {
        if (size < 0)
        {
            throw std::out_of_range("Negative size");
        }

        if (size > 0)
        {
            data = new T[size];
            for (int index = 0; index < size; index++)
            {
                data[index] = default_item;
            }
        }
    }

    DynamicArray(const T* items, int size) : data(nullptr), size(size)
    {
        if (size < 0)
        {
            throw std::out_of_range("Negative size");
        }

        if (items == nullptr && size > 0)
        {
            throw std::invalid_argument("Source array is null");
        }

        if (size > 0)
        {
            data = new T[size];
            for (int index = 0; index < size; index++)
            {
                data[index] = items[index];
            }
        }
    }

    DynamicArray(const DynamicArray<T>& other) : data(nullptr), size(other.size)
    {
        if (size > 0)
        {
            data = new T[size];
            for (int index = 0; index < size; index++)
            {
                data[index] = other.data[index];
            }
        }
    }

    ~DynamicArray()
    {
        delete[] data;
    }

    DynamicArray<T>& operator=(DynamicArray<T> other)
    {
        std::swap(data, other.data);
        std::swap(size, other.size);
        return *this;
    }

    int GetSize() const
    {
        return size;
    }

    const T& Get(int index) const
    {
        CheckIndex(index);
        return data[index];
    }

    T& Get(int index)
    {
        CheckIndex(index);
        return data[index];
    }

    DynamicArray<T>& Set(int index, const T& value)
    {
        CheckIndex(index);
        data[index] = value;
        return *this;
    }

    DynamicArray<T>& Resize(int new_size, const T& default_item)
    {
        if (new_size < 0)
        {
            throw std::out_of_range("Negative size");
        }

        if (new_size == size)
        {
            return *this;
        }

        T* new_data = (new_size > 0) ? new T[new_size] : nullptr;
        int elements_to_copy = std::min(size, new_size);

        for (int index = 0; index < elements_to_copy; index++)
        {
            new_data[index] = data[index];
        }

        for (int index = elements_to_copy; index < new_size; index++)
        {
            new_data[index] = default_item;
        }

        delete[] data;
        data = new_data;
        size = new_size;
        return *this;
    }

    T& operator[](int index)
    {
        return Get(index);
    }

    const T& operator[](int index) const
    {
        return Get(index);
    }
};

#endif // DYNAMIC_ARRAY_H
