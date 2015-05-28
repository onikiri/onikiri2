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
// EventDelegate acts as a proxy for user event handler
//

#ifndef SIM_FOUNDATION_EVENT_DELEGATE_H
#define SIM_FOUNDATION_EVENT_DELEGATE_H

#include "Sim/Foundation/Event/EventBase.h"
#include "Utility/SharedPtrObjectPool.h"

namespace Onikiri
{

    template <class ClassType, class ParamType>
    class EventDelegate : 
        public EventBase< EventDelegate<ClassType, ParamType> >
    {
        using EventBase< EventDelegate<ClassType, ParamType> >::SetPriority;
        using EventBase< EventDelegate<ClassType, ParamType> >::GetPriority;
        typedef void (ClassType::*MethodType)(ParamType);
        ClassType* m_obj;
        MethodType m_func;
        ParamType  m_param; 

    public:
        EventDelegate(ClassType* obj, MethodType func, ParamType param, int priority = RP_DEFAULT_EVENT) : 
            m_obj   ( obj ),
            m_func  ( func ),
            m_param ( param )
        {
            SetPriority(priority);
        };

        EventDelegate(const EventDelegate& ref) : 
            m_obj   ( ref.obj ),
            m_func  ( ref.func ),
            m_param ( ref.param )
        {
            SetPriority( ref.GetPriority() );
        };

        void Update()
        {
            (m_obj->*m_func)( m_param );
        };
    };

} //namespace Onikiri



#endif

