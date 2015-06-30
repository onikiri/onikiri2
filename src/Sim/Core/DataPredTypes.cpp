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
#include "Sim/Core/DataPredTypes.h"
#include "Utility/RuntimeError.h"

using namespace Onikiri;


void DataPredMissRecovery::Validate()
{
    if( m_policy == POLICY_END || m_from == FROM_END ){
        THROW_RUNTIME_ERROR( "A policy or a recovery point is not set." );
    }

    if( m_policy == POLICY_REDISPATCH_ALL ){
        THROW_RUNTIME_ERROR( "A recovery policy 'Redispatch' is not implemented." );
    }

    if( m_policy == POLICY_REISSUE_SELECTIVE && m_from == FROM_NEXT_OF_PRODUCER ){
        THROW_RUNTIME_ERROR( "Cannot reschedule ops based on a recovery policy 'ReissueSelective' from a 'NextOfProducer' op." );
    }

/*  if( m_policy == POLICY_REFETCH && m_from != FROM_NEXT_OF_PRODUCER ){
        THROW_RUNTIME_ERROR( "A recovery policy 'Refetch' From' Consumer' or 'Producer' is not implemented." );
    }*/
}

//
// Instruction types where each speculation requires check-pointing.
//                          Producer / Consumer
//  LatPredRecovery:        load     / any
//  AddrMatchPredRecovery:  store    / load
//  ValuePredRecovery:      any      / any
//  PartialLoadRecovery:    store    / load
//
bool DataPredMissRecovery::IsRequiredBeforeCheckpoint( const OpClass& opClass ) const
{
    if( m_policy != POLICY_REFETCH ){
        // An only re-fetch policy requires check-pointing.
        return false;
    }

    switch( m_type ){
    case TYPE_LATENCY:  
        // Producer : load
        if( opClass.IsLoad() && m_from == FROM_PRODUCER ){ 
            return true;
        }
        // Consumer : any
        if( m_from == FROM_CONSUMER ){ 
            return true;
        }
        break;

    case TYPE_ADDRESS_MATCH:
    case TYPE_PARTIAL_LOAD:
        // Producer : store
        if( opClass.IsStore() && m_from == FROM_PRODUCER ){ 
            return true;
        }
        // Consumer : load
        if( opClass.IsLoad() && m_from == FROM_CONSUMER ){ 
            return true;
        }
        break;

    case TYPE_VALUE:
        // Not implemented...
        /*
        // Producer : any
        if( m_from == FROM_PRODUCER ){ 
            return true;
        }
        // Consumer : any
        if( m_from == FROM_CONSUMER ){ 
            return true;
        }
        */
        break;

    default:
        ASSERT( false, "An unknown recovery type." );
        break;
    }

    return false;
}

bool DataPredMissRecovery::IsRequiredAfterCheckpoint ( const OpClass& opClass ) const
{
    if( m_policy != POLICY_REFETCH ){
        // An only re-fetch policy requires check-pointing.
        return false;
    }

    switch( m_type ){
    case TYPE_LATENCY:  
        // Producer : load
        if( opClass.IsLoad() && m_from == FROM_NEXT_OF_PRODUCER ){ 
            return true;
        }
        break;

    case TYPE_ADDRESS_MATCH:
    case TYPE_PARTIAL_LOAD:
        // Producer : store
        if( opClass.IsStore() && m_from == FROM_NEXT_OF_PRODUCER ){ 
            return true;
        }
        break;

    case TYPE_VALUE:
        // Not implemented...
        /*
        // Producer : any
        if( m_from == FROM_NEXT_OF_PRODUCER ){ 
            return true;
        }
        // Consumer : any
        if( m_from == FROM_NEXT_OF_PRODUCER ){ 
            return true;
        }
        */
        break;

    default:
        ASSERT( false, "An unknown recovery type." );
        break;
    }

    return false;
}
