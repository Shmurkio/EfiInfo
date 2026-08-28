#pragma once

#include <Memory/Address.hpp>

namespace Memory::Virtual
{
    struct VirtualAddressDomain final
    {
    };

    using VirtualAddress = Memory::Address<VirtualAddressDomain>;
}
