/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DICTIONARYTEXTFORM_H
#define DICTIONARYTEXTFORM_H

#include "dialogwindow.h"

namespace Ui {
    class DictionaryTextForm;
}

class Dictionary;
class DictionaryTextForm : public DialogWindow
{
    Q_OBJECT
public:
    DictionaryTextForm(QWidget *parent = nullptr);
    virtual ~DictionaryTextForm();

    void exec(Dictionary *d);
public slots:
    void accept();
    void dictionaryRemoved(int index, int order, void *oldaddress);
    void dictionaryRenamed(const QString &oldname, int index, int order);
    void dictionaryReset();
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);

    void on_browseButton_clicked();
    void on_removeButton_clicked();
protected:
    virtual bool event(QEvent *e) override;
private:
    void translateTexts();

    Ui::DictionaryTextForm *ui;

    Dictionary *dict;
    QByteArray flagdata;
    bool flagerased;

    typedef DialogWindow base;
};

#endif // DICTIONARYTEXTFORM_H
