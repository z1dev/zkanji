/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DEFINITIONWIDGET_H
#define DEFINITIONWIDGET_H

#include <QWidget>

namespace Ui {
    class DefinitionWidget;
}

class Dictionary;
class DefinitionWidget : public QWidget
{
    Q_OBJECT
public:
    DefinitionWidget(QWidget *parent = nullptr);
    virtual ~DefinitionWidget();

    void setWords(Dictionary *d, const std::vector<int> &words);
protected:
    virtual bool event(QEvent *e) override;
protected slots:
    void dictEntryRemoved(int windex);
    void dictEntryChanged(int windex, bool studydef);
    void on_defEdit_textEdited(const QString &str);
    void on_defEdit_focusChanged(bool activated);
    void on_defSaveButton_clicked(bool checked);
    void on_defResetButton_clicked(bool);
private:
    // The position of a word index in list. Returns the position of the next word index if
    // windex is not found.
    int indexpos(int windex);
    // Changes the enabled state of the edit control and save/reset buttons.
    void update();

    Ui::DefinitionWidget *ui;

    Dictionary *dict;

    // Sorted word indexes of dict handled in the widget.
    std::vector<int> list;
    // Boolean value (0 or 1) to each list item whether they have a custom meaning.
    std::vector<uchar> data;
    // Definition of the currently edited single word in the dictionary.
    QString original;

    // When set to true, changes to word entries are ignored and update() is not called.
    bool ignorechange;

    typedef QWidget base;
};


#endif // DEFINITIONWIDGET_H
