#pragma once

#include <Memory/Physical/PhysicalAddress.hpp>

#include <algorithm>
#include <cstddef>
#include <expected>

namespace Memory::Physical::Detail
{
    inline constexpr auto PhysicalPageSize = std::size_t{ 0x1000 };

    struct PhysicalPageFragment final
    {
        PhysicalAddress Address;
        PhysicalAddress PageAddress;
        std::size_t PageOffset;
        std::size_t Size;
    };

    template<typename Operation>
    [[nodiscard]]
    auto ForEachPhysicalPageFragment(
        const PhysicalAddress Address,
        const std::size_t Size,
        Operation&& Transfer
    ) noexcept -> std::expected<std::size_t, MemoryError>
    {
        if (Size == 0)
        {
            return 0;
        }

        if (!Address.Offset(Size - 1))
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::AddressOverflow,
                    .FaultAddress = Address.Value()
                }
            };
        }

        auto TotalBytesTransferred = std::size_t{};

        while (TotalBytesTransferred < Size)
        {
            const auto CurrentAddress = Address.Offset(
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

            const auto PageAddress = CurrentAddress->AlignDown(
                PhysicalPageSize
            );

            if (!PageAddress)
            {
                auto Error = PageAddress.error();
                Error.BytesTransferred = TotalBytesTransferred;

                return std::unexpected
                {
                    Error
                };
            }

            const auto PageOffset = static_cast<std::size_t>(CurrentAddress->Value() - PageAddress->Value());
            const auto FragmentSize = (std::min)(
                Size - TotalBytesTransferred,
                PhysicalPageSize - PageOffset
            );

            const auto Result = Transfer(
                PhysicalPageFragment
                {
                    .Address = *CurrentAddress,
                    .PageAddress = *PageAddress,
                    .PageOffset = PageOffset,
                    .Size = FragmentSize
                },
                TotalBytesTransferred
            );

            if (!Result)
            {
                auto Error = Result.error();
                Error.BytesTransferred += TotalBytesTransferred;

                return std::unexpected
                {
                    Error
                };
            }

            if (*Result != FragmentSize)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::PartialTransfer,
                        .BytesTransferred = TotalBytesTransferred + *Result,
                        .FaultAddress = CurrentAddress->Value() + *Result
                    }
                };
            }

            TotalBytesTransferred += FragmentSize;
        }

        return TotalBytesTransferred;
    }
}
