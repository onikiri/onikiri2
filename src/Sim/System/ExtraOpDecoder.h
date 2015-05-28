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


//
// 拡張命令デコーダのフック
// SystemManager から
// EmulatorIF::SetExtraOpDecoder( ExtraOpDecoderIF* )
// を通じてエミュレータに登録を行う．
// ユーザーは，s_extraOpDecodeHook を介してデコードを行う．
//

#ifndef __ONIKIRI_SYSTEM_MANAGER_EXTRA_OP_MANAGER_H__
#define __ONIKIRI_SYSTEM_MANAGER_EXTRA_OP_MANAGER_H__

#include "Interface/ExtraOpDecoderIF.h"
#include "Sim/Foundation/Hook/Hook.h"

namespace Onikiri
{
    struct ExtraOpDecodeArgs
    {
        u32 codeWord;
        std::pair<ExtraOpInfoIF**, int>* decodedOps;
        bool decoded;
    };

    class ExtraOpDecoder : public ExtraOpDecoderIF
    {
    public:
        ExtraOpDecoder();
        bool Decode( u32 codeWord, std::pair<ExtraOpInfoIF**, int>* decodedOps );

        // Prototype : void Decode( HookParameter<ExtraOpDecoder, ExtraOpDecodeArgs>* args )
        static HookPoint<ExtraOpDecoder, ExtraOpDecodeArgs> s_extraOpDecodeHook;
    };
}

#endif // __ONIKIRI_SYSTEM_MANAGER_EXTRA_OP_MANAGER_H__
