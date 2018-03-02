/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef JLPTREPLACEFORM_H
#define JLPTREPLACEFORM_H

#include "dialogwindow.h"

namespace Ui {
    class JLPTReplaceForm;
}

class Dictionary;
class JLPTReplaceForm : public DialogWindow
{
    Q_OBJECT
public:
    JLPTReplaceForm(QWidget *parent = nullptr);
    virtual ~JLPTReplaceForm();

    void exec(QString filepath, Dictionary *dict, const std::vector<std::tuple<QString, QString, int>> &words);
public slots:
    void skip();
    void replace();
    void undo();
protected:
    virtual void closeEvent(QCloseEvent *e) override;
private:
    void updateButtons();

    // Re-saves the JLPTNData.txt after creating a backup of the old file.
    void saveJLPTData();

    Ui::JLPTReplaceForm *ui;

    // List of [kanji, kana, jlptn level]
    std::vector<std::tuple<QString, QString, int>> list;

    // Indexes of words in the commons list for each replacement added in the add order. When
    // a word was skipped, -1 is inserted in this list.
    std::vector<int> commonslist;

    int pos;
    int skipped;

    QString path;


    typedef DialogWindow    base;
};

#endif // JLPTREPLACEFORM_H
