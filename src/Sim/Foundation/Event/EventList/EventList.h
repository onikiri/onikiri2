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
// Event list
//

#ifndef SIM_FOUNDATION_EVENT_EVENT_LIST_H
#define SIM_FOUNDATION_EVENT_EVENT_LIST_H

#include "Sim/Foundation/Event/EventBase.h"

namespace Onikiri
{
#if 1
    // vector を使ったナイーブな実装
    class EventList
    {
    public:
        // A bit mask for selective event cancel.
        typedef u32 MaskType;
        static const int EVENT_MASK_POS_DEFAULT = 0;
        static const int EVENT_MASK_POS_USER    = 1;

        static const MaskType EVENT_MASK_DEFAULT = 1 << EVENT_MASK_POS_DEFAULT;
        static const MaskType EVENT_MASK_ALL     = ~((MaskType)0);

        struct Entry
        {
            EventPtr event;
            MaskType mask;

            INLINE Entry( const EventPtr& e = EventPtr(), MaskType m = EVENT_MASK_DEFAULT ) : 
                event( e ),
                mask ( m )
            {
            }
        };

        typedef pool_vector< Entry > ListType;
        typedef ListType::iterator IteratorType;
        typedef ListType::const_iterator ConstIteratorType;

        EventList()
        {
            m_inProcess = false;
        }

        INLINE void AddEvent( const EventPtr& eventPtr, MaskType mask = EVENT_MASK_DEFAULT )
        {
            ASSERT( !m_inProcess );
            m_list.push_back( Entry( eventPtr, mask ) );
        }

        // Cancel events specified by the 'mask'.
        INLINE void Cancel( MaskType mask = EVENT_MASK_ALL )
        {
            ASSERT( !m_inProcess );
            m_inProcess = true;

            if( mask == EVENT_MASK_ALL ){
                for( ListType::iterator i = m_list.begin(); i != m_list.end(); ++i ){
                    i->event->Cancel();
                }
                m_list.clear();
            }
            else{
                for( ListType::iterator i = m_list.begin(); i != m_list.end(); ++i ){
                    if( mask & i->mask ){
                        i->event->Cancel();
                    }
                }
            }
            m_inProcess = false;
        }

        int Size() const
        {
            return (int)m_list.size();
        }

        INLINE void Clear()
        {
            m_list.clear();
        }
    
        ConstIteratorType begin() const
        {
            return m_list.begin();
        }

        ConstIteratorType end() const
        {
            return m_list.end();
        }

    private:
        ListType m_list;
        bool m_inProcess;

    };
#else
    // サイズと位置を自前で管理
    class EventList
    {
    private:
        typedef pool_vector< EventPtr > ListType;
        ListType m_list;
        int m_size;

    public:
        typedef pool_vector< EventPtr >::iterator       IteratorType;
        typedef pool_vector< EventPtr >::const_iterator ConstIteratorType;

        EventList()
        {
            m_size = 0;
        }

        void AddEvent(const EventPtr& eventPtr)
        {
            int size = m_size;
            if( (int)m_list.size() <= size ){
                m_list.resize( size + 1 );
            }
            m_list[size] = eventPtr;
            m_size = size + 1;
        }

        void Cancel()
        {
            IteratorType begin = m_list.begin();
            IteratorType end = begin + m_size;
            for( IteratorType i = begin; i != end; ++i ){
                (*i)->Cancel();
                *i = NULL;  // ポインタをクリア     
            }
            m_size = 0;
        }

        int Size()
        {
            return (int)m_size;
        }

        void Clear()
        {
            IteratorType begin = m_list.begin();
            IteratorType end = begin + m_size;
            for( IteratorType i = begin; i != end; ++i ){
                *i = NULL;  // ポインタをクリア     
            }
            m_size = 0;
        }

        void Trigger()
        {
            IteratorType begin = m_list.begin();
            IteratorType end = begin + m_size;
            for( IteratorType i = begin; i != end; ++i ){
                (*i)->Trigger();
            }
        }
    
        ConstIteratorType begin() const
        {
            return m_list.begin();
        }

        ConstIteratorType end() const
        {
            return begin() + m_size;
        }
    };
#endif
}


#endif  // SIM_FOUNDATION_EVENT_E_EVENT_LIST_H

