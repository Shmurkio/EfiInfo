#pragma once

#include <Memory/Detail/WriteOperations.hpp>
#include <Memory/Physical/PhysicalMemoryReader.hpp>
#include <Memory/Physical/PhysicalMemoryWriter.hpp>
#include <Memory/Virtual/VirtualAddressTranslator.hpp>

#include <cstddef>
#include <expected>
#include <span>
#include <type_traits>

namespace Memory::Virtual
{
    class VirtualMemoryWriter final
    {
    public:
        VirtualMemoryWriter(
            const Physical::PhysicalMemoryReader& PhysicalReader,
            const Physical::PhysicalMemoryWriter& PhysicalWriter
        ) noexcept : PhysicalWriter_{ PhysicalWriter }, Translator_{ PhysicalReader }
        {
        }

        [[nodiscard]]
        auto Write(
            VirtualAddress Destination,
            std::span<const std::byte> Source
        ) const noexcept -> std::expected<std::size_t, MemoryError>;

        [[nodiscard]]
        auto Write(
            VirtualAddress Destination,
            std::span<const std::byte> Source,
            DirectoryTableBase DirectoryTable
        ) const noexcept -> std::expected<std::size_t, MemoryError>;

        template<typename Value> requires std::is_trivially_copyable_v<Value>
        [[nodiscard]]
        auto Write(
            const VirtualAddress Destination,
            const Value& Source
        ) const noexcept -> std::expected<void, MemoryError>
        {
            return Memory::Detail::WriteValue(
                Source,
                [&](const std::span<const std::byte> SourceBytes)
                {
                    return Write(
                        Destination,
                        SourceBytes
                    );
                }
            );
        }

        template<typename Value> requires std::is_trivially_copyable_v<Value>
        [[nodiscard]]
        auto Write(
            const VirtualAddress Destination,
            const Value& Source,
            const DirectoryTableBase DirectoryTable
        ) const noexcept -> std::expected<void, MemoryError>
        {
            return Memory::Detail::WriteValue(
                Source,
                [&](const std::span<const std::byte> SourceBytes)
                {
                    return Write(
                        Destination,
                        SourceBytes,
                        DirectoryTable
                    );
                }
            );
        }

    private:
        const Physical::PhysicalMemoryWriter& PhysicalWriter_;
        VirtualAddressTranslator Translator_;
    };
}
