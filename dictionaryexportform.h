/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DICTIONARYEXPORTFORM_H
#define DICTIONARYEXPORTFORM_H

#include "dialogwindow.h"

namespace Ui {
    class DictionaryExportForm;
}

class Dictionary;
class CheckedGroupTreeModel;
class QPushButton;
class DictionaryExportForm : public DialogWindow
{
    Q_OBJECT
public:
    DictionaryExportForm(QWidget *parent = nullptr);
    virtual ~DictionaryExportForm();

    bool exec(Dictionary *dict);
public slots:
    void exportClicked();
    void modelDataChanged();
    void on_dictCBox_currentIndexChanged(int ix);
    void on_groupBox_toggled(bool on);
private:
    void translateTexts();

    Ui::DictionaryExportForm *ui;

    QPushButton *exportbutton;
    CheckedGroupTreeModel *kanjimodel;
    CheckedGroupTreeModel *wordsmodel;

    typedef DialogWindow    base;
};


#endif // DICTIONARYEXPORTFORM_H
