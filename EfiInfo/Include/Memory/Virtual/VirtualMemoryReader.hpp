#pragma once

#include <Memory/Detail/ReadOperations.hpp>
#include <Memory/Physical/PhysicalMemoryReader.hpp>
#include <Memory/Virtual/VirtualAddressTranslator.hpp>
#include <Memory/Virtual/VirtualMemoryRegion.hpp>

#include <cstddef>
#include <expected>
#include <span>
#include <type_traits>
#include <vector>

namespace Memory::Virtual
{
    class VirtualMemoryReader final
    {
    public:
        explicit VirtualMemoryReader(
            const Physical::PhysicalMemoryReader& PhysicalReader
        ) noexcept : PhysicalReader_{ PhysicalReader }, Translator_{ PhysicalReader }
        {
        }

        [[nodiscard]]
        auto ReadInto(
            VirtualAddress Source,
            std::span<std::byte> Destination
        ) const noexcept -> std::expected<std::size_t, MemoryError>;

        [[nodiscard]]
        auto ReadInto(
            VirtualAddress Source,
            std::span<std::byte> Destination,
            DirectoryTableBase DirectoryTable
        ) const noexcept -> std::expected<std::size_t, MemoryError>;

        template<typename Value> requires std::is_trivially_copyable_v<Value>
        [[nodiscard]]
        auto Read(
            const VirtualAddress Source
        ) const noexcept -> std::expected<Value, MemoryError>
        {
            return Memory::Detail::ReadValue<Value>(
                [&](const std::span<std::byte> Destination)
                {
                    return ReadInto(
                        Source,
                        Destination
                    );
                }
            );
        }

        template<typename Value> requires std::is_trivially_copyable_v<Value>
        [[nodiscard]]
        auto Read(
            const VirtualAddress Source,
            const DirectoryTableBase DirectoryTable
        ) const noexcept -> std::expected<Value, MemoryError>
        {
            return Memory::Detail::ReadValue<Value>(
                [&](const std::span<std::byte> Destination)
                {
                    return ReadInto(
                        Source,
                        Destination,
                        DirectoryTable
                    );
                }
            );
        }

        [[nodiscard]]
        auto Read(
            VirtualMemoryRegion Region
        ) const noexcept -> std::expected<std::vector<std::byte>, MemoryError>;

        [[nodiscard]]
        auto Read(
            VirtualMemoryRegion Region,
            DirectoryTableBase DirectoryTable
        ) const noexcept -> std::expected<std::vector<std::byte>, MemoryError>;

    private:
        const Physical::PhysicalMemoryReader& PhysicalReader_;
        VirtualAddressTranslator Translator_;
    };
}
