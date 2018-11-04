/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef CHECKED_CAST_H
#define CHECKED_CAST_H

#include <type_traits>
#include <limits>

#ifndef NDEBUG
#ifndef _DEBUG
#define NDEBUG
#define NDEBUG_ADDED_SEPARATELY
#endif
#endif

#include <cassert>

/*
// Converts T value to RESULT_TYPE, which must be a signed number. RESULT_TYPE is by default
// int32_t. The conversion asserts that the value is not out of limits of the result type.
template<typename T, typename RESULT_TYPE = int32_t>
typename std::enable_if<std::is_signed<RESULT_TYPE>::value && std::is_unsigned<T>::value && (sizeof(T) >= sizeof(RESULT_TYPE)), RESULT_TYPE>::type tosigned(T val)
{
    assert(val <= static_cast<typename std::make_unsigned<RESULT_TYPE>::type>(std::numeric_limits<RESULT_TYPE>::max()));

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, which must be a signed number. RESULT_TYPE is by default
// int32_t. The conversion asserts that the value is not out of limits of the result type.
template<typename T, typename RESULT_TYPE = int32_t>
typename std::enable_if<std::is_signed<RESULT_TYPE>::value && std::is_signed<T>::value && (sizeof(T) > sizeof(RESULT_TYPE)), RESULT_TYPE>::type tosigned(T val)
{
    assert(val <= std::numeric_limits<RESULT_TYPE>::max() && val >= std::numeric_limits<RESULT_TYPE>::min());

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, which must be a signed number. RESULT_TYPE is by default
// int32_t. The conversion asserts that the value is not out of limits of the result type.
template<typename T, typename RESULT_TYPE = int32_t>
typename std::enable_if<std::is_signed<RESULT_TYPE>::value && (sizeof(T) < sizeof(RESULT_TYPE) || (std::is_signed<T>::value && sizeof(T) == sizeof(RESULT_TYPE))), RESULT_TYPE>::type tosigned(T val)
{
    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, which must be an unsigned number. RESULT_TYPE is by
// default uint32_t. The conversion asserts that the value is not out of limits of the
// result type.
template<typename T, typename RESULT_TYPE = uint32_t>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_signed<T>::value && (sizeof(T) >= sizeof(RESULT_TYPE)), RESULT_TYPE>::type tounsigned(T val)
{
    assert(val >= 0 && static_cast<typename std::make_unsigned<T>::type>(val) <= std::numeric_limits<RESULT_TYPE>::max());

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, which must be an unsigned number. RESULT_TYPE is by
// default uint32_t. The conversion asserts that the value is not out of limits of the
// result type.
template<typename T, typename RESULT_TYPE = uint32_t>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_signed<T>::value && (sizeof(T) < sizeof(RESULT_TYPE)), RESULT_TYPE>::type tounsigned(T val)
{
    assert(val >= 0);

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, which must be an unsigned number. RESULT_TYPE is by
// default uint32_t. The conversion asserts that the value is not out of limits of the
// result type.
template<typename T, typename RESULT_TYPE = uint32_t>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_unsigned<T>::value && (sizeof(T) <= sizeof(RESULT_TYPE)), RESULT_TYPE>::type tounsigned(T val)
{
    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, which must be an unsigned number. RESULT_TYPE is by
// default uint32_t. The conversion asserts that the value is not out of limits of the
// result type.
template<typename T, typename RESULT_TYPE = uint32_t>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_unsigned<T>::value && (sizeof(T) > sizeof(RESULT_TYPE)), RESULT_TYPE>::type tounsigned(T val)
{
    assert(val < std::numeric_limits<RESULT_TYPE>::max());

    return static_cast<RESULT_TYPE>(val);
}
*/

/*
inline int tosigned(size_t val)
{
    assert(val <= std::numeric_limits<int>::max());
    return static_cast<int>(val);
}

inline unsigned int tounsigned(size_t val)
{
    assert(val <= std::numeric_limits<unsigned int>::max());
    return static_cast<unsigned int>(val);
}

// Make value signed. The value is checked for being too large, in which case the assert
// fails. Types smaller than int are converted to int.
template<typename T>
typename std::enable_if<(sizeof(T) < sizeof(int)), int>::type tosigned(T val)
{
    return (int)val;
}

// Make value unsigned. The value is checked for being negative, in which case the assert
// fails. Types smaller than unsigned int are converted to unsigned int.
template<typename T>
typename std::enable_if<(sizeof(T) < sizeof(unsigned int)), unsigned int>::type tounsigned(T val)
{
    return (unsigned int)val;
}

// Make value signed. The value is checked for being too large, in which case the assert
// fails. Types smaller than int are converted to int.
template<typename T>
typename std::enable_if<std::is_unsigned<T>::value && (sizeof(T) >= sizeof(int)), typename std::make_signed<T>::type>::type tosigned(T val)
{
    assert(val <= static_cast<T>(std::numeric_limits<typename std::make_signed<T>::type>::max()));

    return static_cast<typename std::make_signed<T>::type>(val);
}

// Make value signed. The value is checked for being too large, in which case the assert
// fails. Types smaller than int are converted to int.
template<typename T>
typename std::enable_if<std::is_signed<T>::value && (sizeof(T) >= sizeof(int)), T>::type tosigned(T val)
{
    return val;
}

// Make value unsigned. The value is checked for being negative, in which case the assert
// fails. Types smaller than unsigned int are converted to unsigned int.
template<typename T>
typename std::enable_if<std::is_signed<T>::value && (sizeof(T) >= sizeof(int)), typename std::make_unsigned<T>::type>::type tounsigned(T val)
{
    assert(val >= 0);

    return static_cast<typename std::make_unsigned<T>::type>(val);
}

// Make value unsigned. The value is checked for being negative, in which case the assert
// fails. Types smaller than unsigned int are converted to unsigned int.
template<typename T>
typename std::enable_if<std::is_unsigned<T>::value && (sizeof(T) >= sizeof(int)), T>::type tounsigned(T val)
{
    return val;
}

// Makes a value signed or unsigned to match the first template parameter's signedness. If the
// converted type is smaller than the first parameter, it is converted to the first parameter
// type. If it is larger, but smaller than an int, it's converted to an int type.
template<typename T, typename S>
typename std::enable_if<(sizeof(T) > sizeof(S)), T>::type tosignedness(S val)
{
    return (T)val;
}

// Makes a value signed or unsigned to match the first template parameter's signedness. If the
// converted type is smaller than the first parameter, it is converted to the first parameter
// type. If it is larger, but smaller than an int, it's converted to an int type.
template<typename T, typename S>
typename std::enable_if<std::is_signed<T>::value == std::is_signed<S>::value && (sizeof(T) <= sizeof(S)), typename std::conditional<(sizeof(S) >= sizeof(int)), S, typename std::conditional<std::is_signed<S>::value, int, unsigned int>::type >::type>::type tosignedness(S val)
{
    return static_cast<typename std::conditional<(sizeof(S) >= sizeof(int)), S, typename std::conditional<std::is_signed<S>::value, int, unsigned int>::type >::type>(val);
}

// Makes a value signed or unsigned to match the first template parameter's signedness. If the
// converted type is smaller than the first parameter, it is converted to the first parameter
// type. If it is larger, but smaller than an int, it's converted to an int type.
template<typename T, typename S>
typename std::enable_if<std::is_signed<T>::value && std::is_unsigned<S>::value && (sizeof(T) <= sizeof(S) && sizeof(S) >= sizeof(int)), typename std::make_signed<S>::type>::type tosignedness(S val)
{
    return tosigned(val);
}

// Makes a value signed or unsigned to match the first template parameter's signedness. If the
// converted type is smaller than the first parameter, it is converted to the first parameter
// type. If it is larger, but smaller than an int, it's converted to an int type.
template<typename T, typename S>
typename std::enable_if<std::is_signed<T>::value && std::is_unsigned<S>::value && (sizeof(T) <= sizeof(S) && sizeof(S) < sizeof(int)), int>::type tosignedness(S val)
{
    return static_cast<int>(val);
}

// Makes a value signed or unsigned to match the first template parameter's signedness. If the
// converted type is smaller than the first parameter, it is converted to the first parameter
// type. If it is larger, but smaller than an int, it's converted to an int type.
template<typename T, typename S>
typename std::enable_if<std::is_unsigned<T>::value && std::is_signed<S>::value && (sizeof(T) <= sizeof(S) && sizeof(S) >= sizeof(unsigned int)), typename std::make_unsigned<S>::type>::type tosignedness(S val)
{
    return tounsigned(val);
}

// Makes a value signed or unsigned to match the first template parameter's signedness. If the
// converted type is smaller than the first parameter, it is converted to the first parameter
// type. If it is larger, but smaller than an int, it's converted to an int type.
template<typename T, typename S>
typename std::enable_if<std::is_unsigned<T>::value && std::is_signed<S>::value && (sizeof(T) <= sizeof(S) && sizeof(S) < sizeof(unsigned int)), unsigned int>::type tosignedness(S val)
{
    return static_cast<unsigned int>(val);
}
*/


// Converts T value to RESULT_TYPE, while asserting that it stays within conversion limits.
template<typename RESULT_TYPE, typename T>
typename std::enable_if<std::is_signed<RESULT_TYPE>::value && std::is_unsigned<T>::value && (sizeof(T) >= sizeof(RESULT_TYPE)), RESULT_TYPE>::type tosignedness(T val)
{
    assert(val <= static_cast<typename std::make_unsigned<RESULT_TYPE>::type>(std::numeric_limits<RESULT_TYPE>::max()));

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, while asserting that it stays within conversion limits.
template<typename RESULT_TYPE, typename T>
typename std::enable_if<std::is_signed<RESULT_TYPE>::value && std::is_signed<T>::value && (sizeof(T) > sizeof(RESULT_TYPE)), RESULT_TYPE>::type tosignedness(T val)
{
    assert(val <= std::numeric_limits<RESULT_TYPE>::max() && val >= std::numeric_limits<RESULT_TYPE>::min());

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, while asserting that it stays within conversion limits.
template<typename RESULT_TYPE, typename T>
typename std::enable_if<std::is_signed<RESULT_TYPE>::value && (sizeof(T) < sizeof(RESULT_TYPE) || (std::is_signed<T>::value && sizeof(T) == sizeof(RESULT_TYPE))), RESULT_TYPE>::type tosignedness(T val)
{
    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, while asserting that it stays within conversion limits.
template<typename RESULT_TYPE, typename T>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_signed<T>::value && (sizeof(T) >= sizeof(RESULT_TYPE)), RESULT_TYPE>::type tosignedness(T val)
{
    assert(val >= 0 && static_cast<typename std::make_unsigned<T>::type>(val) <= std::numeric_limits<RESULT_TYPE>::max());

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, while asserting that it stays within conversion limits.
template<typename RESULT_TYPE, typename T>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_signed<T>::value && (sizeof(T) < sizeof(RESULT_TYPE)), RESULT_TYPE>::type tosignedness(T val)
{
    assert(val >= 0);

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, while asserting that it stays within conversion limits.
template<typename RESULT_TYPE, typename T>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_unsigned<T>::value && (sizeof(T) <= sizeof(RESULT_TYPE)), RESULT_TYPE>::type tosignedness(T val)
{
    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, while asserting that it stays within conversion limits.
template<typename RESULT_TYPE, typename T>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_unsigned<T>::value && (sizeof(T) > sizeof(RESULT_TYPE)), RESULT_TYPE>::type tosignedness(T val)
{
    assert(val < std::numeric_limits<RESULT_TYPE>::max());

    return static_cast<RESULT_TYPE>(val);
}

// Converts T value to RESULT_TYPE, which must be a signed type. RESULT_TYPE is by default
// int32_t. The conversion asserts that the value is not out of limits of the result type.
template<typename RESULT_TYPE = int32_t, typename T = uint32_t>
typename std::enable_if<std::is_signed<RESULT_TYPE>::value, RESULT_TYPE>::type tosigned(T val)
{
    return tosignedness<RESULT_TYPE, T>(val);
}

// Converts T value to RESULT_TYPE, which must be an unsigned type. RESULT_TYPE is by default
// uint32_t. The conversion asserts that the value is not out of limits of the result type.
template<typename RESULT_TYPE = uint32_t, typename T = int32_t>
typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value, RESULT_TYPE>::type tounsigned(T val)
{
    return tosignedness<RESULT_TYPE, T>(val);
}


#ifdef NDEBUG_ADDED_SEPARATELY
#undef NDEBUG_ADDED_SEPARATELY
#undef NDEBUG
#endif

#endif // CHECKED_CAST_H

