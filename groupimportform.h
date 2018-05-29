/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GROUPIMPORTFORM_H
#define GROUPIMPORTFORM_H

#include <QPushButton>
#include "dialogwindow.h"

namespace Ui {
    class GroupImportForm;
}

class Dictionary;
class GroupTreeModel;

template<typename T>
class GroupCategory;
class WordGroup;
class KanjiGroup;
typedef GroupCategory<WordGroup>    WordGroupCategory;
typedef GroupCategory<KanjiGroup>   KanjiGroupCategory;
class GroupImportForm : public DialogWindow
{
    Q_OBJECT
public:
    GroupImportForm(QWidget *parent);
    ~GroupImportForm();

    bool exec(Dictionary *dict);

    Dictionary* dictionary() const;
    KanjiGroupCategory* kanjiCategory() const;
    WordGroupCategory* wordsCategory() const;
    bool importKanjiExamples() const;
    bool importWordMeanings() const;
private slots:
    void selectionChanged();
    void on_dictCBox_currentIndexChanged(int ix);
    void on_kanjiGroupsBox_toggled(bool checked);
    void on_wordGroupsBox_toggled(bool checked);
private:
    void translateTexts();

    Ui::GroupImportForm *ui;

    typedef DialogWindow    base;
};


#endif // GROUPIMPORTFORM_H
