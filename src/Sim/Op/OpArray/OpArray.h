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


#ifndef SIM_OP_OP_ARRAY_OP_ARRAY_H
#define SIM_OP_OP_ARRAY_OP_ARRAY_H

#include "Utility/RuntimeError.h"

namespace Onikiri
{

    // forward declaration
    class OpIterator;
    class Op;

    class OpArray 
    {
    public:
        typedef int ID;

        class ArrayID
        {
        public:
            ArrayID() : 
                m_op(0),
                m_opArray(0),
                m_id(0)
            {}
            
            ArrayID(
                Op* op, 
                OpArray* opArray,
                ID id
            ) :
                m_op(op), 
                m_opArray(opArray), 
                m_id(id)
            {}
            
            ArrayID(const ArrayID& arrayID) : 
                m_op(arrayID.m_op), 
                m_opArray(arrayID.m_opArray), 
                m_id(arrayID.m_id)
            {}

            virtual ~ArrayID();

            // accessors
            Op* GetOp() const  { return m_op; }
            void SetOp(Op* op)
            {
                ASSERT(m_op == 0, "SetOp called twice.");           
                m_op = op;
            }

            const OpArray* GetOpArray() const { return m_opArray; }
            const ID       GetID()      const { return m_id;     }
        
        private:
            Op*         m_op;
            OpArray*    m_opArray;
            ID          m_id;

        };  // class ArrayID

    public:
        // constructor & destructor
        OpArray( int capacity );
        virtual ~OpArray();
        
        // copy 禁止
        OpArray( const OpArray& opArray )
        {
            THROW_RUNTIME_ERROR("cannot create OpArray copy");
        };
        
        // 新しいOpを作って、OpIteratorを返す
        OpIterator CreateOp();

        // OpIteratorの指すOpを解放する
        void ReleaseOp(const OpIterator& opIterator);
        // OpIteratorの指すOpをCopyも含めて全て解放する
        void ReleaseAll(const OpIterator& opIterator);

        // OpIteratorの指すOpが割り当て済みかどうかを返す
        bool IsAlive(const OpIterator& opIterator) const;
        bool IsAlive( ID id ) const
        {
            ASSERT(
                id >= 0 && id < m_capacity,
                "id range error(id = %d, capacity = %d)",
                id,
                m_capacity
            );

            return m_alive[id];
        }

        bool IsFull() const
        {
            return m_freeList.size() == 0;
        }

        bool IsEmpty() const
        {
            return GetSize() == 0;
        }

        int GetSize() const 
        {
            return GetCapacity() - static_cast<int>(m_freeList.size());
        }

        int GetCapacity() const
        {
            return m_capacity;
        }

    protected:
        // Op の pool 
        // IDの配列
        std::vector<OpArray::ArrayID*> m_body;

        // 使用中かどうかのフラグ
        // Copy方向の配列
        boost::dynamic_bitset<> m_alive;

        // 割り当て可能かどうかのリスト
        pool_vector<ID> m_freeList;

        // 確保するサイズ
        int m_capacity;
    };


    class OpIterator 
    {
    public:
        INLINE OpIterator()
            : m_arrayID(0)
        {
        }

        INLINE OpIterator( OpArray::ArrayID* arrayID )
            : m_arrayID(arrayID)
        {
        }

        INLINE OpIterator( const OpIterator& iterator )
            : m_arrayID( iterator.m_arrayID )
        {
        }

        const OpArray::ArrayID* GetArrayID() const
        {
            return m_arrayID;
        }

        Op* GetOp() const
        {
            return m_arrayID->GetOp();
        }

        const OpArray::ID GetID() const
        {
            return m_arrayID->GetID();
        }

        bool IsAlive() const
        {
            return m_arrayID->GetOpArray()->IsAlive(*this);
        }

        bool IsNull() const
        {
            return m_arrayID == 0 || GetOp() == 0;
        }
        
        // pointer と互換を取るための operator
        Op& operator*() const
        {
            return *m_arrayID->GetOp();
        }

        Op* operator->() const
        {
            return m_arrayID->GetOp();
        }

        bool operator==(const OpIterator& rhv) const
        {
            return m_arrayID == rhv.GetArrayID();
        }

        bool operator!=(const OpIterator& rhv) const
        {
            return !(*this == rhv);
        }

    protected:
        OpArray::ArrayID* m_arrayID;
    };


    
}; // namespace Onikiri

#endif // SIM_OP_OP_ARRAY_OP_ARRAY_H

