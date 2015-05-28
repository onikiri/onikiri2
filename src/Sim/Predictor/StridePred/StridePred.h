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


#ifndef __STRIDEPREDICTOR_H__
#define __STRIDEPREDICTOR_H__

#include <list>
#include <vector>
#include <algorithm>
using namespace std;

namespace Onikiri {
    template <typename T>
    class StridePred
    {
    protected:
        // テーブルのエントリ
        struct Stream
        {
            T next;             // 予測値
            T diff;             // 差分
            unsigned int conf;  // 確信度
            unsigned int time;  // 更新されてからの時間
        };

        // テーブルから予測とするエントリを選択する
        class SelectStream
        {
        public:
            bool operator()(Stream left,Stream right) const
            {
                if(left.conf == right.conf)
                    return left.time < right.time;
                else
                    return left.conf > right.conf;
            }
        };
        // テーブルから削除するエントリを選択する
        class SelectDeleteEntry
        {
        public:
            bool operator()(Stream left,Stream right) const
            {
                if(left.time == right.time)
                    return left.conf < right.conf;
                else
                    return left.time > right.time;
            }
        };

        unsigned int m_historySize;
        list<T> m_historyTable;
        unsigned int m_streamSize;
        vector<Stream> m_streamTable;

        void Analyze(T t)
        {
            vector<Stream>::iterator sitr = m_streamTable.begin();
            while(sitr != m_streamTable.end())
            {
                (*sitr).time++;
                if((*sitr).next == t)
                {
                    (*sitr).next += (*sitr).diff;
                    (*sitr).conf++;
                    (*sitr).time = 0;
                }
                sitr++;
            }
            list<T>::iterator titr = m_historyTable.begin();
            sort(m_streamTable.begin(),m_streamTable.end(),SelectDeleteEntry());
            while(titr != m_historyTable.end())
            {
                Stream s;
                s.time = 0;
                s.conf = 1;
                s.diff = t - (*titr);
                s.next = t + s.diff;
                if(CheckDuplication(s))
                {
                    if(m_streamTable.size() == m_streamSize)
                        m_streamTable.erase(m_streamTable.begin());
                    m_streamTable.push_back(s);
                }
                titr++;
            }
        }

        bool CheckDuplication(Stream s)
        {
            vector<Stream>::iterator itr = m_streamTable.begin();
            while(itr != m_streamTable.end())
            {
                if(s.next == (*itr).next && s.diff == (*itr).diff)
                    return false;
                itr++;
            }
            return true;
        }
    public:
        StridePred(int historySize, int streamSize)
        {
            Initialize(historySize,streamSize);
        }
        ~StridePred(){}

        void Initialize(int historySize, int streamSize)
        {
            m_historySize = historySize;
            m_streamSize = streamSize;
            m_historyTable.clear();
            m_streamTable.clear();
        }

        void Update(T t)
        {
            Analyze(t);
            m_historyTable.push_back(t);
            if(m_historySize < m_historyTable.size())
                m_historyTable.pop_front();
        }

        void Predict(T* pt, int num)
        {
            sort(m_streamTable.begin(),m_streamTable.end(),SelectStream());
            vector<Stream>::iterator itr = m_streamTable.begin();
            for(int i = 0;i < num && itr != m_streamTable.end();i++,itr++)
            {
                pt[i] = (*itr).next;
            }
        }

        //for debug
        void Update(T* pt, int num)
        {
            for(int i = 0;i < num;i++)
                Update(pt[i]);
        }

        void PrintStream()
        {
            sort(m_streamTable.begin(),m_streamTable.end(),SelectStream());
            vector<Stream>::iterator itr = m_streamTable.begin();
            while(itr != m_streamTable.end())
                PrintStream(*(itr++));
        }
        void PrintStream(Stream s)
        {
            cout << "next:" << s.next << " diff" << s.diff << " conf:" << s.conf << " time:" << s.time << "\n";
            for(int i = s.conf + 1;i > 0;i--)
            {
                cout << s.next - i*s.diff << " ";
            }
            
            cout << "\n";
        }
    };
}; // namespace Onikiri

#endif // __STRIDEPREDICTOR_H__
