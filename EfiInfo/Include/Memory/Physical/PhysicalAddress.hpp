#pragma once

#include <Memory/Address.hpp>

namespace Memory::Physical
{
    struct PhysicalAddressDomain final
    {
    };

    using PhysicalAddress = Memory::Address<PhysicalAddressDomain>;
}
