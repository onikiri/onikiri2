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


#ifndef SIM_FOUNDATION_EVENT_EVENTBASE_H
#define SIM_FOUNDATION_EVENT_EVENTBASE_H

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/ResourcePriority.h"

namespace Onikiri
{

// Take statistics of events.
//#define ONIKIRI_EVENT_STAT

#ifdef  ONIKIRI_EVENT_STAT
    class EventStatistics
    {
    public:
        template <typename Event> 
        void Add( const Event& event )
        {
            m_typeMap[ typeid( event ).name() ]++;
        }

        ~EventStatistics()
        {
            for( unordered_map< const char*, int >::iterator i = m_typeMap.begin(); i != m_typeMap.end(); ++i ){
                printf( "%s:\t%d\n", i->first, i->second );     
            }
        }
    protected:
        unordered_map< const char*, int > m_typeMap;
    };
#endif

    class EventBaseImplement
    {
    public:
        EventBaseImplement() :
            m_refCount(0),
            m_priority(RP_DEFAULT_EVENT),
            m_canceled(false),
            m_updated(false)
        {
        }

        virtual ~EventBaseImplement(){};

        //
        // Reference count operations
        //

        INLINE void AddRef()
        {
            m_refCount++;
        };

        INLINE void Release()
        {
            --m_refCount;
            if( m_refCount == 0 ){
                Destruct();
            }
        }

        //
        // Event related methods
        //

        bool IsCanceled() const
        {
            return m_canceled;
        }

        bool IsUpdated() const
        {
            return m_updated;
        }

        void SetUpdated()
        {
            m_updated = true;
        }

        void TriggerEvaluate()
        {
            if( !IsCanceled() ){
                Evaluate();
            }
        }

        void TriggerUpdate()
        {
            ASSERT( !IsUpdated() );
            if( !IsCanceled() ){
                Update();
            }
            SetUpdated();
        }

        void Cancel()
        {
            m_canceled = true;
        }

        int GetPriority() const
        {
            return m_priority; 
        }

        void SetPriority( int priority )
        {
            m_priority = priority;
        }

        virtual void Destruct() = 0;
        virtual void Update() = 0;
        virtual void Evaluate(){};

    protected:
        int m_refCount;
        int m_priority;
        bool m_canceled;
        bool m_updated;

#ifdef  ONIKIRI_EVENT_STAT
        static EventStatistics* GetStat()
        {
            static EventStatistics stat;
            return &stat;
        }
#endif
    };

    INLINE static void intrusive_ptr_add_ref( EventBaseImplement* ptr )
    {
        ptr->AddRef();
    }

    INLINE static void intrusive_ptr_release( EventBaseImplement* ptr )
    {
        ptr->Release();
    }

    typedef boost::intrusive_ptr<EventBaseImplement> EventPtr;



    // イベントの基底クラス
    template < typename T >
    class EventBase :
        public EventBaseImplement,
        public PooledIntrusivePtrObject< T, EventBaseImplement >
    {
    public:

#ifdef  ONIKIRI_EVENT_STAT
        EventBase()
        {
            GetStat()->Add( *this );
        }
#endif

        virtual void Destruct()
        {
            PooledIntrusivePtrObject< T, EventBaseImplement >::Destruct( static_cast<T*>(this) );
        };

        // EventPtr T::Construct() is defined in PooledIntrusivePtrObject.

    };



}; // namespace Onikiri

#endif // SIM_EVENT_EVENTBASE_H

