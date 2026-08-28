#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>

namespace Memory
{
    enum class MemoryErrorCode : std::uint8_t
    {
        AddressOverflow,
        InvalidAlignment,
        EmptyRegion,
        InvalidRange,
        CopyFailed,
        PartialTransfer,
        MappingFailed,
        AccessDenied,
        NonCanonicalAddress,
        InvalidDirectoryTableBase,
        PageNotPresent,
        InvalidPageTableEntry,
        UnsupportedPagingMode
    };

    struct MemoryError final
    {
        MemoryErrorCode Code{};
        std::int32_t NativeStatus{};
        std::size_t BytesTransferred{};
        std::uint64_t FaultAddress{};
        std::uint8_t PageTableLevel{};

        constexpr auto operator<=>(const MemoryError&) const noexcept = default;
    };
}
