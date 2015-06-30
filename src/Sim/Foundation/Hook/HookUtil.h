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
// Hook ÇÃåƒÇ—èoÇµÇä»ó™âª
//


#ifndef SIM_FOUNDATION_HOOK_HOOK_UTIL_H
#define SIM_FOUNDATION_HOOK_HOOK_UTIL_H

#include "Sim/Foundation/Hook/Hook.h"

namespace Onikiri
{
    //
    //  Usage : 
    //
    //      void Class::MethodBody();
    // 
    //      HookEntry(
    //          this,
    //          &Class::MethodBody,
    //          &hookPoint
    //      );
    //
    template <
        typename ClassType, 
        typename HookPointType
    >
    NOINLINE void HookEntryBody( 
        ClassType* obj, 
        void (ClassType::*MethodPtr)(), 
        HookPointType* hookPoint 
    ){
        hookPoint->Trigger(obj, HookType::HOOK_BEFORE);

        if( !hookPoint->HasAround() ) {
            // ñ{óàÇÃèàóù
            (obj->*MethodPtr)(); 
        } else {
            // ë„ÇÌÇËÇÃèàóùÇ™ìoò^Ç≥ÇÍÇƒÇ¢ÇÈéûÇÕÇªÇøÇÁÇåƒÇ‘
            hookPoint->Trigger(obj, HookType::HOOK_AROUND);
        }

        hookPoint->Trigger(obj, HookType::HOOK_AFTER);
    }

    //
    template <
        typename ClassType, 
        typename HookPointType
    >
    INLINE void HookEntry( 
        ClassType* obj, 
        void (ClassType::*MethodPtr)(), 
        HookPointType* hookPoint 
    ){
        if( hookPoint->IsAnyHookRegistered() ){
            HookEntryBody( obj, MethodPtr, hookPoint );
        }
        else{
            // ñ{óàÇÃèàóù
            (obj->*MethodPtr)(); 
        }
    }


    //
    //  Usage : 
    //
    //      void Class::MethodBody( HookParam* hookParam );
    // 
    //      HookEntry(
    //          this,
    //          &Class::MethodBody,
    //          &hookPoint,
    //          &hookParam 
    //      );
    //
    template <
        typename ClassType, 
        typename HookPointType,
        typename HookParamType
    >
    NOINLINE void HookEntryBody( 
        ClassType* obj, 
        void (ClassType::*MethodPtr)( HookParamType* param ), 
        HookPointType* hookPoint, 
        HookParamType* hookParam 
    ){
        hookPoint->Trigger(obj, hookParam, HookType::HOOK_BEFORE);

        if( !hookPoint->HasAround() ) {
            // ñ{óàÇÃèàóù
            (obj->*MethodPtr)(hookParam); 
        } else {
            // ë„ÇÌÇËÇÃèàóùÇ™ìoò^Ç≥ÇÍÇƒÇ¢ÇÈéûÇÕÇªÇøÇÁÇåƒÇ‘
            hookPoint->Trigger(obj, hookParam, HookType::HOOK_AROUND);
        }

        hookPoint->Trigger(obj, hookParam, HookType::HOOK_AFTER);
    }

    template <
        typename ClassType, 
        typename HookPointType,
        typename HookParamType
    >
    INLINE void HookEntry( 
        ClassType* obj, 
        void (ClassType::*MethodPtr)( HookParamType* param ), 
        HookPointType* hookPoint, 
        HookParamType* hookParam 
    ){
        if( hookPoint->IsAnyHookRegistered() ){
            HookEntryBody( obj, MethodPtr, hookPoint, hookParam );
        }
        else{
            // ñ{óàÇÃèàóù
            (obj->*MethodPtr)(hookParam); 
        }
    }

    //
    //  Usage : 
    //
    //      void Class::MethodBody( OpIterator op );
    // 
    //      HookEntry(
    //          op,
    //          this,
    //          &Class::MethodBody,
    //          &hookPoint
    //      );
    //
    template <
        typename ClassType, 
        typename HookPointType
    >
    NOINLINE void HookEntryBody( 
        ClassType* obj, 
        void (ClassType::*MethodPtr)( OpIterator op ), 
        HookPointType* hookPoint,
        OpIterator op
    ){
        hookPoint->Trigger( op, obj, HookType::HOOK_BEFORE);

        if( !hookPoint->HasAround() ) {
            // ñ{óàÇÃèàóù
            (obj->*MethodPtr)( op ); 
        } else {
            // ë„ÇÌÇËÇÃèàóùÇ™ìoò^Ç≥ÇÍÇƒÇ¢ÇÈéûÇÕÇªÇøÇÁÇåƒÇ‘
            hookPoint->Trigger( op, obj, HookType::HOOK_AROUND);
        }

        hookPoint->Trigger( op, obj, HookType::HOOK_AFTER);
    }

    template <
        typename ClassType, 
        typename HookPointType
    >
    INLINE void HookEntry( 
        ClassType* obj, 
        void (ClassType::*MethodPtr)( OpIterator op ), 
        HookPointType* hookPoint,
        OpIterator op
    ){
        if( hookPoint->IsAnyHookRegistered() ){
            HookEntryBody( obj, MethodPtr, hookPoint, op ); 
        }
        else{
            // ñ{óàÇÃèàóù
            (obj->*MethodPtr)( op ); 
        }
    }


    //
    //  Usage : 
    //
    //      void Class::MethodBody( OpIterator op, Parameter param );
    // 
    //      HookEntry(
    //          this,
    //          &Class::MethodBody,
    //          &hookPoint,
    //          op,
    //          param
    //      );
    //
    template <
        typename ClassType, 
        typename HookPointType,
        typename Parameter
    >
    NOINLINE void HookEntryBody( 
        ClassType* obj, 
        void (ClassType::*MethodPtr)( OpIterator op, Parameter param ), 
        HookPointType* hookPoint,
        OpIterator op,
        Parameter param
    ){
        hookPoint->Trigger( op, obj, param, HookType::HOOK_BEFORE);

        if( !hookPoint->HasAround() ) {
            // ñ{óàÇÃèàóù
            (obj->*MethodPtr)( op, param ); 
        } else {
            // ë„ÇÌÇËÇÃèàóùÇ™ìoò^Ç≥ÇÍÇƒÇ¢ÇÈéûÇÕÇªÇøÇÁÇåƒÇ‘
            hookPoint->Trigger( op, obj, param, HookType::HOOK_AROUND);
        }

        hookPoint->Trigger( op, obj, param, HookType::HOOK_AFTER);
    }

    template <
        typename ClassType, 
        typename HookPointType,
        typename Parameter
    >
    INLINE void HookEntry( 
        ClassType* obj, 
        void (ClassType::*MethodPtr)( OpIterator op, Parameter param ), 
        HookPointType* hookPoint,
        OpIterator op,
        Parameter param
    ){
        if( hookPoint->IsAnyHookRegistered() ){
            HookEntryBody( obj, MethodPtr, hookPoint, op, param ); 
        }
        else{
            // ñ{óàÇÃèàóù
            (obj->*MethodPtr)( op, param ); 
        }
    }

    //
    // Todo: sort arguments, implement macros with a class and decltype.
    //

    //
    // hookPoint->Trigger( caller, HookType::HOOK_BEFORE );
    //
    // if( !hookPoint->HasAround() ) {
    //    // Original processes.
    // } 
    // else {
    //    // If an 'AROUND' hook is registered, call it.
    //    hookPoint->Trigger( caller, HookType::HOOK_AROUND );
    // }
    //
    // hookPoint->Trigger( caller, HookType::HOOK_AFTER );
    //

    //
    //  Usage : 
    //
    //      Hook prototype: void HookMethod();
    // 
    //      HOOK_SECTION( s_hookPoint ){
    //          // Original processes.
    //      }
    //
    template < typename Caller, typename HookPoint >
    INLINE bool HookSectionBefore( Caller* caller, HookPoint* hookPoint )
    {
        hookPoint->Trigger( caller, HookType::HOOK_BEFORE );
        if( hookPoint->HasAround() ){
            HookSectionAfter( caller, hookPoint );
            return true;    // Exit the loop in HOOK_SECTION.
        }
        else{
            return false;   // Not exit the loop, process a original routine.
        }
    }

    template < typename Caller, typename HookPoint >
    INLINE bool HookSectionAfter( Caller* caller, HookPoint* hookPoint )
    {
        if( hookPoint->HasAround() ){
            hookPoint->Trigger( caller, HookType::HOOK_AROUND );
        }

        hookPoint->Trigger( caller, HookType::HOOK_AFTER );
        return true;
    }

    #define HOOK_SECTION( hookPoint ) \
        for( \
            bool onikiri_exitSection = HookSectionBefore( this, &hookPoint ); \
            !onikiri_exitSection; \
            onikiri_exitSection = HookSectionAfter( this, &hookPoint ) \
        )



    //
    //  Usage : 
    //
    //      Hook prototype: void HookMethod( OpIterator op );
    // 
    //      HOOK_SECTION( s_hookPoint, op ){
    //          // Original processes.
    //      }
    //
    template < typename Caller, typename HookPoint >
    INLINE bool HookSectionBefore( Caller* caller, HookPoint* hookPoint, OpIterator op )
    {
        hookPoint->Trigger( op, caller, HookType::HOOK_BEFORE );
        if( hookPoint->HasAround() ){
            HookSectionAfter( caller, hookPoint, op );
            return true;    // Exit the loop in HOOK_SECTION.
        }
        else{
            return false;   // Not exit the loop, process a original routine.
        }
    }

    template < typename Caller, typename HookPoint >
    INLINE bool HookSectionAfter( Caller* caller, HookPoint* hookPoint, OpIterator op  )
    {
        if( hookPoint->HasAround() ){
            hookPoint->Trigger( op, caller, HookType::HOOK_AROUND );
        }

        hookPoint->Trigger( op, caller, HookType::HOOK_AFTER );
        return true;
    }

    #define HOOK_SECTION_OP( hookPoint, op ) \
        for( \
            bool onikiri_exitSection = HookSectionBefore( this, &hookPoint, op ); \
            !onikiri_exitSection; \
            onikiri_exitSection = HookSectionAfter( this, &hookPoint, op ) \
        )

    
    //
    //  Usage : 
    //
    //      Hook prototype: void HookMethod( Parameter* param );
    // 
    //      HOOK_SECTION( s_hookPoint, param ){
    //          // Original processes.
    //      }
    //
    template < typename Caller, typename HookPoint, typename Parameter >
    INLINE bool HookSectionBefore( Caller* caller, HookPoint* hookPoint, Parameter* param )
    {
        hookPoint->Trigger( caller, param, HookType::HOOK_BEFORE );
        if( hookPoint->HasAround() ){
            HookSectionAfter( caller, hookPoint, param );
            return true;    // Exit the loop in HOOK_SECTION.
        }
        else{
            return false;   // Not exit the loop, process a original routine.
        }
    }

    template < typename Caller, typename HookPoint, typename Parameter >
    INLINE bool HookSectionAfter( Caller* caller, HookPoint* hookPoint, Parameter* param )
    {
        if( hookPoint->HasAround() ){
            hookPoint->Trigger( caller, param, HookType::HOOK_AROUND );
        }

        hookPoint->Trigger( caller, param, HookType::HOOK_AFTER );
        return true;
    }

    #define HOOK_SECTION_PARAM( hookPoint, param ) \
        for( \
            bool onikiri_exitSection = HookSectionBefore( this, &hookPoint, &param ); \
            !onikiri_exitSection; \
            onikiri_exitSection = HookSectionAfter( this, &hookPoint, &param ) \
        )

    //
    //  Usage : 
    //
    //      Hook prototype: void HookMethod( OpIterator op, Parameter* param );
    // 
    //      HOOK_SECTION( s_hookPoint, op, param ){
    //          // Original processes.
    //      }
    //
    template < typename Caller, typename HookPoint, typename Parameter >
    INLINE bool HookSectionBefore( Caller* caller, HookPoint* hookPoint, OpIterator op, Parameter* param )
    {
        hookPoint->Trigger( op, caller, param, HookType::HOOK_BEFORE );
        if( hookPoint->HasAround() ){
            HookSectionAfter( caller, hookPoint, op, param );
            return true;    // Exit the loop in HOOK_SECTION.
        }
        else{
            return false;   // Not exit the loop, process a original routine.
        }
    }

    template < typename Caller, typename HookPoint, typename Parameter >
    INLINE bool HookSectionAfter( Caller* caller, HookPoint* hookPoint, OpIterator op, Parameter* param )
    {
        if( hookPoint->HasAround() ){
            hookPoint->Trigger( op, caller, param, HookType::HOOK_AROUND );
        }

        hookPoint->Trigger( op, caller, param, HookType::HOOK_AFTER );
        return true;
    }

    #define HOOK_SECTION_OP_PARAM( hookPoint, op, param ) \
        for( \
            bool onikiri_exitSection = HookSectionBefore( this, &hookPoint, op, &param ); \
            !onikiri_exitSection; \
            onikiri_exitSection = HookSectionAfter( this, &hookPoint, op, &param ) \
        )

}

#endif

