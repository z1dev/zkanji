/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDDECKSTATSFORM_H
#define WORDDECKSTATSFORM_H

#include <QtCharts/QChart>
QT_CHARTS_USE_NAMESPACE
#include "dialogwindow.h"

namespace Ui {
    class WordDeckStatsForm;
}

enum class DeckStatPage { Items, Tests, Decks };

class WordDeck;
class WordDeckStatsForm : public DialogWindow
{
    Q_OBJECT
public:
    WordDeckStatsForm(QWidget *parent = nullptr);
    virtual ~WordDeckStatsForm();

    void exec(WordDeck *d);
protected:
    virtual bool eventFilter(QObject *o, QEvent *e) override;
private:
    void showStat(DeckStatPage page);

    Ui::WordDeckStatsForm *ui;


    DeckStatPage viewed;

    WordDeck *deck;
    //QAbstractSeries *data;
    
    typedef DialogWindow    base;
};
#endif // WORDDECKSTATSFORM_H

