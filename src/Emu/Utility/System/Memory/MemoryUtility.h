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


#ifndef __EMULATORUTILITY_MEMORY_UTILITY_H__
#define __EMULATORUTILITY_MEMORY_UTILITY_H__

#include "Emu/Utility/System/Memory/MemorySystem.h"

namespace Onikiri {
    namespace EmulatorUtility {

        class EmuMemAccess : public MemAccess
        {
        public:

            EmuMemAccess( u64 address, int size, bool sign = false )
            {
                this->address.address = address;
                this->size = size;
                this->sign = sign;
            }

            EmuMemAccess( u64 address, int size, u64 value )
            {
                this->address.address = address;
                this->size  = size;
                this->value = value;
            }

        };

        // target メモリ中のバッファを host の連続領域に一時的にコピーする．デストラクト時に書き戻す (readOnly のときは書き戻しを行わない)
        // Getにより返されたバッファに書き込むことで，あたかも target のメモリに直接書いたように見える 
        class TargetBuffer
        {
        public:
            TargetBuffer(MemorySystem* memory, u64 targetAddr, size_t size, bool readOnly = false);
            ~TargetBuffer();
            void* Get();
            const void* Get() const;
        private:
            MemorySystem* m_memory;
            u8* m_buf;
            u64 m_targetAddr;
            size_t m_bufSize;
            bool m_readOnly;
        };

        // Utility

        // targetAddr にある文字列の長さを得る
        u64 TargetStrlen(MemorySystem* mem, u64 targetAddr);

        // Get c string data from the targetAddr.
        std::string StrCpyToHost(MemorySystem* mem, u64 targetAddr);

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
