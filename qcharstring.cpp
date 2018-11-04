/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QStringList>
#include <QDataStream>
#include "qcharstring.h"
#include "zkanjimain.h"

#include "checked_cast.h"

//-------------------------------------------------------------


size_t ushortlen(const ushort *str)
{
    const ushort *pos = str;
    while (*pos != 0)
        ++pos;
    return pos - str;
}

size_t qcharlen(const QChar *str)
{
    const QChar *pos = str;
    while (*pos != 0)
        ++pos;
    return pos - str;
}

int qcharcmp(const QChar *s1, const QChar *s2)
{
    if (s1 == s2)
        return 0;

    while (s1->unicode() == s2->unicode() && s1->unicode() != 0)
        ++s1, ++s2;

    return s1->unicode() - s2->unicode();
}

int qcharncmp(const QChar *s1, const QChar *s2, int len)
{
    if (s1 == s2 || len <= 0)
        return 0;

    while (len != 1 && s1->unicode() == s2->unicode() && s1->unicode() != 0)
        ++s1, ++s2, --len;

    return s1->unicode() - s2->unicode();
}

//uint qcharmlen(const QChar *s1, const QChar *s2, int maxlen)
//{
//    int pos = 0;
//    while (pos != maxlen && s1[pos] != 0 && s2[pos] != 0 && s1[pos] == s2[pos])
//        ++pos;
//    return pos;
//}

//void qcharccpy(QChar* &dest, const QChar *src)
//{
//    int len = qcharlen(src);
//    dest = new QChar[len + 1];
//    qcharncpy(dest, src, len + 1);
//}

//void qcharnccpy(QChar* &dest, const QChar *src, int length)
//{
//    if (length < 0)
//        length = 0;
//    dest = new QChar[length + 1];
//    qcharncpy(dest, src, length);
//    dest[length] = QChar(0);
//}

void qcharcpy(QChar *dest, const QChar *src)
{
    int len = tosigned(qcharlen(src));
    qcharncpy(dest, src, len + 1);
}

void qcharncpy(QChar *dest, const QChar *src, int len)
{
    if (len <= 0)
        return;
    memcpy(dest, src, len * sizeof(QChar));
}

const QChar* qcharstr(const QChar *hay, const QChar *needle, int haylength, int needlelength)
{
    int hlen = haylength == -1 ? tosigned(qcharlen(hay)) : haylength;
    int nlen = needlelength == -1 ? tosigned(qcharlen(needle)) : needlelength;
    if (nlen > hlen)
        return nullptr;

    while (hlen >= nlen)
    {
        if (!qcharncmp(hay, needle, nlen))
            return hay;
        ++hay;
        --hlen;
    }

    return nullptr;
}

QCharKind qcharisdelim(QChar ch)
{
    if (ch.isLetterOrNumber())
        return QCharKind::Normal;
    return QCharKind::Delimiter;
}

QCharKind qcharisspace(QChar ch)
{
    if (ch.unicode() == 0x0020)
        return QCharKind::Delimiter;
    return QCharKind::Normal;
}

const QChar* qcharchr(const QChar *str, QChar ch, int length)
{
    if (length == -1)
    {
        while (*str != ch)
        {
            if (*str == 0)
                return nullptr;
            ++str;
        }
    }
    else
    {
        while (length != 0 && *str != ch)
        {
            --length;
            ++str;
        }
        if (length == 0)
            return nullptr;
    }

    return str;
}

int wordcompare(const QChar *kanjia, const QChar *kanaa, const QChar *kanjib, const QChar *kanab)
{
    int r = qcharcmp(kanjia, kanjib);
    if (!r)
        return qcharcmp(kanaa, kanab);
    return r;
}


//-------------------------------------------------------------


QCharStringConstIterator operator+(QCharStringConstIterator::difference_type n, const QCharStringConstIterator &b)
{
    return b + n;
}

QCharStringIterator operator+(QCharStringIterator::difference_type n, const QCharStringIterator &b)
{
    return b + n;
}


//-------------------------------------------------------------


QCharStringConstIterator::QCharStringConstIterator() : owner(nullptr), pos(nullptr) {}
QCharStringConstIterator::QCharStringConstIterator(const QCharStringIterator &b) : owner(b.owner), pos(b.pos) {}
QCharStringConstIterator& QCharStringConstIterator::operator=(const QCharStringIterator &b) { owner = b.owner; pos = b.pos; return *this; }

QCharStringConstIterator& QCharStringConstIterator::operator++()
{
#ifdef _DEBUG
    if (pos == nullptr || *pos == QChar(0))
        throw "Check your access.";
#endif
    ++pos;
    return *this;
}

QCharStringConstIterator QCharStringConstIterator::operator++(int)
{
#ifdef _DEBUG
    if (pos == nullptr || *pos == QChar(0))
        throw "Check your access.";
#endif
    QCharStringConstIterator copy(*this);
    ++pos;
    return copy;
}


QCharStringConstIterator& QCharStringConstIterator::operator--()
{
#ifdef _DEBUG
    if (pos == nullptr || pos == owner->data())
        throw "Check your access.";
#endif
    --pos;
    return *this;
}

QCharStringConstIterator QCharStringConstIterator::operator--(int)
{
#ifdef _DEBUG
    if (pos == nullptr || pos == owner->data())
        throw "Check your access.";
#endif
    QCharStringConstIterator copy(*this);
    --pos;
    return copy;
}

QCharStringConstIterator QCharStringConstIterator::operator+(difference_type n) const
{
#ifdef _DEBUG
    if (pos == nullptr || pos - owner->data() + n < 0 || pos - owner->data() + n > tosigned(owner->size()))
        throw "Check your access.";
#endif
    return QCharStringConstIterator(owner, pos + n);
}

QCharStringConstIterator QCharStringConstIterator::operator-(difference_type n) const
{
#ifdef _DEBUG
    if (pos == nullptr || pos - owner->data() - n < 0 || pos - owner->data() - n > tosigned(owner->size()))
        throw "Check your access.";
#endif
    return QCharStringConstIterator(owner, pos - n);
}

QCharStringConstIterator& QCharStringConstIterator::operator+=(difference_type n)
{
#ifdef _DEBUG
    if (pos == nullptr || pos - owner->data() + n < 0 || pos - owner->data() + n > tosigned(owner->size()))
        throw "Check your access.";
#endif
    pos += n;
    return *this;
}

QCharStringConstIterator& QCharStringConstIterator::operator-=(difference_type n)
{
#ifdef _DEBUG
    if (pos == nullptr || pos - owner->data() - n < 0 || pos - owner->data() - n > tosigned(owner->size()))
        throw "Check your access.";
#endif
    pos -= n;
    return *this;
}

QCharStringConstIterator::difference_type QCharStringConstIterator::operator-(const QCharStringConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner /*|| pos < b.pos*/)
        throw "Check your access.";
#endif
    return pos - b.pos;
}

bool QCharStringConstIterator::operator==(const QCharStringConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner)
        throw "Check your access.";
#endif

    return pos == b.pos;
}

bool QCharStringConstIterator::operator!=(const QCharStringConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner)
        throw "Check your access.";
#endif
    return pos != b.pos;
}

bool QCharStringConstIterator::operator<(const QCharStringConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner)
        throw "Check your access.";
#endif
    return pos < b.pos;
}

bool QCharStringConstIterator::operator>(const QCharStringConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner)
        throw "Check your access.";
#endif
    return pos > b.pos;
}

bool QCharStringConstIterator::operator<=(const QCharStringConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner)
        throw "Check your access.";
#endif
    return pos <= b.pos;
}

bool QCharStringConstIterator::operator>=(const QCharStringConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner)
        throw "Check your access.";
#endif
    return pos >= b.pos;
}


const QChar* QCharStringConstIterator::operator[](difference_type n) const
{
#ifdef _DEBUG
    if (pos == nullptr || pos - owner->data() + n < 0 || pos - owner->data() + n > tosigned(owner->size()))
        throw "Check your access.";
#endif
    return pos + n;
}

const QChar* QCharStringConstIterator::operator*() const
{
#ifdef _DEBUG
    if (pos == nullptr || *pos == QChar(0))
        throw "Check your access.";
#endif
    return pos;
}

const QChar* QCharStringConstIterator::operator->() const
{
#ifdef _DEBUG
    if (pos == nullptr || *pos == QChar(0))
        throw "Check your access.";
#endif
    return pos;
}

QCharStringConstIterator::QCharStringConstIterator(const QCharString *owner, QChar *pos) : owner(owner), pos(pos) {}


//-------------------------------------------------------------


QCharStringIterator::QCharStringIterator() : base() {}
QCharStringIterator::QCharStringIterator(const QCharStringConstIterator &b) : base(b) {}

QCharStringIterator& QCharStringIterator::operator=(const QCharStringConstIterator &b)
{
    base::operator=(b);
    return *this;
}

QCharStringIterator& QCharStringIterator::operator++()
{
    base::operator++();
    return *this;
}

QCharStringIterator QCharStringIterator::operator++(int)
{
    QCharStringIterator copy(*this);
    base::operator++();
    return copy;
}

QCharStringIterator& QCharStringIterator::operator--()
{
    base::operator--();
    return *this;
}

QCharStringIterator QCharStringIterator::operator--(int)
{
    QCharStringIterator copy(*this);
    base::operator--();
    return copy;
}

QCharStringIterator QCharStringIterator::operator+(difference_type n) const
{
    return QCharStringIterator(base::operator+(n));
}

QCharStringIterator QCharStringIterator::operator-(difference_type n) const
{
    return QCharStringIterator(base::operator-(n));
}

QCharStringIterator& QCharStringIterator::operator+=(difference_type n)
{
    base::operator+=(n);
    return *this;
}

QCharStringIterator& QCharStringIterator::operator-=(difference_type n)
{
    base::operator-=(n);
    return *this;
}

QChar* QCharStringIterator::operator[](difference_type n)
{
#ifdef _DEBUG
    if (pos == nullptr || pos - owner->data() + n < 0 || pos - owner->data() + n > tosigned(owner->size()))
        throw "Check your access.";
#endif
    return pos + n;
}

QChar* QCharStringIterator::operator*()
{
#ifdef _DEBUG
    if (pos == nullptr || *pos == QChar(0))
        throw "Check your access.";
#endif
    return pos;
}

QChar* QCharStringIterator::operator->()
{
#ifdef _DEBUG
    if (pos == nullptr || *pos == QChar(0))
        throw "Check your access.";
#endif
    return pos;
}

const QChar* QCharStringIterator::operator[](difference_type n) const
{
#ifdef _DEBUG
    if (pos == nullptr || pos - owner->data() + n < 0 || pos - owner->data() + n > tosigned(owner->size()))
        throw "Check your access.";
#endif
    return pos + n;
}

const QChar* QCharStringIterator::operator*() const
{
#ifdef _DEBUG
    if (pos == nullptr || *pos == QChar(0))
        throw "Check your access.";
#endif
    return pos;
}

const QChar* QCharStringIterator::operator->() const
{
#ifdef _DEBUG
    if (pos == nullptr || *pos == QChar(0))
        throw "Check your access.";
#endif
    return pos;
}

QCharStringIterator::QCharStringIterator(QCharString *owner, QChar *pos) : base(owner, pos) {}


//-------------------------------------------------------------


QCharString::QCharString() : arr(nullptr)
#ifdef _DEBUG
    , siz(0)
#endif
{}

QCharString::QCharString(QCharString &&src) : arr(nullptr)
#ifdef _DEBUG
    , siz(0)
#endif
{
    std::swap(arr, src.arr);
#ifdef _DEBUG
    std::swap(siz, src.siz);
#endif
}

QCharString& QCharString::operator=(QCharString &&src)
{
    std::swap(arr, src.arr);
#ifdef _DEBUG
    std::swap(siz, src.siz);
#endif

    return *this;
}

QCharString::QCharString(const QCharString &src) : arr(nullptr)
#ifdef _DEBUG
    , siz(src.siz)
#endif
{
#ifndef _DEBUG
    size_type siz = src.size();
#endif

    if (siz == 0)
        return;
    arr = new QChar[siz + 1];
    memcpy(arr, src.arr, sizeof(QChar) * (siz + 1));
}

QCharString& QCharString::operator=(const QCharString &src)
{
    if (&src == this)
        return *this;

    delete[] arr;
    arr = nullptr;

#ifdef _DEBUG
    siz = src.size();
#else
    size_type siz = src.size();
#endif

    if (siz == 0)
        return *this;
    arr = new QChar[siz + 1];

    memcpy(arr, src.arr, sizeof(QChar) * (siz + 1));

    return *this;
}

QCharString::~QCharString()
{
    delete[] arr;
}

QCharString::iterator QCharString::begin()
{
    if (arr == nullptr)
        return iterator(this, nullptr);
    return iterator(this, arr);
}

QCharString::const_iterator QCharString::begin() const
{
    if (arr == nullptr)
        return const_iterator(this, nullptr);
    return const_iterator(this, arr);
}

QCharString::reverse_iterator QCharString::rbegin()
{
    if (arr == nullptr)
        return std::reverse_iterator<iterator>(iterator(this, nullptr));
    return std::reverse_iterator<iterator>(iterator(this, arr + size()));
}

QCharString::const_reverse_iterator QCharString::rbegin() const
{
    if (arr == nullptr)
        return std::reverse_iterator<const_iterator>(const_iterator(this, nullptr));
    return std::reverse_iterator<const_iterator>(const_iterator(this, arr + size()));
}

QCharString::iterator QCharString::end()
{
    if (arr == nullptr)
        return iterator(this, nullptr);
    return iterator(this, arr + size());
}

QCharString::const_iterator QCharString::end() const
{
    if (arr == nullptr)
        return const_iterator(this, nullptr);
    return const_iterator(this, arr + size());
}

QCharString::reverse_iterator QCharString::rend()
{
    if (arr == nullptr)
        return std::reverse_iterator<iterator>(iterator(this, nullptr));
    return std::reverse_iterator<iterator>(iterator(this, arr));
}

QCharString::const_reverse_iterator QCharString::rend() const
{
    if (arr == nullptr)
        return std::reverse_iterator<const_iterator>(const_iterator(this, nullptr));
    return std::reverse_iterator<const_iterator>(const_iterator(this, arr));
}

QCharString::const_iterator QCharString::cbegin() const
{
    return begin();
}

QCharString::const_reverse_iterator QCharString::crbegin() const
{
    return rbegin();
}

QCharString::const_iterator QCharString::cend() const
{
    return end();
}

QCharString::const_reverse_iterator QCharString::crend() const
{
    return rend();
}

void QCharString::copy(const QChar *str, int length)
{
    if (length == -1)
        length = tosigned(qcharlen(str));
    delete[] arr;

    arr = new QChar[length + 1];
    memcpy(arr, str, sizeof(QChar) * length);
    arr[length] = QChar(0);
#ifdef _DEBUG
    siz = length;
#endif
}

void QCharString::setSize(size_type length)
{
#ifdef _DEBUG
    siz = length;
#endif
    delete[] arr;
    if (length <= 0)
    {
        arr = nullptr;
        return;
    }
    arr = new QChar[length + 1];
    arr[length] = QChar(0);
    for (size_type ix = 0; ix != length; ++ix)
        arr[ix] = QChar(' ');
}

void QCharString::resize(size_type length)
{
#ifdef _DEBUG
    siz = length;
#endif

    if (length <= 0)
    {
        delete[] arr;
        arr = nullptr;
        return;
    }

    size_type oldsize = size();

    if (oldsize == length)
        return;

    QChar *tmp = arr;
    arr = new QChar[length + 1];
    arr[length] = QChar(0);

    if (oldsize != 0)
        memcpy(arr, tmp, sizeof(QChar) * std::min(length, oldsize));
    delete[] tmp;

    if (oldsize < length)
        for (size_type ix = oldsize; ix != length; ++ix)
            arr[ix] = QChar(' ');
}

QCharString::size_type QCharString::size() const
{
    // When debugging, siz holds the length of the string, but it is only used for error
    // checking. It cannot be returned here as sometimes the size is changed AFTER siz is set.
    return arr == nullptr ? 0 : tosignedness<size_type>(qcharlen(arr));
}

bool QCharString::empty() const
{
    return arr == nullptr || arr[0].unicode() == 0;
}

void QCharString::clear()
{
    delete[] arr;
    arr = nullptr;
#ifdef _DEBUG
    siz = 0;
#endif
}

int QCharString::compare(const QCharString &other) const
{
    if (arr == nullptr || other.arr == nullptr)
    {
        if (arr == other.arr)
            return 0;
        return arr == nullptr ? -1 : 1;
    }
    return qcharcmp(arr, other.arr);
}

QChar* QCharString::data()
{
    return arr;
}

const QChar* QCharString::data() const
{
    return arr;
}

const QChar* QCharString::rightData(size_type n) const
{
    return arr + std::max(0, tosigned(size() - n));
}

QChar& QCharString::operator[](size_type n)
{
#ifdef _DEBUG
    if (n >= siz || n < 0)
        throw "invalid call";
#endif
    return arr[n];
}

const QChar& QCharString::operator[](size_type n) const
{
#ifdef _DEBUG
    if (n >= siz || n < 0)
        throw "invalid call";
#endif
    return arr[n];
}

bool QCharString::operator==(const QCharString &other) const
{
    if ((arr == nullptr || arr[0] == 0) != (other.arr == nullptr || other.arr[0] == 0))
        return false;
    return arr == other.arr || qcharcmp(arr, other.arr) == 0;
}

bool QCharString::operator!=(const QCharString &other) const
{
    if ((arr == nullptr || arr[0] == 0) != (other.arr == nullptr || other.arr[0] == 0))
        return true;
    return arr != other.arr && qcharcmp(arr, other.arr) != 0;
}

QString QCharString::toQString(size_type pos, int len) const
{
    if (arr == nullptr)
        return QString();
    return QString(arr + pos, (len == -1) ? (size() - pos) : len);
}

QString QCharString::toLower() const
{
    if (arr == nullptr)
        return QString();
    return QString::fromRawData(arr, size()).toLower();
}

QString QCharString::toUpper() const
{
    if (arr == nullptr)
        return QString();
    return QString::fromRawData(arr, size()).toUpper();
}

QString QCharString::toQStringRaw() const
{
    if (arr == nullptr)
        return QString();
    return QString::fromRawData(arr, /*(len == -1) ?*/ size() /*: len*/);
}

QByteArray QCharString::toUtf8(int len) const
{
    if (arr == nullptr)
        return QByteArray();
    return QString::fromRawData(arr, len == -1 ? size() : len).toUtf8();
}

int QCharString::find(const QChar *str, int length) const
{
    if (arr == nullptr || str == nullptr)
        return -1;

    if (length == -1)
        length = tosigned(qcharlen(str));

    const QChar *pos = qcharstr(arr, str, -1, length);
    if (pos == nullptr)
        return -1;
    return pos - arr;
}

int QCharString::find(QChar ch) const
{
    if (arr == nullptr)
        return -1;

    const QChar *pos = qcharchr(arr, ch);
    if (pos == nullptr)
        return -1;
    return pos - arr;
}

bool operator<(const QCharString &a, const QCharString &b)
{
    if (a.data() == nullptr || b.data() == nullptr)
        return b.data() != nullptr;
    return qcharcmp(a.data(), b.data()) < 0;
}

bool operator>(const QCharString &a, const QCharString &b)
{
    if (a.data() == nullptr || b.data() == nullptr)
        return a.data() != nullptr;
    return qcharcmp(a.data(), b.data()) > 0;
}


// Friend operators
template <typename STR>
bool operator==(const STR &a, const QCharString &b)
{
    QCharString::size_type len = b.size();
    if (a.size() != tosignedness<decltype(a.size())>(len))
        return false;
    return qcharncmp(a.constData(), b.arr, len) == 0;
}

template <typename STR>
bool operator==(const QCharString &a, const STR &b)
{
    QCharString::size_type len = a.size();
    if (b.size() != tosignedness<decltype(b.size())>(len))
        return false;
    return qcharncmp(b.constData(), a.arr, len) == 0;
}

template <typename STR>
bool operator!=(const STR &a, const QCharString &b)
{
    QCharString::size_type len = b.size();
    if (a.size() != tosignedness<decltype(a.size())>(len))
        return true;
    return qcharncmp(a.constData(), b.arr, len) != 0;
}

template <typename STR>
bool operator!=(const QCharString &a, const STR &b)
{
    QCharString::size_type len = a.size();
    if (b.size() != tosignedness<decltype(b.size())>(len))
        return true;
    return qcharncmp(b.constData(), a.arr, len) != 0;
}

bool operator==(const QString &a, const QCharString &b)
{
    return operator==<QString>(a, b);
}

bool operator!=(const QString &a, const QCharString &b)
{
    return operator!=<QString>(a, b);
}

bool operator==(const QCharString &a, const QString &b)
{
    return operator==<QString>(a, b);
}

bool operator!=(const QCharString &a, const QString &b)
{
    return operator!=<QString>(a, b);
}

bool operator==(const QStringRef &a, const QCharString &b)
{
    return operator==<QStringRef>(a, b);
}

bool operator!=(const QStringRef &a, const QCharString &b)
{
    return operator!=<QStringRef>(a, b);
}

bool operator==(const QCharString &a, const QStringRef &b)
{
    return operator==<QStringRef>(a, b);
}

bool operator!=(const QCharString &a, const QStringRef &b)
{
    return operator!=<QStringRef>(a, b);
}

bool operator==(const QChar *a, const QCharString &b)
{
    return qcharcmp(a, b.data()) == 0;
}

bool operator==(const QCharString &a, const QChar *b)
{
    return qcharcmp(a.data(), b) == 0;
}

bool operator!=(const QChar *a, const QCharString &b)
{
    return qcharcmp(a, b.data()) != 0;
}

bool operator!=(const QCharString &a, const QChar *b)
{
    return qcharcmp(a.data(), b) != 0;
}


//-------------------------------------------------------------


QString& operator+=(QString &a, QCharString &str)
{
    a += str.toQString();
    return a;
}

void QCharString::swap(QCharString &src)
{
    std::swap(arr, src.arr);

#ifdef _DEBUG
    std::swap(siz, src.siz);
#endif
}

namespace std
{
    void swap(QCharString &a, QCharString &b)
    {
        a.swap(b);
    }

    void swap(QCharStringList &a, QCharStringList &b)
    {
        a.swap(b);
    }
}


//-------------------------------------------------------------


QCharStringListConstIterator operator+(QCharStringListConstIterator::difference_type n, const QCharStringListConstIterator &b)
{
    return b + n;
}

QCharStringListIterator operator+(QCharStringListIterator::difference_type n, const QCharStringListIterator &b)
{
    return b + n;
}


//-------------------------------------------------------------


QCharStringListConstIterator::QCharStringListConstIterator() : owner(nullptr), pos(-1) {}
QCharStringListConstIterator::QCharStringListConstIterator(const QCharStringListIterator &b) : owner(b.owner), pos(b.pos) {}
QCharStringListConstIterator::QCharStringListConstIterator(const QCharStringList *owner, int pos) : owner(owner), pos(pos) {}
QCharStringListConstIterator& QCharStringListConstIterator::operator=(const QCharStringListIterator &b) { owner = b.owner; pos = b.pos; return *this; }

QCharStringListConstIterator& QCharStringListConstIterator::operator++()
{
#ifdef _DEBUG
    if (pos == -1 || owner == nullptr || pos >= tosigned(owner->size()))
        throw "Check your access.";
#endif
    ++pos;
    if (pos == tosigned(owner->size()))
        pos = -1;
    return *this;
}

QCharStringListConstIterator QCharStringListConstIterator::operator++(int)
{
#ifdef _DEBUG
    if (pos == -1 || owner == nullptr || pos >= tosigned(owner->size()))
        throw "Check your access.";
#endif
    QCharStringListConstIterator copy(*this);
    ++pos;
    if (pos == tosigned(owner->size()))
        pos = -1;
    return copy;
}


QCharStringListConstIterator& QCharStringListConstIterator::operator--()
{
#ifdef _DEBUG
    if (pos == 0 || owner == nullptr || pos >= tosigned(owner->size()) || owner->size() == 0)
        throw "Check your access.";
#endif
    if (pos == -1)
        pos = owner->size();
    --pos;
    return *this;
}

QCharStringListConstIterator QCharStringListConstIterator::operator--(int)
{
#ifdef _DEBUG
    if (pos == 0 || owner == nullptr || pos >= tosigned(owner->size()) || owner->size() == 0)
        throw "Check your access.";
#endif
    QCharStringListConstIterator copy(*this);
    if (pos == -1)
        pos = owner->size();
    --pos;
    return copy;
}

QCharStringListConstIterator QCharStringListConstIterator::operator+(difference_type n) const
{
    int val = pos == -1 ? owner->size() : pos;
#ifdef _DEBUG
    if ((pos == -1 && n > 0) || owner == nullptr || val + n > tosigned(owner->size()) || val + n < 0)
        throw "Check your access.";
#endif
    return QCharStringListConstIterator(owner, val + n == tosigned(owner->size()) ? -1 : val + n);
}

QCharStringListConstIterator QCharStringListConstIterator::operator-(difference_type n) const
{
    int val = pos == -1 ? owner->size() : pos;
#ifdef _DEBUG
    if ((pos == -1 && n < 0) || owner == nullptr || val - n > tosigned(owner->size()) || val - n < 0)
        throw "Check your access.";
#endif
    return QCharStringListConstIterator(owner, val - n == tosigned(owner->size()) ? -1 : val - n);
}

QCharStringListConstIterator& QCharStringListConstIterator::operator+=(difference_type n)
{
    int val = pos == -1 ? owner->size() : pos;
#ifdef _DEBUG
    if ((pos == -1 && n > 0) || owner == nullptr || val + n > tosigned(owner->size()) || val + n < 0)
        throw "Check your access.";
#endif
    pos = val + n == tosigned(owner->size()) ? -1 : val + n;
    return *this;
}

QCharStringListConstIterator& QCharStringListConstIterator::operator-=(difference_type n)
{
    int val = pos == -1 ? owner->size() : pos;
#ifdef _DEBUG
    if ((pos == -1 && n < 0) || owner == nullptr || val - n > tosigned(owner->size()) || val - n < 0)
        throw "Check your access.";
#endif
    pos = val - n == tosigned(owner->size()) ? -1 : val - n;
    return *this;
}

 QCharStringListConstIterator::difference_type QCharStringListConstIterator::operator-(const QCharStringListConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner || pos >= tosigned(owner->size()) || b.pos >= tosigned(b.owner->size()))
        throw "Check your access.";
#endif
    int val = pos == -1 ? tosigned(owner->size()) : pos;
    int bval = b.pos == -1 ? tosigned(b.owner->size()) : b.pos;
    return val - bval;
}

bool QCharStringListConstIterator::operator==(const QCharStringListConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner || pos >= tosigned(owner->size()) || b.pos >= tosigned(b.owner->size()))
        throw "Check your access.";
#endif

    return pos == b.pos;
}

bool QCharStringListConstIterator::operator!=(const QCharStringListConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner || pos >= tosigned(owner->size()) || b.pos >= tosigned(b.owner->size()))
        throw "Check your access.";
#endif
    return pos != b.pos;
}

bool QCharStringListConstIterator::operator<(const QCharStringListConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner || pos >= tosigned(owner->size()) || b.pos >= tosigned(b.owner->size()))
        throw "Check your access.";
#endif
    return pos != b.pos && (b.pos == -1 || (pos != -1 && pos < b.pos));
}

bool QCharStringListConstIterator::operator>(const QCharStringListConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner || pos >= tosigned(owner->size()) || b.pos >= tosigned(b.owner->size()))
        throw "Check your access.";
#endif
    return pos != b.pos && (pos == -1 || (b.pos != -1 && pos > b.pos));
}

bool QCharStringListConstIterator::operator<=(const QCharStringListConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner || pos >= tosigned(owner->size()) || b.pos >= tosigned(b.owner->size()))
        throw "Check your access.";
#endif
    return pos == b.pos || b.pos == -1 || (pos != -1 && pos < b.pos);
}

bool QCharStringListConstIterator::operator>=(const QCharStringListConstIterator &b) const
{
#ifdef _DEBUG
    if (owner == nullptr || owner != b.owner || pos >= tosigned(owner->size()) || b.pos >= tosigned(b.owner->size()))
        throw "Check your access.";
#endif
    return pos == b.pos || pos == -1 || (b.pos != -1 && pos > b.pos);
}


const QCharString& QCharStringListConstIterator::operator*() const
{
#ifdef _DEBUG
    if (pos == -1 || owner == nullptr || pos >= tosigned(owner->size()))
        throw "Check your access.";
#endif
    return owner->items(pos);
}

const QCharString* QCharStringListConstIterator::operator->() const
{
#ifdef _DEBUG
    if (pos == -1 || owner == nullptr || pos >= tosigned(owner->size()))
        throw "Check your access.";
#endif
    return &owner->items(pos);
}


//-------------------------------------------------------------


QCharStringListIterator::QCharStringListIterator() : base() {}
QCharStringListIterator::QCharStringListIterator(const QCharStringListConstIterator &b) : base(b) {}
QCharStringListIterator::QCharStringListIterator(QCharStringList *owner, int pos) : base(owner, pos) {}

QCharStringListIterator& QCharStringListIterator::operator=(const QCharStringListConstIterator &b)
{
    base::operator=(b);
    return *this;
}

QCharStringListIterator& QCharStringListIterator::operator++()
{
    base::operator++();
    return *this;
}

QCharStringListIterator QCharStringListIterator::operator++(int)
{
    QCharStringListIterator copy(*this);
    base::operator++();
    return copy;
}

QCharStringListIterator& QCharStringListIterator::operator--()
{
    base::operator--();
    return *this;
}

QCharStringListIterator QCharStringListIterator::operator--(int)
{
    QCharStringListIterator copy(*this);
    base::operator--();
    return copy;
}

QCharStringListIterator QCharStringListIterator::operator+(difference_type n) const
{
    return QCharStringListIterator(base::operator+(n));
}

QCharStringListIterator QCharStringListIterator::operator-(difference_type n) const
{
    return QCharStringListIterator(base::operator-(n));
}

QCharStringListIterator& QCharStringListIterator::operator+=(difference_type n)
{
    base::operator+=(n);
    return *this;
}

QCharStringListIterator& QCharStringListIterator::operator-=(difference_type n)
{
    base::operator-=(n);
    return *this;
}

QCharString& QCharStringListIterator::operator*()
{
#ifdef _DEBUG
    if (pos == -1 || owner == nullptr || pos >= tosigned(owner->size()))
        throw "Check your access.";
#endif
    return const_cast<QCharStringList*>(owner)->items(pos);
}

QCharString* QCharStringListIterator::operator->()
{
#ifdef _DEBUG
    if (pos == -1 || owner == nullptr || pos >= tosigned(owner->size()))
        throw "Check your access.";
#endif
    return &const_cast<QCharStringList*>(owner)->items(pos);
}

const QCharString&  QCharStringListIterator::operator*() const
{
#ifdef _DEBUG
    if (pos == -1 || owner == nullptr || pos >= tosigned(owner->size()))
        throw "Check your access.";
#endif
    return owner->items(pos);
}

const QCharString* QCharStringListIterator::operator->() const
{
#ifdef _DEBUG
    if (pos == -1 || owner == nullptr || pos >= tosigned(owner->size()))
        throw "Check your access.";
#endif
    return &owner->items(pos);
}


//-------------------------------------------------------------


QCharStringList::QCharStringList() : reserved(0), used(0), arr(nullptr)
{
    ;
}

QCharStringList::QCharStringList(QCharStringList &&src) : reserved(0), used(0), arr(nullptr)
{
    std::swap(reserved, src.reserved);
    std::swap(used, src.used);
    std::swap(arr, src.arr);
}

QCharStringList& QCharStringList::operator=(QCharStringList &&src)
{
    std::swap(reserved, src.reserved);
    std::swap(used, src.used);
    std::swap(arr, src.arr);
    return *this;
}

QCharStringList::QCharStringList(const QCharStringList &src) : reserved(src.reserved), used(src.used), arr(nullptr)
{
    if (reserved == 0)
        return;

    arr = new QCharString[reserved];
    for (size_type ix = 0; ix != used; ++ix)
        arr[ix] = src.arr[ix];
}

QCharStringList& QCharStringList::operator=(const QCharStringList &src)
{
    used = src.used;
    if (reserved != src.reserved)
    {
        delete arr;
        reserved = src.reserved;
        if (reserved == 0)
        {
            arr = nullptr;
            return *this;
        }
        arr = new QCharString[reserved];
    }

    for (size_type ix = 0; ix != used; ++ix)
        arr[ix] = src.arr[ix];

    return *this;
}

QCharStringList::~QCharStringList()
{
    delete[] arr;
}

QCharStringList::size_type QCharStringList::maxSize()
{
    return std::numeric_limits<size_type>::max() - 1;
}

bool QCharStringList::operator==(const QCharStringList &b) const
{
    if (used != b.used)
        return false;
    for (size_type ix = 0; ix != used; ++ix)
        if (arr[ix] != b.arr[ix])
            return false;
    return true;
}

void QCharStringList::swap(QCharStringList &src)
{
    std::swap(reserved, src.reserved);
    std::swap(used, src.used);
    std::swap(arr, src.arr);

}

QCharStringList::iterator QCharStringList::begin()
{
    if (arr == nullptr)
        return iterator(this, -1);
    return iterator(this, 0);
}

QCharStringList::const_iterator QCharStringList::begin() const
{
    if (arr == nullptr)
        return const_iterator(this, -1);
    return const_iterator(this, 0);
}

QCharStringList::reverse_iterator QCharStringList::rbegin()
{
    //if (arr == nullptr)
    return std::reverse_iterator<iterator>(iterator(this, -1));
    //return std::reverse_iterator<iterator>(iterator(this, size()));
}

QCharStringList::const_reverse_iterator QCharStringList::rbegin() const
{
    //if (arr == nullptr)
    return std::reverse_iterator<const_iterator>(const_iterator(this, -1));
    //return std::reverse_iterator<const_iterator>(const_iterator(this, arr + size()));
}

QCharStringList::iterator QCharStringList::end()
{
    //if (arr == nullptr)
    return iterator(this, -1);
    //return iterator(this, arr + size());
}

QCharStringList::const_iterator QCharStringList::end() const
{
    //if (arr == nullptr)
    return const_iterator(this, -1);
    //return const_iterator(this, arr + size());
}

QCharStringList::reverse_iterator QCharStringList::rend()
{
    if (arr == nullptr)
        return std::reverse_iterator<iterator>(iterator(this, -1));
    return std::reverse_iterator<iterator>(iterator(this, 0));
}

QCharStringList::const_reverse_iterator QCharStringList::rend() const
{
    if (arr == nullptr)
        return std::reverse_iterator<const_iterator>(const_iterator(this, -1));
    return std::reverse_iterator<const_iterator>(const_iterator(this, 0));
}

QCharStringList::const_iterator QCharStringList::cbegin() const
{
    return begin();
}

QCharStringList::const_reverse_iterator QCharStringList::crbegin() const
{
    return rbegin();
}

QCharStringList::const_iterator QCharStringList::cend() const
{
    return end();
}

QCharStringList::const_reverse_iterator QCharStringList::crend() const
{
    return rend();
}

void QCharStringList::reserve(size_type n)
{
    if (reserved >= n)
        return;

    QCharString *tmp = arr;
    arr = new QCharString[n];
    for (size_type ix = 0; ix < used; ++ix)
        arr[ix] = std::move(tmp[ix]);

    delete[] tmp;
    reserved = n;
}

void QCharStringList::add(const QChar *str, int length)
{
    if (length == -1)
        length = tosigned(qcharlen(str));

    if (used == reserved)
        grow();

    arr[used].copy(str, length);
    ++used;
}

void QCharStringList::add(const QCharString &str, int length)
{
    add(str.data(), length);
}

void QCharStringList::add(QCharString &&str)
{
    if (used == reserved)
        grow();
    arr[used] = std::move(str);
    ++used;
}

void QCharStringList::add(const QCharStringList &src, bool dup)
{
    if (dup)
        reserve(used + src.used);

    size_type iy;
    for (size_type ix = 0; ix != src.used; ++ix)
    {
        // Check for duplicates and skip if found.
        if (!dup)
        {
            for (iy = 0; iy != used; ++iy)
            {
                if (qcharcmp(src.items(ix).data(), items(iy).data()) == 0)
                    break;
            }
            if (iy != used)
                continue;
        }

        add(src[ix]);
    }
}

bool QCharStringList::contains(const QChar *str, bool exact)
{
    size_type strsiz = tosignedness<size_type>(qcharlen(str));
    for (size_type ix = 0; ix != used; ++ix)
    {
        size_type siz = arr[ix].size();
        if (exact && siz != strsiz)
            continue;
        if (qcharncmp(str, arr[ix].data(), strsiz) == 0)
            return true;
    }
    return false;
}

void QCharStringList::copy(const QStringList &src)
{
    reserved = 0;
    used = 0;
    delete[] arr;
    arr = nullptr;

    if (src.isEmpty())
        return;

    reserve(src.size());
    used = src.size();
    for (size_type ix = 0; ix != used; ++ix)
        arr[ix].copy(src.at(ix).constData());
}

QCharStringList::size_type QCharStringList::size() const
{
    return used;
}

bool QCharStringList::empty() const
{
    return used == 0;
}

void QCharStringList::clear()
{
    delete[] arr;
    arr = nullptr;
    used = 0;
    reserved = 0;
}

QCharString& QCharStringList::items(size_type ix)
{
    assert(ix >= 0 && ix < used);

    return arr[ix];
}

QCharString& QCharStringList::operator[](size_type ix)
{
    return items(ix);
}

const QCharString& QCharStringList::items(size_type ix) const
{
    assert(ix >= 0 && ix < used);

    return arr[ix];
}

const QCharString& QCharStringList::operator[](size_type ix) const
{
    return items(ix);
}

QString QCharStringList::join(const QString &sep) const
{
    if (used == 0)
        return QString();
    if (used == 1)
        return arr[0].toQStringRaw();

    std::vector<size_type> sizes;
    sizes.reserve(used);

    int seplen = sep.size();
    int slen = seplen * (used - 1);
    for (int ix = 0, siz = tosigned(used); ix != siz; ++ix)
    {
        int asiz = arr[ix].size();
        slen += asiz;
        sizes.push_back(asiz);
    }

    size_type* sizesdata = sizes.data();
    QCharString str;
    str.setSize(slen);
    QChar *data = str.data();
    const QChar *sepdata = sep.constData();
    for (size_type ix = 0; ix != used; ++ix)
    {
        memcpy(data, arr[ix].data(), sizeof(QChar) * sizesdata[ix]);
        data += sizesdata[ix];
        if (ix != used - 1)
        {
            memcpy(data, sepdata, sizeof(QChar) * seplen);
            data += seplen;
        }
    }

    return str.toQString();
}

void QCharStringList::grow()
{
    size_type growsize = 16;

    if (reserved > 1024)
        growsize = 128;
    else if (reserved > 256)
        growsize = 64;
    else if (reserved > 128)
        growsize = 32;

    QCharString *tmp = arr;
    arr = new QCharString[reserved + growsize];
    for (size_type ix = 0; ix < used; ++ix)
        arr[ix] = std::move(tmp[ix]);

    delete[] tmp;
    reserved += growsize;
}


//-------------------------------------------------------------


QDataStream& operator>>(QDataStream &stream, QCharStringList &list)
{
    list.clear();
    qint32 cnt;
    stream >> cnt;

    if (cnt < 0 || cnt > QCharStringList::maxSize())
        throw ZException("Invalid stream size (QCharStringList)");

    list.reserve(cnt);

    for (qint32 ix = 0; ix != cnt; ++ix)
    {
        QCharString str;
        stream >> make_zstr(str, ZStrFormat::Int);
        list.add(std::move(str));
    }

    return stream;
}

QDataStream& operator<<(QDataStream &stream, const QCharStringList &list)
{
    qint32 cnt = list.size();
    stream << cnt;
    for (qint32 ix = 0; ix != cnt; ++ix)
        stream << make_zstr(list[ix], ZStrFormat::Int);

    return stream;
}


//-------------------------------------------------------------
