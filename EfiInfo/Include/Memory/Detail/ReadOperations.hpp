#pragma once

#include <Memory/MemoryError.hpp>

#include <cstddef>
#include <expected>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace Memory::Detail
{
    template<typename Value, typename ReadOperation> requires std::is_trivially_copyable_v<Value>
    [[nodiscard]]
    auto ReadValue(
        ReadOperation&& Operation
    ) noexcept -> std::expected<Value, MemoryError>
    {
        Value Result{};

        const auto Destination = std::as_writable_bytes(
            std::span<Value, 1>
            {
                &Result,
                1
            }
        );

        const auto BytesTransferred = std::forward<ReadOperation>(Operation)(
            Destination
        );

        if (!BytesTransferred)
        {
            return std::unexpected
            {
                BytesTransferred.error()
            };
        }

        if (*BytesTransferred != sizeof(Value))
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::PartialTransfer,
                    .BytesTransferred = *BytesTransferred
                }
            };
        }

        return Result;
    }

    template<typename ReadOperation>
    [[nodiscard]]
    auto ReadBytes(
        const std::size_t Size,
        ReadOperation&& Operation
    ) noexcept -> std::expected<std::vector<std::byte>, MemoryError>
    {
        std::vector<std::byte> Result(
            Size
        );

        const auto BytesTransferred = std::forward<ReadOperation>(Operation)(
            Result
        );

        if (!BytesTransferred)
        {
            return std::unexpected
            {
                BytesTransferred.error()
            };
        }

        if (*BytesTransferred != Size)
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::PartialTransfer,
                    .BytesTransferred = *BytesTransferred
                }
            };
        }

        return Result;
    }
}
