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


#ifndef MEM_ORDER_OPERATIONS_H
#define MEM_ORDER_OPERATIONS_H

#include "Interface/MemAccess.h"

namespace Onikiri 
{

    class MemOrderOperations
    {
    public:

        MemOrderOperations();
        void SetTargetEndian( bool targetIsLittleEndian )
        {
            m_targetIsLittleEndian = targetIsLittleEndian;
        }

        void SetAlignment( int alignment );

        // For store-load forwarding on a LS-queue.
        u64 ReadPreviousAccess( const MemAccess& load, const MemAccess& store );

        // Merge 2 accesses to a single value.
        u64 MergePartialAccess( const MemAccess& base, const MemAccess& store );

        // Convert endian for store-load forwarding.
        u64 CorrectEndian( u64 src, int size );

        // アドレスの範囲が重なっているか
        bool IsOverlapped( const MemAccess& access1, const MemAccess& access2 ) const;
        bool IsOverlapped( u64 addr1, int size1, u64 addr2, int size2 ) const;

        bool IsOverlappedInAligned( const MemAccess& access1, const MemAccess& access2 ) const;
        bool IsOverlappedInAligned( u64 addr1, int size1, u64 addr2, int size2 ) const;

        // Returns whether a 'inner' access is in a 'outer' access or not.
        bool IsInnerAccess( const MemAccess& inner, const MemAccess& outer ) const;
        bool IsInnerAccess( u64 inner, int innerSize, u64 outer, int outerSize ) const;

    protected:
        bool m_targetIsLittleEndian;    // Whether a target architecture is little endian or not.
        int  m_memoryAlignment;
        u64  m_memoryAlignmentMask;
    };
}; // namespace Onikiri

#endif // MEM_ORDER_OPERATIONS_H

