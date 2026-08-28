#include <Memory/Physical/Detail/PhysicalPage.hpp>
#include <Memory/Physical/PhysicalMemoryWriter.hpp>

#include <cstddef>
#include <cstring>

namespace Memory::Physical
{
    auto PhysicalMemoryWriter::Write(
        PhysicalMemoryView Destination,
        const std::span<const std::byte> Source
    ) const noexcept -> std::expected<std::size_t, MemoryError>
    {
        if (Source.size() > Destination.Size())
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::InvalidRange,
                    .FaultAddress = Destination.Address().Value()
                }
            };
        }

        if (!Source.empty())
        {
            std::memcpy(
                Destination.Bytes().data(),
                Source.data(),
                Source.size()
            );
        }

        return Source.size();
    }

    auto PhysicalMemoryWriter::Write(
        const PhysicalAddress Destination,
        const std::span<const std::byte> Source,
        const PhysicalMemoryCaching Caching
    ) const noexcept -> std::expected<std::size_t, MemoryError>
    {
        return Detail::ForEachPhysicalPageFragment(
            Destination,
            Source.size(),
            [&](
                const Detail::PhysicalPageFragment Fragment,
                const std::size_t SourceOffset
            ) -> std::expected<std::size_t, MemoryError>
            {
                const auto PageRegion = PhysicalMemoryRegion::Create(
                    Fragment.PageAddress,
                    Detail::PhysicalPageSize
                );

                if (!PageRegion)
                {
                    return std::unexpected
                    {
                        PageRegion.error()
                    };
                }

                auto Mapping = PhysicalMemoryMapping::Map(
                    *PageRegion,
                    PhysicalMemoryProtection::ReadWrite,
                    Caching
                );

                if (!Mapping)
                {
                    auto Error = Mapping.error();
                    Error.FaultAddress = Fragment.Address.Value();

                    return std::unexpected
                    {
                        Error
                    };
                }

                auto View = Mapping->View();

                if (!View)
                {
                    auto Error = View.error();
                    Error.FaultAddress = Fragment.Address.Value();

                    return std::unexpected
                    {
                        Error
                    };
                }

                const auto FragmentView = View->Subview(
                    Fragment.PageOffset,
                    Fragment.Size
                );

                if (!FragmentView)
                {
                    return std::unexpected
                    {
                        FragmentView.error()
                    };
                }

                return Write(
                    *FragmentView,
                    Source.subspan(
                        SourceOffset,
                        Fragment.Size
                    )
                );
            }
        );
    }
}
