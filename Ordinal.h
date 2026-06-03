#ifndef ORDINAL_H
#define ORDINAL_H

#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

#include "Cardinal.h"

class Ordinal
{
public:
    static Ordinal Zero()
    {
        return Ordinal(0, 0);
    }

    static Ordinal Finite(size_t n)
    {
        return Ordinal(0, n);
    }

    static Ordinal Omega()
    {
        return Ordinal(1, 0);
    }

    static Ordinal OmegaPlus(size_t n)
    {
        return Ordinal(1, n);
    }

    static Ordinal OmegaTimes(size_t k)
    {
        return Ordinal(k, 0);
    }

    static Ordinal OmegaTimesPlus(size_t k, size_t n)
    {
        return Ordinal(k, n);
    }

    bool IsZero() const
    {
        return omega_coeff == 0 && finite_part == 0;
    }

    bool IsFinite() const
    {
        return omega_coeff == 0;
    }

    bool IsLimit() const
    {
        return omega_coeff > 0 && finite_part == 0;
    }

    bool IsSuccessor() const
    {
        return finite_part > 0;
    }

    size_t GetOmegaCoeff() const
    {
        return omega_coeff;
    }

    size_t GetFinitePart() const
    {
        return finite_part;
    }

    int Compare(const Ordinal& other) const
    {
        if (omega_coeff < other.omega_coeff)
        {
            return -1;
        }

        if (omega_coeff > other.omega_coeff)
        {
            return 1;
        }

        if (finite_part < other.finite_part)
        {
            return -1;
        }

        if (finite_part > other.finite_part)
        {
            return 1;
        }

        return 0;
    }

    Ordinal Add(const Ordinal& rhs) const
    {
        if (rhs.omega_coeff == 0)
        {
            return Ordinal(omega_coeff, CheckedAdd(finite_part, rhs.finite_part));
        }

        return Ordinal(CheckedAdd(omega_coeff, rhs.omega_coeff), rhs.finite_part);
    }

    std::optional<Ordinal> Predecessor() const
    {
        if (finite_part == 0)
        {
            return std::optional<Ordinal>();
        }

        return Ordinal(omega_coeff, finite_part - 1);
    }

    Cardinal ToCardinal() const
    {
        return IsFinite() ? Cardinal::Finite(finite_part) : Cardinal::CountableInfinity();
    }

    bool operator==(const Ordinal& other) const
    {
        return Compare(other) == 0;
    }

    bool operator!=(const Ordinal& other) const
    {
        return Compare(other) != 0;
    }

    bool operator<(const Ordinal& other) const
    {
        return Compare(other) < 0;
    }

    bool operator<=(const Ordinal& other) const
    {
        return Compare(other) <= 0;
    }

    bool operator>(const Ordinal& other) const
    {
        return Compare(other) > 0;
    }

    bool operator>=(const Ordinal& other) const
    {
        return Compare(other) >= 0;
    }

    std::string ToString() const
    {
        if (omega_coeff == 0)
        {
            return std::to_string(finite_part);
        }

        std::string result = "omega";
        if (omega_coeff > 1)
        {
            result += "*" + std::to_string(omega_coeff);
        }

        if (finite_part > 0)
        {
            result += "+" + std::to_string(finite_part);
        }

        return result;
    }

private:
    size_t omega_coeff;
    size_t finite_part;

    Ordinal(size_t omega, size_t finite) : omega_coeff(omega), finite_part(finite) {}

    static size_t CheckedAdd(size_t left, size_t right)
    {
        if (right > std::numeric_limits<size_t>::max() - left)
        {
            throw std::overflow_error("Ordinal addition overflow");
        }

        return left + right;
    }
};

#endif // ORDINAL_H
