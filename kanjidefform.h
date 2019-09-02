/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJIDEFFORM_H
#define KANJIDEFFORM_H

#include "dialogwindow.h"

namespace Ui {
    class KanjiDefinitionForm;
}

class Dictionary;
class DictionaryWordListItemModel;
class QPushButton;
class KanjiDefinitionForm : public DialogWindow
{
    Q_OBJECT
public:
    KanjiDefinitionForm(QWidget *parent = nullptr);
    virtual ~KanjiDefinitionForm();

    void exec(Dictionary *d, ushort kindex);
    void exec(Dictionary *d, const std::vector<ushort> &l);
public slots:
    void on_discardButton_clicked(bool checked);
    void on_acceptButton_clicked(bool checked);
    void on_dictCBox_currentIndexChanged(int index);
    void ok();

    void dictionaryRemoved(int index, int orderindex, void *oldaddress);
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);
protected:
    virtual bool event(QEvent *e) override;
private:
    void translateTexts();

    void updateKanji();
    void updateDictionary();

    Ui::KanjiDefinitionForm *ui;

    Dictionary *dict;

    // Index of all kanji that will be listed in the editor.
    std::vector<ushort> list;
    // The index of all previously edited kanji for the back button.
    std::vector<short> prevkanji;

    // Current kanji being edited in list.
    int pos;
    // Next kanji to be edited in list, if the skip defined box is checked. When this is
    // negative, the last undefined kanji is being edited.
    int nextpos;

    DictionaryWordListItemModel *model;

    typedef DialogWindow    base;
};

#endif // KANJIDEFFORM_H
