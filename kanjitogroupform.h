/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJITOGROUPFORM_H
#define KANJITOGROUPFORM_H

#include <QComboBox>
#include <QPushButton>
#include "dialogwindow.h"

namespace Ui {
    class KanjiToGroupForm;
}

class GroupBase;
class Dictionary;
class KanjiListModel;
class ZDictionaryComboBox;
class KanjiToGroupForm : public DialogWindow
{
    Q_OBJECT
public:
    KanjiToGroupForm(QWidget *parent = nullptr);
    virtual ~KanjiToGroupForm();

    void exec(Dictionary *d, ushort kindex, GroupBase *initialSelection = nullptr);
    void exec(Dictionary *d, const std::vector<ushort> kindexes, GroupBase *initialSelection = nullptr);
protected:
    virtual bool event(QEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;
private slots:
    // Click slot for the button for adding kanji to a group.
    void accept();
    // Items have been selected or deselected in the group widget.
    void selChanged();
    // Changes the dictionary displayed in the group widget.
    void on_dictCBox_currentIndexChanged(int index);

    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);
private:
    void translateTexts();

    Ui::KanjiToGroupForm *ui;

    Dictionary *dict;
    KanjiListModel *model;

    ZDictionaryComboBox *dictCBox;
    QPushButton *acceptButton;

    typedef DialogWindow    base;
};

#endif // KANJITOGROUPFORM_H
