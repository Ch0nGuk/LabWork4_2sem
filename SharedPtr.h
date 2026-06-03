#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <cstddef>
#include <stdexcept>

template <class T>
class SharedPtr
{
public:
    SharedPtr() noexcept : ptr(nullptr), counter(nullptr) {}

    explicit SharedPtr(T* raw_ptr) : ptr(raw_ptr), counter(nullptr)
    {
        if (ptr != nullptr)
        {
            try
            {
                counter = new size_t(1);
            }
            catch (...)
            {
                delete ptr;
                ptr = nullptr;
                throw;
            }
        }
    }

    SharedPtr(const SharedPtr<T>& other) noexcept : ptr(other.ptr), counter(other.counter)
    {
        AddRef();
    }

    SharedPtr(SharedPtr<T>&& other) noexcept : ptr(other.ptr), counter(other.counter)
    {
        other.ptr = nullptr;
        other.counter = nullptr;
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
            size_t* new_counter = other.counter;

            if (new_counter != nullptr)
            {
                ++(*new_counter);
            }

            Release();
            ptr = new_ptr;
            counter = new_counter;
        }

        return *this;
    }

    SharedPtr<T>& operator=(SharedPtr<T>&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            ptr = other.ptr;
            counter = other.counter;
            other.ptr = nullptr;
            other.counter = nullptr;
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
        return (counter == nullptr) ? 0 : *counter;
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
        *this = static_cast<SharedPtr<T>&&>(replacement);
    }

private:
    T* ptr;
    size_t* counter;

    void AddRef() noexcept
    {
        if (counter != nullptr)
        {
            ++(*counter);
        }
    }

    void Release() noexcept
    {
        if (counter != nullptr)
        {
            --(*counter);
            if (*counter == 0)
            {
                delete ptr;
                delete counter;
            }
        }

        ptr = nullptr;
        counter = nullptr;
    }
};

#endif // SHARED_PTR_H
