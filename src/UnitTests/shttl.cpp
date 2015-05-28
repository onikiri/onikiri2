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


#include "SysDeps/UnitTest.h"
#include "Utility/String.h"

#include <algorithm>
#include <stdio.h>

#include "lib/shttl/array2d.h"
#include "lib/shttl/bit.h"
#include "lib/shttl/bitset.h"
#include "lib/shttl/counter.h"
#include "lib/shttl/counter_array.h"
#include "lib/shttl/integer.h"
#include "lib/shttl/lru.h"
#include "lib/shttl/nlu.h"

#include "lib/shttl/setassoc_table.h"

#include "lib/shttl/double_hasher.h"
#include "lib/shttl/simple_hasher.h"
#include "lib/shttl/std_hasher.h"
#include "lib/shttl/static_off_hasher.h"

using namespace shttl;

namespace Onikiri
{
    ONIKIRI_TEST_CLASS(SHTTL)
    {
    public:
        
        ONIKIRI_TEST_METHOD(SHTTL_BitOperations)
        {
            ONIKIRI_TEST_ARE_EQUAL( (u64)0,     xor_convolute(0, 10),   "XOR convolution failed." );
            ONIKIRI_TEST_ARE_EQUAL( (u64)1,     xor_convolute(1, 10),   "XOR convolution failed." );
            ONIKIRI_TEST_ARE_EQUAL( (u64)255,   xor_convolute(255, 10), "XOR convolution failed." );
            ONIKIRI_TEST_ARE_EQUAL( (u64)1023,  xor_convolute(1023 << 10, 10), "XOR convolution failed." );
            ONIKIRI_TEST_ARE_EQUAL( (u64)1022,  xor_convolute((1023 << 10) | 1, 10), "XOR convolution failed." );
        }

        ONIKIRI_TEST_METHOD(SHTTL_Counter)
        {
            // 1bit counter
            {
                counter<u8> c( 
                    0, // init
                    0, // min
                    1, // max
                    1, // add
                    1, // sub
                    1  // threshold
                );

                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );

                ONIKIRI_TEST_IS_TRUE( c.inc() == 1, "" );
                ONIKIRI_TEST_IS_TRUE( c.inc() == 1, "" );

                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );

                ONIKIRI_TEST_IS_TRUE( c.inc() == 1, "" );
                ONIKIRI_TEST_IS_TRUE( c.inc() == 1, "" );

                ONIKIRI_TEST_IS_TRUE( c.above_threshold() == true, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );
                ONIKIRI_TEST_IS_TRUE( c.above_threshold() == false, "" );
            }

            // 2bit counter
            {
                counter<u8> c( 
                    0, // init
                    0, // min
                    3, // max
                    1, // add
                    1, // sub
                    2  // threshold
                );

                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );

                ONIKIRI_TEST_IS_TRUE( c.inc() == 1, "" );
                ONIKIRI_TEST_IS_TRUE( c.inc() == 2, "" );
                ONIKIRI_TEST_IS_TRUE( c.inc() == 3, "" );
                ONIKIRI_TEST_IS_TRUE( c.inc() == 3, "" );

                ONIKIRI_TEST_IS_TRUE( c.dec() == 2, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 1, "" );

                ONIKIRI_TEST_IS_TRUE( c.inc() == 2, "" );
                ONIKIRI_TEST_IS_TRUE( c.inc() == 3, "" );

                ONIKIRI_TEST_IS_TRUE( c.above_threshold() == true, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 2, "" );
                ONIKIRI_TEST_IS_TRUE( c.above_threshold() == true, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 1, "" );
                ONIKIRI_TEST_IS_TRUE( c.above_threshold() == false, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );
                ONIKIRI_TEST_IS_TRUE( c.above_threshold() == false, "" );
                ONIKIRI_TEST_IS_TRUE( c.dec() == 0, "" );
                ONIKIRI_TEST_IS_TRUE( c.above_threshold() == false, "" );
            }
        }

        ONIKIRI_TEST_METHOD(SHTTL_CounterArray)
        {
            // 2bit counter array
            {
                int size = 256;
                counter_array<u8> ca(
                    size, // size
                    0, // init
                    0, // min
                    3, // max
                    1, // add
                    1, // sub
                    2  // threshold
                );

                for( int i = 0; i < size; i++ ){
                    ONIKIRI_TEST_IS_TRUE( ca[i].dec() == 0, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].dec() == 0, "" );

                    ONIKIRI_TEST_IS_TRUE( ca[i].inc() == 1, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].inc() == 2, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].inc() == 3, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].inc() == 3, "" );

                    ONIKIRI_TEST_IS_TRUE( ca[i].dec() == 2, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].dec() == 1, "" );

                    ONIKIRI_TEST_IS_TRUE( ca[i].inc() == 2, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].inc() == 3, "" );

                    ONIKIRI_TEST_IS_TRUE( ca[i].above_threshold() == true, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].dec() == 2, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].above_threshold() == true, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].dec() == 1, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].above_threshold() == false, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].dec() == 0, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].above_threshold() == false, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].dec() == 0, "" );
                    ONIKIRI_TEST_IS_TRUE( ca[i].above_threshold() == false, "" );
                }
            }
        }

        ONIKIRI_TEST_METHOD(SHTTL_LRU)
        {
            LRU_Test< lru_time<u64> > ( "'lru_time<u64>' test failed.");
            LRU_Test< lru_order<u64> >( "'lru_order<u64>' test failed.");
            LRU_Test< lru_list<u64> > ( "'lru_list<u64>' test failed.");
            LRU_Test< lru_time<int> > ( "'lru_time<int>' test failed.");
            LRU_Test< lru_order<int> >( "'lru_order<int>' test failed.");
            LRU_Test< lru_list<int> > ( "'lru_list<int>' test failed.");
            LRU_Test< lru_time<u8> >  ( "'lru_time<u8>' test failed.");
            LRU_Test< lru_order<u8> > ( "'lru_order<u8>' test failed.");
            LRU_Test< lru_list<u8> >  ( "'lru_list<u8>' test failed.");
        }

        ONIKIRI_TEST_METHOD(SHTTL_SetAssociativeTable)
        {
            // u64, int and u16 are key types.
            SetAssocTableTest<u64>();
            SetAssocTableTest<int>();
            SetAssocTableTest<u16>();
            // <u8> cannot pass the test because there is not enough address 
            // space.
        }

        ONIKIRI_TEST_METHOD(SHTTL_FullAssociativeTable)
        {
            // index:0 (full associative), offset:0
            {
                setassoc_table< std::pair<u64, u64>, std_hasher< u64 >, lru<u64> > 
                    table( std_hasher< u64 >(0, 0), 8 );
                TableTest( table, "Set associative table(full associative) test failed." );
            }

            {
                setassoc_table< std::pair<u64, u64>, static_off_hasher< u64, 0 >, lru<u64> > 
                    table( static_off_hasher< u64, 0 >(0), 8 );
                TableTest( table, "Set associative table(full associative) test failed." );
            }
        }

        ONIKIRI_TEST_METHOD(SHTTL_DirectMapTable)
        {
            // way:1 (direct map)
            {
                setassoc_table< std::pair<u64, u64>, std_hasher< u64 >, lru<u64> > 
                    table( std_hasher< u64 >(5, 6), 1 );
                TableTest( table, "Set associative table(direct map) test failed." );
            }
            
            {
                setassoc_table< std::pair<u64, u64>, static_off_hasher< u64, 6 >, lru<u64> > 
                    table( static_off_hasher< u64, 6 >(5), 1 );
                TableTest( table, "Set associative table(direct map) test failed." );
            }
        }
        
    private:

        //
        // --- setassoc_table test ---
        //
        template <class KeyType>
        void SetAssocTableTest()
        {
            {
                setassoc_table< std::pair<KeyType, u64>, std_hasher< KeyType >, lru<u64> >
                    table( std_hasher< KeyType >(5, 6), 8 );
                TableTest( table, "Set associative table test failed." );
            }

            {
                setassoc_table< std::pair<KeyType, u64>, std_hasher< KeyType >, nlu<u64> > 
                    table( std_hasher< KeyType >(5, 6), 8 );
                TableTest( table, "Set associative table test failed." );
            }

            {
                setassoc_table< std::pair<KeyType, u64>, static_off_hasher< KeyType, 6 >, lru<u64> > 
                    table( static_off_hasher< KeyType, 6 >(5), 8 );
                TableTest( table, "Set associative table test failed." );
            }

            {
                setassoc_table< std::pair<KeyType, u64>, simple_hasher< KeyType >, lru<u64> > 
                    table( simple_hasher< KeyType >(5), 8 );
                TableTest( table, "Set associative table test failed." );
            }

            // Iterator set test
            {
                const int ways = 8;
                const int setBits = 5;

                const int magic = rand();

                typedef 
                    setassoc_table< std::pair<KeyType, u64>, std_hasher< KeyType >, lru<u64> >
                    TableType;
                    TableType table( std_hasher< KeyType >(setBits, 0), ways );

                // Write a magic number to all lines in the first set.
                for( int i = 0; i < ways; i++ ){
                    table.write( 
                        static_cast<KeyType>(i*table.set_num()),
                        (size_t)magic 
                    );
                }

                // Check whether a magic number is written to the lines.
                size_t count = 0;
                TableType::hasher_size_type index = table.index(0);
                for( TableType::iterator i = table.begin_set( index ); i != table.end_set( index ); ++i ){
                    ONIKIRI_TEST_IS_TRUE( i->value == magic, "begin_set()/end_set() test failed." );
                    count++;
                }
                ONIKIRI_TEST_IS_TRUE( count == table.way_num(), "begin_set()/end_set() test failed." );

            }
        }

        template <class Table>
        void TableTest( Table& table, const char* msg )
        {
            table.clear();

            // Read from a cleared table.
            // Results must be always miss.
            for(size_t i = 0;i < table.size();i++){
                Table::key_type addr = rand();
                if(i == 0)
                    addr = ~(Table::key_type)0;
                else if(i == 1)
                    addr = 0;

                u64 val_des;
                ONIKIRI_TEST_IS_TRUE( table.find( addr ) == table.end(), msg );
                if( table.find(addr) == table.end() ){
                    ONIKIRI_TEST_IS_TRUE( table.read( addr, &val_des ) == table.end(), msg );
                }
            }

            // write test
            for(size_t i = 0;i < table.size();i++){
                u64 val  = rand();
                Table::key_type addr = rand();

                // special address test
                if(i == 0)
                    addr = ~(Table::key_type)0;
                else if(i == 1)
                    addr = 0;


                // single write test
                u64 val_des;
                table.write( addr, val );
                ONIKIRI_TEST_IS_TRUE( table.read( addr, &val_des ) != table.end(), msg );
                ONIKIRI_TEST_IS_TRUE( val_des == val, msg );
                ONIKIRI_TEST_IS_TRUE( table.find( addr ) != table.end(), msg );

                // multiple write test
                for( int j = 0; j < 1024; j++ ){
                    Table::key_type test_addr = rand();
                    if( table.index(test_addr) != table.index( addr ) )
                        table.write( addr, val );
                }
                ONIKIRI_TEST_IS_TRUE( table.read( addr, &val_des ) != table.end(), msg );
                ONIKIRI_TEST_IS_TRUE( val_des == val, msg );
                ONIKIRI_TEST_IS_TRUE( table.find(addr) != table.end(), msg );

                // invalidate test
                table.invalidate( addr );
                ONIKIRI_TEST_IS_TRUE( table.find(addr) == table.end(), msg );
                if( table.find(addr) != table.end() ){
                    ONIKIRI_TEST_IS_TRUE( table.read( addr, &val_des ) == table.end(), msg );
                }
            }

            // iterator test
            {
                Table::key_type addr = rand();
                u64 val  = rand();

                table.write( addr, val );

                typedef typename Table::iterator iterator;
                for( iterator i = table.begin(); i != table.end(); ++i ){
                    i->valid = false;
                }

                ONIKIRI_TEST_IS_TRUE( table.find( addr ) == table.end(), msg );

                typedef typename Table::const_iterator const_iterator;
                for( const_iterator i = table.begin(); i != table.end(); ++i ){
                    ONIKIRI_TEST_IS_TRUE( !i->valid, msg );
                }
            }
        }

        // LRU algorithm test
        template< typename lru_target >
        void LRU_Test( const char* msg ) 
        {
            const int set_num = 8;
            const int way_num = 8;
            for( int index = 0; index < set_num; index++ ){
                std::vector<size_t> arr( way_num );
                for( int way = 0; way < way_num; way++ ){
                    arr[way] = way;
                }

                do {
                    lru_target lru_oa;
                    lru_oa.construct( set_num, way_num );
                    for(int i = 0; i < way_num; ++i) {
                        lru_target::key_type key = index;
                        lru_oa.touch( index, arr[i], key );
                    }
                    // Check if a first touched index is next replace target();
                    ONIKIRI_TEST_IS_TRUE( lru_oa.target( index ) == arr[0], msg );

                // Check all combinations.
                } while( std::next_permutation( arr.begin(), arr.end() - 1 ) );
            }
        }
    };

}
