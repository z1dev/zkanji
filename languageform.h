/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef LANGUAGEFORM_H
#define LANGUAGEFORM_H

#include "dialogwindow.h"

namespace Ui {
    class LanguageForm;
}

class LanguageForm : public DialogWindow
{
    Q_OBJECT
public:
    LanguageForm(QWidget *parent = nullptr);
    virtual ~LanguageForm();
protected:
    virtual void showEvent(QShowEvent *e) override;
private:
    void translateTexts();

    void okClicked();

    Ui::LanguageForm *ui;

    typedef DialogWindow    base;
};


#endif // LANGUAGEFORM_H

