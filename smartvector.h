/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef SMARTVECTOR_H
#define SMARTVECTOR_H


#include <vector>

namespace nostd
{
    template<typename iteratorT>
    std::reverse_iterator<iteratorT> make_rev_it(iteratorT it)
    {
        return std::reverse_iterator<iteratorT>(it);
    }
}


template<typename T, typename Alloc=std::allocator<T*>> class smartvector;
template<typename T, typename Alloc=std::allocator<T*>> class smartvector_iterator;
template<typename T, typename Alloc=std::allocator<T*>> class smartvector_const_iterator;

template<typename T, typename Alloc> smartvector_iterator<T, Alloc> operator+(typename smartvector_iterator<T, Alloc>::difference_type, const smartvector_iterator<T, Alloc> &);
template<typename T, typename Alloc> smartvector_iterator<T, Alloc> operator-(typename smartvector_iterator<T, Alloc>::difference_type, const smartvector_iterator<T, Alloc> &);

template<typename T, typename Alloc> smartvector_const_iterator<T, Alloc> operator+(typename smartvector_const_iterator<T, Alloc>::difference_type, const smartvector_const_iterator<T, Alloc> &);
template<typename T, typename Alloc> smartvector_const_iterator<T, Alloc> operator-(typename smartvector_const_iterator<T, Alloc>::difference_type, const smartvector_const_iterator<T, Alloc> &);

template<typename T, typename Alloc>
class smartvector_iterator 
{
protected:
    typedef typename std::vector<T*, Alloc>::iterator basetype;
    typedef typename std::vector<T*, Alloc>::const_iterator const_basetype;
private:
    typedef smartvector_iterator<T, Alloc> self_type;
    typedef smartvector_const_iterator<T, Alloc> const_self_type;
    basetype base;
protected:
    basetype _base() { return base; }
    const basetype _base() const { return base; }
public:
    typedef typename std::vector<T*, Alloc>::iterator::iterator_category iterator_category;
    typedef T*   value_type;
    typedef T**  pointer;
    typedef T*&  reference;
    typedef typename basetype::difference_type  difference_type;

    smartvector_iterator() : base() {}

    smartvector_iterator(const self_type &orig) : base(orig.base) {}
    //smartvector_iterator(const const_self_type &orig) : base(orig.base) {}

    smartvector_iterator(const basetype &base) : base(base) {}
    //smartvector_iterator(const const_basetype &base) : base(base) {}


    bool isNull() const { return base.operator*() == nullptr; }
    bool null() const { return base.operator*() == nullptr; }
    T*& operator*() { return base.operator*(); }
    T*& operator->() { return base.operator->(); }
    T*& operator[](difference_type n) { return base[n]; }

    T*& operator*() const { return base.operator*(); }
    T*& operator->() const { return base.operator->(); }
    T*& operator[](difference_type n) const { return base[n]; }

    self_type& operator++() { ++base; return *this; }
    self_type operator++(int) { self_type copy(*this);  ++base; return copy; }
    self_type& operator--() { --base; return *this; }
    self_type operator--(int) { self_type copy(*this);  --base; return copy; }

    self_type operator+(difference_type n) const { return self_type(base + n); }
    self_type operator-(difference_type n) const { return self_type(base - n); }
    self_type& operator+=(difference_type n) { base += n; return *this; }
    self_type& operator-=(difference_type n) { base -= n; return *this; }

    difference_type operator-(const self_type &other) const { return base - other.base; }
    difference_type operator-(const const_self_type &other) const { return base - other.base; }

    bool operator==(const self_type &other) const { return base == other.base; }
    bool operator!=(const self_type &other) const { return base != other.base; }
    bool operator<(const self_type &other) const { return base < other.base; }
    bool operator>(const self_type &other) const { return base > other.base; }
    bool operator<=(const self_type &other) const { return base <= other.base; }
    bool operator>=(const self_type &other) const { return base >= other.base; }

    bool operator==(const const_self_type &other) const { return base == other.base; }
    bool operator!=(const const_self_type &other) const { return base != other.base; }
    bool operator<(const const_self_type &other) const { return base < other.base; }
    bool operator>(const const_self_type &other) const { return base > other.base; }
    bool operator<=(const const_self_type &other) const { return base <= other.base; }
    bool operator>=(const const_self_type &other) const { return base >= other.base; }
private:
    friend smartvector<T, Alloc>;
    friend smartvector_const_iterator<T, Alloc>;

    friend smartvector_iterator<T, Alloc> (::operator+ <>) (difference_type, const smartvector_iterator<T, Alloc> &);
    friend smartvector_iterator<T, Alloc> (::operator- <>) (difference_type, const smartvector_iterator<T, Alloc> &);
};

template<typename T, typename Alloc = std::allocator<T*>>
class smartvector_move_iterator : public smartvector_iterator<T, Alloc>
{
private:
    typedef smartvector_iterator<T, Alloc>  base;
    typedef smartvector_move_iterator<T, Alloc> self_type;
public:
    typedef typename base::difference_type difference_type;

    smartvector_move_iterator() : base() {}
    smartvector_move_iterator(const self_type &orig) : base(orig._base()) {}
    smartvector_move_iterator(const typename base::basetype &origbase) : base(origbase) {}

    self_type& operator++() { base::operator++(); return *this; }
    self_type operator++(int) { self_type copy(*this);  base::operator++(); return copy; }
    self_type& operator--() { base::operator--(); return *this; }
    self_type operator--(int) { self_type copy(*this);  base::operator--(); return copy; }

    self_type operator+(difference_type n) const { return self_type(this->_base() + n); }
    self_type operator-(difference_type n) const { return self_type(this->_base() - n); }
    self_type& operator+=(difference_type n) { base::operator+=(n); return *this; }
    self_type& operator-=(difference_type n) { base::operator-=(n); return *this; }

    difference_type operator-(const self_type &other) const { return this->_base() - other._base(); }

    bool operator==(const self_type &other) const { return this->_base() == other._base(); }
    bool operator!=(const self_type &other) const { return this->_base() != other._base(); }
    bool operator<(const self_type &other) const { return this->_base() < other._base(); }
    bool operator>(const self_type &other) const { return this->_base() > other._base(); }
    bool operator<=(const self_type &other) const { return this->_base() <= other._base(); }
    bool operator>=(const self_type &other) const { return this->_base() >= other._base(); }

    friend smartvector<T, Alloc>;
};

template<typename T, typename Alloc>
class smartvector_const_iterator
{
private:
    typedef smartvector_const_iterator<T, Alloc> self_type;
    typedef smartvector_iterator<T, Alloc> mod_self_type;
    typedef typename std::vector<T*, Alloc>::const_iterator basetype;
    typedef typename std::vector<T*, Alloc>::iterator mod_basetype;
    basetype base;
public:
    typedef typename std::vector<T*, Alloc>::const_iterator::iterator_category iterator_category;
    typedef T*   value_type;
    typedef T**  pointer;
    typedef T*&  reference;
    typedef typename basetype::difference_type  difference_type;

    smartvector_const_iterator() : base() {}

    smartvector_const_iterator(const self_type &orig) : base(orig.base) {}
    smartvector_const_iterator(const mod_self_type &orig) : base(orig.base) {}

    smartvector_const_iterator(const basetype &base) : base(base) {}
    smartvector_const_iterator(const mod_basetype &base) : base(base) {}

    bool isNull() const { return base.operator*() == nullptr; }
    bool null() const { return base.operator*() == nullptr; }
    const T* operator*() const { return base.operator*(); }
    const T* operator->() const { return base.operator->(); }
    const T* operator[](difference_type n) const { return base[n]; }

    self_type& operator++() { ++base; return *this; }
    self_type operator++(int) { self_type copy(*this);  ++base; return copy; }
    self_type& operator--() { --base; return *this; }
    self_type operator--(int) { self_type copy(*this);  --base; return copy; }

    self_type operator+(difference_type n) const { return self_type(base + n); }
    self_type operator-(difference_type n) const { return self_type(base - n); }
    self_type& operator+=(difference_type n) { base += n; return *this; }
    self_type& operator-=(difference_type n) { base -= n; return *this; }

    difference_type operator-(const self_type &other) const { return base - other.base; }
    difference_type operator-(const mod_self_type &other) const { return base - other.base; }

    bool operator==(const self_type &other) const { return base == other.base; }
    bool operator!=(const self_type &other) const { return base != other.base; }
    bool operator<(const self_type &other) const { return base < other.base; }
    bool operator>(const self_type &other) const { return base > other.base; }
    bool operator<=(const self_type &other) const { return base <= other.base; }
    bool operator>=(const self_type &other) const { return base >= other.base; }

    bool operator==(const mod_self_type &other) const { return base == other.base; }
    bool operator!=(const mod_self_type &other) const { return base != other.base; }
    bool operator<(const mod_self_type &other) const { return base < other.base; }
    bool operator>(const mod_self_type &other) const { return base > other.base; }
    bool operator<=(const mod_self_type &other) const { return base <= other.base; }
    bool operator>=(const mod_self_type &other) const { return base >= other.base; }

private:
    friend smartvector<T, Alloc>;
    friend smartvector_iterator<T, Alloc>;

    friend smartvector_const_iterator<T, Alloc> (::operator+ <>) (difference_type, const smartvector_const_iterator<T, Alloc> &);
    friend smartvector_const_iterator<T, Alloc> (::operator- <>) (difference_type, const smartvector_const_iterator<T, Alloc> &);
};



template<typename T, typename Alloc> smartvector_iterator<T, Alloc> operator+(int n, const smartvector_iterator<T, Alloc> &it)
{
    return smartvector_iterator<T, Alloc>(n);
}
template<typename T, typename Alloc> smartvector_iterator<T, Alloc> operator-(int n, const smartvector_iterator<T, Alloc> &it)
{
    return smartvector_iterator<T, Alloc>(-n);
}
template<typename T, typename Alloc> smartvector_const_iterator<T, Alloc> operator+(int n, const smartvector_const_iterator<T, Alloc> &it)
{
    return smartvector_iterator<T, Alloc>(n);
}
template<typename T, typename Alloc> smartvector_const_iterator<T, Alloc> operator-(int n, const smartvector_const_iterator<T, Alloc> &it)
{
    return smartvector_iterator<T, Alloc>(-n);
}


// A vector class that uses std::vector but deletes the elements on erase or clear.
// The T template parameter makes the vector hold T* pointers. T can't be a pointer
// type itself. Passing a pointer for it disables all constructors, which results in
// compile time error. If specified, the custom allocator must be valid for a vector
// that holds T* elements.
template<typename T, typename Alloc>
class smartvector : protected std::vector<T*, Alloc>
{
private:
    typedef std::vector<T*, Alloc>  base;
    typedef smartvector<T, Alloc>   self_type;
    typedef T   value_base_type;

public:
    typedef typename base::size_type size_type;
    typedef T*   value_type;
    typedef T*&   reference;
    typedef const T*&   const_reference;
    typedef typename base::allocator_type    allocator_type;
    typedef typename std::allocator_traits<allocator_type>::pointer pointer;
    typedef typename std::allocator_traits<allocator_type>::const_pointer const_pointer;
    typedef smartvector_iterator<T, Alloc>  iterator;
    typedef smartvector_const_iterator<T, Alloc>    const_iterator;
    typedef std::reverse_iterator<smartvector_iterator<T, Alloc>>   reverse_iterator;
    typedef std::reverse_iterator<smartvector_const_iterator<T, Alloc>> const_reverse_iterator;
    typedef typename base::difference_type  difference_type;

    typedef smartvector_move_iterator<T, Alloc>  move_iterator;

    explicit smartvector(typename std::enable_if<!std::is_pointer<T>::value, const allocator_type&>::type alloc = allocator_type()) : base(alloc) {}
    explicit smartvector(typename std::enable_if<!std::is_pointer<T>::value, size_type>::type n) : base(n) {}

    smartvector(typename std::enable_if<!std::is_pointer<T>::value, size_type>::type n, const value_base_type& val, const allocator_type& alloc = allocator_type()) : base(alloc)
    {
        base::reserve(n);
        for (int ix = 0; ix != n; ++ix)
            push_back(val);
    }

    template<class InputIterator>
    smartvector(typename std::enable_if<!std::is_pointer<T>::value, InputIterator>::type first, InputIterator last, const allocator_type& alloc = allocator_type()) : base(alloc)
    {
        _assign(first, last);
    }

    smartvector(typename std::enable_if<!std::is_pointer<T>::value, const self_type&>::type x) : base(x.get_allocator())
    {
        base::reserve(x.size());
        for (auto it = x.begin(); it != x.end(); ++it)
        {
            if (it.null())
                push_back(nullptr);
            else
                push_back(**it);
        }
    }

    smartvector(typename std::enable_if<!std::is_pointer<T>::value, const self_type&>::type x, const allocator_type& alloc) : base(alloc)
    {
        base::reserve(x.size());
        for (auto it = x.begin(); it != x.end(); ++it)
        {
            if (it.null())
                push_back(nullptr);
            else
                push_back(**it);
        }
    }

    smartvector(typename std::enable_if<!std::is_pointer<T>::value, self_type&&>::type x) : base() { swap(x); /* swap(std::forward<self_type>(x)); */ }
    smartvector(typename std::enable_if<!std::is_pointer<T>::value, std::vector<value_base_type*> &&>::type x) : base() { base::swap(std::forward<std::vector<value_base_type*>>(x)); }

    smartvector(typename std::enable_if<!std::is_pointer<T>::value, std::initializer_list<value_base_type>>::type il, const allocator_type& alloc = allocator_type()) : base(alloc)
    {
        _assign(il);
    }

    ~smartvector() { clear(); }

    size_type size() const
    {
        return base::size();
    }

    void reserve(size_type n)
    {
        base::reserve(n);
    }

    void shrink_to_fit()
    {
        base::shrink_to_fit();
    }

    bool empty() const
    {
        return base::empty();
    }

    size_type capacity() const
    {
        return base::capacity();
    }

    size_type max_size() const
    {
        return base::max_size();
    }
        
    template <class... Args>
    void emplace_back(Args&&... args)
    {
        base::push_back(new value_base_type(std::forward<Args>(args)...));
    }

    template <class... Args>
    iterator emplace(const_iterator position, Args&&... args)
    {
        base::insert(position.base, new value_base_type(std::forward<Args>(args)...));
    }

    // Same as resize but only works if n is smaller than the current size.
    void shrink(size_type n)
    {
        if (n < size())
            resize(n);
    }

private:
    void resize(size_type n)
    {
        if (n < size())
            for (auto it = begin() + n; it != end(); ++it)
                delete *it;
        base::resize(n);
    }
public:
    // Resizes the vector to have space for n number of items. If n is larger
    // than the current size, the vector is padded with nullptr. If smaller,
    // the items with index above n - 1 are deleted before the resize.
    void resizeAddNull(size_type n)
    {
        size_type s = size();
        if (s == n)
            return;

        resize(n);

        if (n < s)
            return;

        memset(&base::operator[](s), 0, sizeof(void*) * (n - s));
    }

    // Resizes the vector by adding or deleting items. If items must be added
    // they are constructed with the passed arguments.
    template <typename... Args>
    void resize(size_type n, Args... args)
    {
        size_type s = size();
        if (s == n)
            return;

        resize(n);

        if (n < s)
            return;

        auto data = &base::operator[](0);
        for (size_type ix = s; ix != n; ++ix)
            data[ix] = new value_base_type(args...);
    }

    template <class InputIterator> void assign(InputIterator first, InputIterator last)
    {
        swap(smartvector<value_base_type>());
        _assign(first, last);
    }

    allocator_type get_allocator() const
    {
        return base::get_allocator();
    }

    value_base_type*& at(size_type n)
    {
        return base::at(n);
    }

    value_base_type* at(size_type n) const
    {
        return const_cast<value_base_type*>(base::at(n));
    }

    value_base_type*& front()
    {
        return base::front();
    }

    value_base_type* front() const
    {
        return const_cast<value_base_type*>(base::front());
    }

    value_base_type*& back()
    {
        return base::back();
    }

    value_base_type* back() const
    {
        return const_cast<value_base_type*>(base::back());
    }

    bool isNull(size_type n)
    {
        return base::operator[](n) == nullptr;
    }

    bool null(size_type n)
    {
        return base::operator[](n) == nullptr;
    }

    value_base_type*& operator[](size_type n)
    {
        return base::operator[](n);
    }

    value_base_type* operator[](size_type n) const
    {
        return const_cast<value_base_type*>(base::operator[](n));
    }

    void assign(size_type n, const value_base_type &val)
    {
        swap(smartvector<value_base_type>());

        base::reserve(n);
        for (int ix = 0; ix != n; ++ix)
            base::push_back(new value_base_type(val));
    }

    void assign(std::initializer_list<value_base_type> il)
    {
        swap(smartvector<value_base_type>());
        _assign(il);
    }

    void push_back(const value_base_type& val)
    {
        base::push_back(new value_base_type(val));
    }

    // The object pointed to by val will be deleted by the vector. Don't push the same
    // pointer twice, or a pointer that can be deleted elsewhere.
    void push_back(value_base_type* val)
    {
        base::push_back(val);
    }

    void push_back(value_base_type&& val)
    {
        base::push_back(new value_base_type(std::forward<value_base_type>(val)));
    }

    void pop_back()
    {
        delete base::back();
        base::pop_back();
    }

    // The object pointed to by val will be deleted by the vector. Don't push the same
    // pointer twice, or a pointer that can be deleted elsewhere.
    iterator insert(const_iterator position, value_base_type *val)
    {
        return base::insert(position.base, val);
    }

    iterator insert(const_iterator position, const value_base_type &val)
    {
        return base::insert(position.base, new value_base_type(val));
    }

    iterator insert(const_iterator position, size_type n, const value_base_type &val)
    {
        if (n == 0)
            return begin() + (position - cbegin());

        int p = position - cbegin();

        base::insert(position.base, n, nullptr);
        iterator it = begin() + p + n;

        while (n-- != 0)
        {
            --it;
            *it = new value_base_type(val);
        }
        return it;
    }

    iterator insert(const_iterator position, size_type n, std::nullptr_t /*nul*/)
    {
        if (n == 0)
            return begin() + (position - cbegin());

        //int p = position - cbegin();

        return base::insert(position.base, n, nullptr);
        //iterator it = begin() + p + n;

        //while (n-- != 0)
        //{
        //    --it;
        //    *it = new value_base_type(val);
        //}
        //return it;
    }

    // Inserting with iterator referencing a pointer in another smartvector of the same type.
    // The elements inserted are copies of the original objects, not pointers.
    template<class InputIterator>
    typename std::enable_if<!std::is_same<InputIterator, move_iterator>::value && (std::is_same<InputIterator, iterator>::value || std::is_same<InputIterator, const_iterator>::value), iterator>::type
        insert(const_iterator position, InputIterator first, InputIterator last)
    {
        if (first == last)
            return begin() + (position - cbegin());

        int n = std::distance(first, last);
#ifdef _DEBUG
        if (n < 0)
            throw "First comes after last.";
#endif
        int p = position - cbegin();
        base::insert(position.base, n, nullptr);
        iterator it = begin() + p + n;

        while (first != last)
        {
            --it;
            --last;
            if (*last != nullptr)
                *it = new value_base_type(**last);
        }

        return it;
    }

    // Inserting with move iterator referencing a pointer in another smartvector of the same
    // type. The elements are inserted, and the originals are zeroed out. After this, the
    // same iterators can be used in the other smartvector to erase the values.
    template<class InputIterator>
    typename std::enable_if<std::is_same<InputIterator, move_iterator>::value, iterator>::type
        insert(const_iterator position, InputIterator first, InputIterator last)
    {
        int p = position - cbegin();
        if (first == last)
            return begin() + p;

#ifdef _DEBUG
        int n = std::distance(first, last);
        if (n < 0)
            throw "First comes after last.";
#endif
        base::insert(position.base, first._base(), last._base());
        //iterator it = begin() + p + n;

        while (first != last)
        {
            *first = nullptr;
            ++first;
            //--it;
            //--last;
            //if (*last != nullptr)
            //    *it = new value_base_type(**last);
        }

        return begin() + p;
    }


    // Inserting with iterator referencing a pointer in another container. Make sure these pointers
    // won't get deleted elsewhere.
    template<class InputIterator>
    typename std::enable_if<!std::is_same<InputIterator, move_iterator>::value && !std::is_same<InputIterator, iterator>::value && !std::is_same<InputIterator, const_iterator>::value &&
        //std::is_pointer<typename InputIterator::value_base_type>::value,
        std::is_pointer<typename std::iterator_traits<InputIterator>::value_type>::value,
        iterator>::type
        insert(const_iterator position, InputIterator first, InputIterator last)
    {
        //if (first == last)
        //    return;
        //int n = std::distance(first, last);
        //int p = position - cbegin();

        return base::insert(position.base, first, last);

        //iterator it = begin() + p;

        //while (first != last)
        //{
        //    if (*first != nullptr)
        //        *it = *first;
        //    ++it;
        //    ++first;
        //}

        //return begin() + p;
    }

    // Inserting with unknown iterator referring to objects by value not by pointer.
    template<class InputIterator>
    typename std::enable_if<
        //!std::is_pointer<typename InputIterator::value_base_type>::value,
        !std::is_pointer<typename std::iterator_traits<InputIterator>::value_type>::value,
        iterator>::type
         insert(const_iterator position, InputIterator first, InputIterator last)
    {
        if (first == last)
            return begin() + (position - cbegin());

        int n = std::distance(first, last);
#ifdef _DEBUG
        if (n < 0)
            throw "First comes after last.";
#endif
        int p = position - cbegin();
        base::insert(position.base, n, nullptr);
        iterator it(position);

        while (first != last)
        {
            *it = new value_base_type(*first);
            ++it;
            ++first;
        }

        return begin() + p;
    }

    iterator insert(const_iterator position, value_base_type&& val)
    {
        return base::insert(position.base, new value_base_type(std::forward<value_base_type>(val)));
    }

    iterator insert(const_iterator position, std::initializer_list<value_base_type> il)
    {
        return insert(position, il.begin(), il.end());
    }

    iterator erase(const_iterator position)
    {
        delete *position;
        return base::erase(position.base);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
#ifdef _DEBUG
        if (first > last)
            throw "First comes after last.";
#endif
        for (auto it = first; it != last; ++it)
            delete *it;
        return base::erase(first.base, last.base);
    }

    // Removes the pointer at the given position without deleting it, and sets result to the
    // removed pointer. Returns the next iterator position after the removed pointer.
    iterator removeAt(const_iterator position, value_base_type* &result)
    {
        result = base::operator[](position - cbegin());
        return base::erase(position.base);
    }

    // Removes the pointer at the given position without deleting it. Returns the next
    // iterator position after the removed pointer.
    iterator removeAt(const_iterator position)
    {
        return base::erase(position.base);
    }

    //// Removes the pointer at the given position without deleting it, and returns
    //// the pointer.
    //value_base_type* removeAt(iterator position)
    //{
    //    value_base_type* val = *position;
    //    base::erase(position.base);
    //    return val;
    //}

    // Removes the pointers between [first, last) without deleting them and places them in
    // result. Returns an iterator to the position after the removed pointers.
    iterator removeAt(const_iterator first, const_iterator last, std::vector<value_base_type*> &result)
    {
        std::vector<value_base_type*>(first.base, last.base).swap(result);
        return base::erase(first.base, last.base);
    }

    // Removes the pointers between [first, last) without deleting them. Returns an iterator
    // to the position after the removed pointers.
    iterator removeAt(const_iterator first, const_iterator last)
    {
        return base::erase(first.base, last.base);
    }


    //// Removes the pointers between [first, last) without deleting them and returns them in a
    //// vector. 
    //std::vector<value_base_type*> removeAt(iterator first, iterator last)
    //{
    //    std::vector<value_base_type*> result(first.base(), last.base());
    //    base::erase(first.base(), last.base());
    //    return result;
    //}

    void swap(self_type &x)
    {
        base::swap(x);
    }

    void swap(base &x)
    {
        base::swap(x);
    }

    void clear() /*noexcept - not in VS2013*/
    {
        if (empty())
            return;

        auto n = size() - 1;
        value_type *d = base::data() + n;
        while (n-- != 0)
        {
            delete *d;
            --d;
        }
        delete *d;

        //for (auto it = base::begin(); it != base::end(); ++it)
        //    delete *it;

        base::clear();
    }

    iterator begin() /*noexcept*/
    {
        return base::begin();
    }

    move_iterator mbegin() /* noexcept*/
    {
        return base::begin();
    }
        
    const_iterator begin() const /*noexcept*/
    {
        return base::begin();
    }

    iterator end() /*noexcept*/
    {
        return base::end();
    }

    move_iterator mend() /* noexcept*/
    {
        return base::end();
    }

    const_iterator end() const /*noexcept*/
    {
        return base::end();
    }

    reverse_iterator rbegin() /*noexcept*/
    {
        return nostd::make_rev_it(iterator(base::end()));
    }

    const_reverse_iterator rbegin() const /*noexcept*/
    {
        return nostd::make_rev_it(const_iterator(base::end()));
    }

    reverse_iterator rend() /*noexcept*/
    {
        return nostd::make_rev_it(iterator(base::begin()));
    }

    const_reverse_iterator rend() const /*noexcept*/
    {
        return nostd::make_rev_it(const_iterator(base::begin()));
    }

    const_iterator cbegin() const /*noexcept*/
    {
        return base::cbegin();
    }

    const_iterator cend() const /*noexcept*/
    {
        return base::cend();
    }

    const_reverse_iterator crbegin() const /*noexcept*/
    {
        return nostd::make_rev_it(const_iterator(base::end()));
    }

    const_reverse_iterator crend() const /*noexcept*/
    {
        return nostd::make_rev_it(const_iterator(base::begin()));
    }

    int indexOf(value_base_type *pt)
    {
        auto it = std::find(base::begin(), base::end(), pt);
        if (it == base::end())
            return -1;
        return it - base::begin();
    }

    iterator findPointer(value_base_type *pt)
    {
        return std::find(base::begin(), base::end(), pt);
    }

    value_base_type** data()
    {
        return base::data();
    }

    const value_base_type *const* data() const
    {
        return base::data();
    }

private:
    void _assign(std::initializer_list<value_base_type> &il)
    {
        _assign(il.begin(), il.end());
        //base::reserve(il.size());
        //for (auto it = il.begin(); it != il.end(); ++it)
        //    push_back(*it)
    }

    // Available if the passed iterator references an object by pointer and not by value.
    template<class InputIterator>
    typename std::enable_if</*std::is_same<InputIterator, iterator>::value || std::is_same<InputIterator, const_iterator>::value ||*/
        //std::is_pointer<typename InputIterator::value_base_type>::value,
        std::is_pointer<typename std::iterator_traits<InputIterator>::value_type>::value,
        void>::type
        _assign(InputIterator first, InputIterator last)
    {
        // Doesn't clear the list. It is done in assign(), but not in the constructor
        // where it's unnecessary.
        base::reserve(std::distance(first, last));
        for (auto it = first; it != last; ++it)
            if (*it != nullptr)
                push_back(**it);
            else
                push_back(nullptr);
    }

    // Available if the passed iterator is NOT referencing a pointer.
    template<class InputIterator>
    typename std::enable_if</*!std::is_same<InputIterator, iterator>::value && !std::is_same<InputIterator, const_iterator>::value ||*/
        //!std::is_pointer<typename InputIterator::value_base_type>::value,
        !std::is_pointer<typename std::iterator_traits<InputIterator>::value_type>::value,
        void>::type
        _assign(InputIterator first, InputIterator last)
    {
        // Doesn't clear the list. It is done in assign(), but not in the constructor
        // where it's unnecessary.
        base::reserve(std::distance(first, last));
        for (auto it = first; it != last; ++it)
            push_back(*it);
    }

    typedef typename base::iterator  base_iterator;
    typedef typename base::const_iterator    base_const_iterator;

    template<typename TT, typename AAlloc>
    friend bool operator==(const smartvector<TT, AAlloc> &a, const smartvector<TT, AAlloc> &b);
    template<typename TT, typename AAlloc>
    friend bool operator!=(const smartvector<TT, AAlloc> &a, const smartvector<TT, AAlloc> &b);
};

template<typename T, typename Alloc>
bool operator==(const smartvector<T, Alloc> &a, const smartvector<T, Alloc> &b)
{
    return static_cast<const std::vector<T*, Alloc>&>(a) == static_cast<const std::vector<T*, Alloc>&>(b);
}

template<typename T, typename Alloc>
bool operator!=(const smartvector<T, Alloc> &a, const smartvector<T, Alloc> &b)
{
    return static_cast<const std::vector<T*, Alloc>&>(a) != static_cast<const std::vector<T*, Alloc>&>(b);
}

namespace std
{
    template <class T, class Alloc = std::allocator<T*>>
    inline void swap(smartvector<T, Alloc> &x, smartvector<T, Alloc> &y)
    {
        x.swap(y);
    }
}


#endif
