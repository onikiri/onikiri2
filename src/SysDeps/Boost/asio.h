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


// <boost/asio.hpp> should be included independently,
// because asio.hpp includes system related files such as winnt.h
// and it conflicts other files.

#ifndef SYSDEPS_BOOST_ASIO_H
#define SYSDEPS_BOOST_ASIO_H

#ifdef HOST_IS_CYGWIN
    // push & pop is available since gcc 4.6 but
    // currently Cygwin gcc is 4.5.3
    // #pragma GCC diagnostic push

    // Missing braces in boost/asio/ip/impl/address_v6.ipp and
    // boost/asio/ip/detail/impl/endpoint.ipp.
#   pragma GCC diagnostic ignored "-Wmissing-braces"

    // Strict-aliasing rules are broken in
    // boost/asio/detail/impl/win_iocp_handle_service.ipp
#   pragma GCC diagnostic ignored "-Wstrict-aliasing"

#   include <boost/asio.hpp>

    // alternative to push/pop
#   pragma GCC diagnostic error "-Wmissing-braces"
#   pragma GCC diagnostic error "-Wstrict-aliasing"

    // #pragma GCC diagnostic pop
#else
#   include <boost/asio.hpp>
#endif


#endif
