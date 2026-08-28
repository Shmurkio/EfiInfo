#include <ntifs.h>

#include <Memory/Memory.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <memory>
#include <new>
#include <optional>
#include <span>

namespace Physical = Memory::Physical;
namespace Virtual = Memory::Virtual;

namespace
{
    constexpr auto EfiSystemTableSignature = std::uint64_t{ 0x5453595320494249 };
    constexpr auto EfiSystemTableAlignment = std::size_t{ 8 };
    constexpr auto ScanBufferSize = std::size_t{ 1024 * 1024 };
    constexpr auto PhysicalScanStart = std::uint64_t{ 0x00000000 };
    constexpr auto PhysicalScanSize = std::uint64_t{ 0x100000000 };
    constexpr auto MaximumEfiSystemTableSize = std::size_t{ 4096 };
    constexpr auto MaximumConfigurationTableEntries = std::uint64_t{ 65536 };

    struct EfiTableHeader final
    {
        std::uint64_t Signature;
        std::uint32_t Revision;
        std::uint32_t HeaderSize;
        std::uint32_t Crc32;
        std::uint32_t Reserved;
    };

    struct EfiSystemTable final
    {
        EfiTableHeader Header;
        std::uint64_t FirmwareVendor;
        std::uint32_t FirmwareRevision;
        std::uint64_t ConsoleInHandle;
        std::uint64_t ConsoleIn;
        std::uint64_t ConsoleOutHandle;
        std::uint64_t ConsoleOut;
        std::uint64_t StandardErrorHandle;
        std::uint64_t StandardError;
        std::uint64_t RuntimeServices;
        std::uint64_t BootServices;
        std::uint64_t NumberOfTableEntries;
        std::uint64_t ConfigurationTable;
    };

    static_assert(sizeof(EfiTableHeader) == 24);
    static_assert(sizeof(EfiSystemTable) == 120);

    struct EfiSystemTableMatch final
    {
        Physical::PhysicalAddress Address;
        EfiSystemTable Table;
    };

    [[nodiscard]]
    auto CalculateCrc32(
        const std::span<const std::byte> Bytes
    ) noexcept -> std::uint32_t
    {
        auto Crc = std::uint32_t{ 0xFFFFFFFFu };

        for (const auto Byte : Bytes)
        {
            Crc ^= std::to_integer<std::uint8_t>(Byte);

            for (auto Bit = 0; Bit < 8; ++Bit)
            {
                if ((Crc & 1u) != 0)
                {
                    Crc = (Crc >> 1) ^ 0xEDB88320u;
                }
                else
                {
                    Crc >>= 1;
                }
            }
        }

        return ~Crc;
    }

    [[nodiscard]]
    auto ReadEfiSystemTable(
        const Physical::PhysicalMemoryReader& Reader,
        const Physical::PhysicalAddress Address
    ) noexcept -> std::optional<EfiSystemTable>
    {
        const auto Header = Reader.Read<EfiTableHeader>(
            Address
        );

        if (!Header)
        {
            return std::nullopt;
        }

        if (Header->Signature != EfiSystemTableSignature || Header->Reserved != 0 || Header->HeaderSize < sizeof(EfiSystemTable) || Header->HeaderSize > MaximumEfiSystemTableSize)
        {
            return std::nullopt;
        }

        std::array<std::byte, MaximumEfiSystemTableSize> Bytes{};
        const auto TableSize = static_cast<std::size_t>(Header->HeaderSize);

        const auto BytesTransferred = Reader.ReadInto(
            Address,
            std::span{ Bytes }.first(TableSize)
        );

        if (!BytesTransferred || *BytesTransferred != TableSize)
        {
            return std::nullopt;
        }

        const auto ExpectedCrc32 = Header->Crc32;

        std::fill_n(
            Bytes.begin() + offsetof(EfiTableHeader, Crc32),
            sizeof(std::uint32_t),
            std::byte{}
        );

        if (CalculateCrc32(std::span{ Bytes }.first(TableSize)) != ExpectedCrc32)
        {
            return std::nullopt;
        }

        EfiSystemTable Table{};

        std::memcpy(
            &Table,
            Bytes.data(),
            sizeof(Table)
        );

        if (Table.RuntimeServices == 0 || Table.NumberOfTableEntries > MaximumConfigurationTableEntries || (Table.NumberOfTableEntries != 0 && Table.ConfigurationTable == 0))
        {
            return std::nullopt;
        }

        return Table;
    }

    [[nodiscard]]
    auto ScanBuffer(
        const Physical::PhysicalMemoryReader& Reader,
        const Physical::PhysicalAddress BaseAddress,
        const std::span<const std::byte> Bytes
    ) noexcept -> std::optional<EfiSystemTableMatch>
    {
        auto SearchOffset = std::size_t{};
        constexpr auto SignatureFirstByte = static_cast<int>(EfiSystemTableSignature & 0xFFu);

        while (SearchOffset < Bytes.size())
        {
            const auto* const Match = static_cast<const std::byte*>(
                std::memchr(
                    Bytes.data() + SearchOffset,
                    SignatureFirstByte,
                    Bytes.size() - SearchOffset
                )
            );

            if (Match == nullptr)
            {
                break;
            }

            const auto Offset = static_cast<std::size_t>(Match - Bytes.data());
            SearchOffset = Offset + 1;

            const auto CandidateAddress = BaseAddress.Offset(
                Offset
            );

            if (!CandidateAddress || !CandidateAddress->IsAligned(EfiSystemTableAlignment))
            {
                continue;
            }

            if (Offset + sizeof(EfiSystemTableSignature) <= Bytes.size())
            {
                auto Signature = std::uint64_t{};

                std::memcpy(
                    &Signature,
                    Bytes.data() + Offset,
                    sizeof(Signature)
                );

                if (Signature != EfiSystemTableSignature)
                {
                    continue;
                }
            }

            const auto Table = ReadEfiSystemTable(
                Reader,
                *CandidateAddress
            );

            if (Table)
            {
                return EfiSystemTableMatch
                {
                    .Address = *CandidateAddress,
                    .Table = *Table
                };
            }
        }

        return std::nullopt;
    }

    [[nodiscard]]
    auto FindEfiSystemTable(
        const Physical::PhysicalMemoryReader& Reader
    ) noexcept -> std::expected<EfiSystemTableMatch, NTSTATUS>
    {
        auto Buffer = std::unique_ptr<std::byte[]>
        {
            new (std::nothrow) std::byte[ScanBufferSize]
        };

        if (!Buffer)
        {
            return std::unexpected
            {
                STATUS_INSUFFICIENT_RESOURCES
            };
        }

        auto CurrentAddress = PhysicalScanStart;
        auto RemainingBytes = PhysicalScanSize;

        while (RemainingBytes != 0)
        {
            const auto BytesToRead = static_cast<std::size_t>(
                (std::min)(
                    RemainingBytes,
                    static_cast<std::uint64_t>(ScanBufferSize)
                )
            );

            const auto Address = Physical::PhysicalAddress
            {
                CurrentAddress
            };

            const auto ReadResult = Reader.ReadInto(
                Address,
                std::span<std::byte>
                {
                    Buffer.get(),
                    BytesToRead
                }
            );

            auto ReadableBytes = std::size_t{};
            auto BytesToAdvance = BytesToRead;

            if (ReadResult)
            {
                ReadableBytes = *ReadResult;
            }
            else
            {
                ReadableBytes = (std::min)(
                    ReadResult.error().BytesTransferred,
                    BytesToRead
                );

                const auto FaultAddress = ReadResult.error().FaultAddress;

                if (FaultAddress >= CurrentAddress && FaultAddress - CurrentAddress < BytesToRead)
                {
                    const auto FaultOffset = static_cast<std::size_t>(FaultAddress - CurrentAddress);
                    const auto NextPageOffset = (FaultOffset + PAGE_SIZE) & ~(static_cast<std::size_t>(PAGE_SIZE) - 1);

                    BytesToAdvance = (std::min)(
                        BytesToRead,
                        NextPageOffset
                    );
                }
            }

            if (ReadableBytes >= sizeof(EfiSystemTableSignature))
            {
                const auto Match = ScanBuffer(
                    Reader,
                    Address,
                    std::span<const std::byte>
                    {
                        Buffer.get(),
                        ReadableBytes
                    }
                );

                if (Match)
                {
                    return *Match;
                }
            }

            CurrentAddress += BytesToAdvance;
            RemainingBytes -= BytesToAdvance;
        }

        return std::unexpected
        {
            STATUS_NOT_FOUND
        };
    }

    auto DriverUnload(
        [[maybe_unused]] PDRIVER_OBJECT DriverObject
    ) noexcept -> void
    {
        DbgPrintEx(
            0,
            0,
            "[+] Driver unloaded\n"
        );
    }
}

extern "C"
auto DriverEntry(
    PDRIVER_OBJECT DriverObject,
    [[maybe_unused]] PUNICODE_STRING RegistryPath
) noexcept -> NTSTATUS
{
    DriverObject->DriverUnload = DriverUnload;

    const Physical::PhysicalMemoryReader Reader{};

    const auto Result = FindEfiSystemTable(
        Reader
    );

    if (!Result)
    {
        DbgPrintEx(
            0,
            0,
            "[-] EFI_SYSTEM_TABLE not found (status = 0x%08X)\n",
            static_cast<std::uint32_t>(Result.error())
        );

        return STATUS_SUCCESS;
    }

    const auto& Match = *Result;

    DbgPrintEx(
        0,
        0,
        "[+] EFI_SYSTEM_TABLE found at 0x%I64X\n",
        Match.Address.Value()
    );

    DbgPrintEx(
        0,
        0,
        "[+] Revision=0x%08X HeaderSize=0x%X FirmwareRevision=0x%08X\n",
        Match.Table.Header.Revision,
        Match.Table.Header.HeaderSize,
        Match.Table.FirmwareRevision
    );

    DbgPrintEx(
        0,
        0,
        "[+] FirmwareVendor=0x%I64X RuntimeServices=0x%I64X ConfigurationTable=0x%I64X Entries=%I64u\n",
        Match.Table.FirmwareVendor,
        Match.Table.RuntimeServices,
        Match.Table.ConfigurationTable,
        Match.Table.NumberOfTableEntries
    );

    return STATUS_SUCCESS;
}
