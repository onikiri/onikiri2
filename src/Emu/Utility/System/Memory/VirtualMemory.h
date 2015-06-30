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


#ifndef __EMULATORUTILITY_VIRTUAL_MEMORY_H__
#define __EMULATORUTILITY_VIRTUAL_MEMORY_H__

#include "Emu/Utility/System/Memory/HeapAllocator.h"


namespace Onikiri {
    namespace EmulatorUtility {

        // Page size
        static const int VIRTUAL_MEMORY_PAGE_SIZE_BITS = 12;
        
        // Page attribute
        typedef u32 VIRTUAL_MEMORY_ATTR_TYPE;
        static const u32 VIRTUAL_MEMORY_ATTR_READ  = 1 << 0;    // readable
        static const u32 VIRTUAL_MEMORY_ATTR_WRITE = 1 << 1;    // writable
        static const u32 VIRTUAL_MEMORY_ATTR_EXEC  = 1 << 2;    // executable

        // Information about a physical memory page.
        // Each instance of this structure corresponds to each 'physical' page.
        struct PhysicalMemoryPage
        {
            u8* ptr;        // Physical memory address
            int refCount;   // A reference count for a copy-on-write function.
        };

        // Page table entry
        // Each instance of this structure corresponds to each 'logical' page.
        struct PageTableEntry
        {
            PhysicalMemoryPage*      phyPage;   // Physical page
            VIRTUAL_MEMORY_ATTR_TYPE attr;      // Page attribute
        };

        // 1-entry TLB
        class TLB
        {
        protected:
            u64   m_addr;
            bool  m_valid;
            PageTableEntry m_body;
            u64 m_offsetBits;
            u64 m_addrMask;

        public:
            TLB( int offsetBits );
            ~TLB();
            bool Lookup( u64 addr, PageTableEntry* entry ) const;
            void Write( u64 addr, const PageTableEntry& entry ); 
            void Flush();
        };

        // target のアドレスから host のアドレスへの変換を行うクラス
        class PageTable
        {
        public:
            // マップ単位をオフセットで指定して PageTable を構築する (PageSize = 1 << offsetBits)
            explicit PageTable(int offsetBits);
            ~PageTable();

            // 構築時に設定されたページサイズを得る
            size_t GetPageSize() const;

            // 構築時に設定されたオフセットのビット数を得る
            int GetOffsetBits() const;

            // アドレスのページ外部分のみのマスク
            u64 GetOffsetMask() const;

            // target のアドレス addr に割り当てられている host の物理メモリアドレスを得る
            void *TargetToHost(u64 addr);

            // target アドレス空間のtargetAddrを含むマップ単位に，hostAddr の物理メモリを割り当てる (PageSize バイト)
            // Set an attribute ('attr') to target page.
            void AddMap(u64 targetAddr, u8* hostAddr, VIRTUAL_MEMORY_ATTR_TYPE attr);
            bool IsMapped(u64 targetAddr) const;

            // Copy address mapping for copy-on-write.
            // Set an attribute ('dstAttr') to target page.
            void CopyMap( u64 dstTargetAddr, u64 srcTargetAddr, VIRTUAL_MEMORY_ATTR_TYPE dstAttr );

            // targetAddrを含むマップ単位に割り当てられたエントリを書き込む/得る
            bool GetMap( u64 targetAddr, PageTableEntry* page );
            bool SetMap( u64 targetAddr, const PageTableEntry& page );

            // Get a reference count of a page including targetAddr
            int GetPageReferenceCount( u64 targetAddr ); 

            // targetAddrを含むマップ単位への割り当てを解除する
            // 返り値は解除後のリファレンスカウント
            int RemoveMap(u64 targetAddr);

        private:
            class AddrHash
            {
            private:
                int m_offsetBits;
            public:
                explicit AddrHash(int offsetBits) : m_offsetBits(offsetBits) {}

                size_t operator()(const u64 value) const
                {
                    return (size_t)(value >> m_offsetBits);
                }
            };
            typedef unordered_map<u64, PageTableEntry, AddrHash > map_type;
            map_type m_map;
            int m_offsetBits;
            u64 m_offsetMask;
            TLB m_tlb;
            boost::pool<> m_phyPagePool;
        };


        class VirtualMemory
        {
        public:
            VirtualMemory( int pid, bool bigEndian, SystemIF* simSystem );
            ~VirtualMemory();

            // メモリ読み書き
            void ReadMemory( MemAccess* access );
            void WriteMemory( MemAccess* access );

            //
            // Helper methods for memory access.
            // These methods are implemented in this class because
            // implementation using ReadMemory/WriteMemory is too slow.
            //
            // Note: These functions ignore page attribute.
            //

            // targetAddrからsizeバイトにvalueの値を書き込む
            void TargetMemset(u64 targetAddr, int value, u64 size);
            // target のアドレス src から，host のアドレス dst に size バイトをコピーする
            void MemCopyToHost(void* dst, u64 src, u64 size);
            // host のアドレス src から，target のアドレス dst に size バイトをコピーする
            void MemCopyToTarget(u64 dst, const void* src, u64 size);

            // メモリ管理
            u64 GetPageSize() const;

            // addr を含むページに物理メモリを割り当てる．AddHeapBlockした領域と重なっていてはならない
            void AssignPhysicalMemory(u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr);
            // [addr, addr+size) を含むページに物理メモリを割り当てる．同上
            void AssignPhysicalMemory(u64 addr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr);
            // addr を含むページに割り当てた物理メモリを解放する
            void FreePhysicalMemory(u64 addr);
            // [addr, addr+size) を含むページに割り当てた物理メモリを解放する
            void FreePhysicalMemory(u64 addr, u64 size);
            
            // dstAddr を含むページに srcAddr を含むページに現在割り当てられている物理メモリを割り当てる．
            void SetPhysicalMemoryMapping( u64 dstAddr, u64 srcAddr, VIRTUAL_MEMORY_ATTR_TYPE attr );
            // [dstAddr, dstAddr+size) を含むページに物理メモリマッピングをセットする．同上
            void SetPhysicalMemoryMapping( u64 dstAddr, u64 srcAddr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr );

            // addr を含むページに物理メモリを割り当てて，addr を含むページに現在割り当てられていたデータをコピー
            void AssignAndCopyPhysicalMemory( u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr  );
            // [dstAddr, dstAddr+size) を含むページに同上
            void AssignAndCopyPhysicalMemory( u64 addr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr );

            // Write a page attribute to a page including 'addr'.
            void SetPageAttribute( u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr );

            // Copy-on-Write for a memory page including addr.
            // Return whether copy-on-write is done or not.
            bool CopyPageOnWrite( u64 addr );

            // ビッグエンディアンかどうか
            bool IsBigEndian() const {
                return m_bigEndian;
            }

        private:
            // addr から size バイトのメモリ領域を，マップ単位境界で分割する
            // 結果は，MemoryBlockのコンテナへのイテレータ Iter を通して格納する
            // 戻り値は分割された個数
            // ※ Iterは，典型的にはMemoryBlockのコンテナのinserter
            template <typename Iter>
            int SplitAtMapUnitBoundary(u64 addr, u64 size, Iter e) const;

            struct MemoryBlock {
                MemoryBlock(u64 addr, u64 size) { this->addr = addr; this->size = size; }
                u64 addr;
                u64 size;
            };
            typedef std::vector<MemoryBlock> BlockArray;

            // PageTable & FreeList
            PageTable m_pageTbl;
            boost::pool<> m_pool;

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
