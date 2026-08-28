#include <ntifs.h>
#include <intrin.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <new>

namespace
{
    constexpr ULONG PoolTag = static_cast<ULONG>('E') | static_cast<ULONG>('f') << 8 | static_cast<ULONG>('i') << 16 | static_cast<ULONG>('S') << 24;

    struct AllocationHeader final
    {
        void* Allocation;
    };

    [[nodiscard]]
    constexpr auto IsPowerOfTwo(
        const std::size_t Value
    ) noexcept -> bool
    {
        return Value != 0 && (Value & (Value - 1)) == 0;
    }

    [[nodiscard]]
    auto Allocate(
        const std::size_t RequestedSize,
        std::size_t Alignment
    ) noexcept -> void*
    {
        if (Alignment < alignof(std::max_align_t))
        {
            Alignment = alignof(std::max_align_t);
        }

        if (!IsPowerOfTwo(Alignment))
        {
            return nullptr;
        }

        const auto Size = RequestedSize == 0 ? std::size_t{1} : RequestedSize;
        constexpr auto Maximum = (std::numeric_limits<std::size_t>::max)();

        if (Size > Maximum - sizeof(AllocationHeader) - (Alignment - 1))
        {
            return nullptr;
        }

        const auto AllocationSize = Size + sizeof(AllocationHeader) + Alignment - 1;

        auto* const Allocation = ExAllocatePool2(
            POOL_FLAG_NON_PAGED,
            AllocationSize,
            PoolTag
        );

        if (Allocation == nullptr)
        {
            return nullptr;
        }

        const auto FirstAddress = reinterpret_cast<std::uintptr_t>(Allocation) + sizeof(AllocationHeader);
        const auto AlignedAddress = (FirstAddress + Alignment - 1) & ~(static_cast<std::uintptr_t>(Alignment) - 1);

        auto* const Header = reinterpret_cast<AllocationHeader*>(AlignedAddress) - 1;
        Header->Allocation = Allocation;

        return reinterpret_cast<void*>(AlignedAddress);
    }

    [[nodiscard]]
    auto AllocateOrFail(
        const std::size_t Size,
        const std::size_t Alignment
    ) -> void*
    {
        if (auto* const Allocation = Allocate(Size, Alignment))
        {
            return Allocation;
        }

        __fastfail(
            FAST_FAIL_FATAL_APP_EXIT
        );

        __assume(
            false
        );
    }

    void Free(
        void* const Address
    ) noexcept
    {
        if (Address == nullptr)
        {
            return;
        }

        const auto* const Header = reinterpret_cast<const AllocationHeader*>(Address) - 1;

        ExFreePoolWithTag(
            Header->Allocation,
            PoolTag
        );
    }
}

void* operator new(
    const std::size_t Size
)
{
    return AllocateOrFail(
        Size,
        alignof(std::max_align_t)
    );
}

void* operator new[](
    const std::size_t Size
)
{
    return AllocateOrFail(
        Size,
        alignof(std::max_align_t)
    );
}

void* operator new(
    const std::size_t Size,
    const std::align_val_t Alignment
)
{
    return AllocateOrFail(
        Size,
        static_cast<std::size_t>(Alignment)
    );
}

void* operator new[](
    const std::size_t Size,
    const std::align_val_t Alignment
)
{
    return AllocateOrFail(
        Size,
        static_cast<std::size_t>(Alignment)
    );
}

void* operator new(
    const std::size_t Size,
    const std::nothrow_t&
) noexcept
{
    return Allocate(
        Size,
        alignof(std::max_align_t)
    );
}

void* operator new[](
    const std::size_t Size,
    const std::nothrow_t&
) noexcept
{
    return Allocate(
        Size,
        alignof(std::max_align_t)
    );
}

void* operator new(
    const std::size_t Size,
    const std::align_val_t Alignment,
    const std::nothrow_t&
) noexcept
{
    return Allocate(
        Size,
        static_cast<std::size_t>(Alignment)
    );
}

void* operator new[](
    const std::size_t Size,
    const std::align_val_t Alignment,
    const std::nothrow_t&
) noexcept
{
    return Allocate(
        Size,
        static_cast<std::size_t>(Alignment)
    );
}

void operator delete(
    void* const Address
    ) noexcept
{
    Free(
        Address
    );
}

void operator delete[](
    void* const Address
    ) noexcept
{
    Free(
        Address
    );
}

void operator delete(
    void* const Address,
    [[maybe_unused]] const std::size_t Size
) noexcept
{
    Free(
        Address
    );
}

void operator delete[](
    void* const Address,
    [[maybe_unused]] const std::size_t Size
) noexcept
{
    Free(
        Address
    );
}

void operator delete(
    void* const Address,
    [[maybe_unused]] const std::align_val_t Alignment
) noexcept
{
    Free(
        Address
    );
}

void operator delete[](
    void* const Address,
    [[maybe_unused]] const std::align_val_t Alignment
) noexcept
{
    Free(
        Address
    );
}

void operator delete(
    void* const Address,
    [[maybe_unused]] const std::size_t Size,
    [[maybe_unused]] const std::align_val_t Alignment
) noexcept
{
    Free(
        Address
    );
}

void operator delete[](
    void* const Address,
    [[maybe_unused]] const std::size_t Size,
    [[maybe_unused]] const std::align_val_t Alignment
) noexcept
{
    Free(
        Address
    );
}

void operator delete(
    void* const Address,
    const std::nothrow_t&
) noexcept
{
    Free(
        Address
    );
}

void operator delete[](
    void* const Address,
    const std::nothrow_t&
) noexcept
{
    Free(
        Address
    );
}

void operator delete(
    void* const Address,
    [[maybe_unused]] const std::align_val_t Alignment,
    const std::nothrow_t&
) noexcept
{
    Free(
        Address
    );
}

void operator delete[](
    void* const Address,
    [[maybe_unused]] const std::align_val_t Alignment,
    const std::nothrow_t&
) noexcept
{
    Free(
        Address
    );
}

std::_Prhand std::_Raise_handler{};

namespace std
{
    [[noreturn]]
    void __cdecl _Xlength_error(
        [[maybe_unused]] const char* Message
    )
    {
        __fastfail(
            FAST_FAIL_INVALID_ARG
        );
    }
}
