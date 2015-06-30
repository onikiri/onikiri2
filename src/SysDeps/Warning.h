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
// Environmentally dependent warnings
//

#ifndef SYSDEPS_WARNING_H
#define SYSDEPS_WARNING_H

#include "SysDeps/host_type.h"

#ifdef COMPILER_IS_MSVC

#pragma warning( disable: 4100 )    // 引数は関数の本体部で 1 度も参照されません
#pragma warning( disable: 4127 )    // 条件式が定数です
#pragma warning( disable: 4512 )    // 代入演算子を生成できません
#pragma warning( disable: 4702 )    // 制御が渡らないコードです
#pragma warning( disable: 4714 )    // インライン関数ではなく、__forceinline として記述されています


// 以下の警告はC++標準通りの動作を行うことを示しているにすぎないのでdisable
#pragma warning( disable: 4345 )    // POD型のデフォルトコンストラクタに関する警告

#endif


#endif
