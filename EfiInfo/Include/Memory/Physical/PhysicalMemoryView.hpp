#pragma once

#include <Memory/MemoryView.hpp>
#include <Memory/Physical/PhysicalAddress.hpp>

namespace Memory::Physical
{
    using PhysicalMemoryView = Memory::MemoryView<PhysicalAddress>;
}
