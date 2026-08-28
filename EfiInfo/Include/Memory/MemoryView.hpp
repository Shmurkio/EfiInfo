#pragma once

#include <Memory/MemoryError.hpp>

#include <cstddef>
#include <cstring>
#include <expected>
#include <span>
#include <type_traits>

namespace Memory
{
    template<typename AddressType>
    class MemoryView final
    {
    public:
        constexpr MemoryView() noexcept = default;

        constexpr MemoryView(
            const AddressType Address,
            const std::span<std::byte> Bytes
        ) noexcept : Address_{ Address }, Bytes_{ Bytes }
        {
        }

        [[nodiscard]]
        constexpr auto Address() const noexcept -> AddressType
        {
            return Address_;
        }

        [[nodiscard]]
        constexpr auto Bytes() noexcept -> std::span<std::byte>
        {
            return Bytes_;
        }

        [[nodiscard]]
        constexpr auto Bytes() const noexcept -> std::span<const std::byte>
        {
            return Bytes_;
        }

        [[nodiscard]]
        constexpr auto Size() const noexcept -> std::size_t
        {
            return Bytes_.size();
        }

        [[nodiscard]]
        constexpr auto Subview(
            const std::size_t Offset,
            const std::size_t Size
        ) const noexcept -> std::expected<MemoryView, MemoryError>
        {
            if (Offset > Bytes_.size() || Size > Bytes_.size() - Offset)
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

            return MemoryView
            {
                *Address,
                Bytes_.subspan(
                    Offset,
                    Size
                )
            };
        }

        template<typename Value> requires std::is_trivially_copyable_v<Value>
        [[nodiscard]]
        auto Read(
            const std::size_t Offset = 0
        ) const noexcept -> std::expected<Value, MemoryError>
        {
            if (Offset > Bytes_.size() || sizeof(Value) > Bytes_.size() - Offset)
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

            Value Result{};

            std::memcpy(
                &Result,
                Bytes_.data() + Offset,
                sizeof(Result)
            );

            return Result;
        }

    private:
        AddressType Address_{};
        std::span<std::byte> Bytes_{};
    };
}
