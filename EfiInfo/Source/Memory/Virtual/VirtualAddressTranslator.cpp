#include <intrin.h>

#include <Memory/Virtual/VirtualAddressTranslator.hpp>

#include <cstddef>
#include <cstdint>

namespace Memory::Virtual
{
    namespace
    {
        class PageTableEntry final
        {
        public:
            explicit constexpr PageTableEntry(
                const std::uint64_t Value
            ) noexcept : Value_{ Value }
            {
            }

            [[nodiscard]]
            constexpr auto Value() const noexcept -> std::uint64_t
            {
                return Value_;
            }

            [[nodiscard]]
            constexpr auto IsPresent() const noexcept -> bool
            {
                return (Value_ & 0x1) != 0;
            }

            [[nodiscard]]
            constexpr auto IsWritable() const noexcept -> bool
            {
                return (Value_ & 0x2) != 0;
            }

            [[nodiscard]]
            constexpr auto IsUserAccessible() const noexcept -> bool
            {
                return (Value_ & 0x4) != 0;
            }

            [[nodiscard]]
            constexpr auto IsLargePage() const noexcept -> bool
            {
                return (Value_ & 0x80) != 0;
            }

            [[nodiscard]]
            constexpr auto IsExecutable() const noexcept -> bool
            {
                return (Value_ & (1ULL << 63)) == 0;
            }

            [[nodiscard]]
            constexpr auto TableAddress(
                const std::uint64_t PhysicalAddressMask
            ) const noexcept -> Physical::PhysicalAddress
            {
                return Physical::PhysicalAddress
                {
                    Value_ & PhysicalAddressMask
                };
            }

        private:
            std::uint64_t Value_{};
        };

        [[nodiscard]]
        auto PhysicalAddressMask() noexcept -> std::uint64_t
        {
            int Information[4]{};
            __cpuid(
                Information,
                0x8000'0000
            );

            auto BitCount = std::uint32_t{ 36 };

            if (static_cast<std::uint32_t>(Information[0]) >= 0x8000'0008)
            {
                __cpuid(
                    Information,
                    0x8000'0008
                );

                BitCount = static_cast<std::uint32_t>(Information[0]) & 0xFF;
            }

            if (BitCount < 36)
            {
                BitCount = 36;
            }

            if (BitCount > 52)
            {
                BitCount = 52;
            }

            return ((1ULL << BitCount) - 1) & ~0xFFFULL;
        }

        [[nodiscard]]
        auto UsesFiveLevelPaging() noexcept -> bool
        {
            return (__readcr4() & (1ULL << 12)) != 0;
        }

        [[nodiscard]]
        constexpr auto IsCanonical(
            const std::uint64_t Address,
            const std::uint8_t BitCount
        ) noexcept -> bool
        {
            const auto UpperMask = ~((1ULL << BitCount) - 1);
            const auto UpperBits = Address & UpperMask;
            const auto SignBit = (Address >> (BitCount - 1)) & 0x1;

            return SignBit == 0 ? UpperBits == 0 : UpperBits == UpperMask;
        }

        [[nodiscard]]
        constexpr auto Index(
            const VirtualAddress Address,
            const std::uint8_t Shift
        ) noexcept -> std::uint16_t
        {
            return static_cast<std::uint16_t>((Address.Value() >> Shift) & 0x1FF);
        }
    }

    auto VirtualAddressTranslator::ReadEntry(
        const Physical::PhysicalAddress Table,
        const std::uint16_t Index,
        const VirtualAddress Address,
        const std::uint8_t Level
    ) const noexcept -> std::expected<std::uint64_t, MemoryError>
    {
        const auto EntryAddress = Table.Offset(
            static_cast<std::size_t>(Index) * sizeof(std::uint64_t)
        );

        if (!EntryAddress)
        {
            auto Error = EntryAddress.error();
            Error.FaultAddress = Address.Value();
            Error.PageTableLevel = Level;

            return std::unexpected
            {
                Error
            };
        }

        const auto Entry = PhysicalReader_.Read<std::uint64_t>(
            *EntryAddress
        );

        if (!Entry)
        {
            auto Error = Entry.error();
            Error.FaultAddress = Address.Value();
            Error.PageTableLevel = Level;

            return std::unexpected
            {
                Error
            };
        }

        return *Entry;
    }

    auto VirtualAddressTranslator::Translate(
        const VirtualAddress Address,
        const DirectoryTableBase DirectoryTable
    ) const noexcept -> std::expected<VirtualAddressTranslation, MemoryError>
    {
        if (!DirectoryTable)
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::InvalidDirectoryTableBase,
                    .FaultAddress = Address.Value()
                }
            };
        }

        const auto FiveLevelPaging = UsesFiveLevelPaging();
        const auto CanonicalBitCount = static_cast<std::uint8_t>(FiveLevelPaging ? 57 : 48);

        if (!IsCanonical(Address.Value(), CanonicalBitCount))
        {
            return std::unexpected
            {
                MemoryError
                {
                    .Code = MemoryErrorCode::NonCanonicalAddress,
                    .FaultAddress = Address.Value()
                }
            };
        }

        const auto AddressMask = PhysicalAddressMask();
        auto Table = Physical::PhysicalAddress
        {
            DirectoryTable.RootAddress().Value() & AddressMask
        };

        auto Writable = true;
        auto UserAccessible = true;
        auto Executable = true;

        const auto ApplyPermissions = [&](const PageTableEntry Entry)
        {
            Writable = Writable && Entry.IsWritable();
            UserAccessible = UserAccessible && Entry.IsUserAccessible();
            Executable = Executable && Entry.IsExecutable();
        };

        const auto LoadEntry = [&](
            const std::uint8_t Shift,
            const std::uint8_t Level
        ) -> std::expected<PageTableEntry, MemoryError>
        {
            const auto Value = ReadEntry(
                Table,
                Index(Address, Shift),
                Address,
                Level
            );

            if (!Value)
            {
                return std::unexpected
                {
                    Value.error()
                };
            }

            const auto Entry = PageTableEntry
            {
                *Value
            };

            if (!Entry.IsPresent())
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::PageNotPresent,
                        .FaultAddress = Address.Value(),
                        .PageTableLevel = Level
                    }
                };
            }

            ApplyPermissions(
                Entry
            );

            return Entry;
        };

        const auto Advance = [&](
            const PageTableEntry Entry,
            const std::uint8_t Level
        ) -> std::expected<void, MemoryError>
        {
            if (Entry.IsLargePage())
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::InvalidPageTableEntry,
                        .FaultAddress = Address.Value(),
                        .PageTableLevel = Level
                    }
                };
            }

            Table = Entry.TableAddress(
                AddressMask
            );

            if (!Table)
            {
                return std::unexpected
                {
                    MemoryError
                    {
                        .Code = MemoryErrorCode::InvalidPageTableEntry,
                        .FaultAddress = Address.Value(),
                        .PageTableLevel = Level
                    }
                };
            }

            return {};
        };

        if (FiveLevelPaging)
        {
            const auto Entry = LoadEntry(
                48,
                5
            );

            if (!Entry)
            {
                return std::unexpected
                {
                    Entry.error()
                };
            }

            const auto Result = Advance(
                *Entry,
                5
            );

            if (!Result)
            {
                return std::unexpected
                {
                    Result.error()
                };
            }
        }

        const auto Level4Entry = LoadEntry(
            39,
            4
        );

        if (!Level4Entry)
        {
            return std::unexpected
            {
                Level4Entry.error()
            };
        }

        const auto Level4Result = Advance(
            *Level4Entry,
            4
        );

        if (!Level4Result)
        {
            return std::unexpected
            {
                Level4Result.error()
            };
        }

        const auto Level3Entry = LoadEntry(
            30,
            3
        );

        if (!Level3Entry)
        {
            return std::unexpected
            {
                Level3Entry.error()
            };
        }

        if (Level3Entry->IsLargePage())
        {
            constexpr auto PageSize = static_cast<std::uint64_t>(VirtualPageSize::Size1Gb);
            const auto Offset = Address.Value() & (PageSize - 1);
            const auto Base = Level3Entry->Value() & AddressMask & ~(PageSize - 1);

            return VirtualAddressTranslation
            {
                .PhysicalAddress = Physical::PhysicalAddress{ Base + Offset },
                .ContiguousBytes = static_cast<std::size_t>(PageSize - Offset),
                .PageSize = VirtualPageSize::Size1Gb,
                .Writable = Writable,
                .UserAccessible = UserAccessible,
                .Executable = Executable
            };
        }

        const auto Level3Result = Advance(
            *Level3Entry,
            3
        );

        if (!Level3Result)
        {
            return std::unexpected
            {
                Level3Result.error()
            };
        }

        const auto Level2Entry = LoadEntry(
            21,
            2
        );

        if (!Level2Entry)
        {
            return std::unexpected
            {
                Level2Entry.error()
            };
        }

        if (Level2Entry->IsLargePage())
        {
            constexpr auto PageSize = static_cast<std::uint64_t>(VirtualPageSize::Size2Mb);
            const auto Offset = Address.Value() & (PageSize - 1);
            const auto Base = Level2Entry->Value() & AddressMask & ~(PageSize - 1);

            return VirtualAddressTranslation
            {
                .PhysicalAddress = Physical::PhysicalAddress{ Base + Offset },
                .ContiguousBytes = static_cast<std::size_t>(PageSize - Offset),
                .PageSize = VirtualPageSize::Size2Mb,
                .Writable = Writable,
                .UserAccessible = UserAccessible,
                .Executable = Executable
            };
        }

        const auto Level2Result = Advance(
            *Level2Entry,
            2
        );

        if (!Level2Result)
        {
            return std::unexpected
            {
                Level2Result.error()
            };
        }

        const auto Level1Entry = LoadEntry(
            12,
            1
        );

        if (!Level1Entry)
        {
            return std::unexpected
            {
                Level1Entry.error()
            };
        }

        constexpr auto PageSize = static_cast<std::uint64_t>(VirtualPageSize::Size4Kb);
        const auto Offset = Address.Value() & (PageSize - 1);
        const auto Base = Level1Entry->Value() & AddressMask;

        return VirtualAddressTranslation
        {
            .PhysicalAddress = Physical::PhysicalAddress{ Base + Offset },
            .ContiguousBytes = static_cast<std::size_t>(PageSize - Offset),
            .PageSize = VirtualPageSize::Size4Kb,
            .Writable = Writable,
            .UserAccessible = UserAccessible,
            .Executable = Executable
        };
    }
}
