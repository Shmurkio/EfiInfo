#pragma once

#include <Memory/Physical/PhysicalAddress.hpp>

#include <compare>
#include <cstdint>

namespace Memory::Virtual
{
    class DirectoryTableBase final
    {
    public:
        constexpr DirectoryTableBase() noexcept = default;

        explicit constexpr DirectoryTableBase(
            const std::uint64_t Value
        ) noexcept : Value_{ Value }
        {
        }

        [[nodiscard]]
        static auto Current() noexcept -> DirectoryTableBase;

        [[nodiscard]]
        constexpr auto Value() const noexcept -> std::uint64_t
        {
            return Value_;
        }

        [[nodiscard]]
        constexpr auto RootAddress() const noexcept -> Physical::PhysicalAddress
        {
            return Physical::PhysicalAddress
            {
                Value_ & 0x000F'FFFF'FFFF'F000ULL
            };
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return static_cast<bool>(RootAddress());
        }

        constexpr auto operator<=>(const DirectoryTableBase&) const noexcept = default;

    private:
        std::uint64_t Value_{};
    };
}
