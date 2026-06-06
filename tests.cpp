#include "tests.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "DynamicArray.h"
#include "LinkedList.h"
#include "MutableArraySequence.h"
#include "ImmutableArraySequence.h"
#include "ListSequence.h"
#include "SharedPtr.h"
#include "Cardinal.h"
#include "Ordinal.h"
#include "FiniteNode.h"
#include "RecurrenceNode.h"
#include "ConcatNode.h"
#include "InsertNode.h"
#include "LazySequence.h"
#include "sequence_factory.h"

namespace
{
    // Завершает текущий тест ошибкой, если значения не совпали.
    template <typename T>
    void AssertEqual(const T& actual, const T& expected, const std::string& message)
    {
        if (!(actual == expected))
        {
            std::ostringstream error_stream;
            error_stream << message;
            throw std::runtime_error(error_stream.str());
        }
    }

    // Завершает текущий тест ошибкой, если условие ложно.
    void AssertTrue(bool condition, const std::string& message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    // Проверяет, что переданное действие выбрасывает исключение.
    template <typename Action>
    void AssertThrows(Action action, const std::string& message)
    {
        bool did_throw = false;

        try
        {
            action();
        }
        catch (const std::exception&)
        {
            did_throw = true;
        }

        if (!did_throw)
        {
            throw std::runtime_error(message);
        }
    }

    template <typename Exception, typename Action>
    void AssertThrowsExact(Action action, const std::string& message)
    {
        bool did_throw_expected = false;

        try
        {
            action();
        }
        catch (const Exception&)
        {
            did_throw_expected = true;
        }
        catch (const std::exception& error)
        {
            std::ostringstream error_stream;
            error_stream << message << ": wrong exception type: " << error.what();
            throw std::runtime_error(error_stream.str());
        }

        if (!did_throw_expected)
        {
            throw std::runtime_error(message);
        }
    }

    // Пример функции преобразования для тестов последовательностей.
    int Square(int value)
    {
        return value * value;
    }

    // Пример предиката для тестов фильтрации.
    bool IsEven(int value)
    {
        return value % 2 == 0;
    }

    // Пример сворачивающей функции для тестов Reduce.
    int Sum(int left, int right)
    {
        return left + right;
    }

    // Проверяет полное содержимое и длину целочисленной последовательности.
    void AssertSequenceContent(const Sequence<int>* sequence, const int* expected, int count, const std::string& message)
    {
        AssertEqual(sequence->GetLength(), count, message + ": unexpected length");
        for (int index = 0; index < count; index++)
        {
            AssertEqual(sequence->Get(index), expected[index], message + ": unexpected item");
        }
    }

    template <typename T>
    void AssertTypedSequenceContent(const Sequence<T>* sequence, const T* expected, int count, const std::string& message)
    {
        AssertEqual(sequence->GetLength(), count, message + ": unexpected length");
        for (int index = 0; index < count; index++)
        {
            AssertEqual(sequence->Get(index), expected[index], message + ": unexpected item");
        }
    }

    double Half(int value)
    {
        return value / 2.0;
    }

    bool IsPositive(int value)
    {
        return value > 0;
    }

    struct SharedPtrProbe
    {
        explicit SharedPtrProbe(int* deletion_counter) : counter(deletion_counter) {}

        ~SharedPtrProbe()
        {
            (*counter)++;
        }

        int* counter;
    };

    // Проверяет создание, изменение, Resize и ошибочные случаи DynamicArray.
    void TestDynamicArray()
    {
        int items[] = {1, 2, 3};

        DynamicArray<int> array(items, 3);
        DynamicArray<int> default_filled_array(4, 7);
        AssertEqual(array.GetSize(), 3, "DynamicArray GetSize");
        AssertEqual(array.Get(0), 1, "DynamicArray Get first");
        AssertEqual(array.Get(2), 3, "DynamicArray Get last");
        AssertEqual(default_filled_array.GetSize(), 4, "DynamicArray default-filled size");
        AssertEqual(default_filled_array.Get(0), 7, "DynamicArray default-filled first");
        AssertEqual(default_filled_array.Get(3), 7, "DynamicArray default-filled last");

        array.Set(1, 5);
        AssertEqual(array.Get(1), 5, "DynamicArray Set");

        array.Resize(5, 9);
        AssertEqual(array.GetSize(), 5, "DynamicArray Resize bigger");
        AssertEqual(array.Get(0), 1, "DynamicArray data preserved after grow");
        AssertEqual(array.Get(1), 5, "DynamicArray changed item preserved after grow");
        AssertEqual(array.Get(3), 9, "DynamicArray new item after grow");
        AssertEqual(array.Get(4), 9, "DynamicArray last new item after grow");

        array.Resize(2, 0);
        AssertEqual(array.GetSize(), 2, "DynamicArray Resize smaller");
        AssertEqual(array.Get(1), 5, "DynamicArray data preserved after shrink");

        AssertThrows([]() { DynamicArray<int> invalid(-1, 0); }, "DynamicArray negative size must throw");
        AssertThrows([]() { DynamicArray<int> invalid(nullptr, 2); }, "DynamicArray null source must throw");
        AssertThrows([&array]() { array.Get(-1); }, "DynamicArray negative index must throw");
        AssertThrows([&array]() { array.Get(10); }, "DynamicArray too large index must throw");
        AssertThrows([&array]() { array.Set(10, 1); }, "DynamicArray Set out of range must throw");
        AssertThrows([&array]() { array.Resize(-5, 0); }, "DynamicArray negative resize must throw");
    }

    // Проверяет базовые операции списка, подсписки, Concat и ошибочные случаи.
    void TestLinkedList()
    {
        int items[] = {1, 2, 3};

        LinkedList<int> list(items, 3);
        AssertEqual(list.GetSize(), 3, "LinkedList GetSize");
        AssertEqual(list.GetFirst(), 1, "LinkedList GetFirst");
        AssertEqual(list.GetLast(), 3, "LinkedList GetLast");

        list.Append(4);
        list.Prepend(0);
        list.InsertAt(2, 99);
        AssertEqual(list.GetSize(), 6, "LinkedList size after mutating operations");
        AssertEqual(list.Get(0), 0, "LinkedList Prepend");
        AssertEqual(list.Get(2), 99, "LinkedList InsertAt");
        AssertEqual(list.GetLast(), 4, "LinkedList Append");

        LinkedList<int> sub_list = list.GetSubList(1, 3);
        AssertEqual(sub_list.GetSize(), 3, "LinkedList GetSubList size");
        AssertEqual(sub_list.Get(0), 1, "LinkedList GetSubList first item");
        AssertEqual(sub_list.Get(2), 2, "LinkedList GetSubList last item");

        LinkedList<int> concat_list = list.Concat(sub_list);
        AssertEqual(concat_list.GetSize(), 9, "LinkedList Concat size");

        LinkedList<int> empty_list;
        AssertThrows([&empty_list]() { empty_list.GetFirst(); }, "LinkedList GetFirst on empty must throw");
        AssertThrows([&empty_list]() { empty_list.GetLast(); }, "LinkedList GetLast on empty must throw");
        AssertThrows([&list]() { list.Get(-1); }, "LinkedList negative Get must throw");
        AssertThrows([&list]() { list.Get(100); }, "LinkedList too large Get must throw");
        AssertThrows([&list]() { list.InsertAt(-1, 1); }, "LinkedList negative InsertAt must throw");
        AssertThrows([&list]() { list.InsertAt(100, 1); }, "LinkedList too large InsertAt must throw");
        AssertThrows([&list]() { list.GetSubList(3, 1); }, "LinkedList invalid GetSubList must throw");
        AssertThrows([]() { LinkedList<int> invalid(nullptr, 2); }, "LinkedList null source must throw");
    }

    // Проверяет mutable-семантику и операции sequence для массива.
    void TestMutableArraySequence()
    {
        int items[] = {1, 2, 3};
        int expected_after_insert[] = {0, 1, 99, 2, 3, 4};
        int expected_subsequence[] = {1, 99, 2};
        int expected_concat[] = {0, 1, 99, 2, 3, 4, 1, 99, 2};
        int expected_map[] = {0, 1, 9801, 4, 9, 16};
        int expected_where[] = {0, 2, 4};
        int expected_slice[] = {0, 7, 8, 4};

        Sequence<int>* sequence = new MutableArraySequence<int>(items, 3);

        Sequence<int>* appended = sequence->Append(4);
        AssertTrue(appended == sequence, "MutableArraySequence Append must return same object");
        sequence = appended;

        Sequence<int>* prepended = sequence->Prepend(0);
        AssertTrue(prepended == sequence, "MutableArraySequence Prepend must return same object");
        sequence = prepended;

        Sequence<int>* inserted = sequence->InsertAt(2, 99);
        AssertTrue(inserted == sequence, "MutableArraySequence InsertAt must return same object");
        sequence = inserted;
        AssertSequenceContent(sequence, expected_after_insert, 6, "MutableArraySequence content after insert");

        Sequence<int>* subsequence = sequence->GetSubsequence(1, 3);
        AssertSequenceContent(subsequence, expected_subsequence, 3, "MutableArraySequence GetSubsequence");

        Sequence<int>* concatenated = sequence->Concat(*subsequence);
        AssertSequenceContent(concatenated, expected_concat, 9, "MutableArraySequence Concat");

        ListSequenceFactory<int> factory;
        Sequence<int>* mapped = sequence->Map<int>(Square, factory);
        AssertSequenceContent(mapped, expected_map, 6, "MutableArraySequence Map");

        Sequence<int>* filtered = sequence->Where(IsEven);
        AssertSequenceContent(filtered, expected_where, 3, "MutableArraySequence Where");

        AssertEqual(sequence->Reduce(Sum, 0), 109, "MutableArraySequence Reduce");

        int replacement_items[] = {7, 8};
        Sequence<int>* replacement = new MutableArraySequence<int>(replacement_items, 2);
        Sequence<int>* sliced = sequence->Slice(1, 4, *replacement);
        AssertSequenceContent(sliced, expected_slice, 4, "MutableArraySequence Slice");

        AssertThrows([sequence]() { sequence->GetSubsequence(-1, 1); }, "MutableArraySequence invalid subsequence must throw");
        AssertThrows([sequence]() { sequence->InsertAt(100, 1); }, "MutableArraySequence invalid insert must throw");
        AssertThrows([sequence, replacement]() { sequence->Slice(10, 1, *replacement); }, "MutableArraySequence invalid slice must throw");

        delete sequence;
        delete subsequence;
        delete concatenated;
        delete mapped;
        delete filtered;
        delete replacement;
        delete sliced;
    }

    // Проверяет immutable-семантику и операции sequence для массива.
    void TestImmutableArraySequence()
    {
        int items[] = {1, 2, 3};
        int expected_original[] = {1, 2, 3};
        int expected_append[] = {1, 2, 3, 4};
        int expected_prepend[] = {0, 1, 2, 3};
        int expected_insert[] = {1, 99, 2, 3};
        int expected_map[] = {1, 4, 9};
        int expected_where[] = {2};
        int expected_slice[] = {1, 7, 8, 3};

        Sequence<int>* sequence = new ImmutableArraySequence<int>(items, 3);
        Sequence<int>* appended = sequence->Append(4);
        Sequence<int>* prepended = sequence->Prepend(0);
        Sequence<int>* inserted = sequence->InsertAt(1, 99);

        AssertTrue(appended != sequence, "ImmutableArraySequence Append must return new object");
        AssertTrue(prepended != sequence, "ImmutableArraySequence Prepend must return new object");
        AssertTrue(inserted != sequence, "ImmutableArraySequence InsertAt must return new object");

        AssertSequenceContent(sequence, expected_original, 3, "ImmutableArraySequence original content preserved");
        AssertSequenceContent(appended, expected_append, 4, "ImmutableArraySequence Append result");
        AssertSequenceContent(prepended, expected_prepend, 4, "ImmutableArraySequence Prepend result");
        AssertSequenceContent(inserted, expected_insert, 4, "ImmutableArraySequence InsertAt result");

        ListSequenceFactory<int> factory;
        Sequence<int>* mapped = sequence->Map<int>(Square, factory);
        Sequence<int>* filtered = sequence->Where(IsEven);
        AssertSequenceContent(mapped, expected_map, 3, "ImmutableArraySequence Map");
        AssertSequenceContent(filtered, expected_where, 1, "ImmutableArraySequence Where");
        AssertEqual(sequence->Reduce(Sum, 0), 6, "ImmutableArraySequence Reduce");

        int replacement_items[] = {7, 8};
        Sequence<int>* replacement = new ImmutableArraySequence<int>(replacement_items, 2);
        Sequence<int>* sliced = sequence->Slice(1, 1, *replacement);
        AssertSequenceContent(sliced, expected_slice, 4, "ImmutableArraySequence Slice");

        AssertThrows([sequence]() { sequence->GetSubsequence(5, 10); }, "ImmutableArraySequence invalid subsequence must throw");
        AssertThrows([sequence]() { sequence->InsertAt(-1, 1); }, "ImmutableArraySequence invalid insert must throw");
        AssertThrows([sequence, replacement]() { sequence->Slice(0, 10, *replacement); }, "ImmutableArraySequence invalid slice must throw");

        delete sequence;
        delete appended;
        delete prepended;
        delete inserted;
        delete mapped;
        delete filtered;
        delete replacement;
        delete sliced;
    }

    // Проверяет поведение list-based sequence и основные высокоуровневые операции.
    void TestListSequence()
    {
        int items[] = {1, 2, 3};
        int expected_after_insert[] = {0, 1, 99, 2, 3, 4};
        int expected_subsequence[] = {1, 99, 2};
        int expected_map[] = {0, 1, 9801, 4, 9, 16};
        int expected_where[] = {0, 2, 4};
        int expected_slice[] = {0, 7, 8, 4};

        Sequence<int>* sequence = new ListSequence<int>(items, 3);
        sequence = sequence->Append(4);
        sequence = sequence->Prepend(0);
        sequence = sequence->InsertAt(2, 99);
        AssertSequenceContent(sequence, expected_after_insert, 6, "ListSequence content after insert");

        Sequence<int>* subsequence = sequence->GetSubsequence(1, 3);
        AssertSequenceContent(subsequence, expected_subsequence, 3, "ListSequence GetSubsequence");

        ListSequenceFactory<int> factory;
        Sequence<int>* mapped = sequence->Map<int>(Square, factory);
        Sequence<int>* filtered = sequence->Where(IsEven);
        AssertSequenceContent(mapped, expected_map, 6, "ListSequence Map");
        AssertSequenceContent(filtered, expected_where, 3, "ListSequence Where");
        AssertEqual(sequence->Reduce(Sum, 0), 109, "ListSequence Reduce");

        int replacement_items[] = {7, 8};
        Sequence<int>* replacement = new ListSequence<int>(replacement_items, 2);
        Sequence<int>* sliced = sequence->Slice(1, 4, *replacement);
        AssertSequenceContent(sliced, expected_slice, 4, "ListSequence Slice");

        AssertThrows([sequence]() { sequence->GetSubsequence(-1, 1); }, "ListSequence invalid subsequence must throw");
        AssertThrows([sequence]() { sequence->InsertAt(100, 1); }, "ListSequence invalid insert must throw");
        AssertThrows([sequence, replacement]() { sequence->Slice(10, 1, *replacement); }, "ListSequence invalid slice must throw");

        delete sequence;
        delete subsequence;
        delete mapped;
        delete filtered;
        delete replacement;
        delete sliced;
    }

    // Проверяет порядок обхода enumerator и поведение на пустой последовательности.
    void TestEnumerators()
    {
        int items[] = {1, 2, 3};

        Sequence<int>* array_sequence = new MutableArraySequence<int>(items, 3);
        IEnumerator<int>* array_enumerator = array_sequence->GetEnumerator();
        int expected_index = 0;

        while (array_enumerator->MoveNext())
        {
            AssertEqual(array_enumerator->Current(), items[expected_index], "Array enumerator order");
            expected_index++;
        }
        AssertEqual(expected_index, 3, "Array enumerator count");

        Sequence<int>* list_sequence = new ListSequence<int>(items, 3);
        IEnumerator<int>* list_enumerator = list_sequence->GetEnumerator();
        expected_index = 0;

        while (list_enumerator->MoveNext())
        {
            AssertEqual(list_enumerator->Current(), items[expected_index], "List enumerator order");
            expected_index++;
        }
        AssertEqual(expected_index, 3, "List enumerator count");

        Sequence<int>* empty_sequence = new ListSequence<int>();
        IEnumerator<int>* empty_enumerator = empty_sequence->GetEnumerator();
        AssertTrue(!empty_enumerator->MoveNext(), "Empty enumerator must stop immediately");

        delete array_enumerator;
        delete array_sequence;
        delete list_enumerator;
        delete list_sequence;
        delete empty_enumerator;
        delete empty_sequence;
    }

    void TestMapZipUnzip()
    {
        int items[] = {1, 2, 3, 4};
        double expected_half[] = {0.5, 1.0, 1.5, 2.0};
        bool expected_positive[] = {true, true, true, true};
        std::pair<int, int> expected_zip[] = {
            std::make_pair(1, 10),
            std::make_pair(2, 20),
            std::make_pair(3, 30)
        };
        int expected_first_unzip[] = {1, 2, 3};
        int expected_second_unzip[] = {10, 20, 30};

        Sequence<int>* mutable_sequence = new MutableArraySequence<int>(items, 4);
        Sequence<int>* immutable_sequence = new ImmutableArraySequence<int>(items, 4);
        Sequence<int>* list_sequence = new ListSequence<int>(items, 4);
        int other_items[] = {10, 20, 30};
        Sequence<int>* other_sequence = new ListSequence<int>(other_items, 3);
        Sequence<int>* empty_sequence = new ListSequence<int>();

        MutableArraySequenceFactory<double> mutable_double_factory;
        ImmutableArraySequenceFactory<double> immutable_double_factory;
        ListSequenceFactory<bool> bool_factory;
        ListSequenceFactory<std::pair<int, int>> pair_factory;
        ListSequenceFactory<int> int_factory;

        Sequence<double>* mutable_mapped = mutable_sequence->Map<double>(Half, mutable_double_factory);
        Sequence<double>* immutable_mapped = immutable_sequence->Map<double>(Half, immutable_double_factory);
        Sequence<bool>* bool_mapped = list_sequence->Map<bool>(IsPositive, bool_factory);
        Sequence<double>* empty_mapped = empty_sequence->Map<double>(Half, immutable_double_factory);

        AssertTypedSequenceContent(mutable_mapped, expected_half, 4, "Generic Map mutable source to double");
        AssertTypedSequenceContent(immutable_mapped, expected_half, 4, "Generic Map immutable source to double");
        AssertTypedSequenceContent(bool_mapped, expected_positive, 4, "Generic Map list source to bool");
        AssertEqual(empty_mapped->GetLength(), 0, "Generic Map empty sequence");

        Sequence<std::pair<int, int>>* zipped = mutable_sequence->Zip<int>(*other_sequence, pair_factory);
        Sequence<std::pair<int, int>>* zipped_empty = empty_sequence->Zip<int>(*other_sequence, pair_factory);

        AssertTypedSequenceContent(zipped, expected_zip, 3, "Zip must pair elements up to shorter sequence");
        AssertEqual(zipped_empty->GetLength(), 0, "Zip with empty sequence must be empty");

        std::pair<Sequence<int>*, Sequence<int>*> unzipped = zipped->Unzip<int, int>(int_factory, int_factory);
        AssertSequenceContent(unzipped.first, expected_first_unzip, 3, "Unzip first result");
        AssertSequenceContent(unzipped.second, expected_second_unzip, 3, "Unzip second result");

        std::pair<Sequence<int>*, Sequence<int>*> empty_unzipped = zipped_empty->Unzip<int, int>(int_factory, int_factory);
        AssertEqual(empty_unzipped.first->GetLength(), 0, "Unzip first empty result");
        AssertEqual(empty_unzipped.second->GetLength(), 0, "Unzip second empty result");

        delete mutable_sequence;
        delete immutable_sequence;
        delete list_sequence;
        delete other_sequence;
        delete empty_sequence;
        delete mutable_mapped;
        delete immutable_mapped;
        delete bool_mapped;
        delete empty_mapped;
        delete zipped;
        delete zipped_empty;
        delete unzipped.first;
        delete unzipped.second;
        delete empty_unzipped.first;
        delete empty_unzipped.second;
    }

    void TestSharedPtr()
    {
        SharedPtr<int> empty;
        AssertTrue(empty.IsNull(), "SharedPtr default must be null");
        AssertEqual(empty.UseCount(), static_cast<size_t>(0), "SharedPtr empty use count");

        SharedPtr<int> p(new int(5));
        AssertEqual(*p, 5, "SharedPtr raw pointer constructor");
        AssertEqual(p.UseCount(), static_cast<size_t>(1), "SharedPtr initial use count");

        SharedPtr<int> q = p;
        AssertEqual(p.UseCount(), static_cast<size_t>(2), "SharedPtr copy increments source count");
        AssertEqual(q.UseCount(), static_cast<size_t>(2), "SharedPtr copy increments target count");

        q.Reset();
        AssertTrue(q.IsNull(), "SharedPtr reset makes target null");
        AssertEqual(p.UseCount(), static_cast<size_t>(1), "SharedPtr reset decrements count");

        SharedPtr<int> moved(std::move(p));
        AssertTrue(p.IsNull(), "SharedPtr move constructor clears source");
        AssertEqual(moved.UseCount(), static_cast<size_t>(1), "SharedPtr move constructor preserves count");

        SharedPtr<int> assigned;
        assigned = std::move(moved);
        AssertTrue(moved.IsNull(), "SharedPtr move assignment clears source");
        AssertEqual(assigned.UseCount(), static_cast<size_t>(1), "SharedPtr move assignment preserves count");

        int deletions = 0;
        {
            SharedPtr<SharedPtrProbe> first(new SharedPtrProbe(&deletions));
            {
                SharedPtr<SharedPtrProbe> second = first;
                AssertEqual(first.UseCount(), static_cast<size_t>(2), "SharedPtr probe shared count");
            }
            AssertEqual(deletions, 0, "SharedPtr must not delete while references remain");
        }
        AssertEqual(deletions, 1, "SharedPtr must delete exactly once");

        assigned.Reset();
        AssertTrue(assigned.IsNull(), "SharedPtr final reset");
    }

    void TestCardinalOrdinal()
    {
        Cardinal finite = Cardinal::Finite(5);
        Cardinal infinity = Cardinal::CountableInfinity();

        AssertTrue(finite.IsFinite(), "Finite cardinal must report finite");
        AssertTrue(infinity.IsInfinite(), "Infinite cardinal must report infinite");
        AssertTrue(finite < infinity, "Finite cardinal must be less than countable infinity");
        AssertEqual(finite.ToInt(), 5, "Cardinal ToInt");
        AssertThrows([&infinity]() { infinity.ToSizeT(); }, "Infinite cardinal ToSizeT must throw");

        AssertTrue(Ordinal::Finite(5) < Ordinal::Omega(), "Finite ordinal must be before omega");
        AssertTrue(Ordinal::Omega() < Ordinal::OmegaPlus(1), "Omega must be before omega+1");
        AssertTrue(Ordinal::OmegaPlus(100) < Ordinal::OmegaTimes(2), "Omega+n must be before omega*2");
        AssertTrue(Ordinal::OmegaTimes(2) < Ordinal::OmegaTimesPlus(2, 1), "omega*2 must be before omega*2+1");

        AssertTrue(Ordinal::Finite(3).Add(Ordinal::Finite(4)) == Ordinal::Finite(7), "Finite ordinal addition");
        AssertTrue(Ordinal::Finite(3).Add(Ordinal::Omega()) == Ordinal::Omega(), "Finite plus omega");
        AssertTrue(Ordinal::Omega().Add(Ordinal::Finite(1)) == Ordinal::OmegaPlus(1), "Omega plus finite");
        AssertTrue(Ordinal::OmegaPlus(2).Add(Ordinal::Finite(3)) == Ordinal::OmegaPlus(5), "Omega+n plus finite");
        AssertTrue(Ordinal::OmegaPlus(2).Add(Ordinal::Omega()) == Ordinal::OmegaTimes(2), "Omega+n plus omega");
        AssertTrue(Ordinal::OmegaTimesPlus(2, 5).Add(Ordinal::Omega()) == Ordinal::OmegaTimes(3), "Omega*k+n plus omega");

        AssertTrue(Ordinal::Omega().IsLimit(), "Omega must be limit");
        AssertTrue(Ordinal::OmegaTimes(2).IsLimit(), "Omega*2 must be limit");
        AssertTrue(Ordinal::OmegaPlus(1).IsSuccessor(), "Omega+1 must be successor");

        std::optional<Ordinal> predecessor = Ordinal::OmegaPlus(3).Predecessor();
        AssertTrue(predecessor.has_value(), "Omega+3 predecessor must exist");
        AssertTrue(predecessor.value() == Ordinal::OmegaPlus(2), "Omega+3 predecessor value");
        AssertTrue(!Ordinal::Omega().Predecessor().has_value(), "Omega predecessor must not exist");
        AssertTrue(!Ordinal::Zero().Predecessor().has_value(), "Zero predecessor must not exist");

        AssertTrue(Ordinal::OmegaTimesPlus(2, 7).ToCardinal() == Cardinal::CountableInfinity(), "Transfinite ordinal cardinal");
        AssertEqual(Ordinal::OmegaTimesPlus(2, 7).ToString(), std::string("omega*2+7"), "Ordinal ToString");
    }

    void TestOrdinalRemovePrefix()
    {
        AssertTrue(
            Ordinal::Finite(10).RemovePrefix(Ordinal::Finite(3)) == Ordinal::Finite(7),
            "RemovePrefix finite from finite");
        AssertTrue(
            Ordinal::Omega().RemovePrefix(Ordinal::Finite(3)) == Ordinal::Omega(),
            "RemovePrefix finite prefix from omega");
        AssertTrue(
            Ordinal::OmegaPlus(5).RemovePrefix(Ordinal::OmegaPlus(2)) == Ordinal::Finite(3),
            "RemovePrefix omega+n from omega+m");
        AssertTrue(
            Ordinal::OmegaTimesPlus(2, 5).RemovePrefix(Ordinal::OmegaPlus(7)) == Ordinal::OmegaPlus(5),
            "RemovePrefix omega+n from omega*2+m");
        AssertTrue(
            Ordinal::OmegaPlus(3).RemovePrefix(Ordinal::OmegaPlus(3)) == Ordinal::Zero(),
            "RemovePrefix equal prefix");
        AssertThrowsExact<std::out_of_range>(
            []() { Ordinal::Finite(3).RemovePrefix(Ordinal::Finite(4)); },
            "RemovePrefix prefix greater than total must throw");
    }

    void TestFiniteNode()
    {
        FiniteNode<int> empty;
        AssertTrue(empty.GetOrdinalLength() == Ordinal::Zero(), "Empty FiniteNode ordinal length");
        AssertTrue(empty.GetCardinalLength() == Cardinal::Finite(0), "Empty FiniteNode cardinal length");
        AssertEqual(empty.GetMaterializedCount(), static_cast<size_t>(0), "Empty FiniteNode materialized count");
        AssertThrows([&empty]() { empty.Get(Ordinal::Zero()); }, "Empty FiniteNode Get must throw");

        int items[] = {1, 2, 3};
        FiniteNode<int> finite(items, 3);
        AssertEqual(finite.Get(Ordinal::Finite(0)), 1, "FiniteNode first item");
        AssertEqual(finite.Get(Ordinal::Finite(2)), 3, "FiniteNode last item");
        AssertTrue(finite.GetOrdinalLength() == Ordinal::Finite(3), "FiniteNode ordinal length");
        AssertEqual(finite.GetMaterializedCount(), static_cast<size_t>(3), "FiniteNode materialized count");
        AssertThrows([&finite]() { finite.Get(Ordinal::Omega()); }, "FiniteNode omega index must throw");
        AssertThrows([&finite]() { finite.Get(Ordinal::Finite(3)); }, "FiniteNode out of range must throw");

        MutableArraySequence<int> source(items, 3);
        FiniteNode<int> copied(&source);
        source.Prepend(99);
        AssertEqual(copied.Get(Ordinal::Zero()), 1, "FiniteNode must copy source sequence");
    }

    void TestRecurrenceNode()
    {
        int initial_items[] = {0};
        MutableArraySequence<int> initial(initial_items, 1);
        RecurrenceNode<int> naturals(
            [](Sequence<int>* prefix) -> int
            {
                int length = prefix->GetLength();
                return (length == 0) ? 0 : prefix->Get(length - 1) + 1;
            },
            &initial);

        AssertTrue(naturals.GetOrdinalLength() == Ordinal::Omega(), "RecurrenceNode ordinal length");
        AssertTrue(naturals.GetCardinalLength() == Cardinal::CountableInfinity(), "RecurrenceNode cardinal length");
        AssertEqual(naturals.GetMaterializedCount(), static_cast<size_t>(1), "RecurrenceNode initial materialized count");
        AssertEqual(naturals.Get(Ordinal::Finite(0)), 0, "RecurrenceNode Get(0)");
        AssertEqual(naturals.Get(Ordinal::Finite(10)), 10, "RecurrenceNode Get(10)");
        AssertEqual(naturals.GetMaterializedCount(), static_cast<size_t>(11), "RecurrenceNode lazy materialization");
        AssertEqual(naturals.Get(Ordinal::Finite(1000)), 1000, "RecurrenceNode Get(1000)");
        AssertEqual(naturals.GetMaterializedCount(), static_cast<size_t>(1001), "RecurrenceNode materialized count after large get");
        AssertThrows([&naturals]() { naturals.Get(Ordinal::Omega()); }, "RecurrenceNode omega index must throw");

        UniquePtr<LazySequence<int>> fibonacci(LazySequence<int>::Fibonacci());
        UniquePtr<LazySequence<int>> factorials(LazySequence<int>::Factorials());
        AssertEqual(fibonacci->Get(10), 55, "LazySequence Fibonacci");
        AssertEqual(factorials->Get(5), 120, "LazySequence Factorials");
    }

    void TestInsertNode()
    {
        int source_items[] = {1, 2, 3, 4};
        int inserted_items[] = {9, 8};

        SharedPtr<LazyNode<int>> source(new FiniteNode<int>(source_items, 4));
        SharedPtr<LazyNode<int>> inserted(new FiniteNode<int>(inserted_items, 2));
        InsertNode<int> node(source, inserted, Ordinal::Finite(2));

        AssertTrue(node.GetOrdinalLength() == Ordinal::Finite(6), "InsertNode finite result length");
        AssertEqual(node.Get(Ordinal::Finite(0)), 1, "InsertNode prefix item");
        AssertEqual(node.Get(Ordinal::Finite(2)), 9, "InsertNode inserted first item");
        AssertEqual(node.Get(Ordinal::Finite(3)), 8, "InsertNode inserted second item");
        AssertEqual(node.Get(Ordinal::Finite(4)), 3, "InsertNode suffix item");
        AssertThrows([&node]() { node.Get(Ordinal::Finite(6)); }, "InsertNode out of range must throw");

        AssertThrowsExact<std::out_of_range>(
            [&source, &inserted]() { InsertNode<int> invalid(source, inserted, Ordinal::Finite(5)); },
            "InsertNode invalid position must throw");
    }

    void TestConcatNodeAndLazySequence()
    {
        int left_items[] = {1, 2, 3};
        int right_items[] = {4, 5};
        LazySequence<int> finite_left(left_items, 3);
        LazySequence<int> finite_right(right_items, 2);

        UniquePtr<Sequence<int>> finite_concat_base(finite_left.Concat(finite_right));
        LazySequence<int>* finite_concat = dynamic_cast<LazySequence<int>*>(finite_concat_base.get());
        AssertTrue(finite_concat != nullptr, "Concat finite+finite must return LazySequence");
        AssertEqual(finite_concat->Get(0), 1, "finite+finite first");
        AssertEqual(finite_concat->Get(3), 4, "finite+finite right first");
        AssertTrue(finite_concat->GetOrdinalLength() == Ordinal::Finite(5), "finite+finite length");

        int prefix_items[] = {10, 20, 30};
        LazySequence<int> prefix(prefix_items, 3);
        UniquePtr<LazySequence<int>> naturals_a(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> finite_omega_base(prefix.Concat(*naturals_a));
        LazySequence<int>* finite_omega = dynamic_cast<LazySequence<int>*>(finite_omega_base.get());
        AssertEqual(finite_omega->Get(0), 10, "finite+omega finite item");
        AssertEqual(finite_omega->Get(3), 0, "finite+omega first generated item");
        AssertEqual(finite_omega->Get(10), 7, "finite+omega residual index");
        AssertTrue(finite_omega->GetOrdinalLength() == Ordinal::Omega(), "finite+omega length");

        UniquePtr<LazySequence<int>> naturals_b(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> omega_finite_base(naturals_b->Concat(finite_right));
        LazySequence<int>* omega_finite = dynamic_cast<LazySequence<int>*>(omega_finite_base.get());
        AssertEqual(omega_finite->Get(0), 0, "omega+finite first");
        AssertEqual(omega_finite->Get(Ordinal::Omega()), 4, "omega+finite omega item");
        AssertEqual(omega_finite->Get(Ordinal::OmegaPlus(1)), 5, "omega+finite omega+1 item");
        AssertTrue(omega_finite->GetOrdinalLength() == Ordinal::OmegaPlus(2), "omega+finite length");

        UniquePtr<LazySequence<int>> naturals_c(LazySequence<int>::Naturals());
        UniquePtr<LazySequence<int>> naturals_d(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> omega_omega_base(naturals_c->Concat(*naturals_d));
        LazySequence<int>* omega_omega = dynamic_cast<LazySequence<int>*>(omega_omega_base.get());
        AssertEqual(omega_omega->Get(0), 0, "omega+omega first");
        AssertEqual(omega_omega->Get(Ordinal::Omega()), 0, "omega+omega second block first");
        AssertEqual(omega_omega->Get(Ordinal::OmegaPlus(5)), 5, "omega+omega second block residual");
        AssertTrue(omega_omega->GetOrdinalLength() == Ordinal::OmegaTimes(2), "omega+omega length");
        AssertThrows([omega_omega]() { omega_omega->GetLast(); }, "omega+omega GetLast must throw");

        UniquePtr<LazySequence<int>> naturals_e(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> appended_once_base(naturals_e->Append(100));
        LazySequence<int>* appended_once = dynamic_cast<LazySequence<int>*>(appended_once_base.get());
        UniquePtr<Sequence<int>> appended_twice_base(appended_once->Append(200));
        LazySequence<int>* appended_twice = dynamic_cast<LazySequence<int>*>(appended_twice_base.get());
        UniquePtr<LazySequence<int>> naturals_f(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> appended_concat_base(appended_twice->Concat(*naturals_f));
        LazySequence<int>* appended_concat = dynamic_cast<LazySequence<int>*>(appended_concat_base.get());
        AssertEqual(appended_concat->Get(Ordinal::Omega()), 100, "(omega+2)+omega first appended");
        AssertEqual(appended_concat->Get(Ordinal::OmegaPlus(1)), 200, "(omega+2)+omega second appended");
        AssertEqual(appended_concat->Get(Ordinal::OmegaPlus(2)), 0, "(omega+2)+omega right first");
        AssertTrue(appended_concat->GetOrdinalLength() == Ordinal::OmegaTimes(2), "(omega+2)+omega length");
    }

    void TestLazySequence()
    {
        int items[] = {1, 2, 3};
        LazySequence<int> finite(items, 3);
        AssertEqual(finite.Get(0), 1, "Lazy finite Get(0)");
        AssertEqual(finite.GetLast(), 3, "Lazy finite GetLast");
        AssertTrue(finite.GetCardinalLength() == Cardinal::Finite(3), "Lazy finite cardinal length");

        UniquePtr<Sequence<int>> appended_finite_base(finite.Append(4));
        LazySequence<int>* appended_finite = dynamic_cast<LazySequence<int>*>(appended_finite_base.get());
        AssertEqual(appended_finite->Get(3), 4, "Lazy finite append item");
        AssertTrue(appended_finite->GetOrdinalLength() == Ordinal::Finite(4), "Lazy finite append length");
        AssertEqual(finite.GetLength(), 3, "Lazy append must preserve source");

        UniquePtr<LazySequence<int>> naturals(LazySequence<int>::Naturals());
        AssertEqual(naturals->Get(0), 0, "Naturals Get(0)");
        AssertEqual(naturals->Get(10), 10, "Naturals Get(10)");
        AssertEqual(naturals->Get(1000), 1000, "Naturals Get(1000)");
        AssertTrue(naturals->GetOrdinalLength() == Ordinal::Omega(), "Naturals length");
        AssertThrows([&naturals]() { naturals->GetLength(); }, "GetLength for omega must throw");

        UniquePtr<LazySequence<int>> naturals_for_append(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> appended_base(naturals_for_append->Append(999));
        LazySequence<int>* appended = dynamic_cast<LazySequence<int>*>(appended_base.get());
        AssertTrue(appended->GetOrdinalLength() == Ordinal::OmegaPlus(1), "Append after omega length");
        AssertEqual(appended->Get(0), 0, "Append after omega first");
        AssertEqual(appended->Get(100), 100, "Append after omega finite index");
        AssertEqual(appended->Get(Ordinal::Omega()), 999, "Append after omega ordinal index");
        AssertEqual(appended->GetLast(), 999, "Append after omega GetLast");

        UniquePtr<Sequence<int>> appended_twice_base(appended->Append(20));
        LazySequence<int>* appended_twice = dynamic_cast<LazySequence<int>*>(appended_twice_base.get());
        AssertTrue(appended_twice->GetOrdinalLength() == Ordinal::OmegaPlus(2), "Double append after omega length");
        AssertEqual(appended_twice->Get(Ordinal::Omega()), 999, "Double append first ordinal item");
        AssertEqual(appended_twice->Get(Ordinal::OmegaPlus(1)), 20, "Double append second ordinal item");

        UniquePtr<LazySequence<int>> naturals_for_prepend(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> prepended_base(naturals_for_prepend->Prepend(-1));
        LazySequence<int>* prepended = dynamic_cast<LazySequence<int>*>(prepended_base.get());
        AssertTrue(prepended->GetOrdinalLength() == Ordinal::Omega(), "Prepend to omega length");
        AssertEqual(prepended->Get(0), -1, "Prepend to omega first");
        AssertEqual(prepended->Get(1), 0, "Prepend to omega shifted first natural");
        AssertEqual(prepended->Get(2), 1, "Prepend to omega shifted second natural");

        UniquePtr<Sequence<int>> subsequence_base(prepended->GetSubsequence(1, 3));
        int expected_subsequence[] = {0, 1, 2};
        AssertSequenceContent(subsequence_base.get(), expected_subsequence, 3, "Lazy GetSubsequence finite window from omega");

        AssertThrows([prepended]() { prepended->Where(IsEven); }, "Where for omega must throw");

        UniquePtr<IEnumerator<int>> enumerator(appended->GetEnumerator());
        for (int expected = 0; expected < 5; expected++)
        {
            AssertTrue(enumerator->MoveNext(), "Lazy enumerator must move on omega finite prefix");
            AssertEqual(enumerator->Current(), expected, "Lazy enumerator finite index order");
        }
    }

    void TestLazySequenceInsertAtFinite()
    {
        int items[] = {1, 2, 3};
        LazySequence<int> finite(items, 3);

        UniquePtr<Sequence<int>> start_base(finite.InsertAt(0, 10));
        int expected_start[] = {10, 1, 2, 3};
        AssertSequenceContent(start_base.get(), expected_start, 4, "Lazy InsertAt item at start");

        UniquePtr<Sequence<int>> middle_base(finite.InsertAt(1, 20));
        int expected_middle[] = {1, 20, 2, 3};
        AssertSequenceContent(middle_base.get(), expected_middle, 4, "Lazy InsertAt item in middle");

        UniquePtr<Sequence<int>> end_base(finite.InsertAt(3, 30));
        int expected_end[] = {1, 2, 3, 30};
        AssertSequenceContent(end_base.get(), expected_end, 4, "Lazy InsertAt item at end");

        int inserted_items[] = {7, 8};
        MutableArraySequence<int> inserted_sequence(inserted_items, 2);
        UniquePtr<LazySequence<int>> sequence_insert(finite.InsertAt(1, inserted_sequence));
        int expected_sequence_insert[] = {1, 7, 8, 2, 3};
        AssertSequenceContent(sequence_insert.get(), expected_sequence_insert, 5, "Lazy InsertAt finite Sequence");

        inserted_sequence.Prepend(99);
        AssertSequenceContent(sequence_insert.get(), expected_sequence_insert, 5, "Lazy InsertAt must copy mutable Sequence");

        int expected_original[] = {1, 2, 3};
        AssertSequenceContent(&finite, expected_original, 3, "Lazy InsertAt must preserve source");

        LazySequence<int> empty_inserted;
        UniquePtr<LazySequence<int>> empty_result(finite.InsertAt(1, empty_inserted));
        AssertSequenceContent(empty_result.get(), expected_original, 3, "Lazy InsertAt empty Sequence");

        int self_items[] = {1, 2};
        LazySequence<int> self_source(self_items, 2);
        UniquePtr<LazySequence<int>> self_result(self_source.InsertAt(1, self_source));
        int expected_self[] = {1, 1, 2, 2};
        AssertSequenceContent(self_result.get(), expected_self, 4, "Lazy InsertAt self finite");

        LazySequence<int> empty_source;
        UniquePtr<LazySequence<int>> inserted_into_empty(empty_source.InsertAt(Ordinal::Zero(), 42));
        AssertEqual(inserted_into_empty->GetLength(), 1, "Lazy InsertAt empty source length");
        AssertEqual(inserted_into_empty->Get(0), 42, "Lazy InsertAt empty source item");
        AssertThrows([&empty_source]() { empty_source.InsertAt(Ordinal::Finite(1), 42); }, "Lazy InsertAt empty source invalid ordinal");
        AssertThrows([&finite]() { finite.InsertAt(-1, 10); }, "Lazy InsertAt negative int position must throw");
        AssertThrows([&finite]() { finite.InsertAt(Ordinal::Finite(4), 10); }, "Lazy InsertAt ordinal position out of range");
        AssertThrows([&middle_base]() { middle_base->Get(4); }, "Lazy InsertAt Get past result must throw");
    }

    void TestLazySequenceInsertAtInfinite()
    {
        UniquePtr<LazySequence<int>> naturals_for_item(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> item_insert_base(naturals_for_item->InsertAt(3, 100));
        LazySequence<int>* item_insert = dynamic_cast<LazySequence<int>*>(item_insert_base.get());
        AssertTrue(item_insert != nullptr, "Lazy InsertAt item in omega must return LazySequence");
        AssertTrue(item_insert->GetOrdinalLength() == Ordinal::Omega(), "Lazy InsertAt finite item in omega length");
        AssertEqual(item_insert->Get(3), 100, "Lazy InsertAt item in omega inserted item");
        AssertEqual(item_insert->Get(4), 3, "Lazy InsertAt item in omega suffix first");
        AssertEqual(item_insert->Get(100), 99, "Lazy InsertAt item in omega far suffix");

        int finite_items[] = {50, 51};
        LazySequence<int> finite_inserted(finite_items, 2);
        UniquePtr<LazySequence<int>> naturals_for_sequence(LazySequence<int>::Naturals());
        UniquePtr<LazySequence<int>> sequence_insert(naturals_for_sequence->InsertAt(3, finite_inserted));
        AssertTrue(sequence_insert->GetOrdinalLength() == Ordinal::Omega(), "Lazy InsertAt finite Sequence in omega length");
        AssertEqual(sequence_insert->Get(3), 50, "Lazy InsertAt finite Sequence first inserted");
        AssertEqual(sequence_insert->Get(4), 51, "Lazy InsertAt finite Sequence second inserted");
        AssertEqual(sequence_insert->Get(5), 3, "Lazy InsertAt finite Sequence suffix first");

        UniquePtr<LazySequence<int>> source_naturals(LazySequence<int>::Naturals());
        UniquePtr<LazySequence<int>> inserted_naturals(LazySequence<int>::Naturals());
        UniquePtr<LazySequence<int>> omega_insert(source_naturals->InsertAt(3, *inserted_naturals));
        AssertTrue(omega_insert->GetOrdinalLength() == Ordinal::OmegaTimes(2), "Lazy InsertAt omega into omega length");
        AssertEqual(omega_insert->Get(3), 0, "Lazy InsertAt omega inserted first");
        AssertEqual(omega_insert->Get(Ordinal::Omega()), 3, "Lazy InsertAt omega suffix first");
        AssertEqual(omega_insert->Get(Ordinal::OmegaPlus(5)), 8, "Lazy InsertAt omega suffix offset");

        UniquePtr<LazySequence<int>> tail_source_base(LazySequence<int>::Naturals());
        UniquePtr<Sequence<int>> tail_one_base(tail_source_base->Append(100));
        LazySequence<int>* tail_one = dynamic_cast<LazySequence<int>*>(tail_one_base.get());
        UniquePtr<Sequence<int>> tail_two_base(tail_one->Append(200));
        LazySequence<int>* tail_two = dynamic_cast<LazySequence<int>*>(tail_two_base.get());

        UniquePtr<LazySequence<int>> omega_for_position(LazySequence<int>::Naturals());
        UniquePtr<LazySequence<int>> insert_at_omega(tail_two->InsertAt(Ordinal::Omega(), *omega_for_position));
        AssertTrue(insert_at_omega->GetOrdinalLength() == Ordinal::OmegaTimesPlus(2, 2), "Lazy InsertAt position omega length");
        AssertEqual(insert_at_omega->Get(Ordinal::Omega()), 0, "Lazy InsertAt position omega inserted first");
        AssertEqual(insert_at_omega->Get(Ordinal::OmegaPlus(5)), 5, "Lazy InsertAt position omega inserted offset");
        AssertEqual(insert_at_omega->Get(Ordinal::OmegaTimes(2)), 100, "Lazy InsertAt position omega suffix first");
        AssertEqual(insert_at_omega->Get(Ordinal::OmegaTimesPlus(2, 1)), 200, "Lazy InsertAt position omega suffix second");

        UniquePtr<LazySequence<int>> item_at_omega_plus_one(tail_two->InsertAt(Ordinal::OmegaPlus(1), 999));
        AssertTrue(item_at_omega_plus_one->GetOrdinalLength() == Ordinal::OmegaPlus(3), "Lazy InsertAt omega+n item length");
        AssertEqual(item_at_omega_plus_one->Get(Ordinal::Omega()), 100, "Lazy InsertAt omega+n prefix tail");
        AssertEqual(item_at_omega_plus_one->Get(Ordinal::OmegaPlus(1)), 999, "Lazy InsertAt omega+n inserted item");
        AssertEqual(item_at_omega_plus_one->Get(Ordinal::OmegaPlus(2)), 200, "Lazy InsertAt omega+n suffix item");

        UniquePtr<LazySequence<int>> append_by_insert_source(LazySequence<int>::Naturals());
        UniquePtr<LazySequence<int>> append_by_insert(append_by_insert_source->InsertAt(Ordinal::Omega(), 777));
        AssertTrue(append_by_insert->GetOrdinalLength() == Ordinal::OmegaPlus(1), "Lazy InsertAt at omega end length");
        AssertEqual(append_by_insert->Get(Ordinal::Omega()), 777, "Lazy InsertAt at omega end item");

        UniquePtr<LazySequence<int>> self_naturals(LazySequence<int>::Naturals());
        UniquePtr<LazySequence<int>> self_insert(self_naturals->InsertAt(3, *self_naturals));
        AssertTrue(self_insert->GetOrdinalLength() == Ordinal::OmegaTimes(2), "Lazy InsertAt self omega length");
        AssertEqual(self_insert->Get(3), 0, "Lazy InsertAt self omega inserted first");
        AssertEqual(self_insert->Get(Ordinal::Omega()), 3, "Lazy InsertAt self omega suffix first");

        AssertThrowsExact<std::out_of_range>(
            [&item_at_omega_plus_one]() { item_at_omega_plus_one->Get(Ordinal::OmegaPlus(3)); },
            "Lazy InsertAt Get past transfinite result must throw");
    }

    void TestLazySequenceInsertAtSharesCache()
    {
        UniquePtr<LazySequence<int>> source(LazySequence<int>::Naturals());
        size_t before = source->GetMaterializedCount();

        UniquePtr<Sequence<int>> inserted_base(source->InsertAt(3, 100));
        LazySequence<int>* inserted = dynamic_cast<LazySequence<int>*>(inserted_base.get());
        AssertEqual(inserted->Get(100), 99, "Lazy InsertAt shared cache result value");

        size_t after = source->GetMaterializedCount();
        AssertTrue(after > before, "Lazy InsertAt must share RecurrenceNode cache with source");
    }

    void TestLazySequenceSliceStubs()
    {
        int items[] = {1, 2, 3};
        LazySequence<int> finite(items, 3);
        LazySequence<int> replacement(items, 3);

        AssertThrowsExact<std::logic_error>(
            [&finite]() { finite.Slice(0, 1); },
            "Lazy Slice finite must throw logic_error");
        AssertThrowsExact<std::logic_error>(
            [&finite, &replacement]() { finite.Slice(0, 1, replacement); },
            "Lazy Slice replacement finite must throw logic_error");

        UniquePtr<LazySequence<int>> naturals(LazySequence<int>::Naturals());
        AssertThrowsExact<std::logic_error>(
            [&naturals]() { naturals->Slice(0, 1); },
            "Lazy Slice omega must throw logic_error");
        AssertThrowsExact<std::logic_error>(
            [&naturals, &replacement]() { naturals->Slice(0, 1, replacement); },
            "Lazy Slice replacement omega must throw logic_error");
    }
}

// Запускает полный локальный набор тестов ядра лабы.
void RunAllTests()
{
    TestDynamicArray();
    TestLinkedList();
    TestMutableArraySequence();
    TestImmutableArraySequence();
    TestListSequence();
    TestEnumerators();
    TestMapZipUnzip();
    TestSharedPtr();
    TestCardinalOrdinal();
    TestOrdinalRemovePrefix();
    TestFiniteNode();
    TestRecurrenceNode();
    TestInsertNode();
    TestConcatNodeAndLazySequence();
    TestLazySequence();
    TestLazySequenceInsertAtFinite();
    TestLazySequenceInsertAtInfinite();
    TestLazySequenceInsertAtSharesCache();
    TestLazySequenceSliceStubs();

    std::cout << "All tests passed." << std::endl;
}
