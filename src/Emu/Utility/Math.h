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


#ifndef EMU_UTILITY_MATH_H
#define EMU_UTILITY_MATH_H


namespace Onikiri {
namespace EmulatorUtility {

template < typename T >
static bool IsNAN( T value )
{
    return boost::math::isnan( value );
}

// generate signed/unsigned type of T
// ex) signed_unsigned_type<int, false>::type is unsigned int
template <typename T, bool Signed>
    struct signed_unsigned_type;

template <typename T>
struct signed_type
{
    typedef typename signed_unsigned_type<T, true>::type type;
};

template <typename T>
struct unsigned_type
{
    typedef typename signed_unsigned_type<T, false>::type type;
};

#define IMPLEMENT_SIGNED_UNSIGNED_TYPE(Type)        \
template <>                                         \
struct signed_unsigned_type<signed Type, true> {    \
    typedef signed Type type;                       \
};                                                  \
template <>                                         \
struct signed_unsigned_type<signed Type, false> {   \
    typedef unsigned Type type;                     \
};                                                  \
template <>                                         \
struct signed_unsigned_type<unsigned Type, true> {  \
    typedef signed Type type;                       \
};                                                  \
template <>                                         \
struct signed_unsigned_type<unsigned Type, false> { \
    typedef unsigned Type type;                     \
};
#define IMPLEMENT_SIGNED_UNSIGNED_TYPE_FLOAT(Type)  \
template <bool Sign>                                \
struct signed_unsigned_type<Type, Sign> {           \
    typedef Type type;                              \
};

IMPLEMENT_SIGNED_UNSIGNED_TYPE(char)
IMPLEMENT_SIGNED_UNSIGNED_TYPE(short)
IMPLEMENT_SIGNED_UNSIGNED_TYPE(int)
IMPLEMENT_SIGNED_UNSIGNED_TYPE(long)
IMPLEMENT_SIGNED_UNSIGNED_TYPE(long long)
IMPLEMENT_SIGNED_UNSIGNED_TYPE_FLOAT(float)
IMPLEMENT_SIGNED_UNSIGNED_TYPE_FLOAT(double)
IMPLEMENT_SIGNED_UNSIGNED_TYPE_FLOAT(long double)

#undef IMPLEMENT_SIGNED_UNSIGNED_TYPE
#undef IMPLEMENT_SIGNED_UNSIGNED_TYPE_FLOAT

// cast 'T' to 'unsigned T'
template <typename T>
inline typename unsigned_type<T>::type cast_to_unsigned(T x)
{
    return static_cast< typename unsigned_type<T>::type > (x); 
}

// cast 'T' to 'signed T'
template <typename T>
inline typename signed_type<T>::type cast_to_signed(T x)
{
    return static_cast< typename signed_type<T>::type > (x);
}

} // namespace EmulatorUtility {
} // namespace Onikiri

#endif  // #ifndef EMU_UTILITY_MATH_H


