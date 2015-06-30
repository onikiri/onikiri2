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


#ifndef __MEMDEPPRED_H__
#define __MEMDEPPRED_H__

#include "Sim/Predictor/DepPred/DepPredIF.h"
#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri 
{
    class Core;
    
    // メモリに関する依存予測器のインターフェース
    class MemDepPredIF : public DepPredIF 
    {
    public:
        virtual ~MemDepPredIF(){};

        // Rename(Fetch) 時
        virtual void Resolve(OpIterator op) = 0;        // op を 依存元の MemDependency に consumer 命令として割り当てる
        virtual void Allocate(OpIterator op) = 0;       // op を MemDependency の producer 命令として割り当てる
        virtual void Commit(OpIterator op) = 0;
        virtual void Flush(OpIterator op) = 0;      // op が Flush された

        // MemOrderManagerによって、MemOrderのconflictを起こしたopの組(producer, consumer)を教えてもらう
        virtual void OrderConflicted(OpIterator producer, OpIterator consumer) = 0;

        // num数のopをFetchできるかどうかを返す
        virtual bool CanAllocate(OpIterator* infoArray, int numOp) = 0;

    };

}; // namespace Onikiri

#endif // __MEMDEPPRED_H__

