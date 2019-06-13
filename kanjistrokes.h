/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef RECOGNIZER_H
#define RECOGNIZER_H

#include <QPainter>
//#include <QPoint>
//#include <QRect>

#include <cmath>
#include <vector>
#include <map>
#include <memory>
#include "smartvector.h"
#include "fastarray.h"
#include "bits.h"
#include "qcharstring.h"

class RadianDistance;
class KanjiElementList;

namespace ZKanji
{
    // Returns the global kanji element object for kanji handwriting
    // recognition and stroke order diagram drawing.
    KanjiElementList* elements();

    // Returns a newly created kanji element object with data from file.
    // Destroys the old object if it was created.
    void initElements(const QString &filename);
}

// A radian type used in the kanji stroke recognition. The value stored is
// always a positive number in the range of [0, 2*Pi)
class Radian
{
public:
    enum Direction {
        up = 0,
        down = 1,
        left = 0,
        right = 2,
        upLeft = left | up, /* 0 */
        upRight = right | up, /* 2 */
        downLeft = down | left, /* 1 */
        downRight = down | right, /* 3 */
        none = 0xff,
    };

    Radian();
    Radian(double val);

    double radDiff(const Radian &r) const;

    bool operator>(const Radian &r) const;
    bool operator<(const Radian &r) const;
    bool operator>=(const Radian &r) const;
    bool operator<=(const Radian &r) const;

    RadianDistance operator-(const Radian &r) const;
    //Radian& operator-=(const Radian &r);

    // Returns the radian's absolute value.
    double abs();
    // Returns the sinus of the radian's absolute value.
    double sin();

    //Direction horz();
    //Direction vert();
    //Direction diag();

private:
    double val;

    void normalize();
};

// Result of subtracting one Radian value from another. Holds the radian of the shortest arc
// between the two radians. This value can be negative.
class RadianDistance
{
public:
    // Returns the radian's absolute value.
    double abs();
    // Returns the sinus of the stored radian's absolute value.
    double sin();
private:
    RadianDistance(double val);
    double val;
    friend class Radian;
};

namespace RecMath
{
    extern const double tinyval;
    extern const double smallval;

    // The distance between two values is "very small".
    inline bool fnear(const double &a, const double &b)
    {
        return std::fabs(a - b) < tinyval;
    }

    // The distance between a value and itself rounded is "very small".
    inline bool fnearw(const double &a)
    {
        return fnear(a, std::round(a));
    }

    // The counter-clockwise angle in radians between a line going through
    // from point a to point b, and a line on the x axis pointing right. The
    // result is in the range of [0, PI).
    Radian rad(const QPointF &a, const QPointF &b);
    // The length of a line segment from point a to point b.
    double len(const QPointF &a, const QPointF &b);

    // Returns the distance of point q from the line segment (p1, p2).
    double pointDist(const QPointF &p1, const QPointF &p2, const QPointF &q);
}

// A sequence of points that make up a stroke of a kanji character drawn by
// the user or loaded as a model stroke. Used in handwriting recognition. The
// kanji stroke order drawing uses KanjiElement, ElementVariant, ElementStroke
// and ElementPoint structures.
class Stroke
{
public:
    typedef size_t  size_type;

    Stroke();
    Stroke(const Stroke &src);
    Stroke(Stroke &&src);

    Stroke& operator=(const Stroke &src);
    Stroke& operator=(Stroke &&src);

    // Number of points in the stroke.
    size_type size() const;
    // There are no points added to the stroke.
    bool empty() const;
    // Adds a new point at the end of the stroke. If a point at the same
    // position is already at the back of the stroke, the new point is not
    // added.
    void add(const QPointF &pt);
    // Removes every data from the stroke, including every point.
    void clear();
    // Bounding rectangle enclosing the stroke's points.
    QRectF bounds() const;
    // Width of the bounding rectangle enclosing the stroke's points.
    double width() const;
    // Height of the bounding rectangle enclosing the stroke's points.
    double height() const;

    // Returns the number of segments in the stroke. Same as size - 1 if size
    // is greater than 1. A segment is a line segment between neighboring
    // points.
    int segmentCount() const;
    // Returns the length of a given segment. A segment is a line segment
    // between neighboring points.
    double segmentLength(int index) const;

    // Returns the angle of the segment relative to the positive x axis
    // measured counter-clockwise.
    Radian segmentAngle(int index) const;

    // Returns the section index the segment belongs to.
    ushort segmentSection(int index) const;

    // Number of sections in the stroke. Sections are parts of the stroke with
    // larger angles separating them from each other.
    int sectionCount() const;
    // Returns the length of a section in the stroke. The length is the sum
    // of the lengths of each segment within the section.
    double sectionLength(int index) const;

    // Number of segments belonging to a given section.
    int segmentsInSection(int index) const;

    // The full length of the stroke.
    double length() const;

    Radian angle() const;

    // Compares this stroke to another stroke and returns their distance.
    // The value of distance is only useful when compared to other distances.
    int compare(const Stroke &other) const;

    //QPointF& operator[](int index);
    const QPointF& operator[](int index) const;

    // Fills pt array with cnt number of points that lie on this stroke at
    // equal distances. These points are not likely to contain the original
    // points of the stroke.
    void splitPoints(QPointF *pt, int cnt) const;
private:
    // Holds data of a single point in the stroke and the line segment from
    // that point to the next. For the endpoint, the line segment information
    // is undefined.
    struct StrokePoint
    {
        // Point coordinates in the stroke.
        QPointF pt;

        // Length of segment from this point to the next.
        double length;

        // Angle in radian of the segment from this point to the next, and
        // the x axis. The angle is measured counter-clockwise relative to the
        // axis pointing in the positive x direction.
        Radian angle;

        // A stroke is made up of segments (parts between two adjacent points)
        // and is divided into sections. A section ends and the next one starts
        // at bigger breaks in the flow of a stroke. Section boundaries are at
        // points where the angle between a segment and the next segment is
        // larger than PI/2 * 0.8.

        // Section index where the segment starting with the point belongs.
        ushort section;
    };

    // The points in the stroke.
    std::vector<StrokePoint> list;

    // Length of the whole stroke, sum of the lengths of each segment.
    double len;
    // Angle of the segment from the first point to the end point.
    Radian ang;

    // Number of sections in the stroke.
    int sectcnt;

    // Bounding rectangle of the points.
    QRectF dim;
};

struct RecognizerComparison
{
    // Model's index.
    ushort index;

    // A stroke's distance from the model. The larger the value the greater the distance. The
    // actual value is only useful when comparing it with other distances.
    int distance;
};
typedef fastarray<RecognizerComparison> RecognizerComparisons;

// Contains strokes in a simplified and processed form. When new strokes are
// added, this is automatically done for them.
class StrokeList
{
public:
    typedef size_t  size_type;

    StrokeList();

    // Adds a stroke to the list of strokes, simplifying it. If mousedrawn is
    // true, the simplification will be more aggressive.
    void add(Stroke &&s, bool mousedrawn = false);

    // No strokes added.
    bool empty() const;

    // Updates size, removing any elements above newsize. It's invalid to call with a value
    // higher than the current size.
    void resize(int newsize);

    // Number of strokes added.
    size_type size() const;

    // Removes drawn strokes.
    void clear();

    // Bounding rectangle of all strokes.
    QRectF bounds() const;

    // Width of bounding rectangle of all strokes.
    double width() const;

    // Height of bounding rectangle of all strokes.
    double height() const;

    // Redo comparison of strokes added to the list with the recognizer's model strokes.
    //void reCompare();

    //Stroke& items(int index);
    //const Stroke& items(int index) const;

    //Stroke& operator[](int index);
    const Stroke& operator[](int index) const;

    // Comparison data to each model strokes of the stroke at index.
    const RecognizerComparisons& cmpItems(int index) const;

    // Position data of the stroke at index used in handwriting recognition.
    const fastarray<BitArray>& posItems(int index) const;
private:
    // Removes some points from used drawn stroke that are unnecessary when
    // showing the stroke on the screen.
    Stroke&& simplify(Stroke &&s, bool mousedrawn);

    // Creates a new stroke from the source that consists of important points
    // of source and added points between those at equal distances.
    Stroke dissect(const Stroke &src);

    // Strokes added to the stroke list in simplified and processed forms.
    std::vector<std::pair<Stroke, Stroke>> list;

    // Recognizer comparison data to every model for each stroke in list.
    smartvector<RecognizerComparisons> cmplist;

    // Position bits for each stroke, relative to neighboring strokes used in
    // handwriting recognition.
    fastarray<fastarray<BitArray>> poslist;

    // Bounding rectangle of all the points added to strokelist.
    QRectF dim;
};

struct ElementPoint
{
    enum Type : uchar { MoveTo, LineTo, Curve };

    // X coordinate on a square of [0, 50000] units.
    ushort x;

    // Y coordinate on a square of [0, 50000] units.
    ushort y;

    // X1 control point on a square of [0, 50000] units.
    int c1x;

    // Y1 control point on a square of [0, 50000] units.
    int c1y;

    // X2 control point on a square of [0, 50000] units.
    int c2x;

    // Y2 control point on a square of [0, 50000] units.
    int c2y;

    Type type;
};

// Used as a result for ElementTransform::transformed(). Holds the coordinates
// of an ElementPoint after translate and scale. The values are in device
// coordinates.
struct ElementPointT
{

    double x;
    double y;
    double c1x;
    double c1y;
    double c2x;
    double c2y;

    ElementPoint::Type type;
};

// Used as a transformation "matrix" for ElementPoint objects.
// Only supports scaling and translation. Translation is done
// before the scaling.
class ElementTransform
{
public:
    ElementTransform();
    ElementTransform(int htrans, int vtrans, double hscale, double vscale);

    // Returns a translated and scaled ElementPoint.
    ElementPointT transformed(const ElementPoint &pt) const;

    // Applies the current transformation to the passed ElementTransform. 
    //void transformed(ElementTransform &tr) const;

private:
    int htrans;
    int vtrans;
    double hscale;
    double vscale;
};

enum class OldStrokeTips : uchar {
    NormalTip, EndPointed, EndThin, StartPointed, StartThin, StartPointedEndPointed,
    StartPointedEndThin, StartThinEndPointed, StartThinEndThin, SingleDot
};

enum class StrokeTips : uchar {
    NormalTip = 0x00, EndPointed = 0x01, EndThin = 0x02, StartPointed = 0x04, StartThin = 0x08,
    StartPointedEndPointed = StartPointed | EndPointed,
    StartPointedEndThin = StartPointed | EndThin,
    StartThinEndPointed = StartThin | EndPointed,
    StartThinEndThin = StartThin | EndThin,
    SingleDot = 0x10
};

struct ElementStroke
{
    std::vector<ElementPoint> points;

    uchar tips;
};

struct ElementPart
{
    // Which variant of an element does the part represent.
    uchar variant;

    // Width of the part in the range [0, 50000].
    ushort width;

    // Height of the part in the range [0, 50000].
    ushort height;

    // X origin of the part in the range [0, 50000].
    ushort x;

    // Y origin of the part in the range [0, 50000].
    ushort y;
};

// A variant of a stroke data element. Each variant represents the same
// character part, or full character of the element with slight differences.
struct ElementVariant
{
    // Number of strokes in character.
    uchar strokecnt;

    // Width and height of the variant in values between 0 and 50000.
    ushort width;

    // Width and height of the variant in values between 0 and 50000.
    ushort height;

    // X origin of the variant in the range [0, 50000].
    ushort x;

    // Y origin of the variant in the range [0, 50000].
    ushort y;

    // When false, the variant is made up of at most 4 sub-elements and the
    // partpos data is valid. When true, the variant might still have
    // at most 4 sub-elements in the KanjiElement's parts listed, but the
    // partpos data is not used, and the elements are not shown.
    // In the false case, strokes is not empty and can contain actual strokes.
    bool standalone;

    // 0 - no centerpoint. Starting at base index 1, represents the first
    // stroke of the second half of the variant. The parent's other parts must
    // be drawn before drawing the second half. Only standalone variants
    // have a valid centerpoint. It's ignored if the pattern of the parent
    // is not one of the surrounding patterns (Surrounding patterns are:
    // Ebl, Eblt, Elt, Eltr, Etr, Erbl, Ew, Elr.)
    uchar centerpoint;

    // Holds positions of 4 sub-elements if standalone is false.
    fastarray<ElementPart> partpos;

    // Holds strokes if standalone is true.
    fastarray<ElementStroke> strokes;
};

struct RecognizerData
{
    RecognizerComparison data;
    fastarray<BitArray> pos;
};

enum class KanjiPattern { Undef, Ebl, Eblt, Elt, Eltr, Etr, Erbl, Ew, Elr, W, H2, H3, V2, V3, V4, T, C, B, X, Hira, Kata,
    Count };

// Returns true for: Ebl, Eblt, Elt, Eltr, Etr, Erbl, Ew, Elr
bool surroundingPattern(KanjiPattern p);

// Collection of character stroke data. Used for drawing kanji stroke order
// diagrams, recognizing a handwritten character and editing it (if
// implemented).
struct KanjiElement
{

    // Kanji index this data represents or (ushort)-1.
    ushort owner;

    // Unicode this data represents. It's 0 if owner is a valid kanji or
    // when there's no unicode character matching this element.
    ushort unicode;

    // Pattern of sub-elements.
    KanjiPattern pattern;

    // KanjiElementList index of KanjiElement objects, unless -1.
    short parts[4];

    fastarray<ushort> parents;

    smartvector<ElementVariant> variants;

    fastarray<RecognizerData> recdata;
};

struct ElementRepoItem
{
    // Index of element in KanjiElementList.
    int element;

    // Index of variant of the element.
    uchar variant;
};

struct ElementRepo
{
    QString name;

    std::vector<ElementRepoItem> list;
};

struct KanjiEntry;

enum class StrokeDirection { Unset, Left, Up, Right, Down, UpLeft, UpRight, DownLeft, DownRight  };

// Main class for handwriting recognition. Contains model strokes (to match with user drawn
// strokes), kanji elements (reusable group of strokes), and kanji data made up of elements.
class KanjiElementList
{
public:
    typedef size_t  size_type;

    // Removes all data from the recognizer. Set full to true to remove kanji stroke model
    // data as well.
    void clear(bool full = false);

    void load(const QString &filename);

    // Returns the element index associated with a kanji at kindex. Checking the element value
    // of a KanjiEntry fails before the dictionary is loaded, but this function works as soon
    // as the recognizer data is loaded. Pass a negative value to look for a character with
    // a given unicode value instead.
    int elementOf(int kindex) const;
    // Updates the kanji in the kanji list with the loaded kanji stroke order elements after
    // load. Should only be called once after the main dictionary is loaded.
    void applyElements();

    // Number of stroke models stored.
    int modelCount() const;

    // Compares the passed stroke with the model strokes, and stores the result in result.
    void compareToModels(const Stroke &stroke, RecognizerComparisons &result);

    // Generates stroke position data for the handwriting recognition.
    void computeStrokePositions(BitArray &bits, QRectF orig, QRectF comp, const QRectF &fullsize);

    // Matches the strokes in stroke list with characters that have recognizer data. Stores
    // the indexes of possible character element matches in their order of similarity to
    // strokes in results.
    void findCandidates(const StrokeList &strokes, std::vector<int> &result, int strokecnt = -1, bool includekanji = true, bool includekana = true, bool includeother = true);

    // Number of elements in the recognizer list. Not all elements correspond to a valid
    // character, many are parts of others.
    size_type size() const;

    // The kanji owner of the item at index. If no owner is set for an element the returned
    // value is null. 
    KanjiEntry* itemKanji(int index) const;

    // Index of the kanji owner of the item at element index. If no kanji owner is set for the
    // element, -1 is returned.
    int itemKanjiIndex(int index) const;

    // The character owner of the item at index. If no unicode character or kanji is set for
    // an element, the returned value is null.
    QChar itemUnicode(int index) const;

    // Fills result with parts indexes of the element at index. The result list is not
    // cleared. Set skipelements to true to not list parts that are not kanji in themselves.
    // Setting usekanji to true will return kanji values as positive numbers and elements as
    // negative numbers with the formula: -1 - [element index].
    // The same formula can be used to get the original element index back.
    // Otherwise positive values are returned for the elements.
    void elementParts(int index, bool skipelements, bool usekanji, std::vector<int> &result) const;
    // Fills result with parents indexes of the element at index. Parent elements are those
    // that the element is used as their part. The result list is not cleared.  Set
    // skipelements to true to not list parts that are not kanji in themselves.
    // Setting usekanji to true will return kanji values as positive numbers and elements as
    // negative numbers with the formula: -1 - [element index].
    // The same formula can be used to get the original element index back.
    // Otherwise positive values are returned for the elements.
    void elementParents(int index, bool skipelements, bool usekanji, std::vector<int> &result) const;

    // Draws every stroke of an element's variant with painter in the center of the passed
    // rectangle. Set animated to true when it's important that the drawn element strokes are
    // made up of around the same number of parts independent of the rectangle size.
    void drawElement(QPainter &painter, int element, int variant, const QRectF &rect, bool animated);

    // Returns the number of strokes that make up a variant of an element.
    uchar strokeCount(int element, int variant) const;

    // Number of parts a stroke in an element's variant can be drawn. Pass a valid pixel size
    // in partlen. It'll be used as a unit when dividing a line or curve in the stroke. The
    // partcnt vector receives the parts for each separate segment. This must be passed
    // unchanged to drawStrokePart when drawing. Call this function For each segment before
    // drawing.
    int strokePartCount(int element, int variant, int stroke, const QRectF &rect, double partlen, std::vector<int> &parts) const;

    // Returns the data of the stroke in an element's variant. Namely the starting direction
    // of the stroke and the point where it's first drawn, if it is to be drawn in the passed
    // rectangle.
    void strokeData(int element, int variant, int stroke, const QRectF &rect, StrokeDirection &dir, QPoint &startpoint) const;

    // Returns the width of the pen used for the middle weight lines depending on minsize,
    // which should be the smaller size of the rectangle where the element will be drawn.
    double basePenWidth(int minsize) const;

    // Draws a single stroke of a variant of an element with painter in the center of the
    // passed rectangle. The rectangle should be the size of the fully drawn kanji. Pass
    // valid colors in startcolor and endcolor to draw the stroke with a gradient.
    // Use the same partlen that is passed to strokePartCount() when the same view must show
    // animation and full stroke too, to make them draw from the same parts. When it's not
    // set, partlen is the smaller size of rectangle divided by 50.
    void drawStroke(QPainter &painter, int element, int variant, int stroke, const QRectF &rect, double partlen = 0.0, QColor startcolor = QColor(), QColor endcolor = QColor());

    // Draws part of a stroke in an element's variant with the painter. When drawing curves,
    // only the passed part bit is painted. When drawing straight lines, the line is painted
    // from its starting point to the endpoint of the current part. This can result in
    // straight line parts painted on top of previous parts.
    // Set partialline to false to either draw curve part, or a full line. Setting it to true
    // will avoid drawing curves, and only draw the straight line, if the passed part is the
    // last one to draw.
    // Part is the current part to draw, parts is the result of calling strokePartCount() with
    // the same arguments.
    // Pass valid colors in startcolor and endcolor to draw the stroke with a gradient. The
    // colors should refer to the full stroke, not just to the part.
    void drawStrokePart(QPainter &painter, bool partialline, int element, int variant, int stroke, const QRectF &rect, const std::vector<int> &parts, int part, QColor startcolor = QColor(), QColor endcolor = QColor()) const;
private:
    KanjiElementList();
    KanjiElementList(const QString &filename);

    // Returns the kanji variant's stroke at sindex. The r rectangle should be the position
    // and dimensions where the variant would be drawn. The transformation is updated to be
    // used with stroke to make it fit in r.
    const ElementStroke* findStroke(const KanjiElement *e, const ElementVariant *v, int sindex, QRectF r, ElementTransform &tr) const;

    int strokePartCount(const ElementStroke *s, const ElementTransform &tr, double partlen, bool animated, std::vector<int> &parts) const;

    void strokeData(const ElementStroke *s, const ElementTransform &tr, StrokeDirection &dir, QPoint &startpoint) const;

    // Draws a kanji stroke transformed by tr with painter. The width of the stroke can change
    // depending on its tips, but the normal width is passed in basewidth.
    // Set areasize to the width/height of the square where the variant's stroke is to be
    // drawn. This will help determine the number of steps needed to cut up a bezier curve to
    // make it look smooth.
    //void drawStroke(QPainter &painter, double areasize, double basewidth, ElementStroke *s, const ElementTransform &tr, QColor startcolor, QColor endcolor);

    // Creates a linear gradient which is used when drawing stroke parts. The passed
    // coordinates are for the parts to be drawn, startcnt is the number of parts coming
    // before the drawn one, partcnt is the number of parts the drawn part represents, and
    // endcnt is the number of parts still coming after the drawn part. Pass the base
    // stroke width in dotsize to set the size drawn with the startcolor.
    // The resulting gradient will be as large as the full stroke would be, placed correctly
    // over the part being drawn. The startcolor will be used to draw a visually recognizable
    // "spot" at the beginning of the stroke, while endcolor is for the rest of the stroke.
    QLinearGradient paintGradient(double x1, double y1, double x2, double y2, double dotsize, int startcnt, int partcnt, int endcnt, QColor startcolor, QColor endcolor) const;

    // Draws a kanji stroke part transformed by tr with painter. The width of the stroke can
    // change depending on its tips, but the normal width is passed in basewidth.
    // Set areasize to the width/height of the square where the variant's stroke is to be
    // drawn. This will help determine the number of steps needed to cut up a bezier curve to
    // make it look smooth.
    void drawStrokePart(QPainter &painter, bool partialline, double basewidth, const ElementStroke *s, const ElementTransform &tr, const std::vector<int> &parts, int part, QColor startcolor, QColor endcolor) const;

    // Draws a line with painter between the starting and ending points with pen widths of
    // startw and endw.
    void drawLine(QPainter &painter, const QPointF &start, const QPointF &end, double startw, double endw) const;

    // Approximates the length of a cubic bezier. The maxerror is NOT the error between the
    // approximated length and real length. The bezier is divided until each part can be
    // approximated close enough. The maxerror is the acceptable difference between the length
    // of a straight line of start->end, and start->c1->c2->end.
    double bezierLength(const QPointF &start, const QPointF &c1, const QPointF &c2, const QPointF &end, double maxerror) const;

    // Returns all generated mid points (sc1, sc2, ec1, ec2) and the actual point on the
    // bezier (m) at the given location between [0 - 1]. If the bezier needs to be divided at
    // m, the two new beziers are made up from start->sc1->sc2->m and m->ec1->ec2->end.
    // Note: this is not the fastest way when drawing.
    void _bezierPoints(const QPointF &start, const QPointF &c1, const QPointF &c2, const QPointF &end, double at, QPointF &sc1, QPointF &sc2, QPointF &m, QPointF &ec1, QPointF &ec2) const;

    // Draws a quadratic bezier curve with painter between the starting and ending points
    // throught the 2 control points. The width of the pen is gradually changed between startw
    // and endw. Set areasize to the width/height of the square where the variant's stroke is
    // to be drawn. This will help determine the number of steps needed to cut up a bezier
    // curve to make it look smooth.
    void drawBezier(QPainter &painter, double areasize, const QPointF &start, const QPointF &control1, const QPointF &control2, const QPointF &end, double startw, double endw) const;

    // Loads the data to a list of strokes that are used as model strokes for kanji. When
    // matching handwritten kanji to stored kanji, the drawn strokes are matched with strokes
    // stored in models, and the indexes of model strokes are matched with the recognizer
    // kanji data.
    void loadModels(QDataStream &stream, std::vector<Stroke> &dest);

    // Loads recognizer element repositories. Each repository is a named list of element and
    // variant indexes. The repos are only used in the editor for easier kanji element and
    // variant lookup.
    void loadRepos(QDataStream &stream);

    // The element's every variant can have a user defined name. The names are shown in the
    // handwriting recognizer window's candidate list for example to name characters that are
    // not kanji.
    void loadVariantNames(QDataStream &stream);

    // Difference in position betwee two position bit arrays. Used in handwriting recognition.
    int posDiff(const BitArray &p1, const BitArray &p2);

    // File version after loading.
    int version;

    // List of all kanji elements made up of other elements. Many of them are actual
    // characters.
    smartvector<KanjiElement> list;

    // Model strokes which make up a kanji. When recognizing handwritten characters, the user
    // drawn strokes are matched with the strokes in this list.
    std::vector<Stroke> models;

    // Model strokes which make up non-kanji. When recognizing handwritten characters, the
    // user drawn strokes are matched with the strokes in this list.
    std::vector<Stroke> cmodels;

    // A repository of kanji elements. Used in the original zkanji stroke editor. Kept here in
    // case it'll be implemented later.
    smartvector<ElementRepo> repos;

    // An element's every variant can have a user defined name. The names are shown in the
    // handwriting recognizer window's candidate list for example to name characters that are
    // not kanji. The map contains the unicode for the element's character and the name given
    // to it.
    std::map<int, QCharString> varnames;

    // [kanji index, element index] loaded when loading the KanjiElementList. Only used during
    // startup, before the base dictionary is loaded. Afterwards the elements of the kanji are
    // filled with the values stored here, and this list is cleared.
    std::map<int, int> kanjimap;

    friend void ZKanji::initElements(const QString &filename);
    friend KanjiElementList* ZKanji::elements();
};

#endif // RECOGNIZER_H

