/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef QCHARSTRING_H
#define QCHARSTRING_H

#include <QChar>
#include <QString>

enum class QCharKind;

size_t ushortlen(const ushort *str);
size_t qcharlen(const QChar *str);
int qcharcmp(const QChar *s1, const QChar *s2);
int qcharncmp(const QChar *s1, const QChar *s2, int len);

//// Returns the number of characters that match in s1 and s2 starting from the
//// first character. If maxlen is reached, it's returned. The ending null is
//// not counted.
//uint qcharmlen(const QChar *s1, const QChar *s2, int maxlen = -1);

// Copies src to dest. Dest must have space for qcharlen(src) + 1 characters
// and src must be null terminated.
void qcharcpy(QChar *dest, const QChar *src);

// Copies src to dest. Dest is created with new.
//void qcharccpy(QChar* &dest, const QChar *src);
// Copies length characters from src to dest, and appends a 0 at the end. Dest is created with new.
//void qcharnccpy(QChar* &dest, const QChar *src, int srclength);

// Copies len characters from src to dest, even if 0 is encountered. Dest must have enough space.
void qcharncpy(QChar *dest, const QChar *src, int len);
// Finds needle in hay and returns a pointer to its position or 0 if not found.
const QChar* qcharstr(const QChar *hay, const QChar *needle, int haylength = -1, int needlelength = -1);
// Returns whether the passed ch is considered to be a delimiter character, which is not used for searches.
QCharKind qcharisdelim(QChar ch);
// Returns whether the passed ch is a space character, for breaking up lines when displaying text.
QCharKind qcharisspace(QChar ch);
// Returns a pointer to the first character in str which matches ch. If ch is not found
// returns null. Only set ch to 0 if str is null terminated. If length is set to a positive
// number, only checks that number of characters in str.
const QChar* qcharchr(const QChar *str, QChar ch, int length = -1);

int wordcompare(const QChar *kanjia, const QChar *kanaa, const QChar *kanjib, const QChar *kanab);


class QCharString;
class QCharStringIterator;
class QCharStringConstIterator;

class QCharStringConstIterator
{
public:
    typedef QCharString str_type;
    typedef QChar*  value_type;
    typedef QChar** pointer;
    typedef qint32 difference_type;
    typedef QChar*& reference;
    typedef std::random_access_iterator_tag iterator_category;

    QCharStringConstIterator();
    QCharStringConstIterator(const QCharStringIterator &b);
    QCharStringConstIterator& operator=(const QCharStringIterator &b);

    QCharStringConstIterator& operator++();
    QCharStringConstIterator operator++(int);
    QCharStringConstIterator& operator--();
    QCharStringConstIterator operator--(int);
    QCharStringConstIterator operator+(difference_type n) const;
    QCharStringConstIterator operator-(difference_type n) const;
    QCharStringConstIterator& operator+=(difference_type n);
    QCharStringConstIterator& operator-=(difference_type n);
    difference_type operator-(const QCharStringConstIterator &b) const;
    bool operator==(const QCharStringConstIterator &b) const;
    bool operator!=(const QCharStringConstIterator &b) const;
    bool operator<(const QCharStringConstIterator &b) const;
    bool operator>(const QCharStringConstIterator &b) const;
    bool operator<=(const QCharStringConstIterator &b) const;
    bool operator>=(const QCharStringConstIterator &b) const;

    const QChar* operator[](difference_type n) const;
    const QChar* operator*() const;
    const QChar* operator->() const;

protected:
    const QCharString *owner;
    QChar *pos;

    QCharStringConstIterator(const QCharString *owner, QChar *pos);
private:

    friend class QCharString;
};

class QCharStringIterator : public QCharStringConstIterator
{
private:
    typedef QCharStringConstIterator    base;
public:
    typedef base::str_type str_type;
    typedef base::value_type   value_type;
    typedef base::pointer  pointer;
    typedef base::difference_type  difference_type;
    typedef base::reference    reference;
    typedef base::iterator_category    iterator_category;

    QCharStringIterator();
    QCharStringIterator(const QCharStringConstIterator &b);
    QCharStringIterator& operator=(const QCharStringConstIterator &b);

    QCharStringIterator& operator++();
    QCharStringIterator operator++(int);
    QCharStringIterator& operator--();
    QCharStringIterator operator--(int);
    QCharStringIterator operator+(difference_type n) const;
    QCharStringIterator operator-(difference_type n) const;
    QCharStringIterator& operator+=(difference_type n);
    QCharStringIterator& operator-=(difference_type n);

    QChar* operator[](difference_type n);
    QChar* operator*();
    QChar* operator->();
    const QChar* operator[](difference_type n) const;
    const QChar* operator*() const;
    const QChar* operator->() const;
private:
    QCharStringIterator(QCharString *owner, QChar *pos);

    friend class QCharString;
};

QCharStringIterator operator+(QCharStringIterator::difference_type n, const QCharStringIterator &b);
QCharStringConstIterator operator+(QCharStringConstIterator::difference_type n, const QCharStringConstIterator &b);


// Class for storing qstring like strings. These strings are meant to be faster compared
// to qstring and should take less memory. There is no shared pointer, and the only data stored
// is an array of QChars. (Which only holds a single ushort itself).
// The string cannot be copied, only replaced or moved, so it acts like a unique_ptr in this regard.
// Size is not saved, but rather qcharlen is called when it is required. (In debug mode
// the size is saved and checked at access for debug purposes.)
class QCharString
{
public:
    typedef QCharStringIterator iterator;
    typedef QCharStringConstIterator const_iterator;
    typedef std::reverse_iterator<QCharStringIterator>  reverse_iterator;
    typedef std::reverse_iterator<QCharStringConstIterator>  const_reverse_iterator;
    typedef QChar* value_type;
    typedef QChar*& reference;
    typedef const QChar*& const_reference;
    typedef QChar** pointer;
    typedef const QChar** const_pointer;
    typedef qint32 difference_type;
    typedef qint32 size_type;

    QCharString();
    QCharString(QCharString &&src);
    QCharString& operator=(QCharString &&src);
    QCharString(const QCharString &src);
    QCharString& operator=(const QCharString &src);
    ~QCharString();

    void swap(QCharString &src);

    iterator begin();
    const_iterator begin() const;
    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    iterator end();
    const_iterator end() const;
    reverse_iterator rend();
    const_reverse_iterator rend() const;
    const_iterator cbegin() const;
    const_reverse_iterator crbegin() const;
    const_iterator cend() const;
    const_reverse_iterator crend() const;

    // Fills the array with a copy of str to at most length characters. If length is -1, first
    // qcharlen() is called on str, so it must be null terminated in that case.
    void copy(const QChar *str, int length = -1);

    // Allocates data for length + 1 characters filled with space and the trailing zero. Use
    // the non constant data() function to access the allocated string. Does not copy old
    // contents.
    void setSize(size_type length);

    // Allocates data for length + 1 characters, copying the old contents up to the last
    // character in the old data that fits the length. If the new size is larger, the
    // remaining characters are filled with space and a trailing zero.
    void resize(size_type length);

    // Number of characters in the string data, not counting the null terminating character.
    // It is always re-computed when this function is called.
    size_type size() const;

    // Returns true if the data is not initialized or it contains an array with only the
    // terminating null character.
    bool empty() const;

    // Frees the data in the array setting it to nullptr.
    void clear();

    int compare(const QCharString &other) const;

    // Returns the pure data array. It can be null if the string is not initialized.
    QChar* data();
    // Returns the pure data array. It can be null if the string is not initialized.
    const QChar* data() const;

    // Returns the array of the n rightmost characters. If n is greater than the
    // string size, the whole string is returned.
    const QChar* rightData(size_type n) const;

    // Returns the n-th character in array. There is no error checking, so make sure the
    // passed index is valid.
    QChar& operator[](size_type n);
    // Returns the n-th character in array. There is no error checking, so make sure the
    // passed index is valid.
    const QChar& operator[](size_type n) const;
    // Returns true if the string and the other string results in 0 when compared with
    // qcharcmp().
    bool operator==(const QCharString &other) const;
    // Returns false if the string and the other string results in 0 when compared with
    // qcharcmp.
    bool operator!=(const QCharString &other) const;

    // Creates a QString from pos with at most len characters from the data of the
    // string array. The length counted from pos shouldn't go beyond the data, or the
    // behavior is undefined.
    QString toQString(size_type pos = 0, int len = -1) const;
    // Returns a lower case version of toQString.
    QString toLower() const;
    // Returns an upper case version of toQString.
    QString toUpper() const;

    // Creates a QString from the data in the string array without copying the string data.
    // The data must be available and unmodified while the returned QString exists.
    QString toQStringRaw() const;

    // Converts the stored string and returns the result. Set len to use at most len number of
    // characters from the string.
    QByteArray toUtf8(int len = - 1) const;


    // Looks for the index of at most `length` number of characters of str in the string array
    // and returns an index to the first character if found. Otherwise returns -1.
    int find(const QChar *str, int length = -1) const;

    // Returns the first index of ch in the string if found. Otherwise returns -1.
    int find(QChar ch) const;
private:
    QChar *arr;

    template <typename STR>
    friend bool operator==(const STR &a, const QCharString &b);
    template <typename STR>
    friend bool operator==(const QCharString &a, const STR &b);
    template <typename STR>
    friend bool operator!=(const STR &a, const QCharString &b);
    template <typename STR>
    friend bool operator!=(const QCharString &a, const STR &b);

#ifdef _DEBUG
    size_type siz; // Only used in error checking.
#endif
};

bool operator<(const QCharString &a, const QCharString &b);
bool operator>(const QCharString &a, const QCharString &b);

QDataStream& operator<<(QDataStream &stream, const QCharString &str);
QDataStream& operator>>(QDataStream &stream, QCharString &str);


QString& operator+=(QString &a, QCharString &str);
template <typename STR>
bool operator==(const STR &a, const QCharString &b);
template <typename STR>
bool operator==(const QCharString &a, const STR &b);
template <typename STR>
bool operator!=(const STR &a, const QCharString &b);
template <typename STR>
bool operator!=(const QCharString &a, const STR &b);

bool operator==(const QString &a, const QCharString &b);
bool operator!=(const QString &a, const QCharString &b);
bool operator==(const QCharString &a, const QString &b);
bool operator!=(const QCharString &a, const QString &b);
bool operator==(const QStringRef &a, const QCharString &b);
bool operator!=(const QStringRef &a, const QCharString &b);
bool operator==(const QCharString &a, const QStringRef &b);
bool operator!=(const QCharString &a, const QStringRef &b);

bool operator==(const QChar *a, const QCharString &b);
bool operator==(const QCharString &a, const QChar *b);
bool operator!=(const QChar *a, const QCharString &b);
bool operator!=(const QCharString &a, const QChar *b);


class QCharStringList;
class QCharStringListIterator;

class QCharStringListConstIterator
{
public:
    typedef QCharStringList str_type;
    typedef QCharString  value_type;
    typedef QCharString* pointer;
    typedef qint32 difference_type;
    typedef QCharString& reference;
    typedef std::random_access_iterator_tag iterator_category;

    QCharStringListConstIterator();
    QCharStringListConstIterator(const QCharStringListIterator &b);
    QCharStringListConstIterator& operator=(const QCharStringListIterator &b);

    QCharStringListConstIterator& operator++();
    QCharStringListConstIterator operator++(int);
    QCharStringListConstIterator& operator--();
    QCharStringListConstIterator operator--(int);
    QCharStringListConstIterator operator+(difference_type n) const;
    QCharStringListConstIterator operator-(difference_type n) const;
    QCharStringListConstIterator& operator+=(difference_type n);
    QCharStringListConstIterator& operator-=(difference_type n);
    difference_type operator-(const QCharStringListConstIterator &b) const;
    bool operator==(const QCharStringListConstIterator &b) const;
    bool operator!=(const QCharStringListConstIterator &b) const;
    bool operator<(const QCharStringListConstIterator &b) const;
    bool operator>(const QCharStringListConstIterator &b) const;
    bool operator<=(const QCharStringListConstIterator &b) const;
    bool operator>=(const QCharStringListConstIterator &b) const;

    const QCharString& operator*() const;
    const QCharString* operator->() const;

protected:
    const QCharStringList *owner;
    int pos;

    QCharStringListConstIterator(const QCharStringList *owner, int pos);
private:

    friend class QCharStringList;
};

class QCharStringListIterator : public QCharStringListConstIterator
{
private:
    typedef QCharStringListConstIterator    base;
public:
    typedef base::str_type str_type;
    typedef base::value_type   value_type;
    typedef base::pointer  pointer;
    typedef base::difference_type  difference_type;
    typedef base::reference    reference;
    typedef base::iterator_category    iterator_category;

    QCharStringListIterator();
    QCharStringListIterator(const QCharStringListConstIterator &b);
    QCharStringListIterator& operator=(const QCharStringListConstIterator &b);

    QCharStringListIterator& operator++();
    QCharStringListIterator operator++(int);
    QCharStringListIterator& operator--();
    QCharStringListIterator operator--(int);
    QCharStringListIterator operator+(difference_type n) const;
    QCharStringListIterator operator-(difference_type n) const;
    QCharStringListIterator& operator+=(difference_type n);
    QCharStringListIterator& operator-=(difference_type n);

    QCharString& operator*();
    QCharString* operator->();
    const QCharString& operator*() const;
    const QCharString* operator->() const;
private:
    QCharStringListIterator(QCharStringList *owner, int pos);

    friend class QCharStringList;
};


QCharStringListIterator operator+(QCharStringListIterator::difference_type n, const QCharStringListIterator &b);
QCharStringListConstIterator operator+(QCharStringListConstIterator::difference_type n, const QCharStringListConstIterator &b);


// Very simple class of holding QCharString objects that don't change after they have been
// added. Uses 32 bit sizes instead of being platform dependent, to follow Qt standards.
class QCharStringList
{
public:
    typedef QCharStringListIterator iterator;
    typedef QCharStringListConstIterator const_iterator;
    typedef std::reverse_iterator<QCharStringListIterator>  reverse_iterator;
    typedef std::reverse_iterator<QCharStringListConstIterator>  const_reverse_iterator;
    typedef QCharString value_type;
    typedef QCharString& reference;
    typedef const QCharString& const_reference;
    typedef QCharString* pointer;
    typedef const QCharString* const_pointer;
    typedef qint32 difference_type;
    typedef qint32 size_type;

    QCharStringList();
    QCharStringList(QCharStringList &&src);
    QCharStringList& operator=(QCharStringList &&src);
    QCharStringList(const QCharStringList &);
    QCharStringList& operator=(const QCharStringList &);
    ~QCharStringList();

    static size_type maxSize();

    bool operator==(const QCharStringList &) const;

    void swap(QCharStringList &src);

    iterator begin();
    const_iterator begin() const;
    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    iterator end();
    const_iterator end() const;
    reverse_iterator rend();
    const_reverse_iterator rend() const;
    const_iterator cbegin() const;
    const_reverse_iterator crbegin() const;
    const_iterator cend() const;
    const_reverse_iterator crend() const;

    // Reserves space for n QCharString objects if there's not enough
    // reserved. The size remains unchanged.
    void reserve(size_type n);
    // Copies str into a new QCharString in the list.
    void add(const QChar *str, int length = -1);
    // Copies str into a new QCharString in the list.
    void add(const QCharString &str, int length = -1);
    // Adds str to the list by moving the original.
    void add(QCharString &&str);
    // Adds the strings from src to the list. Set allowduplicate to false
    // if already existing strings shouldn't be added.
    void add(const QCharStringList &src, bool allowduplicate = true);
    // Returns whether a string is added to the list. Set exact to false
    // if str must match the start of an added string, and to true if it
    // must be an exact match.
    bool contains(const QChar *str, bool exact);
    // Sets the contents to match src.
    void copy(const QStringList &src);

    size_type size() const;
    bool empty() const;
    // Removes every element of the list and frees up the used space.
    void clear();
    QCharString& items(size_type ix);
    QCharString& operator[](size_type ix);
    const QCharString& items(size_type ix) const;
    const QCharString& operator[](size_type ix) const;
    // Returns a string constructed by joining every member of the list with
    // sep. If the list has a single item it's returned unchanged. Sep can
    // be an empty string.
    QString join(const QString &sep) const;
private:
    // Increases the space in the reserved array by reallocation.
    // Make sure it is never needed by using reserve(), because this
    // is not optimized and can make it slow to insert many items.
    void grow();

    size_type reserved;
    size_type used;
    QCharString *arr;
};

QDataStream& operator>>(QDataStream &stream, QCharStringList &list);
QDataStream& operator<<(QDataStream &stream, const QCharStringList &list);

namespace std
{
    void swap(QCharString &a, QCharString &b);
    void swap(QCharStringList &a, QCharStringList &b);
}

#endif
