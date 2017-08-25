/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANAPRACTICESETTINGSFORM_H
#define KANAPRACTICESETTINGSFORM_H

#include "dialogwindow.h"

namespace Ui {
    class KanaPracticeSettingsForm;
}


class QCheckBox;
class QSignalMapper;
class QGridLayout;
class KanaPracticeSettingsForm : public DialogWindow
{
    Q_OBJECT
public:
    KanaPracticeSettingsForm(QWidget *parent = nullptr);
    ~KanaPracticeSettingsForm();

    void exec();
public slots:
    void boxToggled(int index);
    void on_checkButton_clicked();
private:
    void setupBoxes(QSignalMapper *map, QGridLayout *g, std::vector<QCheckBox*> &boxes);
    Ui::KanaPracticeSettingsForm *ui;

    bool updating;

    QSignalMapper *hiramap;
    QSignalMapper *katamap;
    std::vector<QCheckBox*> hiraboxes;
    std::vector<QCheckBox*> kataboxes;
    std::vector<uchar> hirause;
    std::vector<uchar> katause;


    typedef DialogWindow    base;
};


#endif // KANAPRACTICESETTINGSFORM_H
