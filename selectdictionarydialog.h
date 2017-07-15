/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef SELECTDICTIONARYDIALOG_H
#define SELECTDICTIONARYDIALOG_H

#include "dialogwindow.h"

namespace Ui {
    class SelectDictionaryDialog;
}

class SelectDictionaryDialog : public DialogWindow
{
    Q_OBJECT
public:
    SelectDictionaryDialog(QWidget *parent = nullptr);
    virtual ~SelectDictionaryDialog();

    int exec(QString labelstring, int firstselected = 0, QString okstr = QString());
public slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
private:
    Ui::SelectDictionaryDialog *ui;

    typedef DialogWindow    base;
};


#endif // SELECTDICTIONARYDIALOG_H
