/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef FASTARRAY_H
#define FASTARRAY_H

#include <vector>
#include "checked_cast.h"

// Dynamically allocated array holding only the array size and the array data.
template<typename T, typename S = size_t>
class fastarray
{
public:
    typedef T   value_type;
    typedef S   size_type;

    fastarray();
    fastarray(size_type initialsize);
    ~fastarray();

    fastarray(fastarray<T, S> &&src);
    fastarray<T, S>& operator=(fastarray<T, S> &&src);

    fastarray(const fastarray<T, S> &src);
    fastarray<T, S>& operator=(const fastarray<T, S> &src);

    fastarray(const std::vector<T> &src);
    fastarray<T, S>& operator=(const std::vector<T> &src);

    void clear();

    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type toVector(std::vector<T> &vec) const;
    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type toVector(std::vector<T> &vec) const;

    size_type size() const;
    bool empty() const;

    // Appends the items from other to the end of the array.
    void append(const fastarray<T, S> &other);

    // Appends the items from other to the end of the array. If this array
    // was empty, swaps the contents with other.
    void append(fastarray<T, S> &&other);

    // Changes the size of the array, keeping the old values with indexes up
    // to size - 1. If new values are created, only the default constructors
    // are called. Integral types will be uninitialized.
    void resize(size_type size);

    // Sets the size of the array, recreating it without keeping any of the
    // old values. The values previously in the array will be deleted, and the
    // new ones will be uninitialized.
    void setSize(size_type siz);

    // Removes an elem from the array at index, shortening the array by one.
    void erase(size_type index);

    void insert(size_type pos, const T &data);

    value_type& operator[](size_type index);
    const value_type& operator[](size_type index) const;

    // Direct access to the stored array.
    value_type* data();

    // Direct access to the stored array.
    const value_type* data() const;

    value_type* begin();
    const value_type* begin() const;
    value_type* end();
    const value_type* end() const;

    value_type& front();
    const value_type& front() const;
    value_type& back();
    const value_type& back() const;
private:
    size_type s;

    value_type *arr;
#ifdef _DEBUG
    inline void checkaccess(size_type index) const
    {
        if (index < 0 || index >= s)
            throw "!!!!";
    }

    inline void checkaccessover(size_type index) const
    {
        if (index < 0 || index > s)
            throw "!!!!";
    }
#endif

    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_move_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _resize(L siz);
    template<typename K = T, typename L = S>
    typename std::enable_if<!std::is_move_assignable<K>::value && std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _resize(L siz);
    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type _resize(L siz);

    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_move_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _append(const fastarray<T, S> &other);
    template<typename K = T, typename L = S>
    typename std::enable_if<!std::is_move_assignable<K>::value && std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _append(const fastarray<T, S> &other);
    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type _append(const fastarray<T, S> &other);

    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _copy(const fastarray<K, L> &src);
    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type _copy(const fastarray<K, L> &src);

    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _copy(const std::vector<K> &src);
    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type _copy(const std::vector<K> &src);

    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_move_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _erase(L index);
    template<typename K = T, typename L = S>
    typename std::enable_if<!std::is_move_assignable<K>::value && std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _erase(L index);
    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type _erase(L index);

    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_move_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _insert(L pos, const K &data);
    template<typename K = T, typename L = S>
    typename std::enable_if<!std::is_move_assignable<K>::value && std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type _insert(L pos, const K &data);
    template<typename K = T, typename L = S>
    typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type _insert(L pos, const K &data);
};


template<typename T, typename S>
bool operator==(const fastarray<T, S> &a, const fastarray<T, S> &b)
{
    if (a.size() != b.size())
        return false;
    for (auto it = a.begin(), it2 = b.begin(); it != a.end(); ++it, ++it2)
        if (*it != *it2)
            return false;
    return true;
}

template<typename T, typename S>
bool operator!=(const fastarray<T, S> &a, const fastarray<T, S> &b)
{
    if (a.size() != b.size())
        return true;
    for (auto it = a.begin(), it2 = b.begin(); it != a.end(); ++it, ++it2)
        if (*it != *it2)
            return true;
    return false;
}


template<typename T, typename S>
fastarray<T, S>::fastarray() : s(0), arr(nullptr)
{
}

template<typename T, typename S>
fastarray<T, S>::fastarray(size_type initialsize) : s(0), arr(nullptr)
{
    setSize(initialsize);
}

template<typename T, typename S>
fastarray<T, S>::~fastarray()
{
    clear();
}

template<typename T, typename S>
fastarray<T, S>::fastarray(fastarray<T, S> &&src) : s(0), arr(nullptr)
{
    *this = std::move(src);
}

template<typename T, typename S>
fastarray<T, S>& fastarray<T, S>::operator=(fastarray<T, S> &&src)
{
    std::swap(s, src.s);
    std::swap(arr, src.arr);
    return *this;
}

template<typename T, typename S>
fastarray<T, S>::fastarray(const fastarray<T, S> &src) : s(0), arr(nullptr)
{
    _copy(src);
}

template<typename T, typename S>
fastarray<T, S>& fastarray<T, S>::operator=(const fastarray<T, S> &src)
{
    _copy(src);
    return *this;
}

template<typename T, typename S>
fastarray<T, S>::fastarray(const std::vector<T> &src) : s(0), arr(nullptr)
{
    _copy(src);
}

template<typename T, typename S>
fastarray<T, S>& fastarray<T, S>::operator=(const std::vector<T> &src)
{
    _copy(src);
    return *this;
}

template<typename T, typename S>
void fastarray<T, S>::clear()
{
    s = 0;

    delete[] arr;
    arr = nullptr;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::toVector(std::vector<T> &vec) const
{
    vec.resize(s);
    auto *dat = &vec[0];
    for (size_type ix = 0; ix != s; ++ix)
        dat[ix] = arr[ix];
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::toVector(std::vector<T> &vec) const
{
    vec.resize(s);
    auto *dat = &vec[0];
    memcpy(dat, arr, sizeof(value_type) * s);
}

template<typename T, typename S>
void fastarray<T, S>::resize(size_type siz)
{
    _resize(siz);
}

template<typename T, typename S>
void fastarray<T, S>::setSize(size_type siz)
{
    if (siz == 0)
    {
        clear();
        return;
    }

    delete[] arr;
    arr = new value_type[siz];
    s = siz;
}

template<typename T, typename S>
void fastarray<T, S>::erase(size_type index)
{
    _erase(index);
}

template<typename T, typename S>
void fastarray<T, S>::insert(size_type pos, const T &data)
{
    _insert(pos, data);
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_move_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_resize(L siz)
{
    if (s == siz)
        return;
    if (siz == 0)
    {
        clear();
        return;
    }
    value_type *tmp = arr;
    arr = new value_type[siz];

    size_type cnt = std::min(s, siz);
    for (size_type ix = 0; ix != cnt; ++ix)
        arr[ix] = std::move(tmp[ix]);
    delete[] tmp;
    s = siz;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<!std::is_move_assignable<K>::value && std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_resize(L siz)
{
    if (s == siz)
        return;
    if (siz == 0)
    {
        clear();
        return;
    }
    value_type *tmp = arr;
    arr = new value_type[siz];

    size_type cnt = std::min(s, siz);
    for (size_type ix = 0; ix != cnt; ++ix)
        arr[ix] = tmp[ix];
    delete[] tmp;
    s = siz;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_resize(L siz)
{
    if (s == siz)
        return;
    if (siz == 0)
    {
        clear();
        return;
    }
    value_type *tmp = arr;
    arr = new value_type[siz];
    memcpy(arr, tmp, sizeof(value_type) * std::min(s, siz));
    delete[] tmp;
    s = siz;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_move_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_append(const fastarray<T, S> &other)
{
    size_type cnt = s;
    resize(s + other.size());

    for (size_type ix = 0, siz = tosignedness<size_type>(other.size()); ix != siz; ++ix)
        arr[cnt + ix] = std::move(other[ix]);
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<!std::is_move_assignable<K>::value && std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_append(const fastarray<T, S> &other)
{
    size_type cnt = s;
    resize(s + other.size());

    for (size_type ix = 0, siz = tosignedness<size_type>(other.size()); ix != siz; ++ix)
        arr[cnt + ix] = other[ix];
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_append(const fastarray<T, S> &other)
{
    size_type cnt = s;
    resize(s + other.size());
    memcpy(arr + cnt, other.arr, sizeof(T) * other.size());
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_copy(const fastarray<K, L> &src)
{
    if (this == &src)
        return;
    if (src.s == 0)
    {
        clear();
        return;
    }
    delete[] arr;
    arr = new value_type[src.s];
    for (size_type ix = 0; ix != src.s; ++ix)
        arr[ix] = src.arr[ix];
    s = src.s;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_copy(const fastarray<K, L> &src)
{
    if (this == &src)
        return;
    if (src.s == 0)
    {
        clear();
        return;
    }
    delete[] arr;
    arr = new value_type[src.s];
    memcpy(arr, src.arr, sizeof(value_type) * src.s); // std::min(s, src.s));
    s = src.s;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_copy(const std::vector<K> &src)
{
    if (src.empty())
    {
        clear();
        return;
    }
    delete[] arr;
    arr = new value_type[src.size()];

    size_type siz = tosignedness<size_type>(src.size());

    auto *dat = &src[0];
    for (size_type ix = 0; ix != siz; ++ix)
        arr[ix] = dat[ix];
    s = siz;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_copy(const std::vector<K> &src)
{
    if (src.empty())
    {
        clear();
        return;
    }
    delete[] arr;

    size_type siz = tosignedness<size_type>(src.size());

    arr = new value_type[siz];
    memcpy(arr, &src[0], sizeof(value_type) * siz); // std::min(s, siz));
    s = siz;
}


template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_move_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_erase(L index)
{
#ifdef _DEBUG
    checkaccess(index);
#endif

    if (s == 1)
    {
        clear();
        return;
    }
    value_type *tmp = arr;
    arr = new value_type[s - 1];

    for (size_type ix = 0, pos = 0; ix != s; ++ix, ++pos)
    {
        if (ix == index)
        {
            --pos;
            continue;
        }

        arr[pos] = std::move(tmp[ix]);
    }

    delete[] tmp;
    --s;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<!std::is_move_assignable<K>::value && std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_erase(L index)
{
#ifdef _DEBUG
    checkaccess(index);
#endif

    if (s == 1)
    {
        clear();
        return;
    }
    value_type *tmp = arr;
    arr = new value_type[s - 1];

    for (size_type ix = 0, pos = 0; ix != s; ++ix, ++pos)
    {
        if (ix == index)
        {
            --pos;
            continue;
        }
        arr[pos] = tmp[ix];
    }

    delete[] tmp;
    --s;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_erase(L index)
{
#ifdef _DEBUG
    checkaccess(index);
#endif

    if (s == 1)
    {
        clear();
        return;
    }

    value_type *tmp = arr;
    arr = new value_type[s - 1];
    if (index != 0)
        memcpy(arr, tmp, sizeof(value_type) * index);

    memcpy(arr + index, tmp + index + 1, sizeof(value_type) * (s - index - 1));
    delete[] tmp;
    --s;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_move_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_insert(L pos, const K &data)
{
#ifdef _DEBUG
    checkaccessover(pos);
#endif

    value_type *tmp = arr;
    arr = new value_type[s + 1];

    for (size_type ix = 0; ix != pos; ++ix)
        arr[ix] = std::move(tmp[ix]);

    for (size_type ix = pos + 1; ix != s + 1; ++ix)
        arr[ix] = std::move(tmp[ix - 1]);
    delete[] tmp;
    ++s;

    arr[pos] = data;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<!std::is_move_assignable<K>::value && std::is_copy_assignable<K>::value && !std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_insert(L pos, const K &data)
{
#ifdef _DEBUG
    checkaccessover(pos);
#endif

    value_type *tmp = arr;
    arr = new value_type[s + 1];

    for (size_type ix = 0; ix != pos; ++ix)
        arr[ix] = tmp[ix];

    for (size_type ix = pos + 1; ix != s + 1; ++ix)
        arr[ix] = tmp[ix - 1];
    delete[] tmp;
    ++s;

    arr[pos] = data;
}

template<typename T, typename S>
template<typename K, typename L>
typename std::enable_if<std::is_trivially_copyable<K>::value, void>::type fastarray<T, S>::_insert(L pos, const K &data)
{
#ifdef _DEBUG
    checkaccessover(pos);
#endif

    value_type *tmp = arr;
    arr = new value_type[s + 1];
    if (pos != 0)
        memcpy(arr, tmp, sizeof(value_type) * pos);
    if (pos != s)
        memcpy(arr + pos + 1, tmp + pos, sizeof(value_type) * (s - pos));

    delete[] tmp;
    ++s;

    arr[pos] = data;
}

template<typename T, typename S>
typename fastarray<T, S>::size_type fastarray<T, S>::size() const
{
    return s;
}

template<typename T, typename S>
bool fastarray<T, S>::empty() const
{
    return s == 0;
}

template<typename T, typename S>
void fastarray<T, S>::append(const fastarray<T, S> &other)
{
    if (other.empty())
        return;

    //size_type cnt = s;
#ifdef _DEBUG
    // Size too big, will overflow
    if (s && s + other.size() <= s)
        throw "Overflow of array sizes.";
#endif

    _append(other);
}

template<typename T, typename S>
void fastarray<T, S>::append(fastarray<T, S> &&other)
{
    if (empty())
    {
        std::swap(*this, other);
        return;
    }
    
    append(other);
}

template<typename T, typename S>
typename fastarray<T, S>::value_type* fastarray<T, S>::data()
{
    return arr;
}

template<typename T, typename S>
const typename fastarray<T, S>::value_type* fastarray<T, S>::data() const
{
    return arr;
}

template<typename T, typename S>
typename fastarray<T, S>::value_type* fastarray<T, S>::begin()
{
    return arr;
}

template<typename T, typename S>
const typename fastarray<T, S>::value_type* fastarray<T, S>::begin() const
{
    return arr;
}

template<typename T, typename S>
typename fastarray<T, S>::value_type* fastarray<T, S>::end()
{
    return arr + s;
}

template<typename T, typename S>
const typename fastarray<T, S>::value_type* fastarray<T, S>::end() const
{
    return arr + s;
}

template<typename T, typename S>
typename fastarray<T, S>::value_type& fastarray<T, S>::front()
{
#ifdef _DEBUG
    if (s == 0)
        throw "Empty array access error.";
#endif
    return arr[0];
}

template<typename T, typename S>
const typename fastarray<T, S>::value_type& fastarray<T, S>::front() const
{
#ifdef _DEBUG
    if (s == 0)
        throw "Empty array access error.";
#endif
    return arr[0];
}

template<typename T, typename S>
typename fastarray<T, S>::value_type& fastarray<T, S>::back()
{
#ifdef _DEBUG
    if (s == 0)
        throw "Empty array access error.";
#endif
    return arr[s - 1];
}

template<typename T, typename S>
const typename fastarray<T, S>::value_type& fastarray<T, S>::back() const
{
#ifdef _DEBUG
    if (s == 0)
        throw "Empty array access error.";
#endif
    return arr[s - 1];
}

template<typename T, typename S>
typename fastarray<T, S>::value_type& fastarray<T, S>::operator[](size_type index)
{
#ifdef _DEBUG
    checkaccess(index);
#endif
    return arr[index];
}

template<typename T, typename S>
const typename fastarray<T, S>::value_type& fastarray<T, S>::operator[](size_type index) const
{
#ifdef _DEBUG
    checkaccess(index);
#endif
    return arr[index];
}


#endif // FASTARRAY_H
