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


#ifndef SIM_EXEC_UNIT_EXEC_UNIT_RESERVER_H
#define SIM_EXEC_UNIT_EXEC_UNIT_RESERVER_H


namespace Onikiri 
{

    class ExecUnitReserver
    {
    public:
        ExecUnitReserver();
        ~ExecUnitReserver();

        // unitCount:
        //   The number of execution units belonging to this reserver.
        // wheelSize: 
        //   The size of a reservation wheel. Usually, 
        //   this is set to the size of a time wheel.
        void Initialize( int unitCount, int wheelSize );

        // Called when each cycle begins.
        void Begin();

        // Called in Evaluate phase.
        bool CanReserve( int n, int time, int period );
        
        // Called in Evaluate phase.
        void Reserve( int n, int time, int period );
        
        // Called in Update phase.
        void Update();

    protected:
        
        // The number of units corresponding to this reserver.
        int m_unitCount;

        // Reservation wheel.
        // This records the number of units used in each cycle.
        std::vector< int > m_wheel;

        // This refers a point corresponding to a current cycle.
        size_t m_current;

        // Reservation queue.
        struct Reservation
        {
            int count;
            int time;
            int period;
        };
        static const int RESERVATION_QUEUE_SIZE = 256;
        typedef fixed_sized_buffer< Reservation, RESERVATION_QUEUE_SIZE > ReservationQueue;
        ReservationQueue m_resvQueue;

        int GetWheelIndex( int delta );
    };

}; // namespace Onikiri

#endif // SIM_EXEC_UNIT_EXEC_UNIT_RESERVER_H

