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


#ifndef __EMULATORUTILITY_MEMORY_SYSTEM_H__
#define __EMULATORUTILITY_MEMORY_SYSTEM_H__

#include "Emu/Utility/System/Memory/HeapAllocator.h"
#include "Emu/Utility/System/Memory/VirtualMemory.h"

namespace Onikiri {
    namespace EmulatorUtility {



        class MemorySystem
        {
            static const int RESERVED_PAGES = 256; // 鬼斬りで予約するページ
            static const int RESERVED_PAGE_NULL = 0;
            static const int RESERVED_PAGE_ZERO_FILLED = 1;
        public:
            MemorySystem( int pid, bool bigEndian, SystemIF* simSystem );
            ~MemorySystem();

            // メモリ管理
            // システムから見えるページサイズ
            u64 GetPageSize();

            // アドレス0からこれで返すアドレスまでは予約．
            u64 GetReservedAddressRange(){ return GetPageSize() * RESERVED_PAGES - 1; };

            // mmapに使うヒープに，[addr, addr+size) の領域を追加する
            void AddHeapBlock(u64 addr, u64 size);

            // brkの初期値 (ロードされたイメージの末尾) を設定する
            void SetInitialBrk(u64 initialBrk);
            u64 Brk(u64 addr);
            u64 MMap(u64 addr, u64 length);
            u64 MRemap(u64 old_addr, u64 old_size, u64 new_size, bool mayMove = false);
            int MUnmap(u64 addr, u64 length);

            // ビッグエンディアンかどうか
            bool IsBigEndian() const 
            {
                return m_bigEndian;
            }

            u64 GetPageSize() const
            {
                return m_virtualMemory.GetPageSize();
            }

            // [addr, addr+size) を含むページに物理メモリを割り当てる．同上
            void AssignPhysicalMemory(u64 addr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr )
            {
                m_virtualMemory.AssignPhysicalMemory( addr, size, attr );
            }

            // メモリ読み書き
            void ReadMemory( MemAccess* access ) 
            {
                m_virtualMemory.ReadMemory( access );
            }

            void WriteMemory( MemAccess* access )
            {
                m_virtualMemory.WriteMemory( access );
            }


            // メモリ関連のヘルパ
            // targetAddrからsizeバイトにvalueの値を書き込む
            void TargetMemset(u64 targetAddr, int value, u64 size )
            {
                m_virtualMemory.TargetMemset( targetAddr, value, size );
            }
            // target のアドレス src から，host のアドレス dst に size バイトをコピーする
            void MemCopyToHost(void* dst, u64 src, u64 size)
            {
                m_virtualMemory.MemCopyToHost( dst, src, size );
            }
            // host のアドレス src から，target のアドレス dst に size バイトをコピーする
            void MemCopyToTarget(u64 dst, const void* src, u64 size)
            {
                m_virtualMemory.MemCopyToTarget( dst, src, size );
            }

        private:

            // Cehck a value is aligned on a page boundary.
            void CheckValueOnPageBoundary( u64 addr, const char* signature );

            VirtualMemory m_virtualMemory;
            HeapAllocator m_heapAlloc;

            // 実行ファイルが占める領域の終端 (currentBrkを含まず) Brk で拡張
            u64 m_currentBrk;

            // メモリ確保，解放時にシミュレータにコールバックを投げるためのインターフェース
            SystemIF* m_simSystem;

            // PID
            int m_pid;

            // ターゲットがビッグエンディアンかどうか
            bool m_bigEndian;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
