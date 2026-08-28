#pragma once

#include <Memory/Detail/WriteOperations.hpp>
#include <Memory/Physical/PhysicalMemoryMapping.hpp>

#include <cstddef>
#include <expected>
#include <span>
#include <type_traits>

namespace Memory::Physical
{
    class PhysicalMemoryWriter final
    {
    public:
        [[nodiscard]]
        auto Write(
            PhysicalMemoryView Destination,
            std::span<const std::byte> Source
        ) const noexcept -> std::expected<std::size_t, MemoryError>;

        [[nodiscard]]
        auto Write(
            PhysicalAddress Destination,
            std::span<const std::byte> Source,
            PhysicalMemoryCaching Caching = PhysicalMemoryCaching::Cached
        ) const noexcept -> std::expected<std::size_t, MemoryError>;

        template<typename Value> requires std::is_trivially_copyable_v<Value>
        [[nodiscard]]
        auto Write(
            const PhysicalMemoryView Destination,
            const Value& Source
        ) const noexcept -> std::expected<void, MemoryError>
        {
            return Memory::Detail::WriteValue(
                Source,
                [&](const std::span<const std::byte> SourceBytes)
                {
                    return Write(
                        Destination,
                        SourceBytes
                    );
                }
            );
        }

        template<typename Value> requires std::is_trivially_copyable_v<Value>
        [[nodiscard]]
        auto Write(
            const PhysicalAddress Destination,
            const Value& Source,
            const PhysicalMemoryCaching Caching = PhysicalMemoryCaching::Cached
        ) const noexcept -> std::expected<void, MemoryError>
        {
            return Memory::Detail::WriteValue(
                Source,
                [&](const std::span<const std::byte> SourceBytes)
                {
                    return Write(
                        Destination,
                        SourceBytes,
                        Caching
                    );
                }
            );
        }
    };
}
