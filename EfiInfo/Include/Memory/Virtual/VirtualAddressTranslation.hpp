#pragma once

#include <Memory/Physical/PhysicalAddress.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>

namespace Memory::Virtual
{
    enum class VirtualPageSize : std::uint64_t
    {
        Size4Kb = 0x1000,
        Size2Mb = 0x20'0000,
        Size1Gb = 0x4000'0000
    };

    struct VirtualAddressTranslation final
    {
        Physical::PhysicalAddress PhysicalAddress{};
        std::size_t ContiguousBytes{};
        VirtualPageSize PageSize{ VirtualPageSize::Size4Kb };
        bool Writable{};
        bool UserAccessible{};
        bool Executable{};

        constexpr auto operator<=>(const VirtualAddressTranslation&) const noexcept = default;
    };
}
