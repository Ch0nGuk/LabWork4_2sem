#ifndef CARDINAL_H
#define CARDINAL_H

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>

class Cardinal
{
public:
    static Cardinal Finite(size_t value)
    {
        return Cardinal(false, value);
    }

    static Cardinal Finite(int value)
    {
        if (value < 0)
        {
            throw std::out_of_range("Negative cardinal");
        }

        return Cardinal(false, static_cast<size_t>(value));
    }

    static Cardinal CountableInfinity()
    {
        return Cardinal(true, 0);
    }

    bool IsFinite() const
    {
        return !is_infinite;
    }

    bool IsInfinite() const
    {
        return is_infinite;
    }

    size_t ToSizeT() const
    {
        if (is_infinite)
        {
            throw std::logic_error("Infinite cardinal cannot be converted to size_t");
        }

        return value;
    }

    int ToInt() const
    {
        if (is_infinite)
        {
            throw std::logic_error("Infinite cardinal cannot be converted to int");
        }

        if (value > static_cast<size_t>(std::numeric_limits<int>::max()))
        {
            throw std::overflow_error("Cardinal does not fit into int");
        }

        return static_cast<int>(value);
    }
    
    bool operator==(const Cardinal& other) const
    {
        return Compare(other) == 0;
    }
    
    bool operator!=(const Cardinal& other) const
    {
        return Compare(other) != 0;
    }
    
    bool operator<(const Cardinal& other) const
    {
        return Compare(other) < 0;
    }
    
    bool operator<=(const Cardinal& other) const
    {
        return Compare(other) <= 0;
    }
    
    bool operator>(const Cardinal& other) const
    {
        return Compare(other) > 0;
    }
    
    bool operator>=(const Cardinal& other) const
    {
        return Compare(other) >= 0;
    }
    
    std::string ToString() const
    {
        return is_infinite ? "countable infinity" : std::to_string(value);
    }
    
    private:

    int Compare(const Cardinal& other) const
    {
        if (is_infinite)
        {
            return other.is_infinite ? 0 : 1;
        }

        if (other.is_infinite)
        {
            return -1;
        }

        if (value < other.value)
        {
            return -1;
        }

        if (value > other.value)
        {
            return 1;
        }

        return 0;
    }

    bool is_infinite;
    size_t value;

    Cardinal(bool infinite, size_t finite_value) : is_infinite(infinite), value(infinite ? 0 : finite_value) {}
};

#endif // CARDINAL_H
