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


#ifndef __EMULATORUTILITY_LINUX64LOADER_H__
#define __EMULATORUTILITY_LINUX64LOADER_H__

#include "Emu/Utility/System/Loader/LoaderIF.h"

namespace Onikiri {

    namespace EmulatorUtility {
        
        class MemorySystem;
        class ElfReader;
        
        // 64-bit Linux (ELF) 用のローダー
        class Linux64Loader : public LoaderIF
        {
        public:
            Linux64Loader(u16 machine);
            virtual ~Linux64Loader();

            // LoaderIF の実装
            virtual void LoadBinary(EmulatorUtility::MemorySystem* memory, const String& command);
            virtual void InitArgs(EmulatorUtility::MemorySystem* memory, u64 stackHead, u64 stackSize, const String& command, const String& commandArgs);
            virtual u64 GetImageBase() const;
            virtual u64 GetEntryPoint() const;
            virtual std::pair<u64, size_t> GetCodeRange() const;
            virtual u64 GetInitialRegValue(int index) const = 0;
        protected:
            virtual u64 CalculateEntryPoint(EmulatorUtility::MemorySystem* memory, const EmulatorUtility::ElfReader& elfReader);
            virtual void CalculateOthers(EmulatorUtility::MemorySystem* memory, const EmulatorUtility::ElfReader& elfReader);
            u64 GetInitialSp() const;
            bool IsBigEndian() const;
        private:
            std::pair<u64, size_t> m_codeRange;

            // Memory write
            void WriteMemory( MemorySystem* memory, u64 address, int size, u64 value );

            // Endian
            bool m_bigEndian;
            // 実行ファイルを読み込んだアドレス
            u64 m_imageBase;
            // エントリーポイント
            u64 m_entryPoint;
            // SPの初期値
            u64 m_initialSp;

            u64 m_elfProgramHeaderOffset;
            u64 m_elfProgramHeaderCount;

            // 読み込むELFファイルのELFマシン種別
            u16 m_machine;
        };

    } // namespace AlphaLinux
} // namespace Onikiri

#endif
