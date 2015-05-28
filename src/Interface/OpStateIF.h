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


#ifndef INTERFACE_OP_STATE_IF_H
#define INTERFACE_OP_STATE_IF_H

#include "Interface/MemIF.h"
#include "Interface/ResourceIF.h"

namespace Onikiri
{
    // Op の dynamic な状態を知るためのクラス
    class OpStateIF : public MemIF
    {
    public:
        // 今まで： 直接このクラスのメンバ変数を見てた
        // 鬼斬２： 関数でシミュレータ側の情報を読む (高速化のことは考えない)

        virtual ~OpStateIF(){};

        // PC
        virtual PC GetPC() const = 0;

        // エミュレータがソースレジスタの値を知るための関数 
        virtual const u64 GetSrc(const int index) const = 0;

        // エミュレータが実行の結果を教える 
        virtual void SetDst(const int index, const u64 value) = 0;
        virtual const u64 GetDst(const int index) const       = 0;

        // エミュレータが実行の結果得られるnext_PCを教える 
        virtual void SetTakenPC(const PC takenPC) = 0;
        virtual PC GetTakenPC() const = 0;

        // エミュレータがtaken/not takenを教える 
        virtual void SetTaken(const bool taken) = 0;
        virtual bool GetTaken() const = 0;
        
    };


}; // namespace Onikiri


#endif // __OPSTATEIF_H__

