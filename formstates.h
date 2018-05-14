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

struct FormSizeStateData
{
    // Size of the window geometry without frame.
    QSize siz;
    // Whether window was maximized when closed and should be restored maximized.
    bool maximized;
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

    bool loop = false;
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

enum class DeckItemViewModes;
enum class DeckStatPages;
enum class DeckStatIntervals;

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

    // Saved size and maximized state of dialog windows.
    extern std::map<QString, FormSizeStateData> maxsizes;

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

    void saveDialogSplitterState(QString statename, const QSize &siz, int size1, int size2);
    void restoreDialogSplitterState(QString statename, QSize &siz, int &size1, int &size2);

    // Saves the window size and the size of widgets in the passed splitter under the given
    // state name. Pass second widget size in size2 if the second widget is hidden. Otherwise
    // the value is ignored.
    void saveDialogSplitterState(QString statename, QMainWindow *window, QSplitter *splitter, const int *size2 = nullptr);
    // Restores the size and widget sizes in a splitter for window under the given state name.
    // Receive a value in size2 if the second widget is hidden and won't be restored, to get
    // its last saved value.
    void restoreDialogSplitterState(QString statename, QMainWindow *window, QSplitter *splitter, int *size2 = nullptr);

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

    // Same as saveDialogSize() but also saves maximized state of window.
    void saveDialogMaximizedAndSize(QString sizename, QMainWindow *window);
    // Same as restoreDialogSize() but also restores maximized state of window.
    void restoreDialogMaximizedAndSize(QString sizename, QMainWindow *window, bool installcloseevent);

    void saveXMLDialogSize(const QSize size, QXmlStreamWriter &writer);
    void loadXMLDialogSize(QXmlStreamReader &reader);

    void saveXMLDialogMaximizedAndSize(const FormSizeStateData dat, QXmlStreamWriter &writer);
    void loadXMLDialogMaximizedAndSize(QXmlStreamReader &reader);

    // Used by restoreDialogSize() as the object for installing event filters.
    class RestoreDialogHelperPrivate : public QObject
    {
        Q_OBJECT
    public:
        // Installs an event filter for the window. When it's closed, its size and possibly
        // maximized state should be saved. The state is only saved if maximized is true.
        void installFor(QString sizename, QMainWindow *window, bool maximized = false);
    protected:
        virtual bool eventFilter(QObject *o, QEvent *e) override;
    protected slots:
        void windowDestroyed(QObject *o);
    private:
        // [window, [identifier string for window, whether saving maximized state]]
        std::map<QMainWindow*, std::pair<QString, bool>> filtered;

        typedef QObject base;
    };
}


#endif // FORMSTATE_H

