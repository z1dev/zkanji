/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJIGROUPFORM_H
#define KANJIGROUPFORM_H

#include <QMainWindow>

namespace Ui {
    class KanjiGroupWidget;
}

class GroupBase;
class QXmlStreamWriter;
class QXmlStreamReader;
class ZStatusBar;
class Dictionary;
class KanjiGroupWidget : public QWidget
{
    Q_OBJECT

public:
    KanjiGroupWidget(QWidget *parent = 0);
    ~KanjiGroupWidget();

    void saveXMLSettings(QXmlStreamWriter &writer) const;
    void loadXMLSettings(QXmlStreamReader &reader);

    void assignStatusBar(ZStatusBar *bar);

    //void makeModeSpace(const QSize &size);

    // Called when the user data has been loaded and the form is ready to display it.
    void setDictionary(Dictionary *d);
protected:
    virtual bool event(QEvent *e) override;
private:
    Ui::KanjiGroupWidget *ui;

    typedef QWidget base;
private slots:
    void kanjiSelectionChanged();
    //void on_groupWidget_currentItemChanged(GroupBase *current);
    void on_groupWidget_selectionChanged();
    void on_delButton_clicked(bool checked);
    void on_upButton_clicked(bool checked);
    void on_downButton_clicked(bool checked);
};


#endif // KANJIGROUPFORM_H

