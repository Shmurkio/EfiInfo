#pragma once

#include <Memory/MemoryRegion.hpp>
#include <Memory/Physical/PhysicalAddress.hpp>

namespace Memory::Physical
{
    using PhysicalMemoryRegion = Memory::MemoryRegion<PhysicalAddress>;
}
