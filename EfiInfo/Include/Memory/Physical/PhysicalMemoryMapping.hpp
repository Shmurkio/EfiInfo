#pragma once

#include <Memory/Physical/PhysicalMemoryRegion.hpp>
#include <Memory/Physical/PhysicalMemoryView.hpp>

#include <cstddef>
#include <expected>
#include <span>

namespace Memory::Physical
{
    enum class PhysicalMemoryProtection
    {
        ReadOnly,
        ReadWrite
    };

    enum class PhysicalMemoryCaching
    {
        Cached,
        NonCached,
        WriteCombined
    };

    class PhysicalMemoryMapping final
    {
    public:
        [[nodiscard]]
        static auto Map(
            PhysicalMemoryRegion Region,
            PhysicalMemoryProtection Protection,
            PhysicalMemoryCaching Caching
        ) noexcept -> std::expected<PhysicalMemoryMapping, MemoryError>;

        ~PhysicalMemoryMapping() noexcept;

        PhysicalMemoryMapping(PhysicalMemoryMapping&& Other) noexcept;
        auto operator=(PhysicalMemoryMapping&& Other) noexcept -> PhysicalMemoryMapping&;

        PhysicalMemoryMapping(const PhysicalMemoryMapping&) = delete;
        auto operator=(const PhysicalMemoryMapping&)  -> PhysicalMemoryMapping& = delete;

        [[nodiscard]]
        auto Region() const noexcept -> PhysicalMemoryRegion;

        [[nodiscard]]
        auto Bytes() const noexcept -> std::span<const std::byte>;

        [[nodiscard]]
        auto View() noexcept -> std::expected<PhysicalMemoryView, MemoryError>;

        [[nodiscard]]
        explicit operator bool() const noexcept;

    private:
        PhysicalMemoryMapping(
            void* Mapping,
            PhysicalMemoryRegion Region,
            PhysicalMemoryProtection Protection
        ) noexcept;

        void Reset() noexcept;

        void* Mapping_{};
        PhysicalMemoryRegion Region_;

        PhysicalMemoryProtection Protection_
        {
            PhysicalMemoryProtection::ReadOnly
        };
    };
}
