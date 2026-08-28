#include <intrin.h>

#include <Memory/Virtual/DirectoryTableBase.hpp>

namespace Memory::Virtual
{
    auto DirectoryTableBase::Current() noexcept -> DirectoryTableBase
    {
        return DirectoryTableBase
        {
            __readcr3()
        };
    }
}
