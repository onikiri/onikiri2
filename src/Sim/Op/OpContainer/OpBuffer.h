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


#ifndef __OP_BUFFER_H__
#define __OP_BUFFER_H__

#include "Sim/Op/OpContainer/OpList.h"
#include "Sim/Foundation/TimeWheel/ClockedResourceBase.h"
#include "Sim/Dumper/Dumper.h"

namespace Onikiri 
{
    class OpBuffer : 
        protected OpList,
        public ClockedResourceBase
    {
    protected:
        void CheckAndDumpStallBegin( OpIterator op )
        {
            if( IsStalledThisCycle() )
                g_dumper.DumpStallBegin( op );
        }


    public:

        typedef OpList::iterator iterator;

        OpBuffer();
        OpBuffer( const OpArray& opArray );
        virtual ~OpBuffer();

        // Dump stall information of containing ops.
        virtual void BeginStall();
        virtual void EndStall();

        // --- OpList
        // Note: These methods are not virtual.
        void resize(const OpArray& opArray)
        {
            OpList::resize( opArray );
        }

        void resize( int capacity )
        {
            OpList::resize( capacity );
        }

        size_t size() const
        {
            return OpList::size();
        }
        size_t count( const OpIterator& op ) const
        {
            return OpList::count( op );
        }

        bool find_and_erase( OpIterator op )
        {
            return OpList::find_and_erase( op );
        }

        iterator insert( iterator pos, const OpIterator& op )
        {
            CheckAndDumpStallBegin( op );
            return OpList::insert( pos, op );
        }

        void push_inorder( OpIterator op )
        {
            CheckAndDumpStallBegin( op );
            OpList::push_inorder( op );
        }

        void push_front( const OpIterator& op )
        {
            CheckAndDumpStallBegin( op );
            OpList::push_front( op );
        }

        void push_back( const OpIterator& op )
        {
            CheckAndDumpStallBegin( op );
            OpList::push_back( op );
        }

        iterator begin()
        {
            return OpList::begin();
        }

        iterator end()
        {
            return OpList::end();
        }
    };
}; // namespace Onikiri

#endif // __OpHASHLIST_H__

