#include <Memory/Virtual/VirtualMemoryWriter.hpp>

#include <algorithm>
#include <cstddef>

namespace Memory::Virtual
{
    auto VirtualMemoryWriter::Write(
        const VirtualAddress Destination,
        const std::span<const std::byte> Source
    ) const noexcept -> std::expected<std::size_t, MemoryError>
    {
        return Write(
            Destination,
            Source,
            DirectoryTableBase::Current()
        );
    }

    auto VirtualMemoryWriter::Write(
        VirtualAddress Destination,
        std::span<const std::byte> Source,
        const DirectoryTableBase DirectoryTable
    ) const noexcept -> std::expected<std::size_t, MemoryError>
    {
        auto TotalBytesTransferred = std::size_t{};

        while (!Source.empty())
        {
            const auto Translation = Translator_.Translate(
                Destination,
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

            if (!Translation->Writable)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::AccessDenied,
                        .BytesTransferred = TotalBytesTransferred,
                        .FaultAddress = Destination.Value()
                    }
                };
            }

            const auto ChunkSize = (std::min)(
                Source.size(),
                Translation->ContiguousBytes
            );

            const auto BytesTransferred = PhysicalWriter_.Write(
                Translation->PhysicalAddress,
                Source.first(ChunkSize),
                Physical::PhysicalMemoryCaching::Cached
            );

            if (!BytesTransferred)
            {
                auto Error = BytesTransferred.error();
                Error.BytesTransferred += TotalBytesTransferred;
                Error.FaultAddress = Destination.Value();

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
                        .FaultAddress = Destination.Value()
                    }
                };
            }

            const auto NextDestination = Destination.Offset(
                ChunkSize
            );

            if (!NextDestination)
            {
                auto Error = NextDestination.error();
                Error.BytesTransferred = TotalBytesTransferred;

                return std::unexpected
                {
                    Error
                };
            }

            Destination = *NextDestination;
            Source = Source.subspan(
                ChunkSize
            );
        }

        return TotalBytesTransferred;
    }
}
