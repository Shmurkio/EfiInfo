#pragma once

#include <Memory/Detail/ReadOperations.hpp>
#include <Memory/Physical/PhysicalMemoryMapping.hpp>
#include <Memory/Physical/PhysicalMemoryRegion.hpp>

#include <atomic>
#include <cstddef>
#include <expected>
#include <span>
#include <type_traits>
#include <vector>

namespace Memory::Physical
{
    struct PhysicalMemoryReadStatistics final
    {
        std::size_t BytesCopied{};
        std::size_t CopyFailures{};
        std::size_t PagesMapped{};
        std::size_t BytesMapped{};
    };

    class PhysicalMemoryReader final
    {
    public:
        [[nodiscard]]
        auto ReadInto(
            PhysicalAddress Source,
            std::span<std::byte> Destination
        ) const noexcept -> std::expected<std::size_t, MemoryError>;

        template<typename Value> requires std::is_trivially_copyable_v<Value>
        [[nodiscard]]
        auto Read(
            const PhysicalAddress Source
        ) const noexcept -> std::expected<Value, MemoryError>
        {
            return Memory::Detail::ReadValue<Value>(
                [&](const std::span<std::byte> Destination)
                {
                    return ReadInto(
                        Source,
                        Destination
                    );
                }
            );
        }

        [[nodiscard]]
        auto Read(
            const PhysicalMemoryRegion Region
        ) const noexcept -> std::expected<std::vector<std::byte>, MemoryError>;

        [[nodiscard]]
        auto Statistics() const noexcept -> PhysicalMemoryReadStatistics;

        void ResetStatistics() const noexcept;

    private:
        [[nodiscard]]
        auto ReadMapped(
            PhysicalAddress Source,
            std::span<std::byte> Destination,
            PhysicalMemoryCaching Caching
        ) const noexcept -> std::expected<std::size_t, MemoryError>;

        mutable std::atomic_size_t BytesCopied_{};
        mutable std::atomic_size_t CopyFailures_{};
        mutable std::atomic_size_t PagesMapped_{};
        mutable std::atomic_size_t BytesMapped_{};
    };
}
