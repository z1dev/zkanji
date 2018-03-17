/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANAPRACTICESETTINGSFORM_H
#define KANAPRACTICESETTINGSFORM_H

#include "dialogwindow.h"

namespace Ui {
    class KanaPracticeSettingsForm;
}

enum class KanaSounds
{
    a, i, u, e, o,
    n,
    ka, ki, ku, ke, ko, sa, si, su, se, so, ta, ti, tu, te, to, na, ni, nu, ne, no,
    ha, hi, hu, he, ho, ma, mi, mu, me, mo, ya, yu, yo, ra, ri, ru, re, ro, wa, wo,

    ga, gi, gu, ge, go, za, zi, zu, ze, zo, da, di, du, de, ddo, ba, bi, bu, be, bo, pa, pi, pu, pe, po,

    kya, kyu, kyo, sha, shu, sho, cha, chu, cho, nya, nyu, nyo, hya, hyu, hyo,
    rya, ryu, ryo,
    gya, gyu, gyo, ja, ju, jo, dya, dyu, dyo, bya, byu, byo, pya, pyu, pyo,

    Count
};


class QCheckBox;
class QSignalMapper;
class QGridLayout;
struct KanarPracticeData;
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
    void on_test1Button_clicked();
    void on_test2Button_clicked();
private:
    void saveState(KanarPracticeData &data);
    void restoreState(const KanarPracticeData &data);

    // Create checkboxes and place them on the form.
    void setupBoxes(QSignalMapper *map, QGridLayout *g, std::vector<QCheckBox*> &boxes);

    // Change status bar text to reflect current state.
    void updateStatus();

    Ui::KanaPracticeSettingsForm *ui;

    // True when programmatically changing kana use so the checkbox signals should be ignored.
    bool updating;
    // Number of hiragana boxes checked.
    int hiracheckcnt;
    // Number of katakana boxes checked.
    int katacheckcnt;

    QSignalMapper *hiramap;
    QSignalMapper *katamap;
    std::vector<QCheckBox*> hiraboxes;
    std::vector<QCheckBox*> kataboxes;
    std::vector<uchar> hirause;
    std::vector<uchar> katause;

    typedef DialogWindow    base;
};

extern const QString kanaStrings[];


#endif // KANAPRACTICESETTINGSFORM_H
