#include <ntifs.h>

#include <Memory/Physical/Detail/PhysicalPage.hpp>
#include <Memory/Physical/PhysicalMemoryReader.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>

namespace Memory::Physical
{
    auto PhysicalMemoryReader::ReadInto(
        const PhysicalAddress Source,
        const std::span<std::byte> Destination
    ) const noexcept -> std::expected<std::size_t, MemoryError>
    {
        if (Destination.empty())
        {
            return 0;
        }

        if (!Source.Offset(Destination.size() - 1))
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::AddressOverflow,
                    .FaultAddress = Source.Value()
                }
            };
        }

        auto TotalBytesTransferred = std::size_t{};

        while (TotalBytesTransferred < Destination.size())
        {
            const auto CurrentAddress = Source.Offset(
                TotalBytesTransferred
            );

            if (!CurrentAddress)
            {
                auto Error = CurrentAddress.error();
                Error.BytesTransferred = TotalBytesTransferred;

                return std::unexpected
                {
                    Error
                };
            }

            const auto RemainingDestination = Destination.subspan(
                TotalBytesTransferred
            );

            MM_COPY_ADDRESS NativeSource{};
            NativeSource.PhysicalAddress.QuadPart = static_cast<LONGLONG>(CurrentAddress->Value());

            SIZE_T BytesTransferred{};

            const auto Status = MmCopyMemory(
                RemainingDestination.data(),
                NativeSource,
                RemainingDestination.size(),
                MM_COPY_MEMORY_PHYSICAL,
                &BytesTransferred
            );

            BytesTransferred = (std::min)(
                BytesTransferred,
                RemainingDestination.size()
            );

            BytesCopied_.fetch_add(
                BytesTransferred,
                std::memory_order_relaxed
            );
            TotalBytesTransferred += BytesTransferred;

            if (TotalBytesTransferred == Destination.size())
            {
                return TotalBytesTransferred;
            }

            CopyFailures_.fetch_add(
                1,
                std::memory_order_relaxed
            );

            const auto FallbackAddress = Source.Offset(
                TotalBytesTransferred
            );

            if (!FallbackAddress)
            {
                auto Error = FallbackAddress.error();
                Error.NativeStatus = static_cast<std::int32_t>(Status);
                Error.BytesTransferred = TotalBytesTransferred;

                return std::unexpected
                {
                    Error
                };
            }

            const auto PageOffset = static_cast<std::size_t>(FallbackAddress->Value() & (Detail::PhysicalPageSize - 1));
            const auto FallbackSize = (std::min)(
                Destination.size() - TotalBytesTransferred,
                Detail::PhysicalPageSize - PageOffset
            );

            const auto MappedBytes = ReadMapped(
                *FallbackAddress,
                Destination.subspan(
                    TotalBytesTransferred,
                    FallbackSize
                ),
                PhysicalMemoryCaching::Cached
            );

            if (!MappedBytes)
            {
                auto Error = MappedBytes.error();
                Error.NativeStatus = static_cast<std::int32_t>(Status);
                Error.BytesTransferred += TotalBytesTransferred;

                return std::unexpected
                {
                    Error
                };
            }

            TotalBytesTransferred += *MappedBytes;
        }

        return TotalBytesTransferred;
    }

    auto PhysicalMemoryReader::ReadMapped(
        const PhysicalAddress Source,
        const std::span<std::byte> Destination,
        const PhysicalMemoryCaching Caching
    ) const noexcept -> std::expected<std::size_t, MemoryError>
    {
        if (Destination.empty())
        {
            return 0;
        }

        const auto PageAddress = Source.AlignDown(
            Detail::PhysicalPageSize
        );

        if (!PageAddress)
        {
            return std::unexpected
            {
                PageAddress.error()
            };
        }

        const auto PageOffset = static_cast<std::size_t>(Source.Value() - PageAddress->Value());

        if (Destination.size() > Detail::PhysicalPageSize - PageOffset)
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::InvalidRange,
                    .FaultAddress = Source.Value()
                }
            };
        }

        const auto Region = PhysicalMemoryRegion::Create(
            *PageAddress,
            Detail::PhysicalPageSize
        );

        if (!Region)
        {
            return std::unexpected
            {
                Region.error()
            };
        }

        auto Mapping = PhysicalMemoryMapping::Map(
            *Region,
            PhysicalMemoryProtection::ReadOnly,
            Caching
        );

        if (!Mapping)
        {
            auto Error = Mapping.error();
            Error.FaultAddress = Source.Value();

            return std::unexpected
            {
                Error
            };
        }

        std::memcpy(
            Destination.data(),
            Mapping->Bytes().data() + PageOffset,
            Destination.size()
        );

        PagesMapped_.fetch_add(
            1,
            std::memory_order_relaxed
        );
        BytesMapped_.fetch_add(
            Destination.size(),
            std::memory_order_relaxed
        );

        return Destination.size();
    }

    auto PhysicalMemoryReader::Read(
        const PhysicalMemoryRegion Region
    ) const noexcept -> std::expected<std::vector<std::byte>, MemoryError>
    {
        return Memory::Detail::ReadBytes(
            Region.Size(),
            [&](const std::span<std::byte> Destination)
            {
                return ReadInto(
                    Region.Address(),
                    Destination
                );
            }
        );
    }

    auto PhysicalMemoryReader::Statistics() const noexcept -> PhysicalMemoryReadStatistics
    {
        return PhysicalMemoryReadStatistics
        {
            .BytesCopied = BytesCopied_.load(std::memory_order_relaxed),
            .CopyFailures = CopyFailures_.load(std::memory_order_relaxed),
            .PagesMapped = PagesMapped_.load(std::memory_order_relaxed),
            .BytesMapped = BytesMapped_.load(std::memory_order_relaxed)
        };
    }

    void PhysicalMemoryReader::ResetStatistics() const noexcept
    {
        BytesCopied_.store(0, std::memory_order_relaxed);
        CopyFailures_.store(0, std::memory_order_relaxed);
        PagesMapped_.store(0, std::memory_order_relaxed);
        BytesMapped_.store(0, std::memory_order_relaxed);
    }
}
