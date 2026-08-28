#pragma once

#include <Memory/MemoryRegion.hpp>
#include <Memory/Virtual/VirtualAddress.hpp>

namespace Memory::Virtual
{
    using VirtualMemoryRegion = Memory::MemoryRegion<VirtualAddress>;
}
