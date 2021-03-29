/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QDataStream>
#include <QMessageBox>
#include <QApplication>
#include <QPainterPath>

#include <cmath>
#include <set>

#include "kanjistrokes.h"
#include "kanji.h"
#include "zkanjimain.h"
#include "generalsettings.h"

#include "checked_cast.h"

namespace ZKanji
{
    KanjiElementList *reclist = nullptr;
    KanjiElementList* elements()
    {
        if (reclist == nullptr)
            reclist = new KanjiElementList();
        return reclist;
    }

    void initElements(const QString &filename)
    {
        if (reclist != nullptr)
            delete reclist;
        reclist = new KanjiElementList(filename);
    }
}


static const double const_PI = 3.14159265358979323846;
static const double const_PI_2 = 1.57079632679489661923;
static const double const_PI_4 = 0.785398163397448309616;

static const double const_PI_14 = const_PI / 14.0;
static const double const_PI_11 = const_PI / 11.0;
static const double const_PI_11_2 = const_PI_2 - const_PI_11;
static const double const_2PI = const_PI * 2.0;
static const double const_3PI_2 = const_PI_2 * 3.0;

double lineLength(QPointF p1, QPointF p2)
{
    double x = p2.x() - p1.x();
    double y = p2.y() - p1.y();
    return std::sqrt(x * x + y * y);
}

// Y bezier coordinates starting/ending and control points.
// When drawing a dot stroke, the width of the stroke is computed like it were the y component
// of a bezier with these coordinates at each point.

static const double dotwsy = 0.15;
static const double dotwc1y = 0.3;
static const double dotwc2y = 2.0;
static const double dotwey = 0.6;



//-------------------------------------------------------------


Radian::Radian() : val(0)
{

}

Radian::Radian(double val) : val(val)
{
    normalize();
}

double Radian::radDiff(const Radian &r) const
{
    double a = r.val;
    double b = val;

    if (std::fabs(a - b) > const_PI)
    {
        if (a > const_PI)
            a -= 2 * const_PI;
        else
            b -= 2 * const_PI;
    }

    return a - b;
}

bool Radian::operator>(const Radian &r) const
{
    return std::fabs(val) > std::fabs(r.val);
}

bool Radian::operator<(const Radian &r) const
{
    return std::fabs(val) < std::fabs(r.val);
}

bool Radian::operator>=(const Radian &r) const
{
    return std::fabs(val) >= std::fabs(r.val);
}

bool Radian::operator<=(const Radian &r) const
{
    return std::fabs(val) <= std::fabs(r.val);
}

RadianDistance Radian::operator-(const Radian &r) const
{
    return RadianDistance(radDiff(r));
}

//Radian& Radian::operator-=(const Radian &r)
//{
//    val = radDiff(r);
//    return *this;
//}

double Radian::abs()
{
    return std::fabs(val);
}

double Radian::sin()
{
    return std::sin(std::fabs(val));
}

//Radian::Direction Radian::horz()
//{
//    if (fabs(val - const_PI) < const_PI_14)
//        return right;
//    if (val < const_PI_14 || val > const_2PI - const_PI_14)
//        return left;
//
//    return none;
//}
//
//Radian::Direction Radian::vert()
//{
//    if (fabs(val - const_PI_2) < const_PI_14)
//        return up;
//    if (fabs(val - const_3PI_2) < const_PI_14)
//        return down;
//
//    return none;
//}
//
//Radian::Direction Radian::diag()
//{
//    if (val > const_PI_11 && val < const_PI_11_2)
//        return upLeft;
//    if (val - const_PI_2 > const_PI_11 && val - const_PI_2 < const_PI_11_2)
//        return upRight;
//    if (val - const_PI > const_PI_11 && val - const_PI < const_PI_11_2)
//        return downRight;
//    if (val - const_3PI_2 > const_PI_11 && val - const_3PI_2 < const_PI_11_2)
//        return downLeft;
//
//    return none;
//}

void Radian::normalize()
{
    while (val < 0)
        val += 2 * const_PI;
    while (val >= 2 * const_PI)
        val -= 2 * const_PI;
}


//-------------------------------------------------------------


RadianDistance::RadianDistance(double val) : val(val)
{

}

double RadianDistance::abs()
{
    return std::fabs(val);
}

double RadianDistance::sin()
{
    return std::sin(std::fabs(val));
}


//-------------------------------------------------------------


namespace RecMath
{
    Radian rad(const QPointF &a, const QPointF &b)
    {
#ifdef _DEBUG
        if (a == b)
            throw "Two points shouldn't be the same.";
#endif
        return Radian(std::atan2(a.y() - b.y(), a.x() - b.x()));
    }

    double len(const QPointF &a, const QPointF &b)
    {
        double xdif = a.x() - b.x();
        double ydif = a.y() - b.y();
        return sqrt(xdif * xdif + ydif * ydif);
    }

    double pointDist(const QPointF &p1, const QPointF &p2, const QPointF &q)
    {
        if (p1 == p2)
            return len(p1, q);

        RadianDistance r = rad(p1, p2) - rad(p1, q);
        return r.sin() * len(p1, q);
    }
}


//-------------------------------------------------------------


Stroke::Stroke() : len(0), sectcnt(0)
{
    ;
}

Stroke::Stroke(const Stroke &src) : list(src.list), len(src.len), sectcnt(src.sectcnt), dim(src.dim)
{
    ;
}

Stroke::Stroke(Stroke &&src) : len(0), sectcnt(0)
{
    *this = std::move(src);
}

Stroke& Stroke::operator=(const Stroke &src)
{
    list = src.list;
    len = src.len;
    sectcnt = src.sectcnt;
    dim = src.dim;
    return *this;
}

Stroke& Stroke::operator=(Stroke &&src)
{
    std::swap(list, src.list);
    std::swap(len, src.len);
    std::swap(sectcnt, src.sectcnt);
    std::swap(dim, src.dim);
    return *this;
}

size_t Stroke::size() const
{
    return list.size();
}

bool Stroke::empty() const
{
    return list.empty();
}

void Stroke::add(const QPointF &pt)
{
    if (!list.empty() && pt == list.back().pt)
        return;

    if (list.empty())
        dim = QRectF(pt.x(), pt.y(), 0., 0.);
    else
    {
        dim.setLeft(std::min(dim.left(), pt.x()));
        dim.setTop(std::min(dim.top(), pt.y()));
        dim.setRight(std::max(dim.right(), pt.x()));
        dim.setBottom(std::max(dim.bottom(), pt.y()));
    }

    StrokePoint p;
    p.pt = pt;
    p.length = 0;
    p.angle = 0;
    if (!list.empty())
    {
        len += (list.back().length = RecMath::len(list.back().pt, pt));
        list.back().angle = RecMath::rad(list.back().pt, pt);
        if (pt != list[0].pt)
            ang = RecMath::rad(list[0].pt, pt);
        else
            ang = 0;

        if (list.size() > 1 && (list.back().angle - std::prev(list.end(), 2)->angle).abs() > const_PI_2 * 0.8)
        {
            //double d = (list.back().angle - std::prev(list.end(), 2)->angle).abs();
            //double m = const_PI_2 * 0.8;
            ++list.back().section;
        }
        p.section = list.back().section;
        sectcnt = p.section + 1;
    }
    else
    {
        len = 0;
        ang = 0;
        sectcnt = 0;
        p.section = 0;
    }

    list.push_back(p);
}

void Stroke::clear()
{
    list.clear();
    len = 0;
    ang = Radian();
    sectcnt = 0;
    dim = QRectF();
}

QRectF Stroke::bounds() const
{
    return dim;
}

double Stroke::width() const
{
    return dim.width();
}

double Stroke::height() const
{
    return dim.height();
}

int Stroke::segmentCount() const
{
    return std::max(0, tosigned(list.size()) - 1);
}

double Stroke::segmentLength(int index) const
{
#ifdef _DEBUG
    if (index < 0 || tosigned(list.size()) < index + 2)
        throw "Index out of range";
#endif

    return list[index].length;
}

Radian Stroke::segmentAngle(int index) const
{
#ifdef _DEBUG
    if (index < 0 || tosigned(list.size()) < index + 2)
        throw "Index out of range";
#endif

    return list[index].angle;
}

ushort Stroke::segmentSection(int index) const
{
#ifdef _DEBUG
    if (index < 0 || tosigned(list.size()) < index + 2)
        throw "Index out of range";
#endif

    return list[index].section;
}

int Stroke::sectionCount() const
{
    return sectcnt;
}

double Stroke::sectionLength(int index) const
{
#ifdef _DEBUG
    if (index < 0 || sectcnt <= index)
        throw "Index out of range.";
#endif

    // Find first segment with section of index.
    int ix;
    int siz = tosigned(list.size());
    for (ix = 0; ix != siz; ++ix)
        if (list[ix].section == index)
            break;

    double r = 0;
    do
    {
        r += list[ix].length;
        ++ix;
    } while (ix != siz && list[ix].section == index);

    return r;
}

int Stroke::segmentsInSection(int index) const
{
    if (list.size() < 2)
        return 0;

    int r = 0;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz - 1; ++ix)
    {
        if (list[ix].section == index)
            ++r;
        else if (list[ix].section > index)
            break;
    }
    return r;
}

double Stroke::length() const
{
    return len;
}

Radian Stroke::angle() const
{
    return ang;
}

int Stroke::compare(const Stroke &other) const
{
    // Computes the distance between this and the other stroke. The comparison is done in two
    // steps. In the first step we look for the possibility of an accidentally drawn hook that
    // should be skipped in the main comparison. The second step is the main comparison.

    int n = other.segmentCount();
    int m = segmentCount();

    double nlen = other.length();
    double mlen = length();

    int ns = -1, ms = -1;

    // Initial matrix width and height.
    int mw = n, mh = m;
    // Initial min matrix width and height.
    int mmw = 0, mmh = 0;

    double wextra = 0;

    double ntmp1 = other.sectionLength(0);
    double ntmp2 = other.sectionLength(other.sectionCount() - 1);
    double mtmp1 = sectionLength(0);
    double mtmp2 = sectionLength(sectionCount() - 1);

    // First section of the other stroke has insignificant length, which might
    // be a "hook".
    bool nstart = other.sectionCount() > 1 && other.sectionLength(0) < std::min(0.2, nlen * 0.18) && other.sectionLength(1) > nlen * 0.18 && ntmp1 < ntmp2;
    // Last section of the other stroke has insignificant length, which might
    // be a "hook".
    bool nend = !nstart && other.sectionCount() > 1 && other.sectionLength(other.sectionCount() - 1) < std::min(0.2, nlen * 0.18) && other.sectionLength(other.sectionCount() - 2) > nlen * 0.18 && ntmp2 < ntmp1;
    // First section of this stroke has insignificant length, which might be a
    // "hook".
    bool mstart = !nend && sectionCount() > 1 && sectionLength(0) < std::min(0.2, mlen * 0.18) && sectionLength(1) > mlen * 0.18 && mtmp1 < mtmp2;
    // Last section of this stroke has insignificant length, which might be a
    // "hook".
    bool mend = !mstart && !nstart && sectionCount() > 1 && sectionLength(sectionCount() - 1) < std::min(0.2, mlen * 0.18) && sectionLength(sectionCount() - 2) > mlen * 0.18 && mtmp2 < mtmp1;
    nstart = nstart && !mend;
    nend = nend && !mstart;

    // Number of segments to skip from the strokes when checking the distance.
    // This value is set if one end of a stroke is determined to be a hook,
    // and the hook distances are separately computed.
    int mx = 0;
    int my = 0;
    // Number of segments to skip from the strokes when checking the distance
    // of hooks.
    int mmx = 0;
    int mmy = 0;

    fastarray<double> matrix;

    if ((nstart && mstart) || (nend && mend))
        nstart = mstart = nend = mend = false;

    if (nstart || mstart)
    {

        //wextra = (!nstart ? mlen : !mstart ? nlen : std::min(nlen, mlen)) * 0.1;
        mmw = other.segmentsInSection(0);
        mmh = segmentsInSection(0);
    }
    if (nend || mend)
    {
        //wextra = (!nend ? mlen : !mend ? nlen : std::min(nlen, mlen)) * 0.1;
        mmw = other.segmentsInSection(other.sectionCount() - 1);
        mmh = segmentsInSection(sectionCount() - 1);

        mmx = n - mmw;
        mmy = m - mmh;
    }
    if (nstart || nend)
        mw = n - mmw;
    if (mstart || mend)
        mh = m - mmh;
    if (nstart)
        mx = mmw;
    if (mstart)
        my = mmh;


    // Using the Levenshtein-distance to check if there's an (unwanted?) hook
    // on one of the strokes.
    if (nstart || nend || mstart || mend)
    {
        wextra = (nlen + mlen) / 20;

        matrix.setSize(mmw * mmh);
        for (int ix = 0; ix != mmw * mmh; ++ix)
        {
            Radian mnsrad = other.segmentAngle(ix % mmw + mmx);
            Radian mmsrad = segmentAngle(ix / mmw + mmy);

            double diff = (mnsrad - mmsrad).abs() * 800;

            int mmxpos = (ix % mmw);
            int mmypos = (ix / mmw);
            int mmix = mmypos * mmw + mmxpos;

            matrix[mmix] = diff;
            if (mmxpos && mmypos)
                matrix[mmix] += std::min(std::min(matrix[mmix - 1], matrix[mmix - mmw]), matrix[mmix - mmw - 1]);
            else if (!mmxpos && mmypos)
                matrix[mmix] += matrix[mmix - mmw];
            else if (mmix)
                matrix[mmix] += matrix[mmix - 1];

        }

        // Backtracking - counting the steps that was needed to reach from the
        // starting matrix position to the end result.
        int msteps = 1;
        int x = mmw - 1, y = mmh - 1;
        while (x > 0 || y > 0)
        {
            msteps++;
            if (x && y)
            {
                if (matrix[(x - 1) + (y - 1) * mmw] < matrix[x + (y - 1) * mmw] && matrix[(x - 1) + (y - 1) * mmw] < matrix[(x - 1) + y * mmw])
                    x--, y--;
                else if (matrix[x + (y - 1) * mmw] < matrix[(x - 1) + y * mmw])
                    y--;
                else
                    x--;
            }
            else if (x)
                x--;
            else
                y--;
        }

        // If the distance of the matched part is too small
        if ((matrix[mmw * mmh - 1] / double(msteps)) < 333.333)
        {
            wextra = 0;
            mw = n;
            mh = m;
            mx = 0;
            my = 0;
        }
    }

    matrix.setSize(mw * mh);

    std::vector<double> nseclen(other.sectionCount(), 0.);
    std::vector<double> mseclen(sectionCount(), 0.);

    // Using the Levenshtein-distance to check distance between the strokes.
    // If a hook was found at one end of a stroke in the previous check, that
    // part is ignored.
    for (int ix = 0; ix < mw * mh; ++ix)
    {
        int mxpos = ix % mw;
        int mypos = ix / mw;
        int npos = mxpos + mx;
        int mpos = mypos + my;

        if (ns != other.segmentSection(npos))
            ns = other.segmentSection(npos);
        if (ms != segmentSection(mpos))
            ms = segmentSection(mpos);

        Radian nsrad = other.segmentAngle(npos);
        Radian msrad = segmentAngle(mpos);

        matrix[ix] = (nsrad - msrad).abs() * 800;

        //uchar mhorz = msrad.horizontal(), nhorz = nsrad.horizontal();
        //uchar mvert = msrad.vertical(), nvert = nsrad.vertical();
        //uchar mdiag = msrad.diagonal(), ndiag = nsrad.diagonal();
        double d = 1.0;

        //if ((ndiag && mdiag && ndiag != mdiag || mvert && nvert && mvert != nvert || mhorz && nhorz && mhorz != nhorz || ndiag && (mhorz || mvert) || mdiag && (nhorz || nvert)))
        //    d *= 1.3;
        //else
        //{
        double &nslen = nseclen[ns];
        if (nslen == 0.)
            nslen = other.sectionLength(ns);
        double &mslen = mseclen[ms];
        if (mslen == 0.)
            mslen = sectionLength(ms);
        // If the sections the segments are in are relatively short and close
        // in size, the distance will mean less.
        if (nslen < nlen * 0.18 && mslen < mlen * 0.18 && std::min(nslen, mslen) / std::max(nslen, mslen) > 0.8)
            d *= 0.5;
        //}
        matrix[ix] *= d;

        if (mxpos && mypos)
            matrix[ix] += std::min(std::min(matrix[ix - 1], matrix[ix - mw]), matrix[ix - mw - 1]);
        else if (!mxpos && mypos)
            matrix[ix] += matrix[ix - mw];
        else if (ix)
            matrix[ix] += matrix[ix - 1];
    }

    double res = matrix[mw * mh - 1];

    // Backtrack from the bottom right of the matrix to the starting (0, 0)
    // position to count the number of steps.

    // Values at each step.
    std::vector<double> values;
    int x = mw - 1, y = mh - 1;

    int steps = 1;

    while (x > 0 || y > 0)
    {
        values.push_back(matrix[x + y * mw]);

        steps++;
        if (x && y)
        {
            if (matrix[(x - 1) + (y - 1) * mw] < matrix[x + (y - 1) * mw] && matrix[(x - 1) + (y - 1) * mw] < matrix[(x - 1) + y * mw])
                x--, y--;
            else if (matrix[x + (y - 1) * mw] < matrix[(x - 1) + y * mw])
                y--;
            else
                x--;
        }
        else if (x)
            x--;
        else
            y--;

        values.back() -= matrix[x + y * mw];
    }
    values.push_back(matrix[0]);

    // The average of the values.
    double avg = res / steps;

    // The average distance of values from the average.
    double avgdif = 0;
    for (int ix = 0; ix != steps; ++ix)
        avgdif = std::fabs(values[ix] - avg);
    avgdif /= steps;


    double avgdifdif = 0;
    for (int ix = 0; ix != steps; ++ix)
        avgdifdif += std::fabs(avgdif - std::fabs(values[ix] - avg));
    //res += avgdifdif;

    //double sdif = double(std::min(m, n)) / double(std::max(m, n));
    //if (sdif < 0.7)
    //    wextra += std::min(20000., 3200. * (0.7 / sdif));

    res = (res / steps) * 10.0 + wextra * 1000;

    return res;
}

//QPointF& Stroke::operator[](int index)
//{
//    return list[index];
//}

const QPointF& Stroke::operator[](int index) const
{
    return list[index].pt;
}

void Stroke::splitPoints(QPointF *pt, int cnt) const
{
#ifdef _DEBUG
    if (cnt < 0)
        throw "bad param.";
    if (cnt > 0 && list.size() < 2)
        throw "Stroke not ready for splitting";
#endif

    if (!cnt)
        return;
    pt[0] = list[0].pt;
    if (cnt == 1)
        return;

    // Length of a single line section if the line is cut to cnt equal sizes.
    double sect = len / (cnt - 1);
    // Current distance from the last added point in pt, where a new point
    // should be added.
    double current = sect;
    // Point position in the passed points array.
    int pos = 1;
    // Point position in stroke.
    int apos = 0;
    int lsiz = tosigned(list.size());

    double l;
    while (apos != lsiz - 1 && pos != cnt - 1)
    {
        l = list[apos].length;

        QPointF dif = list[apos + 1].pt - list[apos].pt;
        while (current <= l && pos != cnt - 1)
        {
            double mul = current / l;
            pt[pos] = QPointF(list[apos].pt.x() + dif.x() * mul, list[apos].pt.y() + dif.y() * mul);
            pos++;
            current += sect;
        }

        current -= l;
        apos++;
    }

    // Take rounding errors into consideration and assume we haven't reached
    // the end of the array. (Taken from the old code, not sure if this is needed.)
    while (pos != cnt - 1)
    {
        pt[pos] = pt[pos - 1];
        pos++;
    }

    pt[cnt - 1] = list.back().pt;
}

//-------------------------------------------------------------

StrokeList::StrokeList()
{

}

void StrokeList::add(Stroke &&s, bool mousedrawn)
{
    Stroke ss(simplify(std::move(s), mousedrawn));
    list.push_back(std::make_pair(ss, dissect(ss)));
    if (dim.isEmpty())
        dim = ss.bounds();
    else
        dim = dim.united(ss.bounds());
    cmplist.push_back(new RecognizerComparisons);
    ZKanji::elements()->compareToModels(list.back().second, *cmplist.back());

    poslist.setSize(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        poslist[ix].setSize(list.size() - 1);
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        for (int iy = 0; iy != siz; ++iy)
        {
            if (iy == ix)
                continue;
            ZKanji::elements()->computeStrokePositions(poslist[ix][iy > ix ? iy - 1 : iy], list[ix].first.bounds(), list[iy].first.bounds(), QRectF(0, 0, std::max(dim.width(), 0.65), std::max(dim.height(), 0.65)));
            ZKanji::elements()->computeStrokePositions(poslist[iy][ix > iy ? ix - 1 : ix], list[iy].first.bounds(), list[ix].first.bounds(), QRectF(0, 0, std::max(dim.width(), 0.65), std::max(dim.height(), 0.65)));
        }
    }
}

bool StrokeList::empty() const
{
    return list.empty();
}

void StrokeList::resize(int newsize)
{
    if (newsize < 0 || newsize >= tosigned(list.size()))
        return;
    list.resize(newsize);
}

StrokeList::size_type StrokeList::size() const
{
    return list.size();
}

void StrokeList::clear()
{
    list.clear();
    cmplist.clear();
    poslist.clear();
    dim = QRectF();
}

QRectF StrokeList::bounds() const
{
    return dim;
}

double StrokeList::width() const
{
    return dim.width();
}

double StrokeList::height() const
{
    return dim.height();
}

//void StrokeList::reCompare()
//{
//    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
//    {
//        cmplist[ix]->clear();
//        ZKanji::elements()->compareToModels(list[ix].second, *cmplist[ix]);
//    }
//}

//Stroke& StrokeList::items(int index)
//{
//    return list[index].second;
//}
//
//const Stroke& StrokeList::items(int index) const
//{
//    return list[index].second;
//}
//
//Stroke& StrokeList::operator[](int index)
//{
//    return list[index].second;
//}

const Stroke& StrokeList::operator[](int index) const
{
    return list[index].first;
}

const RecognizerComparisons& StrokeList::cmpItems(int index) const
{
    return *cmplist[index];
}

const fastarray<BitArray>& StrokeList::posItems(int index) const
{
    return poslist[index];
}

// Structure used in the simplification of strokes. Holds a point in the
// stroke, its distance to the next point and their angle to the x axis.
struct PointRadLen
{
    QPointF p;
    Radian rad;
    double dist;

    PointRadLen(const QPointF &p) : p(p), dist(0) { ; }
};

Stroke&& StrokeList::simplify(Stroke &&s, bool mousedrawn)
{
#ifdef _DEBUG
    if (s.empty())
        throw "No points in stroke";
#endif

    // Distance in a drawing surface with sides of 1, which is considered insignificant. If
    // the mouse is used for drawing the stroke, this distance must be larger due to hand
    // shake.
    const double tiny_dist = mousedrawn ? 0.00725 : 0.00522;

    // Construct a vector from the points of s. Each value holds a point, its distance from
    // the next point, and their angle to the x axis.
    std::vector<PointRadLen> points;
    for (int ix = 0, siz = tosigned(s.size()); ix != siz; ++ix)
    {
        if (ix != 0)
        {
            PointRadLen &pt = points.back();
            pt.dist = s.segmentLength(ix - 1);
            pt.rad = s.segmentAngle(ix - 1);
        }

        points.emplace_back(s[ix]);
    }

    // Remove unnecessary points from the points list that are near the stroke
    // but don't significantly influence its shape.

    // Step 1: join points that are very close to each other.
    while (points.size() > 2)
    {
        // The starting point of a segment whose points will be joined. This
        // will be the index of the shortest segment.
        int m = 0;
        // Length of segment m.
        double mdist = 0;

        // Find the shortest segment of the stroke.
        for (int ix = 0, siz = tosigned(points.size()); ix != siz - 2; ++ix)
        {
            if (ix == 0 || points[ix].dist < mdist)
            {
                mdist = points[ix].dist;
                m = ix;
            }
        }

        // Every segment is larger than the join limit.
        if (mdist > tiny_dist * 3 || m == tosigned(points.size()) - 2)
            break;

        // Position in [0, 1] between points m and m + 1 where the two points
        // will be joined.
        double balance = 0;

        if (m)
        {
            double nextrad = (points[m].rad - points[m + 1].rad).abs();
            double thisrad = (points[m - 1].rad - points[m].rad).abs();

            if (Radian(thisrad) < Radian(const_PI_2 * 0.83) || ((m <= 1 || (points[m - 2].rad - points[m - 1].rad).abs() >= const_PI_4) && nextrad >= const_PI_4))
                balance = std::min(std::max(0.5 + (nextrad - thisrad) / ((2 * const_PI) / 3), 0.0), 1.0);
        }

        // Join points m and m + 1 by updating m and removing m + 1.
        points[m].p = QPointF(points[m].p.x() + (points[m + 1].p.x() - points[m].p.x()) * balance, points[m].p.y() + (points[m + 1].p.y() - points[m].p.y()) * balance);
        points.erase(points.begin() + (m + 1));


        // Very unlikely, but there might be points at the same position after the join.
        if (m + 1 != tosigned(points.size()) && points[m].p == points[m + 1].p)
            points.erase(points.begin() + (m + 1));

        // Recompute the values of point m so they reflect the real radian
        // and distance to the next point.
        points[m].rad = m + 1 != tosigned(points.size()) ? RecMath::rad(points[m].p, points[m + 1].p) : Radian();
        points[m].dist = m + 1 != tosigned(points.size()) ? RecMath::len(points[m].p, points[m + 1].p) : 0;

        if (m)
        {
            // Very unlikely, but there might be points at the same position
            // after the join.
            if (points[m - 1].p == points[m].p)
                points.erase(points.begin() + m);

            // Recompute the values of point m - 1 so they reflect the real
            // radian and distance to the next point.
            points[m - 1].rad = m != tosigned(points.size()) ? RecMath::rad(points[m - 1].p, points[m].p) : Radian();
            points[m - 1].dist = m != tosigned(points.size()) ? RecMath::len(points[m - 1].p, points[m].p) : 0;
        }
    }

    // Step 2: remove points that are very close to a straight line between
    // points ix and ix + 2.

    bool changed = true;
    while (changed)
    {
        changed = false;

        for (int ix = 0, siz = tosigned(points.size()); ix < siz - 2; ++ix)
        {
            double max_dist = tiny_dist;

            if (std::min(points[ix].dist, points[ix + 1].dist) > tiny_dist * 2.)
                max_dist *= std::max(0.5, std::min(points[ix].dist, points[ix + 1].dist) / (tiny_dist * 16.));

            if (RecMath::pointDist(points[ix].p, points[ix + 2].p, points[ix + 1].p) < max_dist / 2.)
            {
                points.erase(points.begin() + (ix + 1));
                --siz;
                points[ix].rad = RecMath::rad(points[ix].p, points[ix + 1].p);
                points[ix].dist = RecMath::len(points[ix].p, points[ix + 1].p);
                --ix;
                changed = true;
            }
        }
    }


    if (points.size() <= 1)
    {
        if (!s.empty())
        {
            points.clear();
            points.emplace_back(s[0]);

            points.emplace_back(QPointF(s[0].x() + 0.25, s[0].y() + 0.25));
        }
#ifdef _DEBUG
        else
            throw "No points left in stroke";
#endif
    }


    s.clear();
    for (int ix = 0, siz = tosigned(points.size()); ix != siz; ++ix)
        s.add(points[ix].p);

    return std::move(s);
}

Stroke StrokeList::dissect(const Stroke &src)
{
    // Number of points to keep between important corner points.
    const int steps = 3;

    // The source stroke is cut up to chkcnt number of points at equal
    // distances. These points might not include the original points of the
    // source, but we only need an approximated stroke that's usable for good
    // comparison.
    int chkcnt = steps * 60 + 1;
    // Temporary array for points that are checked whether to include them in
    // the dissected stroke.
    fastarray<QPointF> pp(chkcnt);
    // Temporary array for points between decidedly important corner points.
    fastarray<QPointF> pp_t(steps + 1);

    // Contains indexes of points in pp that are considered to be included in
    // dest.
    std::vector<int> l;

    l.push_back(0);

    const int divver = 201;
    const double lenmul = 0.037 / ((double)chkcnt / divver);
    const double summul = 0.8 * ((double)chkcnt / divver);
    const double segmul = 0.4;
    const double steepness = 1.46;

    // Returns an array of chkcnt number of points at equal distances on the
    // source stroke.
    src.splitPoints(pp.data(), chkcnt);

    // Length of a single segment in pp.
    double seg = src.length() / (chkcnt - 1);
    // Direction of the last used segment in pp (starting from 0). This is
    // compared with other segments to see if there was a bigger change in
    // direction in the stroke.
    QPointF d = pp[1] - pp[0];
    // Gets larger after every segment which points to a different direction,
    // to have a general idea about the direction change in the stroke since
    // the last added segment.
    double dsum = 0;
    double len = seg;
    int last = -1;

    for (int ix = 1; ix != chkcnt - 1; ++ix)
    {
        QPointF n = pp[ix + 1] - pp[ix];

        // If di is large enough, the difference in direction between the
        // last used and the ix segments is large enough to include the last
        // point in the destination stroke.
        double di = fabs(d.x() - n.x()) + fabs(d.y() - n.y());

        if (last == -1 && di > seg * steepness)
        {
            // Value of di was large enough. The segment starting at ix is
            // considered to be added to dest.
            last = ix;
            dsum = di;
        }
        else if (last > 0 && dsum > len * lenmul)
        {
            l.push_back(last);
            d = n;
            len = 0;
            last = -1;
            dsum = 0;
        }
        else if (last > 0)
            dsum = dsum * summul + di;

        len += seg;

        // The general direction change of the stroke after the last added
        // segment is not large enough. The last segment considered for
        // inclusion had a local direction change but later segments didn't.
        if (last > 0 && dsum < (ix - last) * seg * segmul)
            last = -1;
    }

    l.push_back(chkcnt - 1);


    Stroke dest;
    Stroke dummy;

    // The l vector contains indexes in pp for points to be included in dest.
    // Adding 'steps' number of internal steps between them and filling dest
    // with the result.
    for (int ix = 0, siz = tosigned(l.size()); ix != siz - 1; ++ix)
    {
        dummy.clear();
        for (int iy = l[ix]; iy <= l[ix + 1]; ++iy)
            dummy.add(pp[iy]);

        dummy.splitPoints(pp_t.data(), steps + 1);
        for (int iy = !dest.empty() ? 1 : 0; iy <= steps; ++iy)
            dest.add(pp_t[iy]);
    }

    return dest;
}


//-------------------------------------------------------------


bool surroundingPattern(KanjiPattern p)
{
    return p == KanjiPattern::Ebl || p == KanjiPattern::Eblt || p == KanjiPattern::Elt || p == KanjiPattern::Eltr ||
        p == KanjiPattern::Etr || p == KanjiPattern::Erbl || p == KanjiPattern::Ew || p == KanjiPattern::Elr;
}


//-------------------------------------------------------------

ElementTransform::ElementTransform() : htrans(0), vtrans(0), hscale(1.0), vscale(1.0)
{

}

ElementTransform::ElementTransform(int htrans, int vtrans, double hscale, double vscale) : htrans(htrans), vtrans(vtrans), hscale(hscale), vscale(vscale)
{

}

ElementPointT ElementTransform::transformed(const ElementPoint &pt) const
{
    ElementPointT r;
    r.type = pt.type;
    r.x = (pt.x + htrans) * hscale;
    r.y = (pt.y + vtrans) * vscale;
    r.c1x = (pt.c1x + htrans) * hscale;
    r.c1y = (pt.c1y + vtrans) * vscale;
    r.c2x = (pt.c2x + htrans) * hscale;
    r.c2y = (pt.c2y + vtrans) * vscale;

    return r;
}


//-------------------------------------------------------------


KanjiElementList::KanjiElementList() : version(0)
{

}

KanjiElementList::KanjiElementList(const QString &filename) : version(0)
{
    load(filename);
}

void KanjiElementList::clear(bool /*full*/)
{
    list.clear();
    models.clear();
    cmodels.clear();
    repos.clear();
    varnames.clear();

}

void KanjiElementList::load(const QString &filename)
{
    clear(true);

    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly))
        return;

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    char tmp[9];
    stream.readRawData(tmp, 8);
    tmp[8] = 0;

    bool good = !strncmp("zksod", tmp, 5);

    if (!good)
        throw ZException("Invalid stroke order file format.");

    version = strtol(tmp + 5, 0, 10);

    if (version < 7)
        throw ZException("Stroke order data version too old.");

    quint16 cnt;
    stream >> cnt;

    list.reserve(cnt);
    int lsiz = 0;
    while (lsiz++ != cnt)
        list.push_back(new KanjiElement);


    quint16 ui;
    qint16 si;

    for (int ix = 0; ix != cnt; ++ix)
    {
        KanjiElement *e = list[ix];
        stream >> e->owner;

        if (e->owner < 0x2000)
        {
            if (e->owner > ZKanji::kanjicount)
                throw ZException("Invalid kanji element owner.");
            e->unicode = 0;
            kanjimap.insert(std::make_pair(e->owner, ix));
        }
        else
        {
            if (e->owner == 0x2000)
                e->unicode = 0;
            else
                e->unicode = e->owner;
            e->owner = (ushort)-1;
        }

        qint32 i;
        stream >> i;
        if (i < 0 || i >= (int)KanjiPattern::Count)
            throw ZException("Invalid element pattern.");

        e->pattern = (KanjiPattern)i;

        quint16 pcnt;
        stream >> pcnt;
        e->parents.resize(pcnt);
        for (int iy = 0; iy != pcnt; ++iy)
            stream >> e->parents[iy];
        for (int iy = 0; iy != 4; ++iy)
            stream >> e->parts[iy];

        quint8 vcnt;
        stream >> vcnt;
        e->variants.reserve(vcnt);
        for (int iy = 0; iy != vcnt; ++iy)
        {
            ElementVariant *v = new ElementVariant;
            e->variants.push_back(v);

            // Stroke count.
            stream >> v->strokecnt;

            stream >> ui;
            v->width = ui * 5;
            stream >> ui;
            v->height = ui * 5;
            stream >> si;
            v->x = si * 5;
            stream >> si;
            v->y = si * 5;

            stream >> v->centerpoint;
            stream >> v->standalone;

            if (!v->standalone)
            {
                v->partpos.resize(4);
                for (int j = 0; j != 4; ++j)
                {
                    ElementPart &p = v->partpos[j];
                    stream >> p.variant;

                    stream >> ui;
                    p.width = ui * 5;
                    stream >> ui;
                    p.height = ui * 5;
                    stream >> si;
                    p.x = si * 5;
                    stream >> si;
                    p.y = si * 5;
                }
            }
            else if (v->strokecnt)
            {
                v->strokes.resize(v->strokecnt);
                for (int j = 0; j != v->strokecnt; ++j)
                {
                    ElementStroke &s = v->strokes[j];
                    quint8 ptcnt;
                    stream >> ptcnt;
                    s.points.resize(ptcnt);
                    for (int k = 0; k != ptcnt; ++k)
                    {
                        ElementPoint &ep = s.points[k];
                        stream >> si;
                        ep.x = si * 5;
                        stream >> si;
                        ep.y = si * 5;

                        stream >> si;
                        ep.c1x = si * 5;
                        stream >> si;
                        ep.c1y = si * 5;
                        stream >> si;
                        ep.c2x = si * 5;
                        stream >> si;
                        ep.c2y = si * 5;

                        stream >> i;
                        ep.type = (ElementPoint::Type)i;
                    }

                    quint8 b;
                    stream >> b;
                    OldStrokeTips ot = (OldStrokeTips)b;
                    switch (ot)
                    {
                    case OldStrokeTips::NormalTip:
                    case OldStrokeTips::EndPointed:
                    case OldStrokeTips::EndThin:
                        s.tips = (int)StrokeTips::NormalTip;
                        break;
                    case OldStrokeTips::StartPointed:
                    case OldStrokeTips::StartPointedEndPointed:
                    case OldStrokeTips::StartPointedEndThin:
                        s.tips = (int)StrokeTips::StartPointed;
                        break;
                    case OldStrokeTips::StartThin:
                    case OldStrokeTips::StartThinEndPointed:
                    case OldStrokeTips::StartThinEndThin:
                        s.tips = (int)StrokeTips::StartThin;
                        break;
                    case OldStrokeTips::SingleDot:
                        s.tips = (int)StrokeTips::SingleDot;
                        break;
                    }

                    switch (ot)
                    {
                    case OldStrokeTips::NormalTip:
                    case OldStrokeTips::StartPointed:
                    case OldStrokeTips::StartThin:
                    case OldStrokeTips::SingleDot:
                        break;
                    case OldStrokeTips::EndPointed:
                    case OldStrokeTips::StartPointedEndPointed:
                    case OldStrokeTips::StartThinEndPointed:
                        s.tips |= (int)StrokeTips::EndPointed;
                        break;
                    case OldStrokeTips::EndThin:
                    case OldStrokeTips::StartPointedEndThin:
                    case OldStrokeTips::StartThinEndThin:
                        s.tips |= (int)StrokeTips::EndThin;
                        break;
                    }
                }
            }
        }

        if ((e->owner != (ushort)-1 || e->unicode != 0) && !e->variants.empty() && e->variants[0]->strokecnt != 0)
        {
            e->recdata.resize(e->variants[0]->strokecnt);
            for (int iy = 0; iy != e->variants[0]->strokecnt; ++iy)
                e->recdata[iy].pos.setSize(e->variants[0]->strokecnt - 1);
        }

        if (!e->recdata.empty())
        {
            for (int iy = 0; iy != e->variants[0]->strokecnt; ++iy)
            {
                stream >> e->recdata[iy].data.index;
                stream >> e->recdata[iy].data.distance;

                for (int j = 0; j != e->variants[0]->strokecnt - 1; ++j)
                    e->recdata[iy].pos[j].load(stream);
            }
        }
    }

    loadModels(stream, models);
    loadModels(stream, cmodels);

    loadRepos(stream);

    loadVariantNames(stream);
}

int KanjiElementList::elementOf(int kindex) const
{
    if (kindex < 0)
    {
        for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
            if (list[ix]->unicode == -kindex)
                return ix;
        return -1;
    }
    if (ZKanji::kanjis.empty())
    {
        auto it = kanjimap.find(kindex);
        if (it == kanjimap.end())
            return -1;
        return it->second;
    }
    return ZKanji::kanjis[kindex]->element;
}

void KanjiElementList::applyElements()
{
    if (ZKanji::kanjis.empty())
        return;
    for (const auto &p : kanjimap)
        ZKanji::kanjis[p.first]->element = p.second;
    kanjimap.clear();
}

int KanjiElementList::modelCount() const
{
    return tosigned(models.size() + cmodels.size());
}

void KanjiElementList::compareToModels(const Stroke &stroke, RecognizerComparisons &result)
{
    result.setSize(models.size() + cmodels.size());
    int ssiz = tosigned(models.size());
    for (int ix = 0; ix != ssiz; ++ix)
    {
        //RecognizerComparison cmp;
        result[ix].index = ix;
        result[ix].distance = models[ix].compare(stroke);
    }

    for (int ix = 0, siz = tosigned(cmodels.size()); ix != siz; ++ix)
    {
        //RecognizerComparison cmp;
        result[ssiz + ix].index = ssiz + ix;
        result[ssiz + ix].distance = cmodels[ix].compare(stroke);
    }
}

/*Positions:
*                 0
*                 4
*                 8
*                12
* 1  5  9  13          14 10 6  2
*                15
*                11
*                 7
*                 3
*/
void KanjiElementList::computeStrokePositions(BitArray &bits, QRectF orig, QRectF comp, const QRectF &fullsize)
{
    double hivalue = 100;
    double lovalue = fullsize.width() < 0.5 && fullsize.height() < 0.5 ? 0.015 : 0.03;

    double loxo = orig.width() > 0.1 ? 0. : orig.width() > 0.03 ? 0.005 : lovalue;
    double loyo = orig.height() > 0.1 ? 0. : orig.height() > 0.03 ? 0.005 : lovalue;
    double loxc = comp.width() > 0.1 ? 0. : comp.width() > 0.03 ? 0.005 : lovalue;
    double loyc = comp.height() > 0.1 ? 0. : comp.height() > 0.03 ? 0.005 : lovalue;

    orig.adjust(-loxo, -loyo, loxo, loyo);
    comp.adjust(-loxc, -loyc, loxc, loyc);

    double mnw = std::min(orig.width(), comp.width());
    double mnh = std::min(orig.height(), comp.height());
    double ow = orig.width();
    double oh = orig.height();

    double d = ((double)std::max(fullsize.width(), fullsize.height()) / 6.0);

    bits.setSize(16, false);
    //bits.checkall(16, false);

    QRectF r;

    r = QRectF(QPointF(-hivalue, -hivalue), QPointF(hivalue, orig.top() - std::max(d * 0.63, oh * 0.10)));
    if (comp.intersects(r))
        bits.set(0, true);
    r = QRectF(QPointF(-hivalue, -hivalue), QPointF(orig.left() - std::max(d * 0.63, ow * 0.10), hivalue));
    if (comp.intersects(r))
        bits.set(1, true);
    r = QRectF(QPointF(orig.right() + std::max(d * 0.63, ow * 0.10), -hivalue), QPointF(hivalue, hivalue));
    if (comp.intersects(r))
        bits.set(2, true);
    r = QRectF(QPointF(-hivalue, orig.bottom() + std::max(d * 0.63, oh * 0.10)), QPointF(hivalue, hivalue));
    if (comp.intersects(r))
        bits.set(3, true);

    r = QRectF(QPointF(-hivalue, orig.top() - std::max(d * 0.65, oh * 0.11)), QPointF(hivalue, orig.top() - std::min(d * 0.3, mnh * 0.5)));
    if (comp.intersects(r))
        bits.set(4, true);
    r = QRectF(QPointF(orig.left() - std::max(d * 0.65, ow * 0.11), -hivalue), QPointF(orig.left() - std::min(d * 0.3, mnw * 0.5), hivalue));
    if (comp.intersects(r))
        bits.set(5, true);
    r = QRectF(QPointF(orig.right() + std::min(d * 0.3, mnw * 0.5), -hivalue), QPointF(orig.right() + std::max(d * 0.65, ow * 0.11), hivalue));
    if (comp.intersects(r))
        bits.set(6, true);
    r = QRectF(QPointF(-hivalue, orig.bottom() + std::min(d * 0.3, mnh * 0.5)), QPointF(hivalue, orig.bottom() + std::max(d * 0.65, oh * 0.11)));
    if (comp.intersects(r))
        bits.set(7, true);

    r = QRectF(QPointF(-hivalue, orig.top() - std::min(d * 0.35, mnh * 0.55)), QPointF(hivalue, orig.top() + std::min(d, oh * 0.2)));
    if (comp.intersects(r))
        bits.set(8, true);
    r = QRectF(QPointF(orig.left() - std::min(d * 0.35, mnw * 0.55), -hivalue), QPointF(orig.left() + std::min(d, ow * 0.2), hivalue));
    if (comp.intersects(r))
        bits.set(9, true);
    r = QRectF(QPointF(orig.right() - std::min(d, ow * 0.2), -hivalue), QPointF(orig.right() + std::min(d * 0.35, mnw * 0.55), hivalue));
    if (comp.intersects(r))
        bits.set(10, true);
    r = QRectF(QPointF(-hivalue, orig.bottom() - std::min(d, oh * 0.2)), QPointF(hivalue, orig.bottom() + std::min(d * 0.35, mnh * 0.55)));
    if (comp.intersects(r))
        bits.set(11, true);

    r = QRectF(QPointF(-hivalue, orig.top() + std::min(d * 0.9, oh * 0.18)), QPointF(hivalue, orig.top() + oh * 0.55));
    if (comp.intersects(r))
        bits.set(12, true);
    r = QRectF(QPointF(orig.left() + std::min(d * 0.9, ow * 0.18), -hivalue), QPointF(orig.left() + ow * 0.55, hivalue));
    if (comp.intersects(r))
        bits.set(13, true);
    r = QRectF(QPointF(orig.left() + ow * 0.55, -hivalue), QPointF(orig.right() - std::min(d * 0.9, ow * 0.18), hivalue));
    if (comp.intersects(r))
        bits.set(14, true);
    r = QRectF(QPointF(-hivalue, orig.top() + oh * 0.55), QPointF(hivalue, orig.bottom() - std::min(d * 0.9, ow * 0.18)));
    if (comp.intersects(r))
        bits.set(15, true);
}

void KanjiElementList::findCandidates(const StrokeList &strokes, std::vector<int> &result, int strokecnt, bool kanji, bool kana, bool other)
{
    // Number of items to include in result at most.
    const int cntlimit = 256;
    // Drawn stroke order can be different for each stroke by swplimit position.
    const int swplimit = 1;

    if (strokecnt == -1)
        strokecnt = tosigned(strokes.size());

    //int modelsize = models.size();

    int hasrec = 0;
    int charrec = 0;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        KanjiElement *e = list[ix];
        if (kanji && !e->recdata.empty() && e->owner != (ushort)-1)
            hasrec++;
        else if (!e->recdata.empty() && e->unicode != 0 && ((KANA(e->unicode) && kana) || (VALIDCODE(e->unicode) && other)))
            charrec++;
    }

    int cnt = hasrec + charrec;

    fastarray<RecognizerComparison> value(cnt);
    for (int ix = 0; ix < cnt; ++ix)
    {
        value[ix].index = (ushort)-1;
        value[ix].distance = 2147483647;
    }

    int pos = 0;
    int lowest = 999999;

    int used[255];
    try
    {
        for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        {
            KanjiElement *e = list[ix];

            if (e->recdata.empty() || (e->owner != (ushort)-1 && !kanji) || ((cntlimit >= 0 && abs(e->variants[0]->strokecnt - strokecnt) > cntlimit) || (!cntlimit && e->variants[0]->strokecnt < std::max(1, std::min(strokecnt - 3, strokecnt / 2)))) || (e->unicode != 0 && ((KANA(e->unicode) && !kana) || (VALIDCODE(e->unicode) && !other) || (!other && !kana) )))
                continue;

            memset(used, -1, sizeof(int) * 255);
            value[pos].distance = std::max(0, strokecnt - e->variants[0]->strokecnt) * 40000;
            value[pos].index = ix;
            for (int iy = 0; iy < std::min(e->variants[0]->strokecnt + swplimit, strokecnt) && value[pos].distance < std::max(10000, lowest) * 1.5; ++iy)
            {
                int distmin = -1;
                int sindex = -1;
                for (int k = iy - swplimit; k < iy + swplimit + 1; ++k)
                {
                    if (k < 0 || k >= e->variants[0]->strokecnt || (used[k] >= 0 && (k == 0 || k != iy || used[k - 1] >= 0)))
                        continue;
                    double dval;

                    dval = strokes.cmpItems(iy)[e->recdata[k].data.index].distance / 2.;

                    dval += abs(k - iy) * 300;

                    if (distmin < 0 || distmin > dval)
                    {
                        distmin = dval;
                        sindex = k;
                    }
                }
                if (distmin < 0)
                    distmin = 0;
                else
                {
                    if (used[sindex] >= 0 && used[sindex - 1] < 0)
                        used[sindex - 1] = sindex - 1;
#ifdef _DEBUG
                    else if (used[sindex] >= 0)
                        throw "?";
#endif
                    used[sindex] = iy;
                }
                value[pos].distance += std::min(100000, distmin);
            }

            int compdist = std::max(3000, value[pos].distance);

            int n = std::min(strokecnt, (int)e->variants[0]->strokecnt);
            double posw;
            for (int iy = 0; iy < n && value[pos].distance < std::max(10000, lowest) * 1.5; ++iy)
            {
                int c = used[iy];
                if (c < 0)
                {
                    value[pos].distance += (double)compdist * 0.2 * 196 / n; //maximum pos difference. this should be changed if difference changes
                    continue;
                }

                int c2;
                double dval;

                for (int iz = iy + 1; iz < std::min(n, iy + 3); ++iz)
                {
                    c2 = used[iz];
                    if (c2 >= 0)
                    {
                        posw = iz - iy == 1 ? 0.09 : 0.03;

                        dval = ((double)(posDiff(e->recdata[iy].pos[iz - 1], strokes.posItems(c)[c2 - (c2 > c ? 1 : 0)])) * ((double)compdist * posw)) / n;
                        if (swplimit > 0 && iz == iy + 1 && c == iy && c2 == iz && e->recdata[iy].data.index == e->recdata[iz].data.index)
                        {
                            double dtmp = ((double)(posDiff(e->recdata[iy].pos[iz - 1], strokes.posItems(c2)[c])) * ((double)compdist * posw)) / n;
                            if (dtmp < dval)
                            {
                                dval = dtmp;
                                int k = c;
                                c = used[iy] = c2;
                                used[iz] = k;
                            }
                        }
                        value[pos].distance += dval;
                    }
                }
                for (int iz = iy - 1; iz >= std::max(0, iy - 2); --iz)
                {
                    c2 = used[iz];
                    if (c2 >= 0)
                    {
                        posw = iy - iz == 1 ? 0.09 : 0.03;

                        dval = ((double)(posDiff(e->recdata[iy].pos[iz], strokes.posItems(c)[c2 - (c2 > c ? 1 : 0)])) * ((double)compdist * posw)) / n;
                        value[pos].distance += dval;
                    }
                }

            }

            if (e->variants[0]->width < 5000 && e->variants[0]->height < 5000)
            {
                if (strokes.width() < 0.35 && strokes.height() < 0.35)
                    value[pos].distance = std::max(0.0, value[pos].distance - compdist * 0.05);
                else if (strokes.width() > 0.5 || strokes.height() > 0.5)
                    value[pos].distance += compdist * 0.05;
            }
            else if (e->variants[0]->width > 5000 && e->variants[0]->height > 5000)
            {
                if (strokes.width() < 0.35 && strokes.height() < 0.35)
                    value[pos].distance += compdist * 0.05;
                else if (strokes.width() > 0.5 || strokes.height() > 0.5)
                    value[pos].distance = std::max(0.0, value[pos].distance - compdist * 0.05);
            }

            if (e->variants[0]->strokecnt > strokecnt)
            {
                int d = std::min(4, e->variants[0]->strokecnt - strokecnt);
                value[pos].distance += d * 2500 + std::min((d - 1) * 3333, 10000);
            }

            if (value[pos].distance >= std::max(10000, lowest) * 1.5)
            {

                if (lowest > value[pos].distance)
                {
                    lowest = value[pos].distance;
                }
                else
                {
                    value[pos].index = (ushort)-1;
                    value[pos].distance = 2147483647;
                }
            }
            else
            {

                if (lowest > value[pos].distance)
                    lowest = value[pos].distance;

                pos++;
            }

        }
    }
    catch (...)
    {
        ;
    }
    std::sort(value.data(), value.data() + value.size(), [](const RecognizerComparison &a, const RecognizerComparison &b) {
        return a.distance < b.distance;
    });

    result.clear();
    result.reserve(value.size());
    for (int ix = 0; ix < cnt; ++ix)
    {
        if (value[ix].index == (ushort)-1)
        {
            cnt = ix;
            break;
        }
        result.push_back(value[ix].index);
    }
}

KanjiElementList::size_type KanjiElementList::size() const
{
    return list.size();
}

KanjiEntry* KanjiElementList::itemKanji(int index) const
{
    ushort o = list[index]->owner;
    if (o == (ushort)-1)
        return nullptr;
    return ZKanji::kanjis[o];
}

int KanjiElementList::itemKanjiIndex(int index) const
{
    ushort o = list[index]->owner;
    if (o == (ushort)-1)
        return -1;
    return o;
}

QChar KanjiElementList::itemUnicode(int index) const
{
    KanjiEntry *k = itemKanji(index);
    if (k == nullptr)
        return list[index]->unicode;
    return k->ch.unicode();
}

void KanjiElementList::elementParts(int index, bool skipelements, bool usekanji, std::vector<int> &result) const
{
    const KanjiElement *e = list[index];

    uint resultpos = 0;

    std::set<int> found;

    while (e)
    {
        for (int ix = 0; ix != 4; ++ix)
        {
            if (e->parts[ix] != -1 && found.count(e->parts[ix]) == 0)
            {
                result.push_back(e->parts[ix]);
                found.insert(e->parts[ix]);
            }
        }

        if (resultpos != result.size())
            e = list[result[resultpos++]];
        else
            e = nullptr;
    }

    if (skipelements)
    {
        result.resize(std::remove_if(result.begin(), result.end(), [this](int val) {
            return list[val]->owner == (ushort)-1;
        }) - result.begin());
    }

    if (usekanji)
    {
        for (int &i : result)
            if (list[i]->owner == (ushort)-1)
                i = -1 - i;
            else
                i = list[i]->owner;
    }
}

void KanjiElementList::elementParents(int index, bool skipelements, bool usekanji, std::vector<int> &result) const
{
    const KanjiElement *e = list[index];

    uint resultpos = 0;
    std::set<int> found;

    while (e)
    {
        for (int ix = 0, siz = tosigned(e->parents.size()); ix != siz; ++ix)
        {
            int parent = e->parents[ix];
            if (found.count(parent) == 0)
            {
                result.push_back(parent);
                found.insert(parent);
            }
        }

        if (resultpos != result.size())
            e = list[result[resultpos++]];
        else
            e = nullptr;
    }

    if (skipelements)
    {
        result.resize(std::remove_if(result.begin(), result.end(), [this](int val) {
            return list[val]->owner == (ushort)-1;
        }) - result.begin());
    }

    if (usekanji)
    {
        for (int &i : result)
            if (list[i]->owner == (ushort)-1)
                i = -1 - i;
            else
                i = list[i]->owner;
    }
}

void KanjiElementList::drawElement(QPainter &p, int element, int variant, const QRectF &rect, bool animated)
{
    int cnt = tosigned(strokeCount(element, variant));

    bool antialiased = p.renderHints().testFlag(QPainter::Antialiasing);
    if (!antialiased)
        p.setRenderHint(QPainter::Antialiasing);
    for (int ix = 0; ix != cnt; ++ix)
        drawStroke(p, element, variant, ix, rect, animated ? 0.0 : -1.0);
    if (!antialiased)
        p.setRenderHint(QPainter::Antialiasing, false);
}

uchar KanjiElementList::strokeCount(int element, int variant) const
{
    return list[element]->variants[variant]->strokecnt;
}

int KanjiElementList::strokePartCount(int element, int variant, int stroke, const QRectF &rect, double partlen, std::vector<int> &parts) const
{
    const KanjiElement *e = list[element];
    const ElementVariant *v = e->variants[variant];

    QRectF r = rect;

    double strokew = basePenWidth(std::min(rect.width(), rect.height()));

    // Setting the r rectangle relative to rect where the variant will be stretched. The r
    // rectangle will be centered in rect. Leave padding of strokew / 2 + 1 to make sure it
    // fits.
    // The variants to be drawn are "normalized" in the editor, usually to reach a maximum of
    // 42500 width and 40000 height. Because some kanji must be drawn smaller (e.g. the mouth)
    // these numbers are used directly.
    // To stretch the drawn parts to fill the rectangle, replace these with v->width and
    // v->height.
    double div = std::min(r.width() / 42500, r.height() / 40000);
    r = QRectF(r.left() + (r.width() - v->width * div) / 2.0 + strokew / 2.0 + 2, r.top() + (r.height() - v->height * div) / 2.0 + strokew / 2.0 + 2, v->width * div - strokew - 4, v->height * div - strokew - 4);

    ElementTransform tr;
    const ElementStroke *s = findStroke(e, v, stroke, r, tr);

    return strokePartCount(/*std::min(r.width(), r.height()),*/ s, tr, partlen, std::fabs(partlen + 1.0) > 0.0001, parts);
}

void KanjiElementList::strokeData(int element, int variant, int stroke, const QRectF &rect, StrokeDirection &dir, QPoint &startpoint) const
{
    const KanjiElement *e = list[element];
    const ElementVariant *v = e->variants[variant];

    QRectF r = rect;

    double strokew = basePenWidth(std::min(rect.width(), rect.height()));

    // Setting the r rectangle relative to rect where the variant will be stretched. The r
    // rectangle will be centered in rect. Leave padding of strokew / 2 + 1 to make sure it
    // fits.
    // The variants to be drawn are "normalized" in the editor, usually to reach a maximum of
    // 42500 width and 40000 height. Because some kanji must be drawn smaller (e.g. the mouth)
    // these numbers are used directly.
    // To stretch the drawn parts to fill the rectangle, replace these with v->width and
    // v->height.
    double div = std::min(r.width() / 42500, r.height() / 40000);
    r = QRectF(r.left() + (r.width() - v->width * div) / 2.0 + strokew / 2.0 + 2, r.top() + (r.height() - v->height * div) / 2.0 + strokew / 2.0 + 2, v->width * div - strokew - 4, v->height * div - strokew - 4);

    ElementTransform tr;
    const ElementStroke *s = findStroke(e, v, stroke, r, tr);

    strokeData(s, tr, dir, startpoint);
}

double KanjiElementList::basePenWidth(int minsize) const
{
    return Settings::scaled(std::max(1.0, minsize / 25.0));
}

void KanjiElementList::drawStroke(QPainter &painter, int element, int variant, int stroke, const QRectF &rect, double partlen, QColor startcolor, QColor endcolor)
{
    KanjiElement *e = list[element];
    ElementVariant *v = e->variants[variant];

    QRectF r = rect;

    double strokew = basePenWidth(std::min(rect.width(), rect.height()));

    // Setting the r rectangle relative to rect where the variant will be stretched. The r
    // rectangle will be centered in rect. Leave padding of strokew / 2 + 1 to make sure it
    // fits.
    // The variants to be drawn are "normalized" in the editor, usually to reach a maximum of
    // 42500 width and 40000 height. Because some kanji must be drawn smaller (e.g. the mouth)
    // these numbers are used directly.
    // To stretch the drawn parts to fill the rectangle, replace these with v->width and
    // v->height.
    double div = std::min(r.width() / 42500, r.height() / 40000);
    r = QRectF(r.left() + (r.width() - v->width * div) / 2.0 + strokew / 2.0 + 2, r.top() + (r.height() - v->height * div) / 2.0 + strokew / 2.0 + 2, v->width * div - strokew - 4, v->height * div - strokew - 4);

    ElementTransform tr;
    const ElementStroke *s = findStroke(e, v, stroke, r, tr);

    if (s == nullptr)
        return;

    bool animated = partlen == 0;
    if (partlen <= 0)
        partlen = std::max(2.0, std::min(rect.width(), rect.height()) / 50.0);
    std::vector<int> parts;
    int cnt = strokePartCount(s, tr, partlen, animated, parts);

    //drawStroke(painter, std::min(r.width(), r.height()), strokew, s, tr, startcolor, endcolor);
    for (int ix = 0; ix != cnt; ++ix)
        drawStrokePart(painter, false, strokew, s, tr, parts, ix, startcolor, endcolor);
}

void KanjiElementList::drawStrokePart(QPainter &painter, bool partialline, int element, int variant, int stroke, const QRectF &rect, const std::vector<int> &parts, int part, QColor startcolor, QColor endcolor) const
{
    const KanjiElement *e = list[element];
    const ElementVariant *v = e->variants[variant];

    QRectF r = rect;

    double strokew = basePenWidth(std::min(rect.width(), rect.height()));

    // Setting the r rectangle relative to rect where the variant will be stretched. The r
    // rectangle will be centered in rect. Leave padding of strokew / 2 + 1 to make sure it
    // fits.
    // The variants to be drawn are "normalized" in the editor, usually to reach a maximum of
    // 42500 width and 40000 height. Because some kanji must be drawn smaller (e.g. the mouth)
    // these numbers are used directly.
    // To stretch the drawn parts to fill the rectangle, replace these with v->width and
    // v->height.
    double div = std::min(r.width() / 42500, r.height() / 40000);
    r = QRectF(r.left() + (r.width() - v->width * div) / 2.0 + strokew / 2.0 + 2, r.top() + (r.height() - v->height * div) / 2.0 + strokew / 2.0 + 2, v->width * div - strokew - 4, v->height * div - strokew - 4);

    ElementTransform tr;
    const ElementStroke *s = findStroke(e, v, stroke, r, tr);

    if (s == nullptr)
        return;

    drawStrokePart(painter, partialline, strokew, s, tr, parts, part, startcolor, endcolor);
}

const ElementStroke* KanjiElementList::findStroke(const KanjiElement *e, const ElementVariant *v, int sindex, QRectF r, ElementTransform &tr) const
{
    if (v->standalone)
    {
        // Multiplier applied on every coordinate to fit the r rectangle. The variant can be
        // stretched so its horizontal and vertical coordinates are multiplied separately.
        double hdiv = (double)std::max(0.00001, r.width()) / std::max<ushort>(1, v->width);
        double vdiv = (double)std::max(0.00001, r.height()) / std::max<ushort>(1, v->height);

        // Number of units to translate the variant to be inside r before
        // scaling.
        int htr = r.left() / hdiv - v->x;
        int vtr = r.top() / vdiv - v->y;
        tr = ElementTransform(htr, vtr, hdiv, vdiv);

        return &v->strokes[sindex];
    }

    bool usecenter = surroundingPattern(e->pattern);

    int varix = 0;

    // Find which variant holds the stroke at sindex and the rectangle they would be drawn in.
    for ( ; varix != 4; ++varix)
    {
        if (e->parts[varix] == -1)
            continue;

        const KanjiElement *pe = list[e->parts[varix]];
        const ElementVariant *pv = pe->variants[v->partpos[varix].variant];

        if (varix == 0)
            usecenter = usecenter && pv->standalone && pv->centerpoint != 0;

        if (((varix != 0 || !usecenter) && pv->strokecnt > sindex) || (usecenter && varix == 0 && sindex < pv->centerpoint - 1))
            break;

        if (!usecenter || varix != 0)
            sindex -= pv->strokecnt;
        else
            sindex -= pv->centerpoint - 1;
    }


    if (usecenter && varix == 4)
    {
        varix = 0;
        sindex += list[e->parts[varix]]->variants[v->partpos[varix].variant]->centerpoint - 1;
    }

    if (varix == 4)
        return nullptr;

    const ElementPart &part = v->partpos[varix];

    const KanjiElement *pe = list[e->parts[varix]];
    const ElementVariant *pv = pe->variants[part.variant];

    // The rectangle of the varix part relative to r.
    QRect pr = QRect(r.left() + r.width() * (part.x - v->x) / std::max<ushort>(1, v->width), r.top() + r.height() * (part.y - v->y) / std::max<ushort>(1, v->height), r.width() * part.width / std::max<ushort>(1, v->width), r.height() * part.height / std::max<ushort>(1, v->height));

    return findStroke(pe, pv, sindex, pr, tr);
}

int KanjiElementList::strokePartCount(const ElementStroke *s, const ElementTransform &tr, double partlen, bool animated, std::vector<int> &parts) const
{
    ElementPointT pastpoint = tr.transformed(s->points[0]);

    int partcnt = 0;

    parts.clear();
    parts.reserve(s->points.size() - 1);

    for (int ix = 1, siz = tosigned(s->points.size()); ix != siz; ++ix)
    {
        ElementPointT point = tr.transformed(s->points[ix]);

        double len = 0;
        if (point.type == ElementPoint::LineTo)
            len = std::sqrt((point.x - pastpoint.x) * (point.x - pastpoint.x) + (point.y - pastpoint.y) * (point.y - pastpoint.y));
        else if (point.type == ElementPoint::Curve)
            len = bezierLength(QPointF(pastpoint.x, pastpoint.y), QPointF(point.c1x, point.c1y), QPointF(point.c2x, point.c2y), QPointF(point.x, point.y), partlen / 10);
        else
            throw "Invalid data.";

        int pp = std::ceil(len / partlen);
        partcnt += pp;
        parts.push_back(pp);

        pastpoint = point;
    }

    if (animated)
    {

        // If strokes are shorter, they will be made up of relatively more parts to draw them
        // slower. Longer strokes will draw faster. The part counts are manipulated to get closer
        // to an ideal number.

        int midpartcnt = (20 + partcnt) / 2;
        double partdiv = std::min(4.0, std::max(0.75, double(midpartcnt) / partcnt));
        if (partdiv > 1.2)
            partdiv = (partdiv + partdiv * partdiv) / 2;

        for (int ix = 0, siz = tosigned(parts.size()); ix != siz; ++ix)
        {
            int newcnt = std::max<int>(1, std::ceil(parts[ix] * partdiv));
            partcnt += newcnt - parts[ix];
            parts[ix] = newcnt;
        }
    }

    return partcnt;
}

void KanjiElementList::strokeData(const ElementStroke *s, const ElementTransform &tr, StrokeDirection &dir, QPoint &startpoint) const
{
    ElementPointT pt1 = tr.transformed(s->points[0]);
    ElementPointT pt2 = tr.transformed(s->points[1]);

    dir = StrokeDirection::Unset;

    if (pt2.type == ElementPoint::LineTo)
    {
        double dx = pt2.x - pt1.x;
        double dy = pt2.y - pt1.y;
        double dxv = std::abs(dy != 0 ? dx / dy : dx);
        double dyv = std::abs(dx != 0 ? dy / dx : dy);

        if (dxv < 0.06)
            dir = (dy < 0 ? StrokeDirection::Up : StrokeDirection::Down);
        else if (dyv < 0.1)
            dir = (dx < 0 ? StrokeDirection::Left : StrokeDirection::Right);
        else
            dir = (dy < 0 ? (dx < 0 ? StrokeDirection::UpLeft : StrokeDirection::UpRight) : (dx < 0 ? StrokeDirection::DownLeft : StrokeDirection::DownRight));
    }
    else if (pt2.type == ElementPoint::Curve)
    {
        QPointF dp = QPointF((pt2.c1x + pt2.c2x) / 2.0, (pt2.c1y + pt2.c2y) / 2.0);

        double dx = (dp.x() - pt1.x);
        double dy = (dp.y() - pt1.y);
        double dxv = std::abs(dy != 0 ? dx / dy : dx);
        double dyv = std::fabs(dx != 0 ? dy / dx : dy);
        if (dxv < 0.06)
            dir = (dy < 0 ? StrokeDirection::Up : StrokeDirection::Down);
        else if (dyv < 0.1)
            dir = (dx < 0 ? StrokeDirection::Left : StrokeDirection::Right);
        else
            dir = (dy < 0 ? (dx < 0 ? StrokeDirection::UpLeft : StrokeDirection::UpRight) : (dx < 0 ? StrokeDirection::DownLeft : StrokeDirection::DownRight));
    }

    startpoint = QPoint(pt1.x, pt1.y);
}

//void KanjiElementList::drawStroke(QPainter &painter, double areasize, double basewidth, ElementStroke *s, const ElementTransform &tr, QColor startcolor, QColor endcolor)
//{
//    ElementPointT pastpoint = tr.transformed(s->points[0]);
//
//    for (int ix = 1; ix != s->points.size(); ++ix)
//    {
//        ElementPointT point = tr.transformed(s->points[ix]);
//
//        double startw = basewidth;
//        double endw = basewidth;
//        if (ix == 1)
//        {
//            if ((s->tips & (int)StrokeTips::StartPointed) == (int)StrokeTips::StartPointed)
//                startw = std::max(basewidth * 0.3, 0.2);
//            if ((s->tips & (int)StrokeTips::StartThin) == (int)StrokeTips::StartThin)
//                startw = std::max(basewidth * 0.7, 0.3);
//        }
//        if (ix == s->points.size() - 1)
//        {
//            if ((s->tips & (int)StrokeTips::EndPointed) == (int)StrokeTips::EndPointed)
//                endw = std::max(basewidth * 0.3, 0.2);
//            if ((s->tips & (int)StrokeTips::EndThin) == (int)StrokeTips::EndThin)
//                endw = std::max(basewidth * 0.7, 0.3);
//        }
//
//        if (point.type == ElementPoint::LineTo)
//        {
//            drawLine(painter, QPointF(pastpoint.x, pastpoint.y), QPointF(point.x, point.y), startw, endw);
//        }
//        else if (point.type == ElementPoint::Curve)
//        {
//            // Negative width makes the bezier draw a "dot" stroke.
//            if (s->tips == (int)StrokeTips::SingleDot)
//                startw = -1.0, endw = basewidth;
//
//            // A curve is drawn by cutting up a bezier to small enough line segments and drawing
//            // each segment with drawLine().
//            drawBezier(painter, areasize, QPointF(pastpoint.x, pastpoint.y), QPointF(point.c1x, point.c1y), QPointF(point.c2x, point.c2y), QPointF(point.x, point.y), startw, endw);
//        }
//
//        pastpoint = point;
//    }
//}

QLinearGradient KanjiElementList::paintGradient(double x1, double y1, double x2, double y2, double dotsize, int startcnt, int partcnt, int endcnt, QColor startcolor, QColor endcolor) const
{
    double sx = x1 - (x2 - x1) / partcnt * startcnt;
    double sy = y1 - (y2 - y1) / partcnt * startcnt;
    double ex = x2 + (x2 - x1) / partcnt * endcnt;
    double ey = y2 + (y2 - y1) / partcnt * endcnt;

    QLinearGradient grad(sx, sy, ex, ey);

    double linelen = std::sqrt((sx - ex) * (sx - ex) + (sy - ey) * (sy - ey));
    double spotend = std::min(0.4, dotsize / linelen * 0.4);

    grad.setColorAt(0.0, startcolor);
    grad.setColorAt(spotend, startcolor);
    grad.setColorAt(std::min(0.8, spotend * 4.0), endcolor);

    return grad;
}


void KanjiElementList::drawStrokePart(QPainter &painter, bool partialline, double basewidth, const ElementStroke *s, const ElementTransform &tr, const std::vector<int> &parts, int part, QColor startcolor, QColor endcolor) const
{
    int partspos = 0;
    int ix = 1;
    int siz = tosigned(s->points.size());

    QBrush b;
    int fullcnt = 0;
    int startcnt = 0;
    bool usecolor = startcolor.isValid() && endcolor.isValid();
    if (usecolor)
    {
        for (int iy = 0, sizy = tosigned(parts.size()); iy != sizy; ++iy)
            fullcnt += parts[iy];
    }

    for (; ix != siz; ++ix)
    {
        if (part >= parts[partspos])
        {
            startcnt += parts[partspos];
            part -= parts[partspos];
            ++partspos;
            continue;
        }
        break;
    }

    int partcnt = parts[partspos];
    int endcnt = fullcnt - startcnt - partcnt;

    ElementPointT pastpoint = tr.transformed(s->points[ix - 1]);
    ElementPointT point = tr.transformed(s->points[ix]);

    int salpha = usecolor ? startcolor.alpha() : painter.brush().color().alpha();
    int ealpha = usecolor ? endcolor.alpha() : salpha;
    if (basewidth < 2.0)
    {
        salpha = salpha * basewidth / 2.0;
        ealpha = ealpha * basewidth / 2.0;
    }

    double startw = basewidth;
    double endw = basewidth;
    bool pointstart = (s->tips & (int)StrokeTips::StartPointed) == (int)StrokeTips::StartPointed || s->tips == (int)StrokeTips::SingleDot;
    if (ix == 1)
    {
        if ((s->tips & (int)StrokeTips::StartPointed) == (int)StrokeTips::StartPointed)
            startw = std::max(basewidth * 0.3, 0.2);
        if ((s->tips & (int)StrokeTips::StartThin) == (int)StrokeTips::StartThin)
            startw = std::max(basewidth * 0.7, 0.3);
    }
    if (ix == siz - 1)
    {
        if ((s->tips & (int)StrokeTips::EndPointed) == (int)StrokeTips::EndPointed)
            endw = std::max(basewidth * 0.3, 0.2);
        if ((s->tips & (int)StrokeTips::EndThin) == (int)StrokeTips::EndThin)
            endw = std::max(basewidth * 0.7, 0.3);
    }

    if (point.type == ElementPoint::LineTo)
    {
        bool last = (part == partcnt - 1);
        if (last == partialline)
            return;

        b = painter.brush();
        if (usecolor)
        {
            QColor sc = startcolor;
            sc.setAlpha(salpha);
            QColor ec = endcolor;
            ec.setAlpha(ealpha);
            painter.setBrush(paintGradient(pastpoint.x, pastpoint.y, point.x, point.y, basewidth * (pointstart ? 2 : 1), startcnt, partcnt, endcnt, sc, ec));
        }
        else
        {
            QColor c = b.color();
            c.setAlpha(salpha);
            painter.setBrush(c);
        }


        if (last)
            drawLine(painter, QPointF(pastpoint.x, pastpoint.y), QPointF(point.x, point.y), startw, endw);
        else
        {
            double mul = double(part + 1) / partcnt;
            point.x = pastpoint.x + (point.x - pastpoint.x) * mul;
            point.y = pastpoint.y + (point.y - pastpoint.y) * mul;

            endw = startw + (endw - startw) * mul;

            drawLine(painter, QPointF(pastpoint.x, pastpoint.y), QPointF(point.x, point.y), startw, endw);
        }
        painter.setBrush(b);
    }
    else if (point.type == ElementPoint::Curve)
    {
        if (partialline)
            return;

        double startt = double(part) / partcnt;
        double endt = double(part + 1) / partcnt;

        double tmpstartw = startw;
        double tmpendw = endw;

        // Negative width makes the bezier draw a "dot" stroke.
        if (s->tips != (int)StrokeTips::SingleDot)
        {
            startw = tmpstartw + (tmpendw - tmpstartw) * startt;
            endw = tmpstartw + (tmpendw - tmpstartw) * endt;
        }
        else
        {
            // The widths of a singledot stroke are computed by one component of a bezier.
            startw = basewidth * (std::pow(1.0 - startt, 3) * dotwsy + 3 * startt * std::pow(1 - startt, 2) * dotwc1y + 3 * std::pow(startt, 2) * (1 - startt) * dotwc2y + std::pow(startt, 3) * dotwey);
            endw = basewidth * (std::pow(1.0 - endt, 3) * dotwsy + 3 * endt * std::pow(1 - endt, 2) * dotwc1y + 3 * std::pow(endt, 2) * (1 - endt) * dotwc2y + std::pow(endt, 3) * dotwey);
        }

        double p1x = pastpoint.x;
        double p1y = pastpoint.y;
        double p2x = point.x;
        double p2y = point.y;

        double c1x = point.c1x;
        double c1y = point.c1y;
        double c2x = point.c2x;
        double c2y = point.c2y;

        double ptx1 = std::pow(1.0 - startt, 3) * p1x + 3 * startt * std::pow(1 - startt, 2) * c1x + 3 * std::pow(startt, 2) * (1 - startt) * c2x + std::pow(startt, 3) * p2x;
        double pty1 = std::pow(1.0 - startt, 3) * p1y + 3 * startt * std::pow(1 - startt, 2) * c1y + 3 * std::pow(startt, 2) * (1 - startt) * c2y + std::pow(startt, 3) * p2y;

        double ptx2 = std::pow(1.0 - endt, 3) * p1x + 3 * endt * std::pow(1 - endt, 2) * c1x + 3 * std::pow(endt, 2) * (1 - endt) * c2x + std::pow(endt, 3) * p2x;
        double pty2 = std::pow(1.0 - endt, 3) * p1y + 3 * endt * std::pow(1 - endt, 2) * c1y + 3 * std::pow(endt, 2) * (1 - endt) * c2y + std::pow(endt, 3) * p2y;

        b = painter.brush();
        if (usecolor)
        {
            QColor sc = startcolor;
            sc.setAlpha(salpha);
            QColor ec = endcolor;
            ec.setAlpha(ealpha);
            painter.setBrush(paintGradient(ptx1, pty1, ptx2, pty2, basewidth * (pointstart ? 2 : 1), startcnt + part, 1, endcnt + partcnt - part - 1, sc, ec));
        }
        else
        {
            QColor c = b.color();
            c.setAlpha(salpha);
            painter.setBrush(c);
        }

        drawLine(painter, QPointF(ptx1, pty1), QPointF(ptx2, pty2), startw, endw);

        painter.setBrush(b);
    }

}

void KanjiElementList::drawLine(QPainter &painter, const QPointF &start, const QPointF &end, double startw, double endw) const
{
    // Line drawing with differing start and end widths is done by placing arcs at the start
    // and end points of the path. The filled path will make the line.
    QPainterPath path;

    double sx = start.x();
    double sy = start.y();
    double ex = end.x();
    double ey = end.y();

    if (sy > ey || (fabs(sy - ey) < 0.00001 && sx > ex))
    {
        std::swap(sx, ex);
        std::swap(sy, ey);
        std::swap(startw, endw);
    }

    QRectF startr = QRectF(sx - startw / 2.0, sy - startw / 2.0, startw, startw);
    QRectF endr = QRectF(ex - endw / 2.0, ey - endw / 2.0, endw, endw);

    if (fabs(sx - ex) < 0.00001 && fabs(sy - ey) < 0.00001)
    {
        path.addEllipse(startw > endw ? startr : endr);
        painter.drawPath(path);
        return;
    }

    double deg = std::atan2(ex - sx, ey - sy) * 180 / const_PI;
    if (deg < 0)
        deg = std::min(359.9999, deg + 360.0);

    double enddeg = deg + 180;
    while (enddeg > 360.0)
        enddeg -= 360.0;

    path.arcMoveTo(startr, deg);
    path.arcTo(startr, deg, 180);
    path.arcTo(endr, enddeg, 180);
    path.closeSubpath();

    painter.drawPath(path);
}

double KanjiElementList::bezierLength(const QPointF &start, const QPointF &c1, const QPointF &c2, const QPointF &end, double maxerror) const
{
    // Computes the length of start->end, and start->c1->c2->end. If the difference is is too
    // large, halves the bezier and does the same for the two sides. When the two numbers are
    // close enough, returns the sum of their average for every tested segment.
    double selen = lineLength(start, end);
    double s12elen = lineLength(start, c1) + lineLength(c1, c2) + lineLength(c2, end);

    if (fabs(selen - s12elen) < maxerror)
        return (selen + s12elen) / 2.0;

    QPointF sc1, sc2, ec1, ec2, m;
    _bezierPoints(start, c1, c2, end, 0.5, sc1, sc2, m, ec1, ec2);

    return bezierLength(start, sc1, sc2, m, maxerror) + bezierLength(m, ec1, ec2, end, maxerror);
}

namespace {
    // Helper for _bezierPoints.
    QPointF _bezierPointAt(double x1, double y1, double x2, double y2, double at)
    {
        return QPointF((x2 - x1) * at + x1, (y2 - y1) * at + y1);
    }
}

void KanjiElementList::_bezierPoints(const QPointF &start, const QPointF &c1, const QPointF &c2, const QPointF &end, double at, QPointF &sc1, QPointF &sc2, QPointF &m, QPointF &ec1, QPointF &ec2) const
{
    QPointF cmid = _bezierPointAt(c1.x(), c1.y(), c2.x(), c2.y(), at);

    sc1 = _bezierPointAt(start.x(), start.y(), c1.x(), c1.y(), at);
    ec2 = _bezierPointAt(c2.x(), c2.y(), end.x(), end.y(), at);
    sc2 = _bezierPointAt(sc1.x(), sc1.y(), cmid.x(), cmid.y(), at);
    ec1 = _bezierPointAt(cmid.x(), cmid.y(), ec2.x(), ec2.y(), at);
    m = _bezierPointAt(sc2.x(), sc2.y(), ec1.x(), ec1.y(), at);
}

void KanjiElementList::drawBezier(QPainter &painter, double areasize, const QPointF &start, const QPointF &c1, const QPointF &c2, const QPointF &end, double startw, double endw) const
{
    // For smooth bezier drawing, the curve is cut up depending on the curve length.
    double len = bezierLength(start, c1, c2, end, areasize / 100);
    int steps = std::max(8, int(len / 4));

    double p1x = start.x();
    double p1y = start.y();
    double c1x = c1.x();
    double c1y = c1.y();
    double c2x = c2.x();
    double c2y = c2.y();
    double p2x = end.x();
    double p2y = end.y();

    double oldptx = start.x();
    double oldpty = start.y();

    double w = startw;
    double winc = (endw - startw) / steps;
    double nextw = startw + winc;

    //double pastt = 0.0;

    // Single dot stroke's bezier starts with a small width. Endw is the base width in that case.
    if (startw < -0.1)
        w = endw * dotwsy;
    for (int ix = 0; ix != steps; ++ix)
    {
        double t = double(ix + 1) / steps;

        double ptx = std::pow(1.0 - t, 3) * p1x + 3 * t * std::pow(1 - t, 2) * c1x + 3 * std::pow(t, 2) * (1 - t) * c2x + std::pow(t, 3) * p2x;
        double pty = std::pow(1.0 - t, 3) * p1y + 3 * t * std::pow(1 - t, 2) * c1y + 3 * std::pow(t, 2) * (1 - t) * c2y + std::pow(t, 3) * p2y;

        if (startw < -0.1)
        {
            // Single dot stroke is drawn with changing widths computed from one component of
            // a bezier.

            nextw = endw * (std::pow(1.0 - t, 3) * dotwsy + 3 * t * std::pow(1 - t, 2) * dotwc1y + 3 * std::pow(t, 2) * (1 - t) * dotwc2y + std::pow(t, 3) * dotwey);

            //pastt = t;
        }
        drawLine(painter, QPointF(oldptx, oldpty), QPointF(ptx, pty), w, nextw);
        w = nextw;
        // Not single dot drawing.
        if (startw > -0.1)
            nextw += winc;
        oldptx = ptx;
        oldpty = pty;
    }
}

void KanjiElementList::loadModels(QDataStream &stream, std::vector<Stroke> &dest)
{
    dest.clear();

    quint16 cnt;

    stream >> cnt;
    dest.reserve(cnt);
    for (int ix = 0; ix != cnt; ++ix)
    {
        dest.push_back(Stroke());

        Stroke &s = dest.back();

        quint16 ptcnt;
        stream >> ptcnt;
        for (int iy = 0; iy != ptcnt; ++iy)
        {
            QPointF pt;

            int i;
            stream >> i;
            pt.setX((double)i / 10000.);
            stream >> i;
            pt.setY((double)i / 10000.);

            s.add(pt);
        }
    }
}

void KanjiElementList::loadRepos(QDataStream &stream)
{
    repos.clear();

    quint16 cnt;
    stream >> cnt;

    for (int ix = 0; ix != cnt; ++ix)
    {
        std::unique_ptr<ElementRepo> repo(new ElementRepo);

        stream >> make_zstr(repo->name, ZStrFormat::Byte);

        quint16 ecnt;
        stream >> ecnt;
        repo->list.reserve(ecnt);
        for (int iy = 0; iy != ecnt; ++iy)
        {
            ElementRepoItem elem;
            stream >> elem.element;
            stream >> elem.variant;
            repo->list.push_back(elem);
        }
    }
}

void KanjiElementList::loadVariantNames(QDataStream &stream)
{
    quint16 cnt;
    stream >> cnt;

    for (int ix = 0; ix != cnt; ++ix)
    {
        uint ucode;
        stream >> ucode;

        quint16 clen;
        stream >> clen;

        fastarray<QChar> c;
        c.resize(clen);
        int ccc = 0;
        for (int iy = 0; iy != clen; ++iy)
        {

            stream >> c[iy];
            if (c[iy] == 0)
                ++ccc;
        }
        QCharString str;
        str.copy(c.data(), ccc != 1 ? clen : -1);
        varnames[ucode] = std::move(str);
    }
}

int KanjiElementList::posDiff(const BitArray &p1, const BitArray &p2)
{
    int diff = 0;

    uchar vert[] = { 0, 4, 8, 12, 15, 11, 7, 3 };
    uchar horz[] = { 1, 5, 9, 13, 14, 10, 6, 2 };

    uchar minv = 255, minh = 255, minv1 = 255, minh1 = 255;

    for (int ix = 0; ix < 8; ++ix)
    {
        if (p1.toggled(vert[ix]))
        {
            minv1 = ix;
            for (int iy = 0; iy < 8; ++iy)
            {
                if (p2.toggled(vert[iy]))
                {
                    minv = iy;
                    diff += pow(abs(ix - iy), 2);
                    break;
                }
            }
            break;
        }
    }

    for (int ix = 7; ix >= minv1; --ix)
    {
        if (p1.toggled(vert[ix]))
        {
            for (int iy = 7; iy >= minv; --iy)
            {
                if (p2.toggled(vert[iy]))
                {
                    diff += pow(abs(ix - iy), 2);
                    break;
                }
            }
            break;
        }
    }

    for (int ix = 0; ix < 8; ++ix)
    {
        if (p1.toggled(horz[ix]))
        {
            minh1 = ix;
            for (int iy = 0; iy < 8; ++iy)
            {
                if (p2.toggled(horz[iy]))
                {
                    minh = iy;
                    diff += pow(abs(ix - iy), 2);
                    break;
                }
            }
            break;
        }
    }
    for (int ix = 7; ix >= minh1; --ix)
    {
        if (p1.toggled(horz[ix]))
        {
            for (int iy = 7; iy >= minh; --iy)
            {
                if (p2.toggled(horz[iy]))
                {
                    diff += pow(abs(ix - iy), 2);
                    break;
                }
            }
            break;
        }
    }

    return diff;
}


//-------------------------------------------------------------


