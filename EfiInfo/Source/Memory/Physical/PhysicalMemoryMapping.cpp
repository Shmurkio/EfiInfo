#include <ntifs.h>

#include <Memory/Physical/PhysicalMemoryMapping.hpp>

#include <cstdint>
#include <utility>

namespace Memory::Physical
{
    namespace
    {
        [[nodiscard]]
        constexpr auto ToProtectionFlags(
            const PhysicalMemoryProtection Protection,
            const PhysicalMemoryCaching Caching
        ) noexcept -> ULONG
        {
            auto Flags = Protection == PhysicalMemoryProtection::ReadOnly ? PAGE_READONLY : PAGE_READWRITE;

            switch (Caching)
            {
            case PhysicalMemoryCaching::Cached: break;
            case PhysicalMemoryCaching::NonCached: Flags |= PAGE_NOCACHE; break;
            case PhysicalMemoryCaching::WriteCombined: Flags |= PAGE_WRITECOMBINE; break;
            }

            return Flags;
        }
    }

    auto PhysicalMemoryMapping::Map(
        const PhysicalMemoryRegion Region,
        const PhysicalMemoryProtection Protection,
        const PhysicalMemoryCaching Caching
    ) noexcept -> std::expected<PhysicalMemoryMapping, MemoryError>
    {
        PHYSICAL_ADDRESS NativeAddress{};
        NativeAddress.QuadPart = static_cast<LONGLONG>(Region.Address().Value());

        auto* const Mapping = MmMapIoSpaceEx(
            NativeAddress,
            Region.Size(),
            ToProtectionFlags(Protection, Caching)
        );

        if (Mapping == nullptr)
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::MappingFailed,
                    .FaultAddress = Region.Address().Value()
                }
            };
        }

        return PhysicalMemoryMapping
        {
            Mapping,
            Region,
            Protection
        };
    }

    PhysicalMemoryMapping::PhysicalMemoryMapping(
        void* const Mapping,
        const PhysicalMemoryRegion Region,
        const PhysicalMemoryProtection Protection
    ) noexcept : Mapping_{ Mapping }, Region_{ Region }, Protection_{ Protection }
    {
    }

    PhysicalMemoryMapping::~PhysicalMemoryMapping() noexcept
    {
        Reset();
    }

    PhysicalMemoryMapping::PhysicalMemoryMapping(
        PhysicalMemoryMapping&& Other
    ) noexcept : Mapping_{ std::exchange(Other.Mapping_, nullptr) }, Region_{ Other.Region_ }, Protection_{ Other.Protection_ }
    {
    }

    auto PhysicalMemoryMapping::operator=(
        PhysicalMemoryMapping&& Other
    ) noexcept -> PhysicalMemoryMapping&
    {
        if (this == &Other)
        {
            return *this;
        }

        Reset();
        Mapping_ = std::exchange(Other.Mapping_, nullptr);
        Region_ = Other.Region_;
        Protection_ = Other.Protection_;

        return *this;
    }

    auto PhysicalMemoryMapping::Region() const noexcept -> PhysicalMemoryRegion
    {
        return Region_;
    }

    auto PhysicalMemoryMapping::Bytes() const noexcept -> std::span<const std::byte>
    {
        if (Mapping_ == nullptr)
        {
            return {};
        }

        return
        {
            static_cast<const std::byte*>(Mapping_),
            Region_.Size()
        };
    }

    auto PhysicalMemoryMapping::View() noexcept -> std::expected<PhysicalMemoryView, MemoryError>
    {
        if (Mapping_ == nullptr)
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::MappingFailed,
                    .FaultAddress = Region_.Address().Value()
                }
            };
        }

        if (Protection_ != PhysicalMemoryProtection::ReadWrite)
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::AccessDenied,
                    .FaultAddress = Region_.Address().Value()
                }
            };
        }

        return PhysicalMemoryView
        {
            Region_.Address(),
            {
                static_cast<std::byte*>(Mapping_),
                Region_.Size()
            }
        };
    }

    PhysicalMemoryMapping::operator bool() const noexcept
    {
        return Mapping_ != nullptr;
    }

    void PhysicalMemoryMapping::Reset() noexcept
    {
        if (Mapping_ == nullptr)
        {
            return;
        }

        MmUnmapIoSpace(
            Mapping_,
            Region_.Size()
        );

        Mapping_ = nullptr;
    }
}
