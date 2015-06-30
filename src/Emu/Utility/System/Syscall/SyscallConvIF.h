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


#ifndef __EMULATORUTILITY_SYSCALLCONVIF_H__
#define __EMULATORUTILITY_SYSCALLCONVIF_H__

#include "Interface/OpStateIF.h"

namespace Onikiri {
    class SystemIF;

    namespace EmulatorUtility {
        class OpEmulationState;

        // ターゲットからのシステムコールを変換するクラス
        //
        // SetArg, GetResult の意味はtargetのシステムによって変化するが，典型的には次のようなものを想定：
        // ・SetArg : システムコールの番号，システムコールの引数
        // ・GetResult : システムコールの戻り値，エラーコード
        class SyscallConvIF
        {
        public:
            virtual ~SyscallConvIF() {}

            // システムコールの引数 (index番目) を設定する
            virtual void SetArg(int index, u64 value) = 0;

            // SetArg によって与えられた引数に従ってシステムコールを行う
            virtual void Execute(OpEmulationState* opState) = 0;

            // Exec した結果を得る
            virtual u64 GetResult(int index) = 0;

            // callback するsystemをセット
            virtual void SetSystem(SystemIF* system) = 0;

            static const int RetValueIndex = 0;
            static const int ErrorFlagIndex = 1;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
