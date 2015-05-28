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


#include <pch.h>
#include "Env/Param/ParamPredResult.h"

using namespace Onikiri;

ParamPredResult::ParamPredResult()
{
    Reset();
}

ParamPredResult::~ParamPredResult()
{
}

void ParamPredResult::Hit  ( s64 add )
{
    m_hitCount += add;
}

void ParamPredResult::Miss ( s64 add )
{
    m_missCount += add;
}

void ParamPredResult::Add  ( bool hit, s64 add )
{
    if(hit){
        Hit(add);
    }
    else{
        Miss(add);
    }
}

void ParamPredResult::Reset()
{
    m_hitCount = 0;
    m_missCount = 0;
}

s64 ParamPredResult::GetHitCount() const
{
    return m_hitCount;
}

s64 ParamPredResult::GetMissCount() const
{
    return m_missCount;
}

void ParamPredResult::SetHitCount( s64 hitCount )
{
    m_hitCount = hitCount;
}

void ParamPredResult::SetMissCount( s64 missCount )
{
    m_missCount = missCount;
}


u64 ParamPredResult::GetTotalCount() const
{
    return m_hitCount + m_missCount;
}

double ParamPredResult::GetHitRate() const
{
    return (double)GetHitCount() / (double)GetTotalCount();
}

void ParamPredResult::SetName( const char* name )
{
    m_name = name;
}

const char* ParamPredResult::GetName()
{
    return m_name;
}

