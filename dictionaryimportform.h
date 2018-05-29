/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DICTIONARYIMPORTFORM_H
#define DICTIONARYIMPORTFORM_H

#include <QPushButton>
#include "dialogwindow.h"
#include "zdictionariesmodel.h"

namespace Ui {
    class DictionaryImportForm;
}

class Dictionary;
class WordGroup;
class KanjiGroup;
class DictionaryImportForm : public DialogWindow
{
    Q_OBJECT
public:
    DictionaryImportForm(QWidget *parent = nullptr);
    virtual ~DictionaryImportForm();

    bool exec(Dictionary *dict);

    // The name of the dictionary that was selected by the user.
    QString dictionaryName() const;
    // Whether the user requested a full dictionary import.
    bool fullImport() const;

    // Returns the word group selected in the word group tree when it's appropriate. Returns
    // null when full import is selected, or the check box for word groups is unchecked.
    WordGroup* selectedWordGroup() const;
    // Returns the kanji group selected in the kanji group tree when it's appropriate. Returns
    // null when full import is selected, or the check box for kanji groups is unchecked.
    KanjiGroup* selectedKanjiGroup() const;
public slots:
    void optionChanged();
    void on_dictCBox_currentTextChanged(const QString &text);
private:
    void translateTexts();

    Ui::DictionaryImportForm *ui;

    DictionariesProxyModel *dictmodel;

    typedef DialogWindow    base;
};

#endif // DICTIONARYIMPORTFORM_H
