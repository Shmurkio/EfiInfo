#pragma once

#include <Memory/MemoryError.hpp>

#include <cstddef>
#include <expected>
#include <span>
#include <type_traits>
#include <utility>

namespace Memory::Detail
{
    template<typename Value, typename WriteOperation> requires std::is_trivially_copyable_v<Value>
    [[nodiscard]]
    auto WriteValue(
        const Value& Source,
        WriteOperation&& Operation
    ) noexcept -> std::expected<void, MemoryError>
    {
        const auto SourceBytes = std::as_bytes(
            std::span<const Value, 1>
            {
                &Source,
                1
            }
        );

        const auto BytesTransferred = std::forward<WriteOperation>(Operation)(
            SourceBytes
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

        return {};
    }
}
