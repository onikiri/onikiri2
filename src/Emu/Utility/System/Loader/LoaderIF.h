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


#ifndef __EMULATORUTILITY_LOADERIF_H__
#define __EMULATORUTILITY_LOADERIF_H__

namespace Onikiri {
    namespace EmulatorUtility {

        class MemorySystem;

        // ターゲットのバイナリをメモリにロードし，引数を設定するインターフェイス
        // Linux 用かもしれない
        class LoaderIF
        {
        public:
            virtual ~LoaderIF() {}

            // バイナリ command を memory にロードする
            virtual void LoadBinary(MemorySystem* memory, const String& command) = 0;

            // スタック [stackHead, stackHead+stackSize) に引数を設定する
            virtual void InitArgs(MemorySystem* memory, u64 stackHead, u64 stackSize, const String& command, const String& commandArgs) = 0;

            // バイナリのロードされた領域の先頭アドレスを得る
            // ※バイナリが連続領域にロードされることを仮定している
            virtual u64 GetImageBase() const = 0;

            // エントリポイントを得る
            virtual u64 GetEntryPoint() const = 0;

            // コード領域の範囲を返す (開始アドレス，バイト数) (Emulatorの最適化用)
            virtual std::pair<u64, size_t> GetCodeRange() const = 0;

            // レジスタの初期値を得る
            virtual u64 GetInitialRegValue(int index) const = 0;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
