#pragma once

#include <Memory/MemoryError.hpp>
#include <Memory/Physical/PhysicalMemoryReader.hpp>
#include <Memory/Virtual/DirectoryTableBase.hpp>
#include <Memory/Virtual/VirtualAddress.hpp>
#include <Memory/Virtual/VirtualAddressTranslation.hpp>

#include <cstdint>
#include <expected>

namespace Memory::Virtual
{
    class VirtualAddressTranslator final
    {
    public:
        explicit VirtualAddressTranslator(
            const Physical::PhysicalMemoryReader& PhysicalReader
        ) noexcept : PhysicalReader_{ PhysicalReader }
        {
        }

        [[nodiscard]]
        auto Translate(
            VirtualAddress Address,
            DirectoryTableBase DirectoryTable
        ) const noexcept -> std::expected<VirtualAddressTranslation, MemoryError>;

    private:
        [[nodiscard]]
        auto ReadEntry(
            Physical::PhysicalAddress Table,
            std::uint16_t Index,
            VirtualAddress Address,
            std::uint8_t Level
        ) const noexcept -> std::expected<std::uint64_t, MemoryError>;

        const Physical::PhysicalMemoryReader& PhysicalReader_;
    };
}
