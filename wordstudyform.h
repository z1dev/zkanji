/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDSTUDYFORM_H
#define WORDSTUDYFORM_H

#include <QMainWindow>
#include <QLabel>
#include <QDateTime>
#include <QBasicTimer>

namespace Ui {
    class WordStudyForm;
}

class WordDeck;
class WordStudy;
class Dictionary;
enum class WordParts : uchar;
enum class ModalResult;

struct ChoiceLabel : public QLabel
{
    Q_OBJECT
public:
    ChoiceLabel(int index, QWidget *parent = nullptr);

    void setHeight(int height);

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int h) const override;

    void setText(QString str);

    bool isActive();
    void setActive(bool a);
signals:
    void clicked(int);
protected:
    virtual void enterEvent(QEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
private:
    int h;
    int index;

    // True when enter and leave events should update the label and clicks should fire the
    // clicked event.
    bool active;

    typedef QLabel  base;
};

struct DefinitionLabel : public QLabel
{
    Q_OBJECT
public:
    DefinitionLabel(QWidget *parent = nullptr);

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int h) const override;

    // Changes label font size to fit text.
    void updated();
protected:
    virtual void resizeEvent(QResizeEvent *e) override;
private:
    int ptsize;
    int lineh;

    typedef QLabel  base;
};

class WordStudyForm : public QMainWindow
{
    Q_OBJECT
public:
    // Starts the long term study of the day if there are words left to study and the user
    // accepts. Pass the deck to be studied.
    static void studyDeck(WordDeck *deck);

    WordStudyForm(QWidget *parent = nullptr);
    virtual ~WordStudyForm();

    Dictionary* dictionary();
    void exec(WordDeck *deck);
    void exec(WordStudy *study);
protected:
    virtual void closeEvent(QCloseEvent *e) override;
    virtual bool event(QEvent *e) override;
    virtual bool eventFilter(QObject *obj, QEvent *e) override;
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
private slots:
    // Pressed the show answer or accept buttons. Reveals the correct answer, colors the
    // labels accordingly and shows the page to skip to the next word.
    void answerEntered();
    // Pressed the show hint button.
    void showWordHint();
    // One of the result buttons (good, bad, easy, retry) was pressed.
    void resultSelected();
    // One of the Correct/Wrong decision buttons was pressed.
    void answerDecided();
    // Shows a popup menu to allow changing the previously given answer.
    void on_undoCBox_currentIndexChanged(int index);

    void choiceClicked(int index);

    void suspend();
    void resume();

    void currentToGroup();
    void currentToStudy();
    void currentLookup(int ix);
private:
    // Updates the window to show the next tested word and the correct pages.
    bool showNext();
    // Calls showNext() to show the next word. If that returns false, closes the window
    // setting the modal result, which closeEvent() should handle. Only used when studying a
    // general word group, not for long-term decks.
    void next();

    // Updates the labels that are selectable when answering. Set activate to false to disable
    // user interaction with them (i.e. after an answer is picked.)
    void activateChoices(bool activate);

    // Changes the labels when testing with the long term study. Updates due and passed item
    // counts as well as the ETA display.
    void updateDeckLabels();

    // Changes the labels when studying. Updates due and new item counts and the correct/wrong
    // labels.
    void updateStudyLabels();

    Ui::WordStudyForm *ui;

    //QLabel *dueLabel1;
    //QLabel *dueLabel2;
    //QLabel *testedLabel1;
    //QLabel *testedLabel2;
    //QLabel *etaLabel;
    //QLabel *timeLabel;

    // Index on the status bar to display time.
    int timeindex;

    QBasicTimer timer;

    ModalResult result;

    WordDeck *deck;
    WordStudy *study;

    // UTC time when the test started or it was last resumed.
    QDateTime starttime;
    // Number of tested items since the test started or was last resumed.
    int testedcount;
    // Number of items that were finished with correct or easy answer.
    int passedcount;

    // UTC time when the current item was first shown, to know the time it took the student to
    // answer an item. To compensate for suspended tests, idletime saves the time when the
    // test is suspended, and when it's resumed, itemtime is updated.
    QDateTime itemtime;

    // Last time the user pressed a key or clicked the mouse on the form.
    QDateTime idletime;

    // Number of milliseconds passed since the test started, excluding the time spent on the
    // suspend page.
    int passedtime;
    // The last time the timer message was caught.
    QDateTime lasttime;

    // True when in study mode and an answer was selected, to mark to stop the word timer.
    bool answered;

    // Index of current word to be tested.
    int windex;
    // Tested part of current word. This value can be a combination of WordPartBits values in
    // case more parts are tested.
    int wquestion;
    // Part of word revealed when the "Show hint" button is pressed. It's only used when a
    // secondary hint can be shown.
    WordParts whint;
    // Index of the correct word choice's label in the choices widget.
    int wchoice;
    // Index of choice picked by the student.
    int picked;
    // True before an answer is given to the first item after the window is displayed.
    // Determines whether the undo button is shown.
    bool first;

    QMenu *optionmenu;
    QAction *partaction;

    typedef QMainWindow base;
};

#endif // WORDSTUDYFORM_H
