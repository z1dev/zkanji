/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef IMPORTREPLACEFORM_H
#define IMPORTREPLACEFORM_H

#include <QMainWindow>
#include <qdialog.h>
#include <qeventloop.h>
#include "words.h"
#include "dialogwindow.h"

namespace Ui {
    class ImportReplaceForm;
}

class QLabel;
class DictionaryWordListItemModel;
class LabelDialog : public QDialog
{
    Q_OBJECT
public:
    LabelDialog(QWidget *parent = nullptr);
    ~LabelDialog();
private:
    QLabel *lb;

    typedef QDialog base;
};

class Dictionary;
class OriginalWordsList;
class ImportReplaceForm : public DialogWindow
{
    Q_OBJECT
public:
    ImportReplaceForm(Dictionary *olddir, Dictionary *newdir = nullptr, QWidget *parent = nullptr);
    ~ImportReplaceForm();

    // Shows the form as a modal window. Returns true if the replacements were
    // accepted by the user, and false on cancel.
    bool exec();

    // Indexes of words in olddir from groups and study lists, and the index
    // they should be changed to in the new dictionary.
    std::map<int, int>& changes();
    Dictionary* dictionary();
    OriginalWordsList& originals();
protected:
    virtual void closeEvent(QCloseEvent *e) override;
    virtual bool event(QEvent *e) override;
private slots:
    //  Current row of the dictionary search results table changed.
    void dictRowChanged(int row, int oldrow);
    // Use button clicked.
    void useClicked();
    // Skip button clicked.
    void skipClicked();
    // Current row of the missingTable changed.
    void missingRowChanged(int row, int prev);
private:
    // Searches for differences between the old and new dictionaries to list
    // them in the missing words table. While the init is running, the widgets
    // on the window will be disabled. Closing the window during this time
    // shows a dialog box asking a user to abort.
    bool init();

    Ui::ImportReplaceForm *ui;
    DictionaryWordListItemModel *model;

    //QEventLoop loop;
    LabelDialog dlg;

    Dictionary *olddir;
    // True when newdir was created by the object and not passed in the constructor.
    bool owndir;
    Dictionary *newdir;

    OriginalWordsList orig;

    // List of word indexes in olddir and their corresponding indexes in
    // newdir. If a word is only found in olddir, the other index is
    // set to -1.
    std::map<int, int> list;

    typedef DialogWindow    base;
};

#endif // IMPORTREPLACEFORM_H
