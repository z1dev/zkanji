/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDTODICTIONARYFORM_H
#define WORDTODICTIONARYFORM_H

#include <QtCore>
#include <QMainWindow>
#include <QPushButton>

#include "dialogwindow.h"

namespace Ui {
    class WordToDictionaryForm;
}

class Dictionary;
class DictionariesProxyModel;

class WordToDictionaryForm : public DialogWindow
{
    Q_OBJECT
public:
    WordToDictionaryForm(QWidget *parent = nullptr);
    virtual ~WordToDictionaryForm();

    void exec(Dictionary *d, int windex, Dictionary *initialSel = nullptr, bool updatelastdiag = false);
protected:
    virtual bool event(QEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;
private slots:
    // Click slot for the button for adding word to a group.
    void on_addButton_clicked();
    // Closes the form without adding a word to a group.
    void on_cancelButton_clicked();
    // Clicked the button at the top right, to show the add to dictionary form.
    void on_switchButton_clicked();
    // A new destination dictionary is selected. Update the word displayed at the bottom.
    void on_dictCBox_currentIndexChanged(int index);
    // A definition of an existing destination word has been double clicked. Shows the edit
    // word dialog with the given definition selected after hiding this form.
    void on_meaningsTable_wordDoubleClicked(int windex, int dindex);
    // Notification when the selection in the top table's selection, showing word meanings to
    // copy, changes. Enables/disables the create/copy button.
    //void wordSelChanged(/*const QItemSelection &selected, const QItemSelection &deselected*/);

    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
    void dictionaryReplaced(Dictionary *old, Dictionary *newdict, int index);
private:
    void translateTexts();

    Ui::WordToDictionaryForm *ui;

    DictionariesProxyModel *proxy;

    // Source dictionary.
    Dictionary *dict;
    // Index of word in the source dictionary.
    int index;

    // Destination dictionary.
    Dictionary *dest;
    // Word index in the destination dictionary.
    int dictindex;

    // Height of the bottom words table when shown.
    int expandsize;

    bool updatelast;

    typedef DialogWindow    base;
};

#endif // WORDTODICTIONARYFORM_H

