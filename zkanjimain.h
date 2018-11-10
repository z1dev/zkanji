/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZKANJIMAIN_H
#define ZKANJIMAIN_H

//#include <QGlobal>
#include <QChar>
#include <QApplication>
#include <QRect>
#include <QDateTime>
#include <QDataStream>

#include <functional>
#include <random>
#include <exception>

#include "qcharstring.h"
#include "smartvector.h"

#include "checked_cast.h"


namespace Temporary
{
    // Compatibility code with zkanji prior to 2015, for reading and writing the old data format.
    // We can't expect systems apart from Windows to have a header for SYSTEMTIME, so we define our own.
    typedef struct _SYSTEMTIME {
        quint16 wYear;
        quint16 wMonth;
        quint16 wDayOfWeek;
        quint16 wDay;
        quint16 wHour;
        quint16 wMinute;
        quint16 wSecond;
        quint16 wMilliseconds;
    } SYSTEMTIME, *PSYSTEMTIME;

    QDataStream& operator>>(QDataStream& stream, SYSTEMTIME &st);

    // Reads old gaplist to std map.
    void LoadGapList(QDataStream &stream, std::map<int, QCharString> &dest);
}

template<typename T>
T clamp(T val, T minval, T maxval)
{
    if (val > maxval)
        return maxval;
    if (val < minval)
        return minval;
    return val;
}

#define KANJI(t) (0x4e00 <= int(t) && int(t) <= 0x9faf && ZKanji::validkanji[int(t) - 0x4e00])
#define UNICODE_J(t) ( (int(t) >= 0x3000 && int(t) <= 0x30ff && ZKanji::validcode[int(t) - 0x3000]) || \
                       (int(t) >= 0xff01 && int(t) <= 0xffee && ZKanji::validcode[int(t) - 0xff01 + ZKanji::validcodelowerhalf + 1]) )


#define SHIFT_JIS   932
#define KATAKANA(t) ((ushort)0x30A1 <= t && t <= (ushort)0x30F6)
#define HIRAGANA(t) ((ushort)0x3041 <= t && t <= (ushort)0x3093)
#define KANA(t)     (KATAKANA(t) || HIRAGANA(t))

#define MINITSU         ((ushort)0x3063)
#define HIRATSU         ((ushort)0x3064)
#define MINITSUKATA     ((ushort)0x30C3)
#define MINIKE          ((ushort)0x30F6)
#define MINIKA          ((ushort)0x30F5)
#define NVOWEL          ((ushort)0x3093)
#define KDASH           ((ushort)0x30fc)
#define KDASHB          ((ushort)0xff70)
#define KURIKAESHI      ((ushort)0x3005)
#define KURIKAESHI2     ((ushort)0x3006)
#define GANAKAESHI1     ((ushort)0x309D)
#define GANAKAESHI2     ((ushort)0x309E)
#define MIDDLEDOT       ((ushort)0x30FB)
#define KANAKAESHI1     ((ushort)0x30FD)
#define KANAKAESHI2     ((ushort)0x30FE)
#define ISNVOWEL(x)     ((x) == (ushort)0x3093 || (x) == (ushort)0x30f3)
#define DASH(t)         ((t) == KDASH || (t) == KDASHB)
#define MIDDOT(t)       ((t) == MIDDLEDOT)

#define JAPAN(t)        (KANJI(t) || UNICODE_J(t))
#define VALIDKANA(t)    (KANA(t) || DASH(t))
#define VALIDCODE(t)    (UNICODE_J(t) && !VALIDKANA(t))

// Structure that increments qt_ntfs_permission_lookup at construction and decrements it at
// destruction. Create locally in any function that needs to check file permissions using
// QFileInfo functions on Windows. The class does nothing on other OSes.
struct NTFSPermissionGuard
{
    NTFSPermissionGuard(const NTFSPermissionGuard&) = delete;
    NTFSPermissionGuard(NTFSPermissionGuard&&) = delete;
    NTFSPermissionGuard& operator=(const NTFSPermissionGuard&) = delete;
    NTFSPermissionGuard& operator=(NTFSPermissionGuard&&) = delete;

    NTFSPermissionGuard();
    ~NTFSPermissionGuard();
};

// Error value returned from functions. When an error value is constructed, the static
// Error::last() will return the latest constructed value.
class Error
{
public:
    // The latest error set. Make sure to catch the error in time before it's overwritten.
    static Error last();

    // Error types:
    // NoError - no error occured. If code is nonzero it could mean anything apart from this.
    // General - A general error with a possible error code. Set by default if only an error
    //           code is specified in the constructor.
    // Access  - File access error, when a file couldn't be modified or removed.
    // Write   - Error during a file write. When this code is returned, the file is
    //           compromised.
    enum Types { NoError, General, Access, Write };

    Error();
    Error(int code, QString msg = QString());
    Error(Types type, int code = 0, QString msg = QString());
    Error(const Error &src);
    Error& operator=(const Error &src);

    // Converts the error to a user readable format, which can be shown in a message box.
    // The string lists the error type, code and message. This is not the same as the explicit
    // string conversion operator below, which just returns the message string passed in the
    // constructor.
    // When type is NoError, returns a generic no error message independent of error code or
    // message.
    QString toString() const;

    // Returns a user readable string representing the error type.
    QString typeMessage() const;
    // Returns a user readable string representing the error code.
    QString codeMessage() const;

    // Returns true on no error and false on error, so error checking within an if(someFunc())
    // is read naturally. When type is NoError, returns true even if code is non-zero or msg
    // is not empty.
    operator bool() const;
    explicit operator Types() const;
    // Returns the error code.
    explicit operator int() const;
    // Returns the error message.
    explicit operator QString() const;
private:
    static Error lasterror;

    // The error type. Any value apart from NoError specifies an error and will make this
    // object convert to false when converted to bool.
    Types type = NoError;
    // The error code set as a number. This value depends on the place it's set, which is
    // usually the current progress of an operation.
    int code = 0;
    // The error message, which is only used to give information to the user if set. Doesn't
    // determine whether the Error specifies an actual error or not. Only check type and code
    // for that.
    QString msg;
};


class ZException : public std::exception
{
public:
    ZException(const char * const &msg);
    ZException(const ZException &other);
    ~ZException();
    ZException operator=(const ZException &other);

    virtual const char* what() const
#if !defined(_MSC_VER) || _MSC_VER >= 1900
        noexcept
#endif
        override;
private:
    void copyMsg(const char * const &m);

    char *msg;

    typedef std::exception  base;
};


// QCharTokenizer.
//
// Like strtok, finds tokens between delimiters, but doesn't change the
// original string.
// Instead of a delimiter string, a function is passed in the constructor to
// decide whether a given character is a delimiter, and if it is, what kind.
//
// Construct the tokenizer with the string to be tokenized and the delimiter
// function. Calling next computes the first and then subsequent tokens, until
// it returns false. The current token position can be retrieved with token(),
// and its length with tokenSize(). The part of the string after the current
// token shouldn't be modified during the operation.
// When needed (for example to display between tokens), the delimiters()
// returns the string between the previous and the current token, while
// delimSize() is its length.

// The kind of a character in a string tokenized with QCharTokenizer. The
// function must return this which determines which character is a delimiter.
// Normal - Not a delimiter character.
// Delimiter - A normal delimiter character. Skipped when computing the token.
// BreakBefore - A delimiter character which is also part of the next token.
// BreakAfter - A delimiter character which is part of the current token.
enum class QCharKind { Normal, Delimiter, BreakBefore, BreakAfter };

class QCharTokenizer
{
public:
    QCharTokenizer(const QChar *c, int strlen = -1, std::function<QCharKind(QChar)> delimfunc = qcharisdelim);

    // Call to initialize the first, and then subsequent tokens. Returns false
    // if it reached the end of the string without finding a token.
    bool next();

    // The token found after the last call to next().
    const QChar* token();
    // Character length of the token returned by token(), which is not null
    // terminated in most cases.
    int tokenSize();
    // The string consisting only of delimiters before the current token.
    const QChar* delimiters();
    // Character length of the string returned by delimiters(). Same as
    // the value of token() - delimiters().
    int delimSize();
private:
    // First character of the next token.
    const QChar *pos;
    // Length of the full string.
    int len;
    // Length of the current token.
    int tokenlen;
    // First character of the delimiter string.
    const QChar *delimpos;
    // Function used to determine which character is a delimiter.
    std::function<QCharKind(QChar)> delimfunc;
};


// All unicode strings are written to data files in UTF-8 format. When reading
// or writing a string use make_zstr with QString, QChar* or QCharString
// template parameter with the input or output operator. The ZStrFormat
// specifies what int type is used to save the UTF-8 string's length in bytes.
// The AddNull variants only make a difference for reading QChar* arrays, by
// adding an extra 0 value at the end of the array.
enum class ZStrFormat { Int = 0, Byte = 1, Word = 2, AddNull = 0x1000, ByteAddNull = Byte | AddNull, WordAddNull = Word | AddNull };

template<typename T>
struct ZStr;
template<typename T>
QDataStream& operator>>(QDataStream &stream, ZStr<T> str);
template<typename T>
QDataStream& operator<<(QDataStream &stream, const ZStr<T> &str);


template<typename T>
struct ZStr
{
    T &val;
    int length;
    ZStrFormat format;

    ZStr(T &val, ZStrFormat lsiz = ZStrFormat::Int) : val(val), length(-1), format(lsiz)
    {
        length = val.size();

        switch (format)
        {
        case ZStrFormat::Int:
            if (length > std::numeric_limits<qint32>::max())
                length = std::numeric_limits<qint32>::max();
            break;
        case ZStrFormat::Byte:
        case ZStrFormat::ByteAddNull:
            if (length > std::numeric_limits<quint8>::max())
                length = std::numeric_limits<quint8>::max();
            break;
        case ZStrFormat::Word:
        case ZStrFormat::WordAddNull:
            if (length > std::numeric_limits<quint16>::max())
                length = std::numeric_limits<quint16>::max();
            break;
        default:
            break;
        }
    }

    ZStr(T &val, int length, ZStrFormat lsiz = ZStrFormat::Int) : val(val), length(length), format(lsiz)
    {
        if (length == -1)
            length = val.size();

        switch (format)
        {
        case ZStrFormat::Int:
            if (length > std::numeric_limits<qint32>::max())
                length = std::numeric_limits<qint32>::max();
            break;
        case ZStrFormat::Byte:
        case ZStrFormat::ByteAddNull:
            if (length > std::numeric_limits<quint8>::max())
                length = std::numeric_limits<quint8>::max();
            break;
        case ZStrFormat::Word:
        case ZStrFormat::WordAddNull:
            if (length > std::numeric_limits<quint16>::max())
                length = std::numeric_limits<quint16>::max();
        }
    }

    ZStr(ZStr &&other) : val(other.val), length(other.length), format(other.format) { ; }

    static int maxSize(ZStrFormat format)
    {
        switch (format)
        {
        case ZStrFormat::Int:
            return std::numeric_limits<qint32>::max();
        case ZStrFormat::Byte:
        case ZStrFormat::ByteAddNull:
            return std::numeric_limits<quint8>::max();
        case ZStrFormat::Word:
        case ZStrFormat::WordAddNull:
            return std::numeric_limits<quint16>::max();
        default:
            break;
        }
        return 0;
    }

    int maxSize() const
    {
        return maxSize(format);
    }
private:
    int readLength(QDataStream &stream);
    void writeLength(QDataStream &stream, int len) const;

    friend QDataStream& operator>> <>(QDataStream &stream, ZStr str);
    friend QDataStream& operator<< <>(QDataStream &stream, const ZStr &str);
};

template<typename T> ZStr<T> make_zstr(T &val, ZStrFormat lsiz = ZStrFormat::Int)
{
    return ZStr<T>(val, lsiz);
}

template<typename T> ZStr<T> make_zstr(T &val, int length, ZStrFormat lsiz = ZStrFormat::Int)
{
    return ZStr<T>(val, length, lsiz);
}

template<typename T>
int ZStr<T>::readLength(QDataStream &stream)
{
    union LN
    {
        qint32 len32;
        quint16 len16;
        quint8 len8;
    } ln;

    int len = 0;

    switch (format)
    {
    case ZStrFormat::Int:
        stream >> ln.len32;
        len = ln.len32;
        break;
    case ZStrFormat::Byte:
    case ZStrFormat::ByteAddNull:
        stream >> ln.len8;
        len = ln.len8;
        break;
    case ZStrFormat::Word:
    case ZStrFormat::WordAddNull:
        stream >> ln.len16;
        len = ln.len16;
        break;
    default:
        break;
    }

    return len;
}

template<typename T>
void ZStr<T>::writeLength(QDataStream &stream, int len) const
{
    union LN
    {
        qint32 len32;
        quint16 len16;
        quint8 len8;
    } ln;

    switch (format)
    {
    case ZStrFormat::Int:
        ln.len32 = len;
        stream << ln.len32;
        break;
    case ZStrFormat::Byte:
    case ZStrFormat::ByteAddNull:
        ln.len8 = len;
        stream << ln.len8;
        break;
    case ZStrFormat::Word:
    case ZStrFormat::WordAddNull:
        ln.len16 = len;
        stream << ln.len16;
        break;
    default:
        break;
    }
}


template<typename T>
struct ZDateTimeStr;

template<typename T>
QDataStream& operator>>(QDataStream&, ZDateTimeStr<T>);
template<typename T>
QDataStream& operator<<(QDataStream&, const ZDateTimeStr<T>&);


template<typename T>
struct ZDateTimeStr
{
private:
    T &dt;
public:
    ZDateTimeStr(T &dt) : dt(dt) { ; }
    ZDateTimeStr(ZDateTimeStr &&other) : dt(other.dt) { ; }

    friend QDataStream& operator>> <>(QDataStream&, ZDateTimeStr);
    friend QDataStream& operator<< <>(QDataStream&, const ZDateTimeStr&);
};

#define Z_DATETIME_SIZE sizeof(quint64)

template<typename T>
ZDateTimeStr<T> make_zdate(T &v)
{
    return ZDateTimeStr<T>(v);
}

template<typename S, typename T, typename V>
struct ZVec;

template<typename S, typename T, typename V>
QDataStream& operator>>(QDataStream &stream, ZVec<S, T, V> v);
template<typename S, typename T, typename V>
QDataStream& operator<<(QDataStream &stream, const ZVec<S, T, V> &v);

// Structure for writing vectors to file. Writes the size of the V vector as
// type S. Each member of the vector is cast to T before reading and writing.
// Only valid for vectors holding POD or built in types, or types with default
// constructor and copy-constructor.
// Usage: stream << make_zvec<qint32, qint16>(sometype);
// The size type and member types must be specified to make_zvec.
template<typename S, typename T, typename V>
struct ZVec
{
    ZVec(V& vec) : vec(vec) {}
    ZVec(ZVec &&other) : vec(other.vec) { ; }

private:
    V& vec;
public:
    friend QDataStream& operator>> <>(QDataStream &stream, ZVec v);
    friend QDataStream& operator<< <>(QDataStream &stream, const ZVec &v);
};

template<typename S, typename T, typename V>
ZVec<S, T, V> make_zvec(V &vec)
{
    return ZVec<S, T, V>(vec);
}

template<typename S, typename T, typename V>
QDataStream& operator>>(QDataStream &stream, ZVec<S, T, V> v)
{
    S cnt;
    stream >> cnt;
    v.vec.resize(cnt);
    for (S s = 0; s != cnt; ++s)
    {
        T val;
        stream >> val;
        v.vec[s] = val;
    }

    return stream;
}

template<typename S, typename T, typename V>
QDataStream& operator<<(QDataStream &stream, const ZVec<S, T, V> &v)
{
    stream << (S)v.vec.size();
    for (S s = 0; s != (S)v.vec.size(); ++s)
        stream << (typename std::add_const<T>::type) v.vec[s];

    return stream;
}


class QPoint;
// Rect as it should be. The right and bottom edges are excluded when filling a
// rectangle, but included when drawing a border (quirk of Qt).
class ZRect
{
public:
    ZRect();
    ZRect(int x1, int y1, int w, int h);
    ZRect(const QPoint &topLeft, const QPoint &bottomRight);
    ZRect(const QRect &src);
    ZRect(const ZRect &src);
    ZRect& operator=(const QRect &src);
    ZRect& operator=(const ZRect &src);

    int left() const;
    int top() const;
    int right() const;
    int bottom() const;
    int width() const;
    int height() const;
    bool empty() const;

    // Returns the value of QRect::right().
    int qright() const;
    // Returns the value of QRect::bottom().
    int qbottom() const;

    void setLeft(int left);
    void setTop(int top);
    void setRight(int right);
    void setBottom(int bottom);
    void setWidth(int width);
    void setHeight(int height);

    // Returns the intersected rectangle of this and r.
    ZRect intersected(const ZRect &r) const;
    // Returns whether p is inside this rect.
    bool contains(const QPoint &p) const;
    // Returns a rectangle whose left, top, right and bottom coordinates were adjusted by the
    // passed values.
    ZRect adjusted(int x0, int y0, int x1, int y1) const;

    operator QRect() const;

    // Direct access to the inner rectangle.
    QRect& rect();
private:
    QRect r;
};

// Constructs a ZRect with the given values.
//ZRect ZRectS(int left, int top, int width, int height);


// Min and max functions for a varied number of numbers.

template <typename T1, typename T2>
auto mmax(const T1 &arg1, const T2 &arg2) -> decltype(std::max(arg1, arg2))
{
    return std::max(arg1, arg2);
}
template <typename T1, typename T2, typename ...T>
/*auto*/ typename std::common_type<T1, T2, T...>::type mmax(const T1 &arg1, const T2 &arg2, const T& ... args) //-> decltype(mmax(std::max(arg1, arg2), args...))
{
    return mmax(std::max(arg1, arg2), args...);
}

template <typename T1, typename T2>
auto mmin(const T1 &arg1, const T2 &arg2) -> decltype(std::min(arg1, arg2))
{
    return std::min(arg1, arg2);
}
template <typename T1, typename T2, typename ...T>
/*auto*/ typename std::common_type<T1, T2, T...>::type mmin(const T1 &arg1, const T2& arg2, const T& ... args) //-> decltype(mmin(std::min(arg1, arg2), args...))
{
    return mmin(std::min(arg1, arg2), args...);
}


// Updates minresult and maxresult to contain the numbers found in str in the form of
// [minresult]-[maxresult]. Either min or max result values can be missing (but not both), in
// which case they are set to minval - 1 or maxval + 1. When they are set, values outside the
// valid min and max are clamped. If the second value is less than the first one, it is set to
// be the same as the first value. If the string can't be converted to min and max values, the
// results are set to minval - 1. When the interval marking dash is missing, minresult is set
// to the clamped value between minval and maxval, and maxresult is set to minval - 1. When
// successful, returns true if a range was found, even if one value is not set (has the format
// -[maxresult] for example).
bool findStrIntMinMax(QString str, int minval, int maxval, int &minresult, int &maxresult);

// Returns a string in the form [lo]-[hi] or just [lo]. If lo equals to minval - 1 or hi to
// maxval + 1, they will be missing in the first format. If hi is minval - 1 and lo is valid,
// the second format is returned. If both values are minval - 1, the result is an empty
// string.
QString IntMinMaxToString(int minval, int maxval, int lo, int hi);


// Returns a random number in the range [lo, hi].
int rnd(int lo, int hi);
// Returns the engine that can be used for random number generation. Initialized when the
// program starts and usually used with a std::uniform_int_distribution<int>.
std::default_random_engine& random_engine();


// Quicksort used for big lists that the user must be able to interrupt, or a progress must be
// shown. Same as std::sort with the one difference that the cmp function gets passed a third
// argument of type bool&. Set it to true to abandon the sort. Returns true if it was stopped
// this way.
template<typename Iter, typename Comp>
bool interruptSort(Iter begin, Iter end, Comp cmp)
{
    if (end <= begin || std::next(begin) == end)
        return false;

    Iter pos;
    Iter left;
    Iter right;
    bool stop = false;
    do
    {
        right = std::prev(end);
        left = std::next(begin);

        // The pivot of the sort is the first elem.
        pos = begin;

        while (!stop && left <= right)
        {
            while (!stop && left != end && cmp(*left, *pos, stop))
                ++left;
            while (!stop && cmp(*pos, *right, stop))
                --right;

            if (left <= right)
            {
                if (left != right)
                    std::swap(*left, *right);
                ++left;
                --right;
            }
        }

        if (right != pos)
            std::swap(*pos, *right);

        if (!stop && begin < right)
            stop = interruptSort(begin, right, cmp);
        begin = left;
    } while (!stop && begin < std::prev(end));

    return stop;
}

// Same functionality as std::upper_bound, but the comparison function updates a bool argument
// which when set to true the bound search is aborted. The return value of the aborted search
// is undefined.
// Returns an iterator to the first value between [begin, end) which evaluates to be larger
// than val by the cmp function. The cmp function should take 3 arguments. First is val, which
// is passed unchanged, second is the value pointed to by the current tested position in the
// interval, last is a reference to a bool. Pass 'true' in that bool to stop the search.
template<typename Iter, typename Value, typename Comp>
Iter interruptUpperBound(Iter first, Iter last, Value val, Comp cmp)
{
    if (last <= first)
        return first;

    bool stop = false;
    int count = std::distance(first, last);
    while (0 < count && !stop)
    {
        int half = count / 2;
        Iter it = first;
        std::advance(it, half);

        if (!cmp(val, *it, stop))
        {
            first = ++it;
            count -= half + 1;
        }
        else
            count = half;
    }

    return first;
}


// Removes a value from a vector, decrementing any values higher than that value. Returns the
// index of the value removed, or -1 if the value was not found.
template<typename T>
int removeIndexFromList(int index, std::vector<T> &vec)
{
    int indexindex = -1;
    for (int ix = 0, siz = tosigned(vec.size()); ix != siz; ++ix)
    {
        if (vec[ix] == index)
            indexindex = ix;
        else if (vec[ix] > index)
            --vec[ix];
    }

    if (indexindex == -1)
        return -1;
    std::vector<T> tmp;
    std::swap(tmp, vec);
    vec.reserve(tmp.size() - 1);

    if (indexindex > 0)
        vec.insert(vec.end(), tmp.begin(), tmp.begin() + indexindex);
    if (indexindex < tosigned(tmp.size()) - 1)
        vec.insert(vec.end(), tmp.begin() + indexindex + 1, tmp.end());

    return indexindex;
}


class Dictionary;
class KanjiElementList;
class QMainWindow;
namespace ZKanji
{
    // Returns whether the base data can be found in the zkanji data folder.
    // When true, the program must import dictionary data before it loads, or
    // it must quit. Defaults to false.
    bool noData();
    // Set whether there is no base data in the zkanji data folder.
    void setNoData(bool nodata);

    void generateValidKanji();
    void generateValidUnicode();

    void setAppFolder(QString path);
    // Folder with the executable and the data sub-dir for non-writable
    // dictionary and other data. Does not contain trailing slash.
    const QString& appFolder();
    void setUserFolder(QString path);
    // Read/writable folder where users can save their data. Does not contain
    // trailing slash.
    const QString& userFolder();
    void setLoadFolder(QString path);
    // Readable folder for reading data from on startup. Matches the user
    // folder if not importing. Does not contain trailing slash.
    const QString& loadFolder();

    extern const ushort kanjicount;
    extern std::vector<uchar> validkanji;
    extern std::vector<uchar> validcode;
    extern uchar validcodelowerhalf; // Highest value of unicode symbol found between 0x3000 and 0x30ff in words. Starts at 0 for word found at 0x3000.
    extern const int validcodecount;

    // Removes kanji and radical data after a full dictionary import. Other
    // data is not loaded yet at this point, while the imported dictionary
    // is not added to the global dictionary list.
    void cleanupImport();

    // Creates the recognizer and loads kanji handwriting recognizer data from file.
    void initElements(const QString &filename);

    QDateTime QDateTimeUTCFromTDateTime(double d);

    // Shows the kanji information window with a kanji by the passed index.
    void showKanjiInfo(/*QWidget *owner,*/ Dictionary *d, int index);

}


//-------------------------------------------------------------


template<> QDataStream& operator>>(QDataStream& stream, ZStr<QString> str);
template<> QDataStream& operator>>(QDataStream& stream, ZStr<QChar*> str);
template<> QDataStream& operator>>(QDataStream& stream, ZStr<QCharString> str);
template<> QDataStream& operator<<(QDataStream& stream, const ZStr<const QString> &str);
template<> QDataStream& operator<<(QDataStream& stream, const ZStr<const QChar*> &str);
template<> QDataStream& operator<<(QDataStream& stream, const ZStr<const QCharString> &str);
template<> QDataStream& operator<<(QDataStream& stream, const ZStr<QString> &str);
template<> QDataStream& operator<<(QDataStream& stream, const ZStr<QChar*> &str);
template<> QDataStream& operator<<(QDataStream& stream, const ZStr<QCharString> &str);

template<> QDataStream& operator<<(QDataStream &stream, const ZDateTimeStr<const QDateTime> &date);
template<> QDataStream& operator<<(QDataStream &stream, const ZDateTimeStr<QDateTime> &date);
template<> QDataStream& operator>>(QDataStream &stream, ZDateTimeStr<QDateTime> date);
template<> QDataStream& operator<<(QDataStream &stream, const ZDateTimeStr<const QDate> &date);
template<> QDataStream& operator<<(QDataStream &stream, const ZDateTimeStr<QDate> &date);
template<> QDataStream& operator>>(QDataStream &stream, ZDateTimeStr<QDate> date);


//-------------------------------------------------------------



#endif
