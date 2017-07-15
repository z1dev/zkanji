/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDTOGROUPFORM_H
#define WORDTOGROUPFORM_H

#include <QPushButton>
#include "dialogwindow.h"

namespace Ui {
    class WordToGroupForm;
}

class Dictionary;
class GroupBase;
class WordToGroupForm : public DialogWindow
{
    Q_OBJECT
public:
    WordToGroupForm(QWidget *parent = nullptr);
    virtual ~WordToGroupForm();

    void exec(Dictionary *d, int windex, GroupBase *initialSelection = nullptr);
    void exec(Dictionary *d, const std::vector<int> &indexes, GroupBase *initialSelection = nullptr);
protected:
    virtual void closeEvent(QCloseEvent *e) override;
private slots:
    // Click slot for the button for adding word to a group.
    void accept();
    // Clicked the button at the top right, to show the add to dictionary form.
    void on_switchButton_clicked();
    // An item has been selected in the group widget.
    void selChanged();

    void dictionaryAdded();
    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
private:
    Ui::WordToGroupForm *ui;

    Dictionary *dict;
    std::vector<int> list;

    QPushButton *acceptButton;

    typedef DialogWindow base;
};

#endif // WORDTOGROUPFORM_H