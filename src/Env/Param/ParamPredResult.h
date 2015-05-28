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
// Utility class for prediction results
//

#ifndef __ONIKIRI_PARAM_PRED_RESULT_H
#define __ONIKIRI_PARAM_PRED_RESULT_H

#include "Env/Param/ParamDB.h"
#include "Env/Env.h"

/*
    ParamPredResult m_predResult;
    BEGIN_PARAM_MAP( GetResultPath() )
        RESULT_PRED_ENTRY( "PredictorName", m_predResult )
    END_PARAM_MAP()

*/

namespace Onikiri
{
    #define RESULT_PRED_ENTRY(relativePath, result) \
        { \
            result.SetName(relativePath); \
            CHAIN_PARAM_MAP(relativePath, result); \
        } \

    class ParamPredResult : public ParamExchangeChild
    {
        s64 m_hitCount;
        s64 m_missCount;
        String m_name;
    public:
        BEGIN_PARAM_MAP("")
            RESULT_ENTRY("@Hit",        m_hitCount )
            RESULT_ENTRY("@Miss",       m_missCount )
            RESULT_ENTRY("@Total",      GetTotalCount() )
            RESULT_ENTRY("@HitRate",    GetHitRate() )
        END_PARAM_MAP()

        ParamPredResult();
        ~ParamPredResult();
        void Hit  ( s64 add = 1 );
        void Miss ( s64 add = 1  );
        void Add  ( bool hit, s64 add = 1 );
        void Reset();

        s64 GetHitCount() const;
        s64 GetMissCount() const;
        void SetHitCount( s64 hitCount );
        void SetMissCount( s64 missCount );

        u64 GetTotalCount() const;
        double GetHitRate() const;

        void SetName( const char* name );
        const char* GetName();
    };
}   // namespace Onikiri

#endif


