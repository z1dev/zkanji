/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPoint>
#include <QDir>
#include <QStringBuilder>
#include "zkanjimain.h"
#include "kanji.h"
#include "studydecks.h"
#include "words.h"

//-------------------------------------------------------------

namespace
{
    std::random_device rd;
    std::default_random_engine eng(rd());
}

namespace Temporary
{
    // Compatibility code with zkanji prior to 2015, for reading and writing the old data format.
    QDataStream& operator>>(QDataStream& stream, SYSTEMTIME &st)
    {
        stream >> st.wYear;
        stream >> st.wMonth;
        stream >> st.wDayOfWeek;
        stream >> st.wDay;
        stream >> st.wHour;
        stream >> st.wMinute;
        stream >> st.wSecond;
        stream >> st.wMilliseconds;

        return stream;
    }

    void LoadGapList(QDataStream &stream, std::map<int, QCharString> &dest)
    {
        bool firstempty;
        stream >> firstempty;

        quint32 cnt;
        stream >> cnt;

        qint32 poscnt;
        stream >> poscnt;

        std::vector<quint32> posvec;
        posvec.resize(poscnt);
        quint32 *pos = posvec.data();

        stream.readRawData((char*)pos, poscnt * sizeof(quint32));
        //    fread(pos, sizeof(unsigned int), poscount, f);

        quint32 used;
        stream >> used;

        if (!used)
            return;

        int pospos = firstempty ? 0 : -1;
        int mappos = (firstempty ? pos[0] : 0);
        int endpos;
        endpos = (poscnt > (pospos + 1) ? pos[pospos + 1] : cnt);

        for (unsigned int ix = 0; ix != used; ++ix)
        {
            stream >> make_zstr(dest[mappos++], ZStrFormat::Word);

            if (mappos == endpos)
            {
                pospos += 2;
                if (pospos < poscnt)
                {
                    mappos = pos[pospos];
                    endpos = (poscnt > (pospos + 1) ? pos[pospos + 1] : cnt);
                }
            }
        }
    }
}


//-------------------------------------------------------------


namespace ZKanji
{
    //std::vector<ushort> partlist[252];
    const ushort kanjicount = 6355;

    std::vector<uchar> validkanji;
    std::vector<uchar> validcode;
    // -1 is debug value that's never used.
    uchar validcodelowerhalf = (uchar)-1;
    const int validcodecount = (0x30ff - 0x3000) + (0xffee - 0xff01) + 2; // (0x30ff - 0x3000) + (0xffe0 - 0xff00); - This was the original count
}


//-------------------------------------------------------------


extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;

NTFSPermissionGuard::NTFSPermissionGuard()
{
#ifdef Q_OS_WIN
    ++qt_ntfs_permission_lookup;
#endif
}

NTFSPermissionGuard::~NTFSPermissionGuard()
{
#ifdef Q_OS_WIN
    --qt_ntfs_permission_lookup;
#endif
}


//-------------------------------------------------------------


Error Error::lasterror;

Error Error::last()
{
    return lasterror;
}

Error::Error()
{
    ;
}

Error::Error(const Error &src)
{
    *this = src;
}

Error& Error::operator=(const Error &src)
{
    type = src.type;
    code = src.code;
    msg = src.msg;
    return *this;
}


Error::Error(int code, QString msg) : code(code), msg(msg)
{
    lasterror = *this;
}
 
Error::Error(Types type, int code, QString msg) : type(type), code(code), msg(msg)
{
    lasterror = *this;
}

QString Error::toString() const
{
    if (type == NoError)
        return typeMessage();

    if (code == 0 && msg.isEmpty())
        return typeMessage();
    if (code != 0 && msg.isEmpty())
        return typeMessage() % QStringLiteral(" ") % codeMessage();
    if (code == 0 && !msg.isEmpty())
        return typeMessage() % QStringLiteral(" ") % msg;
    return typeMessage() % QStringLiteral(" ") % codeMessage() % QStringLiteral(" ") % msg;
}

QString Error::typeMessage() const
{
    switch (type)
    {
    case NoError:
        return qApp->translate("GlobalUI", "No error occured.");
    case Access:
        return qApp->translate("GlobalUI", "Access denied.");
    case Write:
        return qApp->translate("GlobalUI", "Write error.");
    case General:
    default:
        return qApp->translate("GlobalUI", "An error occured.");
    }
}

QString Error::codeMessage() const
{
    if (code == 0)
        return QString();
    return qApp->translate("GlobalUI", "Error code: %1.").arg(code);
}

Error::operator bool() const
{
    return type == NoError;
}

Error::operator Types() const
{
    return type;
}

Error::operator int() const
{
    return code;
}

Error::operator QString() const
{
    return msg;
}


//-------------------------------------------------------------


ZException::ZException(const char * const &m) : msg(nullptr)
{
    copyMsg(m);
}

ZException::ZException(const ZException &other) : msg(nullptr)
{
    *this = other;
}

ZException::~ZException()
{
    delete[] msg;
}

ZException ZException::operator=(const ZException &other)
{
    if (this == &other)
        return *this;

    copyMsg(other.msg);

    return *this;
}

const char* ZException::what() const 
#if !defined(_MSC_VER) || _MSC_VER >= 1900
    noexcept
#endif
{
    return msg != nullptr ? msg : "Exception occurred.";
}

void ZException::copyMsg(const char * const &m)
{
    if (msg == m)
        return;

    delete[] msg;
    msg = nullptr;
    if (m == nullptr)
        return;

    msg = new char[strlen(m) + 1];
    if (msg == nullptr)
        return;
    strcpy(msg, m);
}


//-------------------------------------------------------------


QCharTokenizer::QCharTokenizer(const QChar *c, int len, std::function<QCharKind(QChar)> delimfunc) : pos(c), len(len), tokenlen(0), delimpos(nullptr), delimfunc(delimfunc)
{
    if (len == -1)
        this->len = tosigned(qcharlen(c));
}

bool QCharTokenizer::next()
{
    if (!pos || tokenlen == len)
        return false;

    if (tokenlen)
    {
        pos += tokenlen;
        len -= tokenlen;
    }
    delimpos = pos;
    tokenlen = 0;

    while (tokenlen != len)
    {
        QCharKind kind = delimfunc(pos[tokenlen]);

        if (tokenlen == 0 && kind == QCharKind::Delimiter)
        {
            ++pos;
            --len;
            continue;
        }

        if (kind != QCharKind::Delimiter && (kind != QCharKind::BreakBefore || tokenlen == 0))
        {
            ++tokenlen;
            if (kind != QCharKind::BreakAfter)
                continue;
        }

        break;
    }

    return len != 0;
}

const QChar* QCharTokenizer::token()
{
#ifdef _DEBUG
    if (tokenlen == 0)
        throw "Incorrect, don't even come here!";
#endif

    return pos;
}

int QCharTokenizer::tokenSize()
{
    return tokenlen;
}

const QChar* QCharTokenizer::delimiters()
{
#ifdef _DEBUG
    if (delimpos == nullptr)
        throw "Incorrect, don't even come here!";
#endif

    return delimpos;
}

int QCharTokenizer::delimSize()
{
    return pos - delimpos;
}


//-------------------------------------------------------------


template<>
QDataStream& operator>>(QDataStream& stream, ZStr<QString> str)
{
    int len = str.readLength(stream);
    if (len < 0 || len > str.maxSize())
        throw ZException("Invalid stream size (ZStr<QString>)");

    QByteArray data(len, 0);
    stream.readRawData(data.data(), len);

    QString s = QString::fromUtf8(data, len);

    str.val = s;

    return stream;
}

template<>
QDataStream& operator>>(QDataStream& stream, ZStr<QChar*> str)
{
    int len = str.readLength(stream);
    if (len < 0 || len > str.maxSize())
        throw ZException("Invalid stream size (ZStr<QChar*>)");

    QByteArray data(len, 0);
    stream.readRawData(data.data(), len);

    QString s = QString::fromUtf8(data, len);
    len = s.size();

    if (((int)str.format & (int)ZStrFormat::AddNull) == (int)ZStrFormat::AddNull)
    {
        str.val = new QChar[len + 1];
        str.val[len] = 0;
    }
    else
        str.val = new QChar[len];

    memcpy(str.val, s.constData(), len * sizeof(QChar));

    return stream;
}

template<>
QDataStream& operator>>(QDataStream& stream, ZStr<QCharString> str)
{
    int len = str.readLength(stream);
    if (len < 0 || len > str.maxSize())
        throw ZException("Invalid stream size (ZStr<QCharString>)");

    QByteArray data(len, 0);
    stream.readRawData(data.data(), len);

    QString s = QString::fromUtf8(data, len);
    len = s.size();

    str.val.copy(s.constData(), len);

    return stream;
}

template<>
QDataStream& operator<<(QDataStream& stream, const ZStr<const QString> &str)
{
    QByteArray data;
    if (str.length == -1)
        data = str.val.toUtf8();
    else
    {
        int len = std::min(str.length, str.val.size());
        data = str.val.leftRef(len).toUtf8();
        if (len < str.length)
        {
            std::vector<char> vec(str.length - len, ' ');
            data.append(vec.data(), tosigned(vec.size()));
        }
    }

    str.writeLength(stream, data.size());
    stream.writeRawData(data.data(), data.size());

    return stream;
}

template<>
QDataStream& operator<<(QDataStream& stream, const ZStr<const QChar*> &str)
{
    QByteArray data;
    if (str.length == -1)
        data = QString::fromRawData(str.val, tosigned(qcharlen(str.val))).toUtf8();
    else
        data = QString::fromRawData(str.val, str.length).toUtf8();

    str.writeLength(stream, data.size());
    stream.writeRawData(data.data(), data.size());

    return stream;
}

template<>
QDataStream& operator<<(QDataStream& stream, const ZStr<const QCharString> &str)
{
    QByteArray data;
    if (str.length == -1)
        data = str.val.toUtf8();
    else
    {
        int len = std::min<int>(str.length, str.val.size());
        data = str.val.toUtf8(len);
        if (len < str.length)
        {
            std::vector<char> vec(str.length - len, ' ');
            data.append(vec.data(), tosigned(vec.size()));
        }
    }

    str.writeLength(stream, data.size());
    stream.writeRawData(data.data(), data.size());

    return stream;
}

template<>
QDataStream& operator<<(QDataStream& stream, const ZStr<QString> &str)
{
    QByteArray data;
    if (str.length == -1)
        data = str.val.toUtf8();
    else
    {
        int len = std::min(str.length, str.val.size());
        data = str.val.leftRef(len).toUtf8();
        if (len < str.length)
        {
            std::vector<char> vec(str.length - len, ' ');
            data.append(vec.data(), tosigned(vec.size()));
        }
    }

    str.writeLength(stream, data.size());
    stream.writeRawData(data.data(), data.size());

    return stream;
}

template<>
QDataStream& operator<<(QDataStream& stream, const ZStr<QChar*> &str)
{
    QByteArray data;
    if (str.length == -1)
        data = QString::fromRawData(str.val, tosigned(qcharlen(str.val))).toUtf8();
    else
        data = QString::fromRawData(str.val, str.length).toUtf8();

    str.writeLength(stream, data.size());
    stream.writeRawData(data.data(), data.size());

    return stream;
}

template<>
QDataStream& operator<<(QDataStream& stream, const ZStr<QCharString> &str)
{
    QByteArray data;
    if (str.length == -1)
        data = str.val.toUtf8();
    else
    {
        int len = std::min<int>(str.length, str.val.size());
        data = str.val.toUtf8(len);
        if (len < str.length)
        {
            std::vector<char> vec(str.length - len, ' ');
            data.append(vec.data(), tosigned(vec.size()));
        }
    }

    str.writeLength(stream, data.size());
    stream.writeRawData(data.data(), data.size());

    return stream;
}


//-------------------------------------------------------------


template<>
QDataStream& operator<<(QDataStream &stream, const ZDateTimeStr<const QDateTime> &date)
{
    if (date.dt.isValid())
    {
        if (date.dt.timeSpec() != Qt::UTC)
            throw "Date must be in UTC format to write to stream.";

        // Constructing a quint64 by only saving as many bits of each component as required:
        // year 12 month 4 day 5 - hour 5 minute 6 second 6 millisecond 10 = 48bits
        QDate d = date.dt.date();
        QTime t = date.dt.time();
        quint64 data = (quint64(d.year()) & 0xfff) | (quint64(d.month() & 0xf) << 12) | (quint64(d.day() & 0x1f) << 16) |
            (quint64(t.hour() & 0x1f) << 21) | (quint64(t.minute() & 0x3f) << 26) | (quint64(t.second() & 0x3f) << 32) |
            (quint64(t.msec() & 0x3ff) << 38);

        //QString datestr = date.dt.toString("yyyy-MM-dd HH:mm:ss.zzz");
        //stream.writeRawData(datestr.toLatin1().constData(), 23);
        stream << data;
    }
    else
        //for (int ix = 0; ix != 23; ++ix)
            stream << (quint64)0;

    return stream;
}

template<>
QDataStream& operator<<(QDataStream &stream, const ZDateTimeStr<QDateTime> &date)
{
    return stream << ZDateTimeStr<const QDateTime>(date.dt);
}

template<>
QDataStream& operator>>(QDataStream &stream, ZDateTimeStr<QDateTime> date)
{
    quint64 u64;
    stream >> u64;

    //char datestr[23];
    //stream.readRawData(datestr, 23);

    // Invalid date time was written
    //if (datestr[0] == 0)
    if (u64 == 0)
    {
        date.dt = QDateTime();
        date.dt.setTimeSpec(Qt::UTC);
        return stream;
    }

    //date.dt = QDateTime::fromString(QString::fromLatin1(datestr, 23), QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    date.dt = QDateTime(QDate(u64 & 0xfff, (u64 >> 12) & 0xf, (u64 >> 16) & 0x1f), QTime((u64 >> 21) & 0x1f, (u64 >> 26) & 0x3f, (u64 >> 32) & 0x3f, (u64 >> 38) & 0x3ff));
    date.dt.setTimeSpec(Qt::UTC);



#ifdef _DEBUG
    if (!date.dt.isValid())
        throw "Invalid date time in file.";
#endif

    return stream;
}

template<>
QDataStream& operator<<(QDataStream &stream, const ZDateTimeStr<const QDate> &date)
{
    if (date.dt.isValid())
    {
        // Constructing a quint32 by only saving as many bits of each component as required:
        // year 12 month 4 day 5 = 21bits

        //QString datestr = date.dt.toString("yyyy-MM-dd");
        //stream.writeRawData(datestr.toLatin1().constData(), 10);

        // Constructing a quint64 by only saving as many bits of each component as required:
        // year 12 month 4 day 5 - hour 5 minute 6 second 6 millisecond 10 = 48bits
        QDate d = date.dt;
        quint32 data = (quint64(d.year()) & 0xfff) | (quint64(d.month() & 0xf) << 12) | (quint64(d.day() & 0x1f) << 16);

        return (stream << data);
    }
    else
        //for (int ix = 0; ix != 10; ++ix)
            stream << (quint32)0;

    return stream;
}

template<>
QDataStream& operator<<(QDataStream &stream, const ZDateTimeStr<QDate> &date)
{
    return stream << ZDateTimeStr<const QDate>(date.dt);
}

template<>
QDataStream& operator>>(QDataStream &stream, ZDateTimeStr<QDate> date)
{
    //char datestr[10];
    //stream.readRawData(datestr, 10);
    quint32 u32;
    stream >> u32;

    // Invalid date time was written
    //if (datestr[0] == 0)
    if (u32 == 0)
    {
        date.dt = QDate();
        return stream;
    }

    //date.dt = QDate::fromString(QString::fromLatin1(datestr, 10), QStringLiteral("yyyy-MM-dd"));
    date.dt = QDate(u32 & 0xfff, (u32 >> 12) & 0xf, (u32 >> 16) & 0x1f);

    return stream;
}


//-------------------------------------------------------------


ZRect::ZRect()
{
    ;
}

ZRect::ZRect(int x1, int y1, int w, int h) : r(x1, y1, w, h)
{
    ;
}

ZRect::ZRect(const QPoint &topLeft, const QPoint &bottomRight) : r(topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y())
{
    ;
}

ZRect::ZRect(const QRect &r) : r(r)
{
    ;
}

ZRect::ZRect(const ZRect &src) : r(src.r)
{
}

ZRect& ZRect::operator=(const QRect &src)
{
    r = src;
    return *this;
}

ZRect& ZRect::operator=(const ZRect &src)
{
    r = src.r;
    return *this;
}

int ZRect::left() const
{
    return r.left();
}

int ZRect::top() const
{
    return r.top();
}

int ZRect::right() const
{
    return r.right() + 1;
}

int ZRect::bottom() const
{
    return r.bottom() + 1;
}

int ZRect::width() const
{
    return r.width();
}

int ZRect::height() const
{
    return r.height();
}

bool ZRect::empty() const
{
    return r.isEmpty();
}

int ZRect::qright() const
{
    return r.right();
}

int ZRect::qbottom() const
{
    return r.bottom();
}

void ZRect::setLeft(int left)
{
    r.setLeft(left);
}

void ZRect::setTop(int top)
{
    r.setTop(top);
}

void ZRect::setRight(int right)
{
    r.setRight(right - 1);
}

void ZRect::setBottom(int bottom)
{
    r.setBottom(bottom - 1);
}

void ZRect::setWidth(int width)
{
    r.setWidth(width);
}

void ZRect::setHeight(int height)
{
    r.setHeight(height);
}

ZRect ZRect::intersected(const ZRect &other) const
{
    return r.intersected(other.r);
}

bool ZRect::contains(const QPoint &p) const
{
    return r.contains(p);
}

ZRect ZRect::adjusted(int x0, int y0, int x1, int y1) const
{
    return ZRect(r.adjusted(x0, y0, x1, y1));
}

ZRect::operator QRect() const
{
    return r;
}

QRect& ZRect::rect()
{
    return r;
}

//ZRect ZRectS(int left, int top, int width, int height)
//{
//    return ZRect(left, top, left + width, top + height);
//}


//-------------------------------------------------------------


bool findStrIntMinMax(QString str, int minval, int maxval, int &minresult, int &maxresult)
{
#ifdef _DEBUG
    if (minval == std::numeric_limits<int>::min() || maxval == std::numeric_limits<int>::max())
        throw "Minresult and maxresult must be set to minval - 1 or maxval + 1 so using the limits is invalid.";
#endif

    int dash;
    int val;
    int val2;
    bool success;

    str = str.trimmed();
    dash = str.indexOf('-');
    if (dash == -1)
    {
        val = str.toInt(&success);
        if (success)
        {
            minresult = clamp(val, minval, maxval);
            maxresult = minval - 1;
        }
    }
    else
    {
        if (dash == 0)
        {
            if (str.size() == 1)
                success = false;
            else
            {
                minresult = minval - 1;
                success = true;
            }
        }
        else
        {
            val = str.leftRef(dash).toInt(&success);

            if (success)
                minresult = clamp(val, minval, maxval);
        }

        if (success && dash == str.size() - 1)
            maxresult = maxval + 1;
        else if (success)
        {
            val2 = str.rightRef(str.size() - dash - 1).toInt(&success);
            if (success)
                maxresult = clamp(val2, minresult, maxval);
        }

    }
    if (!success)
    {
        minresult = maxresult = minval - 1;
        return false;
    }

    return dash != -1;
}

QString IntMinMaxToString(int minval, int maxval, int lo, int hi)
{
    if ((lo < minval && hi < minval) || lo > maxval)
        return QString();

    if (hi < minval)
        return QString::number(lo);

    if (lo >= minval && hi <= maxval)
        return QString("%1-%2").arg(lo).arg(hi);

    if (lo < minval)
        return QString("-%1").arg(hi);
    return QString("%1-").arg(lo);
}

int rnd(int lo, int hi)
{
    std::uniform_int_distribution<int> intdist(lo, hi);
    return intdist(eng);
}

std::default_random_engine& random_engine()
{
    return eng;
}


//-------------------------------------------------------------

namespace ZKanji
{
    void cleanupImport()
    {
        originals.clear();
        validkanji.clear();
        radlist.clear();
        kanjis.clear();
        radkmap.clear();
        radklist.clear();
        radkcnt.clear();
        commons.clear();
    }

    QDateTime QDateTimeUTCFromTDateTime(double d)
    {
        if (d == 0)
            return QDateTime();

        // A TDateTime holds the number of days that have passed since 12/30/1899 in
        // a double. Negative values are valid too, but the fractional part is always
        // used as it were positive. A value of 0.25 means a quarter day (6 hours) and
        // is equivalent to -0.25.

        // Date 0 of TDateTime.
        QDateTime t(QDate(1899, 12, 30), QTime(0, 0, 0, 0)); 

        return t.addDays((qint64)d).addMSecs((d - (qint64)d) * (24 * 60 * 60 * 1000)).toUTC();
    }

    static bool nobasedatafound = false;
    bool noData()
    {
        return nobasedatafound;
    }

    void setNoData(bool nodata)
    {
        nobasedatafound = nodata;
    }

    void generateValidKanji()
    {
        if (validkanji.empty())
            validkanji.resize(0x9faf - 0x4e00 + 1, 0);
        //memset(validkanji, 0, (0x9faf - 0x4e00 + 1) * sizeof(bool));
        uchar *vkdata = &validkanji[0];
        for (int ix = 0, siz = tosigned(kanjis.size()); ix != siz;  ++ix)
            vkdata[kanjis[ix]->ch.unicode() - 0x4e00] = true;
    }

    void generateValidUnicode()
    {
        //QChar validchar[validcodecount] = { };
        // Valid characters are those in the ranges:
        // 0x3000 - 0x3029, 0x3030 - 0x3037, 0x303B - 0x303E, 0x3041 - 0x3049, 0x304A - 3096,  0x309F - 0x30FF
        // 0xFF01 - 0xFF9F, 0xFFE0 - 0xFFE6, 0xFFE8 - 0xFFEE

        if (!validcode.empty())
            return;

        validcode.resize(validcodecount, false);
        uchar *vcdata = &validcode[0];

        //memset(validcode, 0, validcodecount * sizeof(bool));

        // -1 is debug value that's never used.
        validcodelowerhalf = (uchar)-1;

        ushort ranges[] = { 0x3000, 0x3029,   0x3030, 0x3037,   0x303B, 0x303E,   0x3041, 0x3049,   
                            0x304A, 0x3096,   0x309F, 0x30FF,   0xFF01, 0xFF9F,   0xFFE0, 0xFFE6, 
                            0xFFE8, 0xFFEE };

        for (int rix = 0; rix < 9; ++rix)
        {
            for (int ix = ranges[rix * 2]; ix <= ranges[rix * 2 + 1]; ++ix)
            {
                if (ix >= 0x3000 && ix <= 0x30ff)
                {
                    vcdata[ix - 0x3000] = 1;
                    validcodelowerhalf = ix - 0x3000;
                }
                else if (ix >= 0xff01 && ix <= 0xffee)
                    vcdata[ix - 0xff01 + validcodelowerhalf + 1] = 1;
            }
        }
    }

    static QString appdir;
    void setAppFolder(QString path)
    {
        while (!path.isEmpty() && path.at(path.size() - 1) == '/')
            path.chop(1);
        appdir = path;
    }

    const QString& appFolder()
    {
        return appdir;
    }

    static QString userdir;
    void setUserFolder(QString path)
    {
        while (!path.isEmpty() && path.at(path.size() - 1) == '/')
            path.chop(1);

        userdir = path;
        QDir d(userdir);
        if (!d.exists())
            d.mkpath(userdir + "/data");
    }

    const QString& userFolder()
    {
        return userdir;
    }

    static QString loaddir;
    void setLoadFolder(QString path)
    {
        while (!path.isEmpty() && path.at(path.size() - 1) == '/')
            path.chop(1);

        loaddir = path;
    }

    const QString& loadFolder()
    {
        if (!loaddir.isEmpty())
            return loaddir;
        return userdir;
    }
}


//-------------------------------------------------------------
