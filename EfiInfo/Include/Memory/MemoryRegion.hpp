#pragma once

#include <Memory/MemoryError.hpp>

#include <compare>
#include <cstddef>
#include <expected>

namespace Memory
{
    template<typename AddressType>
    class MemoryRegion final
    {
    public:
        [[nodiscard]]
        static constexpr auto Create(
            const AddressType Address,
            const std::size_t Size
        ) noexcept -> std::expected<MemoryRegion, MemoryError>
        {
            if (Size == 0)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::EmptyRegion,
                        .FaultAddress = Address.Value()
                    }
                };
            }

            if (!Address.Offset(Size - 1))
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::AddressOverflow,
                        .FaultAddress = Address.Value()
                    }
                };
            }

            return MemoryRegion
            {
                Address,
                Size
            };
        }

        [[nodiscard]]
        constexpr auto Address() const noexcept -> AddressType
        {
            return Address_;
        }

        [[nodiscard]]
        constexpr auto Size() const noexcept -> std::size_t
        {
            return Size_;
        }

        [[nodiscard]]
        constexpr auto Contains(
            const AddressType Address
        ) const noexcept -> bool
        {
            return Address >= Address_ && Address.Value() - Address_.Value() < Size_;
        }

        [[nodiscard]]
        constexpr auto Subregion(
            const std::size_t Offset,
            const std::size_t Size
        ) const noexcept -> std::expected<MemoryRegion, MemoryError>
        {
            if (Size == 0)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::EmptyRegion,
                        .FaultAddress = Address_.Value()
                    }
                };
            }

            if (Offset > Size_ || Size > Size_ - Offset)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::InvalidRange,
                        .FaultAddress = Address_.Value()
                    }
                };
            }

            const auto Address = Address_.Offset(
                Offset
            );

            if (!Address)
            {
                return std::unexpected
                {
                    Address.error()
                };
            }

            return MemoryRegion
            {
                *Address,
                Size
            };
        }

        constexpr auto operator<=>(const MemoryRegion&) const noexcept = default;

    private:
        constexpr MemoryRegion(
            const AddressType Address,
            const std::size_t Size
        ) noexcept : Address_{ Address }, Size_{ Size }
        {
        }

        AddressType Address_{};
        std::size_t Size_{};
    };
}
