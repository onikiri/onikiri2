// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#include <pch.h>
#include "Emu/Utility/System/Syscall/SyscallConstConv.h"

using namespace Onikiri;
using namespace EmulatorUtility;

SyscallConstConvEnum::SyscallConstConvEnum
  (u32* host_consts, u32* target_consts, int consts_size)
    : m_hostConsts(host_consts), m_targetConsts(target_consts),
      m_constsSize(consts_size)
{
}

SyscallConstConvBitwise::SyscallConstConvBitwise
  (u32* host_consts, u32* target_consts, int consts_size)
    : m_hostConsts(host_consts), m_targetConsts(target_consts),
      m_constsSize(consts_size)
{
}

u32 SyscallConstConvEnum::HostToTarget(u32 val)
{
  return Convert(val, m_hostConsts, m_targetConsts);
}

u32 SyscallConstConvEnum::TargetToHost(u32 val)
{
  return Convert(val, m_targetConsts, m_hostConsts);
}

u32 SyscallConstConvBitwise::HostToTarget(u32 val)
{
  return Convert(val, m_hostConsts, m_targetConsts);
}

u32 SyscallConstConvBitwise::TargetToHost(u32 val)
{
  return Convert(val, m_targetConsts, m_hostConsts);
}

// body

u32 SyscallConstConvEnum::Convert(u32 val, u32* from, u32* to)
{
  for (int i = 0; i < m_constsSize; i++, from++, to ++) {
    if (*from == val) {
//      std::cerr << *from << "-> " << *to << std::endl;
      return *to;
    }
  }

  //std::cerr << "SyscallConstConvEnum::Convert : unknown syscall const" << std::endl;
  return 0;
}

u32 SyscallConstConvBitwise::Convert(u32 val, u32* from, u32* to)
{
  u32 result = 0;
  for (int i = 0; i < m_constsSize; i++, from++, to ++) {
    if (val & *from)
      result |= *to;
  }

  //std::cerr << val << "-> " << result << std::endl;

  return result;
}
