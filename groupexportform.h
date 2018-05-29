/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GROUPEXPORTFORM_H
#define GROUPEXPORTFORM_H

#include <QPushButton>
#include "dialogwindow.h"

namespace Ui {
    class GroupExportForm;
};

class Dictionary;
class CheckedGroupTreeModel;
class GroupExportForm : public DialogWindow
{
    Q_OBJECT
public:
    GroupExportForm(QWidget *parent = nullptr);
    ~GroupExportForm();

    bool exec(Dictionary *dict);
public slots:
    void exportClicked();
    void modelDataChanged();
    void on_dictCBox_currentIndexChanged(int ix);
private:
    void translateTexts();

    Ui::GroupExportForm *ui;

    CheckedGroupTreeModel *kanjimodel;
    CheckedGroupTreeModel *wordsmodel;

    typedef DialogWindow base;
};


#endif // GROUPEXPORTFORM_H
