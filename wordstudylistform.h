/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDSTUDYLISTFORM_H
#define WORDSTUDYLISTFORM_H

#include <QMainWindow>
#include <map>
#include "dialogwindow.h"

namespace Ui {
    class WordStudyListForm;
}

enum class DeckViewModes : int;
class WordDeck;
class StudyListModel;
class DictionaryItemModel;
class Dictionary;
class QMenu;
struct WordStudyListFormData;

struct WordStudySorting {
    int column;
    Qt::SortOrder order;
};


class QMenu;
class QAction;
enum class DictColumnTypes;
class WordStudyListForm : public DialogWindow
{
    Q_OBJECT
public:
    // Shows and returns a WordStudyListForm displaying deck if one exists. Pass true in
    // createshow to make sure the form gets created if it doesn't exist. If a form is already
    // shown, it's only activated when createshow is true.
    static WordStudyListForm* Instance(WordDeck *deck, bool createshow);

    /* constructor is private */

    virtual ~WordStudyListForm();

    void saveState(WordStudyListFormData &data) const;
    void restoreState(const WordStudyListFormData &data);

    // Switches to the queue tab.
    void showQueue();
    // Starts the long term study.
    void startTest();
    // Opens the add words to deck window with the word indexes in list.
    void addQuestions(const std::vector<int> &wordlist);
    // Shows a message box for comfirmation and removes the passed deck items from either the
    // queued items or studied items.
    void removeItems(const std::vector<int> &items, bool queued);
    // Shows a message box for comfirmation and moves the passed studied deck items back to
    // the queue.
    void requeueItems(const std::vector<int> &items);
protected:
    virtual void closeEvent(QCloseEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
protected slots:
    void headerSortChanged(int index, Qt::SortOrder order);
    void rowSelectionChanged();
    void modeButtonClicked(bool checked);
    void partButtonClicked();
    void on_addButton_clicked();
    void on_delButton_clicked();
    void on_backButton_clicked();
    void showColumnContextMenu(const QPoint &p);
    void showContextMenu(QMenu *menu, QAction *insertpos, Dictionary *dict, DictColumnTypes coltype, QString selstr, const std::vector<int> &windexes, const std::vector<ushort> &kindexes);
    void dictContextMenu(const QPoint &pos, const QPoint &globalpos, int selindex);

    void dictReset();
    void dictRemoved(int index, int orderindex, void *oldaddress);
private:
    // Saves the column sizes and visibility for the current mode from the dictionary view.
    void saveColumns();
    // Restores the column sizes and visibility for the current mode to the dictionary view.
    void restoreColumns();

    WordStudyListForm(WordDeck *deck, QWidget *parent = nullptr);
    WordStudyListForm() = delete;

    // Instances of this form for each word deck, to avoid multiple forms for the same deck.
    static std::map<WordDeck*, WordStudyListForm*> insts;

    Ui::WordStudyListForm *ui;
    Dictionary *dict;
    WordDeck *deck;

    StudyListModel *model;

    // Saved sort column and order for the different tabs.
    WordStudySorting queuesort;
    WordStudySorting studiedsort;
    WordStudySorting testedsort;

    // Set during some operations when the sorting is changed programmatically, to ignore the
    // signal sent by the header.
    bool ignoresort;

    std::vector<int> queuesizes;
    std::vector<int> studiedsizes;
    std::vector<int> testedsizes;

    std::vector<char> queuecols;
    std::vector<char> studiedcols;
    std::vector<char> testedcols;

    typedef DialogWindow base;
};

#endif // WORDSTUDYLISTFORM_H
