#include <Memory/Virtual/VirtualMemoryReader.hpp>

#include <algorithm>
#include <cstddef>

namespace Memory::Virtual
{
    auto VirtualMemoryReader::ReadInto(
        const VirtualAddress Source,
        const std::span<std::byte> Destination
    ) const noexcept -> std::expected<std::size_t, MemoryError>
    {
        return ReadInto(
            Source,
            Destination,
            DirectoryTableBase::Current()
        );
    }

    auto VirtualMemoryReader::ReadInto(
        VirtualAddress Source,
        std::span<std::byte> Destination,
        const DirectoryTableBase DirectoryTable
    ) const noexcept -> std::expected<std::size_t, MemoryError>
    {
        auto TotalBytesTransferred = std::size_t{};

        while (!Destination.empty())
        {
            const auto Translation = Translator_.Translate(
                Source,
                DirectoryTable
            );

            if (!Translation)
            {
                auto Error = Translation.error();
                Error.BytesTransferred += TotalBytesTransferred;

                return std::unexpected
                {
                    Error
                };
            }

            const auto ChunkSize = (std::min)(
                Destination.size(),
                Translation->ContiguousBytes
            );

            const auto BytesTransferred = PhysicalReader_.ReadInto(
                Translation->PhysicalAddress,
                Destination.first(ChunkSize)
            );

            if (!BytesTransferred)
            {
                auto Error = BytesTransferred.error();
                Error.BytesTransferred += TotalBytesTransferred;
                Error.FaultAddress = Source.Value();

                return std::unexpected
                {
                    Error
                };
            }

            TotalBytesTransferred += *BytesTransferred;

            if (*BytesTransferred != ChunkSize)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::PartialTransfer,
                        .BytesTransferred = TotalBytesTransferred,
                        .FaultAddress = Source.Value()
                    }
                };
            }

            const auto NextSource = Source.Offset(
                ChunkSize
            );

            if (!NextSource)
            {
                auto Error = NextSource.error();
                Error.BytesTransferred = TotalBytesTransferred;

                return std::unexpected
                {
                    Error
                };
            }

            Source = *NextSource;
            Destination = Destination.subspan(
                ChunkSize
            );
        }

        return TotalBytesTransferred;
    }

    auto VirtualMemoryReader::Read(
        const VirtualMemoryRegion Region
    ) const noexcept -> std::expected<std::vector<std::byte>, MemoryError>
    {
        return Read(
            Region,
            DirectoryTableBase::Current()
        );
    }

    auto VirtualMemoryReader::Read(
        const VirtualMemoryRegion Region,
        const DirectoryTableBase DirectoryTable
    ) const noexcept -> std::expected<std::vector<std::byte>, MemoryError>
    {
        return Memory::Detail::ReadBytes(
            Region.Size(),
            [&](const std::span<std::byte> Destination)
            {
                return ReadInto(
                    Region.Address(),
                    Destination,
                    DirectoryTable
                );
            }
        );
    }
}
