#pragma once

#include <Memory/MemoryError.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>

namespace Memory
{
    template<typename Domain>
    class Address final
    {
    public:
        constexpr Address() noexcept = default;

        explicit constexpr Address(
            const std::uint64_t Value
        ) noexcept : Value_{ Value }
        {
        }

        [[nodiscard]]
        constexpr auto Value() const noexcept -> std::uint64_t
        {
            return Value_;
        }

        [[nodiscard]]
        constexpr auto Offset(
            const std::size_t Amount
        ) const noexcept -> std::expected<Address, MemoryError>
        {
            if (Amount > (std::numeric_limits<std::uint64_t>::max)() - Value_)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::AddressOverflow,
                        .FaultAddress = Value_
                    }
                };
            }

            return Address
            {
                Value_ + static_cast<std::uint64_t>(Amount)
            };
        }

        [[nodiscard]]
        constexpr auto AlignDown(
            const std::size_t Alignment
        ) const noexcept -> std::expected<Address, MemoryError>
        {
            if (!IsPowerOfTwo(Alignment))
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::InvalidAlignment,
                        .FaultAddress = Value_
                    }
                };
            }

            return Address
            {
                Value_ & ~static_cast<std::uint64_t>(Alignment - 1)
            };
        }

        [[nodiscard]]
        constexpr auto AlignUp(
            const std::size_t Alignment
        ) const noexcept -> std::expected<Address, MemoryError>
        {
            if (!IsPowerOfTwo(Alignment))
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::InvalidAlignment,
                        .FaultAddress = Value_
                    }
                };
            }

            const auto Mask = static_cast<std::uint64_t>(Alignment - 1);

            if (Value_ > (std::numeric_limits<std::uint64_t>::max)() - Mask)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::AddressOverflow,
                        .FaultAddress = Value_
                    }
                };
            }

            return Address
            {
                (Value_ + Mask) & ~Mask
            };
        }

        [[nodiscard]]
        constexpr auto IsAligned(
            const std::size_t Alignment
        ) const noexcept -> bool
        {
            return IsPowerOfTwo(Alignment) && (Value_ & static_cast<std::uint64_t>(Alignment - 1)) == 0;
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return Value_ != 0;
        }

        constexpr auto operator<=>(const Address&) const noexcept = default;

    private:
        [[nodiscard]]
        static constexpr auto IsPowerOfTwo(
            const std::size_t Value
        ) noexcept -> bool
        {
            return Value != 0 && (Value & (Value - 1)) == 0;
        }

        std::uint64_t Value_{};
    };
}
