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


#ifndef __EMULATORUTILITY_HEAPALLOCATOR_H__
#define __EMULATORUTILITY_HEAPALLOCATOR_H__

namespace Onikiri {
    namespace EmulatorUtility {

        // ターゲットのヒープ領域を管理する
        // 
        // アドレス空間のみを管理する (物理メモリの割り当ては行わない)
        // 領域の確保はページ単位で行われる
        class HeapAllocator
        {
        private:
            HeapAllocator() {}
        public:
            // pageSize
            explicit HeapAllocator(u64 pageSize);

            // アドレスstartからlengthバイトの領域を，このHeapAllocatorの使用する（空き）領域として追加する
            bool AddMemoryBlock(u64 start, u64 length);

            // addrのアドレスにlengthバイトの領域を確保する
            // addrが0の場合，空き領域の中で適当なアドレスに確保される
            //
            // 戻り値: 確保された領域のアドレス．0の場合エラー
            //        アドレスはページ境界にあることが保証される
            u64 Alloc(u64 addr, u64 length);

            // addrの位置に確保された領域のサイズを old_size から new_size に変更する
            //
            // 戻り値: 領域のアドレス．0の場合エラー
            //        アドレスはページ境界にあることが保証される
            u64 ReAlloc(u64 old_addr, u64 old_size, u64 new_size);

            // Allocしたメモリ領域を開放する
            // 戻り値: 成功時true，失敗時false (管理されていないメモリブロックを渡した等)
            bool Free(u64 addr);
            // Allocした領域の一部または全部を開放する
            bool Free(u64 addr, u64 size);

            // addr にAllocされたメモリ領域のサイズを得る
            u64 GetBlockSize(u64 addr) const;

            // addr は確保されている領域と交差するか
            u64 IsIntersected(u64 addr, u64 length) const;

            u64 GetPageSize() const { return m_pageSize; }

        private:
            struct MemoryBlock
            {
                u64 Addr;
                u64 Bytes;

                explicit MemoryBlock(u64 addr_arg = 0, u64 bytes_arg = 0) : Addr(addr_arg), Bytes(bytes_arg) { }
                bool operator<(const MemoryBlock& mb) const{
                    return Addr < mb.Addr;
                }
                bool operator==(const MemoryBlock& mb) const {
                    return Addr == mb.Addr;
                }
                bool Intersects(const MemoryBlock& mb) const {
                    return (Addr <= mb.Addr && mb.Addr < Addr+Bytes)
                        || (mb.Addr <= Addr && Addr < mb.Addr+mb.Bytes);
                }
                bool Contains(const MemoryBlock& mb) const {
                    return (Addr <= mb.Addr && mb.Addr+mb.Bytes < Addr+Bytes);
                }
            };
            typedef std::list<MemoryBlock> BlockList;

            // m_freeList中の隙間なく隣接しているメモリ領域を1つにまとめる
            void IntegrateFreeBlocks();
            BlockList::iterator FindMemoryBlock(BlockList& blockList, u64 addr);
            BlockList::const_iterator FindMemoryBlock(const BlockList& blockList, u64 addr) const;


            u64 in_pages(u64 bytes) const {
                return (bytes + m_pageSize - 1)/m_pageSize;
            }

            // free listはメモリ領域の中に置かない
            // targetがメモリ破壊してもこのmmapが死なないように
            BlockList m_freeList;
            BlockList m_allocList;
            u64 m_pageSize;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
