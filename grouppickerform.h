/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GROUPPICKER_H
#define GROUPPICKER_H

#include <QMainWindow>
#include <functional>
#include "groupwidget.h"
#include "dialogwindow.h"

namespace Ui {
    class GroupPickerForm;
}

struct WordEntry;
class Dictionary;
class TreeItem;
class GroupBase;
class QPushButton;
// Window shown when a category or group should be selected. Use the static select() or create
// the window and set everything with the setters. The groupSelected() signal is triggered
// when the user clicks the select group button.
class GroupPickerForm : public DialogWindow
{
    Q_OBJECT
signals:
    void groupSelected(Dictionary *dict, GroupBase *selection);
public:
    // Creates a modal group picker form and shows it on screen. Returns the selected group.
    // Pass true in showdicts to show the dictionary combo box. Otherwise the dictionary for
    // the listed groups can't be changed from dict. To create a non-modal form construct the
    // window manually and connect to the groupSelected event.
    static GroupBase* select(GroupWidget::Modes mode, const QString &instructions, Dictionary *dict, bool onlycategories = false, bool showdicts = false, QWidget *parent = nullptr);

    /* Constructor is private */

    ~GroupPickerForm();

    void hideDictionarySelect();
    void setDictionary(Dictionary *dict);
    void setMode(GroupWidget::Modes mode);
    void setInstructions(const QString &instructions);
protected:
    virtual bool event(QEvent *e) override;
private:
    GroupPickerForm() = delete;
    GroupPickerForm(const GroupPickerForm&) = delete;
    GroupPickerForm(GroupPickerForm&&) = delete;
    GroupPickerForm(bool categories, QWidget *parent = nullptr);
    GroupPickerForm(GroupWidget::Modes mode, bool categories, QWidget *parent = nullptr);

    void translateTexts();

    Ui::GroupPickerForm *ui;

    QPushButton *selectButton;

    typedef DialogWindow base;
private slots:
    void selectClicked();
    void on_groupWidget_currentItemChanged(GroupBase *current);
    void on_dictCBox_currentIndexChanged(int index);
};



#endif // GROUPPICKER_H
