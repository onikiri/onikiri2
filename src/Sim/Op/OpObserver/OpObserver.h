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


#ifndef __OpOBSERVER_H__
#define __OpOBSERVER_H__

#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri 
{
    
    class OpObserver;
    class OpNotifier;

    // Opのリタイア/フラッシュを監視するクラス
    class OpObserver {
    public:
        OpObserver();
        OpObserver(OpNotifier* notifier);
        virtual ~OpObserver();

        virtual void Commit(OpIterator op) = 0;
        virtual void Flush(OpIterator op) = 0;
    };

    // OpObserverにOpのリタイア/フラッシュを通知するクラス
    class OpNotifier {
    private:
        std::vector<OpObserver*> m_observer;

        class NotifyFuncObj {
        private:
            OpIterator m_op;
            void (OpObserver::*m_func)(OpIterator);
        public:
            NotifyFuncObj(OpIterator& op, void (OpObserver::*func)(OpIterator))
                : m_op(op), m_func(func)
            {
            }

            void operator()(OpObserver* observer)
            {
                (observer->*m_func)(m_op);
            }

        };

    public:
        OpNotifier();
        virtual ~OpNotifier();

        void Add(OpObserver* observer);

        void NotifyCommit(OpIterator op);
        void NotifyFlush(OpIterator op);
    };

}; // namespace Onikiri

#endif // __OpOBSERVER_H__

