#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <cstddef>
#include <stdexcept>
#include <utility>

template <class T>
class SharedPtr
{
public:
    SharedPtr() noexcept : ptr(nullptr), owner_count(nullptr) {}

    explicit SharedPtr(T* raw_ptr) : ptr(raw_ptr), owner_count(nullptr)
    {
        if (ptr != nullptr)
        {
            try
            {
                owner_count = new size_t(1);
            }
            catch (...)
            {
                delete ptr;
                ptr = nullptr;
                throw;
            }
        }
    }

    SharedPtr(const SharedPtr<T>& other) noexcept : ptr(other.ptr), owner_count(other.owner_count)
    {
        AddRef();
    }

    SharedPtr(SharedPtr<T>&& other) noexcept : ptr(other.ptr), owner_count(other.owner_count)
    {
        other.ptr = nullptr;
        other.owner_count = nullptr;
    }

    ~SharedPtr()
    {
        Release();
    }

    SharedPtr<T>& operator=(const SharedPtr<T>& other) noexcept
    {
        if (this != &other)
        {
            T* new_ptr = other.ptr;
            size_t* new_owner_count = other.owner_count;

            if (new_owner_count != nullptr)
            {
                ++(*new_owner_count);
            }

            Release();
            ptr = new_ptr;
            owner_count = new_owner_count;
        }

        return *this;
    }

    SharedPtr<T>& operator=(SharedPtr<T>&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            ptr = other.ptr;
            owner_count = other.owner_count;
            other.ptr = nullptr;
            other.owner_count = nullptr;
        }

        return *this;
    }

    T* Get() const noexcept
    {
        return ptr;
    }

    T& operator*() const
    {
        if (ptr == nullptr)
        {
            throw std::logic_error("Dereferencing null SharedPtr");
        }

        return *ptr;
    }

    T* operator->() const
    {
        if (ptr == nullptr)
        {
            throw std::logic_error("Dereferencing null SharedPtr");
        }

        return ptr;
    }

    bool IsNull() const noexcept
    {
        return ptr == nullptr;
    }

    explicit operator bool() const noexcept
    {
        return ptr != nullptr;
    }

    size_t UseCount() const noexcept
    {
        return (owner_count == nullptr) ? 0 : *owner_count;
    }

    void Reset()
    {
        Release();
    }

    void Reset(T* raw_ptr)
    {
        if (ptr == raw_ptr)
        {
            return;
        }

        SharedPtr<T> replacement(raw_ptr);
        *this = std::move(replacement);
    }

private:
    T* ptr;
    size_t* owner_count;

    void AddRef() noexcept
    {
        if (owner_count != nullptr)
        {
            ++(*owner_count);
        }
    }

    void Release() noexcept
    {
        if (owner_count != nullptr)
        {
            --(*owner_count);
            if (*owner_count == 0)
            {
                delete ptr;
                delete owner_count;
            }
        }

        ptr = nullptr;
        owner_count = nullptr;
    }
};

#endif // SHARED_PTR_H
