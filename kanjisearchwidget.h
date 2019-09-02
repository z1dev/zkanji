/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJIFORM_H
#define KANJIFORM_H

#include <QMainWindow>
#include <QMenu>
#include <QSignalMapper>
#include <QComboBox>
#include <memory>
#include "zradicalgrid.h"



class ZComboButton : public QComboBox
{
    Q_OBJECT
signals:
    void clicked();
public:
    ZComboButton(QWidget *parent = nullptr);
    virtual ~ZComboButton();
protected:
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
private:
    bool mouseover;
    typedef QComboBox base;
};


enum class KanjiIndexT {
    Unicode, EUCJP, ShiftJIS, JISX0208, Kuten,
    Oneil, Gakken, Halpern, Heisig, HeisigN, HeisigF, Henshall,
    Nelson, NewNelson, SnH, KnK, Busy, Crowley, FlashC, KGuide,
    HalpernN, Deroo, Sakade, HenshallG, Context, HalpernK, HalpernL,
    Tuttle
};

enum class KanjiFromT {
    All, Clipbrd, Common, Jouyou, JLPT,
    Oneil, Gakken, Halpern, Heisig, HeisigN, HeisigF, Henshall,
    Nelson, NewNelson, SnH, KnK, KnKOld, Busy, Crowley, FlashC, KGuide,
    HalpernN, Deroo, Sakade, HenshallG, Context, HalpernK, HalpernL,
    Tuttle
};

struct KanjiFilterData;

enum class KanjiGridSortOrder;

// Filters in the order they appear in the "filters" member of kanjiFilterData.
enum class KanjiFilters : uchar {
    Strokes, JLPT, Meaning, Reading,
    Jouyou, Radicals, Index, SKIP };

struct KanjiFilterData
{
    uchar filters = 0xff;

    KanjiGridSortOrder ordertype = (KanjiGridSortOrder)0;
    KanjiFromT fromtype = KanjiFromT::All;

    int strokemin = 0;
    int strokemax = 0;
    QString meaning;
    bool meaningafter = true;
    QString reading;
    bool readingafter = true;
    bool readingstrict = false;
    bool readingoku = true;
    int jlptmin = -1;
    int jlptmax = -1;
    int jouyou = 0;
    int skip1 = 0;
    int skip2 = -1;
    int skip3 = -1;
    KanjiIndexT indextype = KanjiIndexT::Unicode;
    QString index;

    // Radical filters.
    RadicalFilter rads;
};

// Returns whether the dat.filters member contains the (1 << (int)f) bit set.
bool filterActive(KanjiFilterData dat, KanjiFilters f);
// Returns whether the dat byte contains the (1 << (int)f) bit set.
bool filterActive(uchar dat, KanjiFilters f);

// Checks or unchecks the bit for a given filter in dat.filters.
void setFilter(KanjiFilterData &dat, KanjiFilters f, bool set = true);
// Checks or unchecks the bit for a given filter in dat.
void setFilter(uchar &dat, KanjiFilters f, bool set = true);

struct RuntimeKanjiFilters
{
    KanjiFilterData data;

    // Temporarily selected radicals in the radicals form, that might or might not be added to
    // the final radical filters later.
    std::vector<ushort> tmprads;
};

namespace Ui {
    class KanjiSearchWidget;
}

class ZRadicalGrid;
class RadicalForm;
class ZFlowLayout;
class QXmlStreamWriter;
class QXmlStreamReader;
class ZStatusBar;
class Dictionary;

enum class CommandCategories;
class KanjiSearchWidget : public QWidget
{
    Q_OBJECT
signals:
    void activated(QObject*);
public:
    KanjiSearchWidget(QWidget *parent = nullptr);
    virtual ~KanjiSearchWidget();

    void saveXMLSettings(QXmlStreamWriter &writer) const;
    void loadXMLSettings(QXmlStreamReader &reader);

    void saveState(KanjiFilterData &data) const;
    void restoreState(const KanjiFilterData &data);

    void assignStatusBar(ZStatusBar *bar);

    // Clears the kanji search settings.
    void setDictionary(Dictionary *d);

    // Returns a command category when the widget is visible, it's on a ZKanjiWidget, and that
    // widget it either active, or is the sole widget on the form in the given category. The
    // returned value depends on the current view mode of the ZKanjiWidget. A 0 return value
    // means the parent form's menu shouldn't be changed.
    CommandCategories activeCategory() const;

    // See UICommands::Commands in globalui.h
    void executeCommand(int command);

    // Returns whether a given command is valid and active in the widget in its current state.
    void commandState(int command, bool &enabled, bool &checked, bool &visible) const;

public slots:
    void reset();
protected:
    virtual bool event(QEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;
    virtual void contextMenuEvent(QContextMenuEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
private:
    Ui::KanjiSearchWidget *ui;
private:
    // Creates a KanjiFilters structure filled with the current data.
    void fillFilters(RuntimeKanjiFilters &f) const;
    // Fills the interface to use the filters in f.
    //void useFilters(RuntimeKanjiFilters f);

    // Returns whether the passed two filters would give the same result, even if some
    // settings are different. (For example because one setting is not used in either.)
    bool filtersMatch(const RuntimeKanjiFilters &a, const RuntimeKanjiFilters &b);
    // Nothing is set in the filters, so the kanji grid should show the whole list.
    bool filtersEmpty(const RuntimeKanjiFilters &f);

    // Creates a list of kanji indexes given the passed filter.
    void listFilteredKanji(const RuntimeKanjiFilters &f, std::vector<ushort> &list);

    // Filters that were used the last time the kanji list was filtered.
    RuntimeKanjiFilters filters;

    // When true, even if different input is sent to the controls, the kanji won't be
    // filtered.
    bool ignorefilter;

    // Previously saved radical filters.
    //smartvector<RadicalFilter> radfilters;

    // The window allowing selection of radicals to filter kanji. Null when not shown.
    RadicalForm *radform;

    // Popup menu to select which filter is active.
    QMenu filterMenu;
    // Mapping for filterMenu actions.
    QSignalMapper filterMap;

    typedef QWidget base;
private slots:
    void on_resetButton_clicked();
    void on_f1Button_clicked();
    void on_f2Button_clicked();
    void on_f3Button_clicked();
    void on_f4Button_clicked();
    void on_f5Button_clicked();
    void on_f6Button_clicked();
    void on_f7Button_clicked();
    void on_f8Button_clicked();
    void on_allButton_clicked(bool checked);

    void radicalsCButtonClicked();

    void showHideAction(int index);

    void skipButtonsToggled(bool checked);

    // Updates the list shown in the kanji grid.
    void filterKanji(bool forced = false);
    // Same as filterKanji(false).
    void filter();
    // Sorts the kanji in the kanji grid.
    void sortKanji();

    // Called when a new item is selected in the combobox of the radicals.
    void radicalSelected(int ix);
    // Called when the new radicals have been selected in either in the radicals window or in
    // the radicals combo box.
    void radicalsChanged();
    // Called when the grouping of radicals changes in the radicals window.
    void radicalGroupingChanged(bool grouping);
    // The radical form's Ok button has been pressed.
    void radicalsOk();
    // Called when the radicals selection form is about to be closed.
    void radicalsClosed();

    // Called when a radical has been added to the global radicals filter model. Updates the
    // listed radicals in the radicals combo box.
    void radsAdded();
    // Called when a radical has been removed from the global radicals filter model. Updates
    // the listed radicals in the radicals combo box.
    void radsRemoved(int index);
    // Called when a radical has been moved in the global radicals filter model. Updates the
    // listed radicals in the radicals combo box.
    void radsMoved(int index, bool up);
    // Called when the global radicals filter model has been cleared. Updates the listed
    // radicals in the radicals combo box.
    void radsCleared();
};


#endif // KANJIFORM_H
