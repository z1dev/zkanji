/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZUI_H
#define ZUI_H

#include <QValidator>
#include <QPixmap>
#include <QMessageBox>
#include <QBoxLayout>
#include "smartvector.h"

// Classes and functions used globally by several parts of the interface, that are
// directly not related to the workings of the core zkanji itself.

class DictionaryGroupItemModel;
class GroupTreeModel;
class Dictionary;
class WordGroup;
class KanjiGroup;
class KanjiGroupModel;
class MainKanjiListModel;
class RadicalFiltersModel;
class JapaneseValidator;
class QXmlStreamWriter;
class QXmlStreamReader;
class QPainter;
class QMainWindow;
class QLabel;
class QByteArray;

// Called at program initialization after the user data has loaded,
// to create the models for each data view.
//void generateStore(Dictionary *d);

// Retrieves the model for the word group trees.
//GroupTreeModel* wordGroupTreeModel(Dictionary *d);
// Retrieves the model for the kanji group trees.
//GroupTreeModel* kanjiGroupTreeModel(Dictionary *d);

// Retrieves the model of kanji representing every kanji in the program.
MainKanjiListModel& mainKanjiListModel();
// Retrieves the model which holds data of previously searched radicals.
RadicalFiltersModel& radicalFiltersModel();
// Retrieves a validator that finds any Japanese input valid.
JapaneseValidator& japaneseValidator();
// Retrieves a validator that finds only kana input valid.
JapaneseValidator& kanaValidator();

// Saves the currently stored radical filters in xml format with the passed writer.
void saveXMLRadicalFilters(QXmlStreamWriter &writer);
// Reads radical filters from an xml with the passed reader.
void loadXMLRadicalFilters(QXmlStreamReader &reader);
// Reads word filters from an xml with the passed reader.
void saveXMLWordFilters(QXmlStreamWriter &writer);
// Saves the currently stored word filters in xml format with the passed writer.
void loadXMLWordFilters(QXmlStreamReader &reader);
// Returns whether there are any custom word filters stored.
bool wordFiltersEmpty();


//// Returns the name of the font used for kanji in a kanji grid.
//QString kanjiFontName();
//// Returns the name of the font used for words (both written form and kana
//// readings) in the dictionary.
//QString kanaFontName();
//// Returns the name of the font used for word definitions in the dictionary.
//QString meaningFontName();
//FontStyle meaningFontStyle();
//// Returns the name of the font used for information text in the dictionary.
//QString smallFontName();
//FontStyle smallFontStyle();
//// Returns the name of the font used for information text in the dictionary.
//QString extrasFontName();
//FontStyle extrasFontStyle();

//// Returns the name of the font used for words (both written form and kana
//// readings) when printing.
//QString printKanaFontName();
//// Returns the name of the font used for word definitions when printing.
//QString printMeaningFontName();
//// Returns the name of the font used for word type text when printing.
//QString printSmallFontName();

// Draws text with painter at x and y with the specified clip rect. The text is placed to draw
// its baseline at basey. Set hcenter to align text horizontally to the center of the passed
// clip rectangle. In that case x is ignored.
void drawTextBaseline(QPainter *p, int x, int basey, bool hcenter, QRect clip, QString str);
// Draws text with painter at pos with the specified clip rect. The text is placed to draw
// its baseline at pos.y(). Set hcenter to align text horizontally to the center of the passed
// clip rectangle. In that case the x coordinate of pos is ignored.
void drawTextBaseline(QPainter *p, QPointF pos, bool hcenter, QRectF clip, QString str);

// Updates the passed font's size to fit a given height. Pass a string which
// will be used for measurement. This string can be anything, but it's best
// if it holds characters that extend vertically the most.
void adjustFontSize(QFont &f, int height, QPainter *p = nullptr/*, QString str*/);

// Used in restrictWidgetSize to specify whether the minimum, the maximum or both width
// restriction sizes should be set.
enum class AdjustedValue { Min, Max, MinMax };

// Updates the minimum and/or maximum width of the passed widget, to fit charnum number of
// average width characters. The measurement uses the currently set font in the widget.
void restrictWidgetSize(QWidget *widget, double charnum, AdjustedValue val = AdjustedValue::MinMax);
// Returns the width in pixels for widget if it was restricted to the given charnum number of
// average width characters using the widget's current font.
int restrictedWidgetSize(QWidget *widget, double charnum);
// Updates the minimum and/or maximum width of the passed widget, to fit charnum number of
// wide characters. The measurement uses the currently set font in the widget.
void restrictWidgetWiderSize(QWidget *widget, double charnum, AdjustedValue val = AdjustedValue::MinMax);
// Returns the width in pixels for widget if it was restricted to the given charnum number of
// wide characters using the widget's current font.
int restrictedWidgetWiderSize(QWidget *widget, double charnum);

// Updates layouts and geometry of the given widget without showing its window.
void updateWindowGeometry(QWidget *widget);


// Returns the width of the label's current text.
int fixedLabelWidth(QLabel *label);
// Updates the layout of a widget for each parent of QLabels to have the size policy set to
// QLayout::SetMinimumSize which ensures that word wrapping labels fit and get resized
// correctly.
void fixWrapLabelsHeight(QWidget *form, int labelwidth
#ifdef _DEBUG
    , QSet<QString> *fixed = nullptr, QSet<QString> *notfixed = nullptr
#endif
);

void helper_createStatusWidget(QWidget *w, QLabel *lb1, QString lbstr1, double sizing1);

template<typename... PARAMS>
void helper_createStatusWidget(QWidget *w, QLabel *lb1, QString lbstr1, double sizing1, QLabel *lb2, QString lbstr2, double sizing2, PARAMS... params)
{
    helper_createStatusWidget(w, lb1, lbstr1, sizing1);
    helper_createStatusWidget(w, lb2, lbstr2, sizing2, params...);
}


// Creates a widget that can be added to a status bar. Parent is the widget responsible for
// the destruction of the created widget. Labels are added to the widget into a horizontal
// layout with the initial strings of lbstr%. Every label lb% can be a null pointer, but if
// any other value is passed, it must be a valid label which will be used. The value of
// sizing% determines the sizes of the label. If it's negative, the size of the label will be
// horizontally expanding, if 0, it will have a fixed size. A positive value will restrict the
// size of the label to that number of average character widths of the current font used. Set
// the spacing to other than -1, to change the default spacing of the horizontal layout.
template<typename... PARAMS>
QWidget* createStatusWidget(QWidget *parent, int spacing, QLabel *lb1, QString lbstr1, double sizing1, PARAMS... params)
{
    QWidget *w = new QWidget(parent);
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, w);
    layout->setMargin(0);
    if (spacing != -1)
        layout->setSpacing(spacing);

    helper_createStatusWidget(w, lb1, lbstr1, sizing1, params...);

    return w;
}

struct SARButton
{
    QString text;
    QMessageBox::ButtonRole role;
    SARButton(const QString &text, QMessageBox::ButtonRole role) : text(text), role(role) {}
    SARButton(const SARButton& btn) : text(btn.text), role(btn.role) {}
    SARButton& operator=(const SARButton& btn) { text = btn.text; role = btn.role; return *this; }
};

// Shows a custom message box with title, text and details string. Pass the
// button string and button roles for the buttons that will be added to the
// dialog. Returns the index of the button that was clicked.
int showAndReturn(QString title, QString text, QString details, const std::vector<SARButton> &buttons, int defaultbtn = -1);
// Shows a custom message box with title, text and details string. Pass the
// button string and button roles for the buttons that will be added to the
// dialog. Returns the index of the button that was clicked.
int showAndReturn(QWidget *parent, QString title, QString text, QString details, const std::vector<SARButton> &buttons, int defaultbtn = -1);


// Returns a short string representation of the given date time's date portion. The string in
// unset is returned if the date time is invalid.
QString formatDate(const QDateTime &d, QString unset = QString());
// Returns a short string representation of the given date. The string in unset is returned if
// the date time is invalid.
QString formatDate(const QDate &d, QString unset = QString());
// Returns a short string representation of the given date time's date portion.  The string in
// unset is returned if the date time is invalid.
QString formatDateTime(const QDateTime &d, QString unset = QString());
// Returns the string representation of the interval given in milliseconds. The returned value
// is in the form of N days, or N months etc.
QString formatSpacing(quint32 msec);
// Fixes the passed date time to represent the long term study day of the date and returns a
// short string representation of its date portion.
QString fixFormatDate(const QDateTime &d, QString unset);
// Fixes the passed date time to represent the long term study day of the date and returns a
// short string representation of its date portion. If the date is on the same study date as
// today, the string "Today" or its translation is returned instead.
QString fixFormatPastDate(const QDateTime &d);
// Fixes the passed date time to represent the long term study day of the date and returns a
// short string representation of its date portion. If the date is in the past, "Due" or a
// translation is returned.
QString fixFormatNextDate(const QDateTime &d);

// Returns a modified date time to represent the given day of the long-term study list, by
// subtracting hours from it. Beware that the time part is not cleared, though its value
// becomes unusable.
QDate ltDay(const QDateTime &d);


// Pass an image pointer and a path to an SVG file (usually resource.) If the
// image holds a null pointer, the function creates the image with the
// specified width and height, loads the SVG and paints it onto the image.
// The rendered SVG picture will take up the full dimensions of the image,
// even if this stretches the picture. When img does not hold null, the call
// does nothing with it.
// Returns the (new) pointer in img.
//QImage* makeImageFromSvg(QImage* &img, QString svgpath, int width, int height);

// Returns an image for rendering from a given SVG. The image will be of size width*height.
// The path can be a resource, for example ":/resourcename.SVG" to read the resource image.
// The created image is cached, and the same image is returned if calling the function with
// the same width and height. Multiple versions can be cached of the same image by passing
// a different value in version, to use different sizes of the same image at the same time.
// The svgpath is case-sensitive, the value of version must be 0 or positive.
QImage imageFromSvg(QString svgpath, int width, int height, int version = 0);

// Renders the SVG specified by svgpath on dest in the r rectangle. Dest must have been
// created in QImage::Format_ARGB32_Premultiplied format. The SVG image is rendered on dest
// without filling the background.
//QImage renderFromSvg(QImage &dest, QString svgpath, QRect r);

// Renders the SVG image specified by svgpath on a QPixmap of size (w, h) in the r rectangle.
// The result is a pixmap with the SVG image rendered on a transparent background.
QPixmap renderFromSvg(QString svgpath, int w, int h, QRect r);

//void renderFromSvg(QPainter &dest, QString svgpath, QRect r);

// Adds a triangle at the bottom right corner of an image to be displayed as icon on buttons.
// Because the image is not scalable, this must be called with a pre-scaled pixmap
QPixmap triangleImage(const QPixmap &img);

// Returns the size of the image returned by triangleImage(), if the argument had siz size.
// Because that function works with pre-scaled image, siz must be scaled.
QSize triangleSize(const QSize &siz);

// Index of screen holding r rectangle, or -1 if no screen intersects with r. When multiple
// screens intersect the rectangle, the first one with the highest intersecting area is
// returned.
int screenNumber(const QRect &r);

// Loads an SVG image and renders it as a QImage at the given size, returning the picture.
// Set ori to the color used originally in the SVG image and col to the replacement color.
// Only works on SVG images where the textual representation of the 'ori' color is found.
QImage loadColorSVG(QString name, QSize size, QColor ori, QColor col);


enum class Flags { FromJapanese, ToJapanese, Flag, Browse };
namespace ZKanji
{
    // Returns a pixmap rendering of an image specified for the dictionary named dictname,
    // with the passed size. The flags argument specifies the type of flag image returned,
    // which can be paired with the Japanese flag.
    QPixmap dictionaryFlag(QSize size, QString dictname, Flags flag);
    // Returns a pixmap rendering of an image for a dictionary at the default icon size for
    // menus.
    QPixmap dictionaryMenuFlag(const QString &dictname);
    // Erases all cached image data of the dictionary named dictname.
    void eraseDictionaryFlag(const QString &dictname);

    // Caches a data array, which must contain a valid SVG image file as the flag image of the
    // dictionary. This image will be returned by dictionaryFlag() and similar functions after
    // the call. Returns whether the array was a valid image. If returns false, no new flag
    // is assigned to the dictionary.
    bool assignDictionaryFlag(const QByteArray &data, const QString &dictname);
    // Clears saved data of the dictonary flag image. Calls to dictionaryFlag() and similar
    // functions will result in the default image afterwards, unless assignDictionaryFlag() is
    // called again.
    void unassignDictionaryFlag(const QString &dictname);
    // Fills result with the cached dictionary flag image set by assignDictionaryFlag() if
    // it's found. Returns true on success.
    bool getCustomDictionaryFlag(const QString &dictname, QByteArray &result);
    // Updates the custom dictionary flag cache to reflect the changed name of a dictionary.
    // The new name shouldn't conflict with other names.
    void changeDictionaryFlagName(const QString &oldname, const QString &dictname);
    // Returns whether an dictionary has a flag assigned to it with assignDictionaryFlag().
    bool dictionaryHasCustomFlag(const QString &dictname);
}

// Mixes the rgb components of color a and color b separately, returning the result. Color a
// and color b are mixed in the ratio defined by a_part, which specifies how much of color a
// will be used in the result between 0 and 1.0.
QColor mixColors(const QColor &a, const QColor &b, double a_part = (double)0.5);

// Given a 'base' color (for unselected items) and a 'curr' current paint color for 'base'
// (either same as 'base' if the item to paint is unselected, or a highlight color), returns
// a new paint color for the item, if 'col' is to be used as its new base color.
// For example base is normal background color, curr is background color when something is
// selected. Returns what curr will become, if base is replaced by col.
QColor colorFromBase(const QColor &base, const QColor &curr, const QColor &col);

// Deletes a cached image created from an SVG at the passed svgpath. Pass a version number
// to specify which version should be deleted, or -1 to delete every version of the same
// image.
void deleteSvgImageCache(QString svgpath, int version = -1);


// Functions for drawing checkboxes in widgets.
enum class CheckBoxMouseState {
    None = 0,
    Hover = 1,
    Down = 2,
    DownHover = Hover | Down,
};

class QStyleOptionButton;
// Returns draw options for measuring or drawing a single checkbox in a widget.
// Pass the widget which will paint the checkbox. Set mstate to the mouse state for the
// checkbox to be drawn. Pass whether the checkbox should be drawn enabled.
void initCheckBoxOption(const QWidget *widget, CheckBoxMouseState mstate, bool enabled, Qt::CheckState chkstate, QStyleOptionButton *opt);
// Returns the default size of a checkbox.
QSize checkBoxSize();
// Returns the painted rectangle of a checkbox inside a widget, using the style of the widget
// and a rectangle where the checkbox is placed. The actual checkbox is at the left edge and
// centered vertically in the passed rectangle. Set leftpadding to move the rectangle by
// some pixels to the right.
QRect checkBoxRect(const QWidget *widget, int leftpadding, QRect rect, QStyleOptionButton *opt);
// Same as checkBoxRect, used for hit testing.
QRect checkBoxHitRect(const QWidget *widget, int leftpadding, QRect rect, QStyleOptionButton *opt);
// Draws the checkbox in the passed rectangle in widget, with the given padding. The checkbox
// is at the left edge and centered vertically in the passed rectangle. The rectangle in the
// passed options is updated to the checkbox's real rectangle.
void drawCheckBox(QPainter *painter, const QWidget *widget, int leftpadding, QRect rect, QStyleOptionButton *opt);

bool operator==(QKeyEvent *e, const QKeySequence &seq);
bool operator==(const QKeySequence &seq, QKeyEvent *e);

class QSignalMapper;
class QAction;

class JapaneseValidator : public QValidator
{
    Q_OBJECT

public:
    JapaneseValidator(bool acceptkanji, bool acceptsymbols, QObject *parent = nullptr);
    virtual void fixup(QString &input) const override;
    virtual State validate(QString &input, int &pos) const override;

    // Returns whether the given charater is acceptable in a valid input.
    bool validChar(ushort ch) const;
private:
    bool acceptkanji;
    bool acceptsymbols;

    typedef QValidator  base;
};

// Validator for ZLineEdit widgets for text in the format [from]-[to].
class IntRangeValidator : public QValidator
{
    Q_OBJECT
public:
    IntRangeValidator(QObject *parent = nullptr);
    IntRangeValidator(quint32 minimum, quint32 maximum, bool allowempty, QObject *parent = nullptr);
protected:
    virtual void fixup(QString &input) const;
    virtual State validate(QString &input, int &pos) const;
private:
    quint32 lo;
    quint32 hi;
    bool allowempty;
    mutable QString lastgood;
    typedef QValidator  base;
};

// Class for the MOC to generate the translation strings of date and time
// formatting. Shouldn't be instantiated anywhere outside zui.cpp.
// Call the static date formatting functions instead.
class DateTimeFunctions : public QObject
{
    Q_OBJECT
public:
    // Values passed to the format functions to get a specific string
    // representation of a date or time. The result depends on the current
    // language. The examples given are just general guidelines.
    enum FormatTypes {
        // Date without time in the form of eg. yyyy.MM.dd
        NormalDate,
        // Like a normal date, but corrected to the long-term study day
        DayFixedDate,
        // Date and time in the form of eg. yyyy.MM.dd hh:mm:ss
        NormalDateTime,
    };

    // Returns the format string for a full date corresponding to the current settings.
    static QString dateFormatString();

    // Returns the format string for a time corresponding to the current settings.
    static QString timeFormatString();

    // Formats a string for the passed date time in the passed format. Set utc to true if the
    // date should be shown in UTC time instead of local.
    static QString format(QDateTime dt, FormatTypes type, bool utc = false);

    // Returns the string representation of a date depending on the program settings to
    // determine order of parts.
    static QString formatDay(QDate dt);

    // Formats date to string that show when was the last test, with words like "Today" or
    // "Yesterday" or the date if these are not valid. The passed date's time must be UTC.
    static QString formatPast(QDateTime dt);
    static QString formatNext(QDateTime dt);

    // Formats date to string that show when was the last test, with words like "Today" or
    // "Yesterday" or the date if these are not valid.
    static QString formatPastDay(QDate dt);

    // Formats number of seconds as Xd Xy Xm Xd Xm Xs (decades, years, months, days, minutes,
    // seconds). A month will be calculated as 30.417 days, which is the average. 0 length
    // time parts will be left out.
    static QString formatLength(int sec);

    // Formats a string for the passed seconds length spacing in a human readable format.
    // Using words like days, years etc.
    static QString formatSpacing(quint32 msec);

    // Returns a modified date time to represent the given day of the long-term
    // study list, by subtracting hours from it. Beware that the time part is
    // not cleared, though its value becomes unusable.
    static QDate getLTDay(const QDateTime &dt);

    // Returns a string formatting a number as passed time, like the format 00:00:00. If
    // 'showhours' is false, only the minutes and seconds are returned.
    static QString formatPassedTime(int seconds, bool showhours);
private:
    DateTimeFunctions(QObject *parent = nullptr);
    ~DateTimeFunctions();

    typedef QObject base;
};


#endif // ZUI_H

