/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANAREADINGPRACTICEFORM_H
#define KANAREADINGPRACTICEFORM_H

#include <QBasicTimer>
#include <vector>
#include "dialogwindow.h"


namespace Ui {
    class KanaReadingPracticeForm;
}

class QLabel;
class KanaReadingPracticeForm : public DialogWindow
{
    Q_OBJECT
public:
    KanaReadingPracticeForm(QWidget *parent = nullptr);
    virtual ~KanaReadingPracticeForm();

    void exec();
protected slots:
    void on_restartButton_clicked();
protected:
    virtual bool event(QEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;
private:
    // Generates a list of syllables to be shown and restarts the test.
    void reset();
    // Updates the labels to show the next syllable.
    void next();

    // Updates the text of lb to show the kana character at p position in list. Set rlb to a
    // label to show the romaji equivalent of the label text. If p is invalid, the label text
    // is set to an empty string.
    void setLabelText(int p, QLabel *lb, QLabel *rlb = nullptr);

    void answered(bool correct);

    // Stops the test timer and if hide is true, hides the labels showing the passed time.
    void stopTimer(bool hide);
    // Resets the timer and shows the labels showing the passed time.
    void startTimer();

    // Updates text1-text4 labels text with the entered string.
    void setTextLabels();

    void hideLabelAnimation();

    Ui::KanaReadingPracticeForm *ui;

    // Time the test started.
    QDateTime starttime;

    // Indexes of syllables in kanaStrings[]  (from kanapracticesettingsform.h) to be shown to
    // the student.
    std::vector<int> list;

    // Current syllable position in list.
    int pos;
    // Number of times the current syllable has been retried. After 2 the syllable is marked
    // as a mistake.
    int retries;
    // Number of mistakes made in the whole test. Not counted below 2 retries.
    int mistakes;

    // String entered so far.
    QString entered;

    // Label showing number of items due to be tested.
    //QLabel *dueLabel;
    // Label showing number of items that were retried twice and failed.
    //QLabel *wrongLabel;
    // Label showing passed time.
    //QLabel *timeLabel;

    QBasicTimer timer;

    typedef DialogWindow    base;
};

#endif // KANAREADINGPRACTICEFORM_H

