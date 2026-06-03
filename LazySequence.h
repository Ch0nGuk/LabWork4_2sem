#ifndef LAZY_SEQUENCE_H
#define LAZY_SEQUENCE_H

#include <cstddef>
#include <functional>
#include <limits>
#include <stdexcept>

#include "ConcatNode.h"
#include "FiniteNode.h"
#include "LazyNode.h"
#include "MutableArraySequence.h"
#include "RecurrenceNode.h"
#include "Sequence.h"
#include "SharedPtr.h"

template <class T>
class LazySequence : public Sequence<T>
{
private:
    class LazySequenceEnumerator : public IEnumerator<T>
    {
    public:
        explicit LazySequenceEnumerator(const LazySequence<T>* source)
            : sequence(source), next_index(0), current_index(0), has_current(false)
        {
            if (sequence == nullptr)
            {
                throw std::invalid_argument("Enumerator source is null");
            }

            Ordinal length = sequence->GetOrdinalLength();
            is_finite = length.IsFinite();
            finite_count = is_finite ? length.GetFinitePart() : 0;
        }

        bool MoveNext() override
        {
            if (is_finite && next_index >= finite_count)
            {
                return false;
            }

            if (next_index == std::numeric_limits<size_t>::max())
            {
                throw std::overflow_error("Enumerator index overflow");
            }

            current_index = next_index;
            next_index++;
            has_current = true;
            return true;
        }

        const T& Current() const override
        {
            if (!has_current)
            {
                throw std::logic_error("Enumerator is not positioned on an item");
            }

            return sequence->Get(Ordinal::Finite(current_index));
        }

    private:
        const LazySequence<T>* sequence;
        size_t next_index;
        size_t current_index;
        bool has_current;
        bool is_finite;
        size_t finite_count;
    };

public:
    LazySequence()
        : root(SharedPtr<LazyNode<T>>(new FiniteNode<T>())) {}

    LazySequence(const T* items, int count)
        : root(SharedPtr<LazyNode<T>>(new FiniteNode<T>(items, count))) {}

    LazySequence(T* items, int count)
        : root(SharedPtr<LazyNode<T>>(new FiniteNode<T>(items, count))) {}

    explicit LazySequence(const Sequence<T>* sequence)
        : root(SharedPtr<LazyNode<T>>(new FiniteNode<T>(sequence))) {}

    explicit LazySequence(Sequence<T>* sequence)
        : root(SharedPtr<LazyNode<T>>(new FiniteNode<T>(sequence))) {}

    LazySequence(
        std::function<T(Sequence<T>*)> generator_rule,
        Sequence<T>* initial_values)
        : root(SharedPtr<LazyNode<T>>(new RecurrenceNode<T>(generator_rule, initial_values))) {}

    LazySequence(const LazySequence<T>& other) : root(other.root) {}

    LazySequence(LazySequence<T>&& other) noexcept : root(static_cast<SharedPtr<LazyNode<T>>&&>(other.root)) {}

    LazySequence<T>& operator=(const LazySequence<T>& other)
    {
        if (this != &other)
        {
            root = other.root;
        }

        return *this;
    }

    LazySequence<T>& operator=(LazySequence<T>&& other) noexcept
    {
        if (this != &other)
        {
            root = static_cast<SharedPtr<LazyNode<T>>&&>(other.root);
        }

        return *this;
    }

    ~LazySequence() override = default;

    const T& GetFirst() const override
    {
        if (GetOrdinalLength().IsZero())
        {
            throw std::out_of_range("Sequence is empty");
        }

        return Get(Ordinal::Zero());
    }

    const T& GetLast() const override
    {
        Ordinal length = GetOrdinalLength();
        if (length.IsZero())
        {
            throw std::out_of_range("Sequence is empty");
        }

        std::optional<Ordinal> predecessor = length.Predecessor();
        if (!predecessor.has_value())
        {
            throw std::logic_error("Limit ordinal has no last element");
        }

        return Get(predecessor.value());
    }

    const T& Get(int index) const override
    {
        if (index < 0)
        {
            throw std::out_of_range("Index out of range");
        }

        return Get(Ordinal::Finite(static_cast<size_t>(index)));
    }

    const T& Get(const Ordinal& index) const
    {
        if (root.IsNull())
        {
            throw std::logic_error("LazySequence root is null");
        }

        if (index >= GetOrdinalLength())
        {
            throw std::out_of_range("Index out of range");
        }

        return root->Get(index);
    }

    int GetLength() const override
    {
        return GetOrdinalLength().ToCardinal().ToInt();
    }

    Ordinal GetOrdinalLength() const
    {
        if (root.IsNull())
        {
            throw std::logic_error("LazySequence root is null");
        }

        return root->GetOrdinalLength();
    }

    Cardinal GetCardinalLength() const
    {
        if (root.IsNull())
        {
            throw std::logic_error("LazySequence root is null");
        }

        return root->GetCardinalLength();
    }

    size_t GetMaterializedCount() const
    {
        if (root.IsNull())
        {
            throw std::logic_error("LazySequence root is null");
        }

        return root->GetMaterializedCount();
    }

    Sequence<T>* Append(const T& item) override
    {
        return AppendInternal(item);
    }

    Sequence<T>* Prepend(const T& item) override
    {
        return PrependInternal(item);
    }

    Sequence<T>* InsertAt(int index, const T& item) override
    {
        return InsertAtInternal(index, item);
    }

    Sequence<T>* GetSubsequence(int start_index, int end_index) const override
    {
        if (start_index < 0 || end_index < 0 || start_index > end_index)
        {
            throw std::out_of_range("Index out of range");
        }

        Ordinal end_ordinal = Ordinal::Finite(static_cast<size_t>(end_index));
        if (end_ordinal >= GetOrdinalLength())
        {
            throw std::out_of_range("Index out of range");
        }

        int count = end_index - start_index + 1;
        DynamicArray<T> data(count, Get(start_index));
        for (int offset = 0; offset < count; offset++)
        {
            data.Set(offset, Get(start_index + offset));
        }

        return new LazySequence<T>(SharedPtr<LazyNode<T>>(new FiniteNode<T>(data)));
    }

    Sequence<T>* Concat(const Sequence<T>& other) const override
    {
        const LazySequence<T>* other_lazy = dynamic_cast<const LazySequence<T>*>(&other);
        SharedPtr<LazyNode<T>> right_root =
            (other_lazy != nullptr)
                ? other_lazy->root
                : SharedPtr<LazyNode<T>>(new FiniteNode<T>(&other));

        return new LazySequence<T>(SharedPtr<LazyNode<T>>(new ConcatNode<T>(root, right_root)));
    }

    Sequence<T>* Slice(int start_index, int count) const override
    {
        EnsureFiniteLength("Slice is implemented only for finite LazySequence");

        int length = GetLength();
        ValidateSliceRange(start_index, count, length);

        int new_size = length - count;
        if (new_size == 0)
        {
            return new LazySequence<T>();
        }

        const T& default_item = (start_index > 0) ? Get(0) : Get(start_index + count);
        DynamicArray<T> data(new_size, default_item);
        int target_index = 0;

        for (int index = 0; index < start_index; index++)
        {
            data.Set(target_index, Get(index));
            target_index++;
        }

        for (int index = start_index + count; index < length; index++)
        {
            data.Set(target_index, Get(index));
            target_index++;
        }

        return new LazySequence<T>(SharedPtr<LazyNode<T>>(new FiniteNode<T>(data)));
    }

    Sequence<T>* Slice(int start_index, int count, const Sequence<T>& replacement) const override
    {
        EnsureFiniteLength("Slice replacement is implemented only for finite LazySequence");

        int length = GetLength();
        ValidateSliceRange(start_index, count, length);

        int replacement_size = replacement.GetLength();
        if (replacement_size > std::numeric_limits<int>::max() - (length - count))
        {
            throw std::overflow_error("Slice result length overflow");
        }

        int new_size = length - count + replacement_size;
        if (new_size == 0)
        {
            return new LazySequence<T>();
        }

        const T& default_item =
            (start_index > 0)
                ? Get(0)
                : ((replacement_size > 0) ? replacement.Get(0) : Get(start_index + count));
        DynamicArray<T> data(new_size, default_item);
        int target_index = 0;

        for (int index = 0; index < start_index; index++)
        {
            data.Set(target_index, Get(index));
            target_index++;
        }

        for (int index = 0; index < replacement_size; index++)
        {
            data.Set(target_index, replacement.Get(index));
            target_index++;
        }

        for (int index = start_index + count; index < length; index++)
        {
            data.Set(target_index, Get(index));
            target_index++;
        }

        return new LazySequence<T>(SharedPtr<LazyNode<T>>(new FiniteNode<T>(data)));
    }

    IEnumerator<T>* GetEnumerator() const override
    {
        return new LazySequenceEnumerator(this);
    }

    Sequence<T>* Instance() override
    {
        return new LazySequence<T>(*this);
    }

    Sequence<T>* Where(bool (*predicate)(T)) const override
    {
        if (predicate == nullptr)
        {
            throw std::invalid_argument("Predicate is null");
        }

        EnsureFiniteLength("Where is implemented only for finite LazySequence");

        int length = GetLength();
        int result_count = 0;
        const T* first_match = nullptr;

        for (int index = 0; index < length; index++)
        {
            const T& value = Get(index);
            if (predicate(value))
            {
                if (first_match == nullptr)
                {
                    first_match = &value;
                }
                result_count++;
            }
        }

        if (result_count == 0)
        {
            return new LazySequence<T>();
        }

        DynamicArray<T> data(result_count, *first_match);
        int target_index = 0;
        for (int index = 0; index < length; index++)
        {
            const T& value = Get(index);
            if (predicate(value))
            {
                data.Set(target_index, value);
                target_index++;
            }
        }

        return new LazySequence<T>(SharedPtr<LazyNode<T>>(new FiniteNode<T>(data)));
    }

    static LazySequence<int>* Naturals()
    {
        int initial_items[] = {0};
        MutableArraySequence<int> initial(initial_items, 1);

        return new LazySequence<int>(
            [](Sequence<int>* prefix) -> int
            {
                int length = prefix->GetLength();
                if (length == 0)
                {
                    return 0;
                }

                int previous = prefix->Get(length - 1);
                if (previous == std::numeric_limits<int>::max())
                {
                    throw std::overflow_error("Natural number overflow");
                }

                return previous + 1;
            },
            &initial);
    }

    static LazySequence<int>* Fibonacci()
    {
        int initial_items[] = {0, 1};
        MutableArraySequence<int> initial(initial_items, 2);

        return new LazySequence<int>(
            [](Sequence<int>* prefix) -> int
            {
                int length = prefix->GetLength();
                if (length == 0)
                {
                    return 0;
                }

                if (length == 1)
                {
                    return 1;
                }

                int left = prefix->Get(length - 1);
                int right = prefix->Get(length - 2);
                if (right > std::numeric_limits<int>::max() - left)
                {
                    throw std::overflow_error("Fibonacci overflow");
                }

                return left + right;
            },
            &initial);
    }

    static LazySequence<int>* Factorials()
    {
        int initial_items[] = {1};
        MutableArraySequence<int> initial(initial_items, 1);

        return new LazySequence<int>(
            [](Sequence<int>* prefix) -> int
            {
                int n = prefix->GetLength();
                if (n == 0)
                {
                    return 1;
                }

                int previous = prefix->Get(n - 1);
                if (n != 0 && previous > std::numeric_limits<int>::max() / n)
                {
                    throw std::overflow_error("Factorial overflow");
                }

                return previous * n;
            },
            &initial);
    }

private:
    SharedPtr<LazyNode<T>> root;

    explicit LazySequence(const SharedPtr<LazyNode<T>>& root_node) : root(root_node)
    {
        if (root.IsNull())
        {
            throw std::invalid_argument("LazySequence root is null");
        }
    }

    void EnsureFiniteLength(const char* message) const
    {
        if (!GetOrdinalLength().IsFinite())
        {
            throw std::logic_error(message);
        }
    }

    static void ValidateSliceRange(int start_index, int count, int length)
    {
        if (start_index < 0 || count < 0 || start_index > length || count > length - start_index)
        {
            throw std::out_of_range("Index out of range");
        }
    }

protected:
    Sequence<T>* AppendInternal(const T& item) override
    {
        SharedPtr<LazyNode<T>> item_node(new FiniteNode<T>(&item, 1));
        return new LazySequence<T>(SharedPtr<LazyNode<T>>(new ConcatNode<T>(root, item_node)));
    }

    Sequence<T>* PrependInternal(const T& item) override
    {
        SharedPtr<LazyNode<T>> item_node(new FiniteNode<T>(&item, 1));
        return new LazySequence<T>(SharedPtr<LazyNode<T>>(new ConcatNode<T>(item_node, root)));
    }

    Sequence<T>* InsertAtInternal(int index, const T& item) override
    {
        EnsureFiniteLength("InsertAt is implemented only for finite LazySequence");

        int length = GetLength();
        if (index < 0 || index > length)
        {
            throw std::out_of_range("Index out of range");
        }

        if (length == std::numeric_limits<int>::max())
        {
            throw std::overflow_error("InsertAt result length overflow");
        }

        DynamicArray<T> data(length + 1, item);
        for (int source_index = 0; source_index < index; source_index++)
        {
            data.Set(source_index, Get(source_index));
        }

        data.Set(index, item);

        for (int source_index = index; source_index < length; source_index++)
        {
            data.Set(source_index + 1, Get(source_index));
        }

        return new LazySequence<T>(SharedPtr<LazyNode<T>>(new FiniteNode<T>(data)));
    }
};

#endif // LAZY_SEQUENCE_H
