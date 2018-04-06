/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef FORMSTATE_H
#define FORMSTATE_H

// Collection of classes that can save and restore the state (size, widget data etc.) of
// windows in the program.

#include <QString>
#include <QRect>
#include <QObject>

#include <map>
#include <vector>

class QMainWindow;
class CollectWordsForm;
class QSplitter;

class QXmlStreamWriter;

struct SplitterFormData
{
    // Top left corner of the window with frame.
    //QPoint pos;
    // Top left corner of screen that contained the window last.
    //QPoint screenpos;

    // Size of the window geometry without frame.
    QSize siz;
    // Splitter widget sizes.
    int wsizes[2];
};


struct ListStateData
{
    // List holding pairs of [column index, column width].
    std::vector<std::pair<int, int>> columns;
};

enum class SearchMode : uchar;
enum class ExampleDisplay : uchar;
enum class Inclusion : uchar;

struct DictionaryWidgetData
{
    SearchMode mode = (SearchMode)1;
    bool multi = false;
    bool filter = false;

    bool showex = true;
    ExampleDisplay exmode = (ExampleDisplay)0;

    bool frombefore = false;
    bool fromafter = true;
    bool fromstrict = false;
    bool frominfl = false;

    bool toafter = true;
    bool tostrict = false;

    Inclusion conditionex = (Inclusion)0;
    Inclusion conditiongroup = (Inclusion)0;

    // List holding pairs of [Filter name, Inclusion data] pairs.
    std::vector<std::pair<QString, Inclusion>> conditions;

    // Only saving sort and tabledata if this is set to true. WordStudyListForm has to save
    // column states separately, and saving them here is useless.
    bool savecolumndata = true;

    int sortcolumn = -1;
    Qt::SortOrder sortorder = Qt::AscendingOrder;

    ListStateData tabledata;
};

struct CollectFormData
{
    QString kanjinum = "3";
    int kanalen = 8;
    int minfreq = 1000;

    bool strict = true;
    bool limit = true;

    // Saved dimensions of the window.
    QSize siz;

    DictionaryWidgetData dict;
};

struct KanjiInfoData
{
    // Window dimensions.
    QSize siz;

    // Position of the window.
    QPoint pos;

    // Screen position and size. If this doesn't match when position is to be restored, the
    // window will be shown at the default position instead.
    QRect screen;

    bool grid = true;
    bool sod = true;

    bool words = false;
    bool similar = false;
    bool parts = false;
    bool partof = false;

    bool shadow = false;
    bool numbers = false;
    bool dir = false;
    int speed = 3;

    // Splitter widget sizes. First value is the stroke diagram with all its controls and
    // kanji information, the second is the dictionary list. The first value is only used when
    // the dictionary words listing should be also visible. Otherwise when opening the
    // splitter, the first value is based on the current size of the top part.
    int toph = -1;
    int dicth = -1;

    // The dictionary widget is set to show kanji example words.
    bool refdict = false;
    DictionaryWidgetData dict;
};

struct KanjiInfoFormData
{
    // The information window is locked. Getting information of another kanji will force open
    // the next window.
    bool locked = false;

    int kindex;
    QString dictname;

    KanjiInfoData data;
};

enum class DeckItemViewModes : int;
enum class DeckStatPages : int;
enum class DeckStatIntervals : int;

struct WordStudyListFormDataItems
{
    bool showkanji = true;
    bool showkana = true;
    bool showdef = true;

    DeckItemViewModes mode = (DeckItemViewModes)0;

    DictionaryWidgetData dict;

    struct SortData
    {
        int column = -1;
        Qt::SortOrder order;
    };

    SortData queuesort;
    SortData studysort;
    SortData testedsort;

    // Column widths.
    std::vector<int> queuesizes;
    std::vector<int> studysizes;
    std::vector<int> testedsizes;

    // Column visibility.
    std::vector<char> queuecols;
    std::vector<char> studycols;
    std::vector<char> testedcols;
};

struct WordStudyListFormDataStats
{
    DeckStatPages page = (DeckStatPages)0;
    DeckStatIntervals itemsinterval = (DeckStatIntervals)0;
    DeckStatIntervals forecastinterval = (DeckStatIntervals)1;
};

struct WordStudyListFormData
{
    // Window dimensions.
    QSize siz;
    WordStudyListFormDataItems items;
    WordStudyListFormDataStats stats;
};

struct PopupDictData
{
    // Position of the popup dictionary. When false, it sits at the bottom of the screen.
    bool floating = false;
    // Popup dictionary window's position when floating. The top left corner of the rectangle
    // is relative to the screen the popup dictionary is on.
    QRect floatrect;
    // Popup dictionary window's size when not floating.
    QSize normalsize;

    DictionaryWidgetData dict;
};

struct KanarPracticeData
{
    // Whether the hiragana at vector positions are to be tested or not. See
    // kanapracticesettingsform.h enum KanaSounds for list.
    std::vector<uchar> hirause;
    std::vector<uchar> katause;
};

struct RecognizerFormData
{
    // Whether the grid is shown in the handwriting recognizer window.
    bool showgrid = true;
    // Whether kana and other characters are accepted in the handwriting recognizer window.
    bool allresults = true;

    // Saved position and size of the handwriting recognizer.
    QRect rect;

    // Top left corner of screen that contained the window last.
    QRect screen;
};


struct KanjiFilterData;
struct PopupKanjiData;
class QXmlStreamWriter;
class QXmlStreamReader;

namespace FormStates
{
    // Saved state of dialog windows with splitter.
    extern std::map<QString, SplitterFormData> splitters;

    // Saved size of dialog windows.
    extern std::map<QString, QSize> sizes;

    // Saved state of the CollectWordsForm window.
    extern CollectFormData collectform;

    extern KanjiInfoData kanjiinfo;
    extern WordStudyListFormData wordstudylist;
    extern PopupKanjiData popupkanji;
    extern PopupDictData popupdict;
    extern KanarPracticeData kanapractice;
    extern RecognizerFormData recognizer;

    bool emptyState(const SplitterFormData &data);
    bool emptyState(const CollectFormData &data);
    bool emptyState(const DictionaryWidgetData &data);
    bool emptyState(const ListStateData &data);
    bool emptyState(const KanjiInfoData &data);
    bool emptyState(const WordStudyListFormDataItems &data);
    bool emptyState(const WordStudyListFormDataStats &data);
    bool emptyState(const WordStudyListFormData &data);
    bool emptyState(const KanjiFilterData &data);
    bool emptyState(const PopupKanjiData &data);
    bool emptyState(const PopupDictData &data);
    bool emptyState(const RecognizerFormData &data);

    void saveXMLSettings(const SplitterFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(SplitterFormData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const CollectFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(CollectFormData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const DictionaryWidgetData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(DictionaryWidgetData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const ListStateData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(ListStateData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const KanjiInfoData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(KanjiInfoData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const KanjiInfoFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(KanjiInfoFormData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const WordStudyListFormDataItems &data, QXmlStreamWriter &writer);
    void loadXMLSettings(WordStudyListFormDataItems &data, QXmlStreamReader &reader);

    void saveXMLSettings(const WordStudyListFormDataStats &data, QXmlStreamWriter &writer);
    void loadXMLSettings(WordStudyListFormDataStats &data, QXmlStreamReader &reader);

    void saveXMLSettings(const WordStudyListFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(WordStudyListFormData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const KanjiFilterData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(KanjiFilterData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const PopupKanjiData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(PopupKanjiData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const PopupDictData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(PopupDictData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const KanarPracticeData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(KanarPracticeData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const RecognizerFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(RecognizerFormData &data, QXmlStreamReader &reader);

    void saveDialogSplitterState(QString statename, QMainWindow *window, QSplitter *splitter);
    void restoreDialogSplitterState(QString statename, QMainWindow *window, QSplitter *splitter);

    void loadXMLDialogSplitterState(QXmlStreamReader &reader);

    // Saves the current geometry size of the passed window in a map with `sizename` as the
    // key. Use restoreDialogSize() to restore the saved size for the name if it exists.
    void saveDialogSize(QString sizename, QMainWindow *window);
    // Restores size of a window saved under `sizename`. If `installcloseevent` is true, an
    // event filter is installed for closeEvent(), that will call saveDialogSize() with the
    // same name and window. Otherwise saveDialogSize() must be called manually at the
    // appropriate time. Only allow the event filtering when the closeEvent() won't reject the
    // close of a window, otherwise the size might be saved early and new windows might block
    // the one already open.
    void restoreDialogSize(QString sizename, QMainWindow *window, bool installcloseevent);

    void saveXMLDialogSize(const QSize size, QXmlStreamWriter &writer);
    void loadXMLDialogSize(QXmlStreamReader &reader);

    // Used by restoreDialogSize() as the object for installing event filters.
    class RestoreDialogHelperPrivate : public QObject
    {
        Q_OBJECT
    public:
        void installFor(QString sizename, QMainWindow *window);
    protected:
        virtual bool eventFilter(QObject *o, QEvent *e) override;
    protected slots:
        void windowDestroyed(QObject *o);
    private:
        std::map<QMainWindow*, QString> filtered;

        typedef QObject base;
    };
}


#endif // FORMSTATE_H

