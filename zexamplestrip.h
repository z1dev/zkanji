/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZEXAMPLESTRIP_H
#define ZEXAMPLESTRIP_H

#include <QBasicTimer>
#include <memory>
#include "zscrollarea.h"
#include "sentences.h"

class Dictionary;
class ZExampleStrip;
struct WordCommons;
struct WordCommonsExample;

enum class ExampleDisplay : uchar { Japanese, Translated, Both };

// Popup window which is shown when the user hovers over a word in a ZExampleStrip, if the
// word has several forms or when a word's single form does not reflect its use in a sentence.
class ZExamplePopup : public QWidget
{
    Q_OBJECT
public:
    ZExamplePopup(ZExampleStrip *owner = nullptr);
    ~ZExamplePopup();

    // Shows the popup window above or below the wordrect, depending on the available desktop
    // space.
    void popup(Dictionary *d, const fastarray<ExampleWordsData::Form, quint16> &wordforms, const QRect &wordrect);
protected:
    virtual bool event(QEvent *e) override;

    virtual void paintEvent(QPaintEvent *e) override;
    virtual void enterEvent(QEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;

    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
private:
    // Position of the word rectangle relative to this popup window.
    enum RectSide { Top = 0x00, Bottom = 0x01, Left = 0x00, Right = 0x02 };

    ZExampleStrip *owner;

    // Height of a single word line.
    int lineheight;

    int rectwidth;
    int side;

    // Index of the hovered word form.
    int hovered;

    const fastarray<ExampleWordsData::Form, quint16> *forms;

    // Contains 1 for word forms available in the current dictionary. The other forms are not
    // presented.
    std::vector<char> available;

    // Number of available word forms.
    int formsavailable;

    // On some systems the mouseEnter and mouseLeave messages are not reliable, and a timer is
    // needed to wait for them to arrive. 
    QBasicTimer waittimer;

    typedef QWidget base;
};

class ZExampleStrip : public ZScrollArea
{
    Q_OBJECT
signals:
    void sentenceChanged();
    void wordSelected(ushort block, uchar line, int wordpos, int wordform);
public:
    ZExampleStrip(QWidget *parent = nullptr);
    ~ZExampleStrip();

    virtual void reset() override;

    // Updates the shown example by the passed word. If the word is the same as before, the
    // current example is not changed. Only loads and displays the requested example data if
    // the widget is visible or becomes visible. If wordpos and wordform are specified, checks
    // the current word whether it matches the shown sentence's word at wordpos with the form
    // wordform, and if it does, the sentence is not changed. The sentenceChanged() signal is
    // still emitted if the dictionary and word index are different.
    void setItem(Dictionary *d, int windex, int wordpos = -1, int wordform = -1);

    // Which of the two sentences are shown.
    ExampleDisplay displayed() const;

    // Set which sentence is shown.
    void setDisplayed(ExampleDisplay newdisp);

    // Returns the number of sentences available for the word, whose examples are shown.
    int sentenceCount() const;

    // The index of the sentence currently displayed in the strip from at most sentenceCount()
    // sentences.
    int currentSentence() const;

    // Changes the sentence being displayed for the current word. The value should be between
    // 0 and sentenceCount() - 1. 
    void setCurrentSentence(int which);

    void showPreviousLinkedSentence();
    void showNextLinkedSentence();

    const WordCommonsExample* currentExample() const;

    // Whether changing currently shown word by clicking inside the text area is allowed.
    bool hasInteraction() const;
    // Allow or deny changing currently shown word by clicking inside the text area.
    void setInteraction(bool allow);

    // Last dictionary  passed to setItem.
    Dictionary* dictionary() const;
    // Last word index passed to setItem.
    int wordIndex() const;
protected:
    virtual bool event(QEvent *e) override;

    virtual void showEvent(QShowEvent *e) override;
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;

    virtual int scrollMin() const override;
    virtual int scrollMax() const override;
    virtual int scrollPage() const override;
    virtual int scrollStep() const override;
    virtual void scrolled(int oldpos, int &newpos) override;
protected slots:
    void dictionaryRemoved(int index, int orderindex, Dictionary *dict);
    void dictionaryChanged();
private:
    // Refreshes the strip, requests the sentence data if needed and displays the sentence for
    // the current word.
    void updateSentence();

    // Draws the Japanese text on the strip at the current scroll position. Fills the wordrect
    // list if necessary.
    void paintJapanese(QPainter *p, QFontMetrics &fm, int y);

    // Tells the widget to repaint one of its word rectangles.
    void updateWordRect(int index);

    // Tells the widget to repaint the area below the words.
    void updateDots();

    // Reacts to clicking one of the word forms either of the hovered or the
    // specified word position.
    void selectForm(int form, int wordpos = -1);

    // Fills the wordrect list holding position of words that can be howered. This is done
    // automatically in paint events. Only call this when the paint event doesn't arrive while
    // the rectangles are needed.
    void fillWordRects();

    // Dictionary of the word.
    Dictionary *dict;

    // Popup listing words above the strip. Only set if a popup is visible.
    std::unique_ptr<ZExamplePopup> popup;

    ExampleDisplay display;

    // Index of the word in dict. No sentences are displayed if index is -1.
    int wordindex;

    // True when a new item has been set but its data has not been loaded yet.
    bool dirty;

    // Current block in the sentences data file.
    ushort block;

    // Current line in block.
    uchar line;

    // Current word position in the line.
    uchar wordpos;

    // Common data of the current word.
    WordCommons *common;

    // Index of the current sentence from the sentences of the word's commons.
    int current;

    // Data of the sentence being displayed.
    ExampleSentenceData sentence;

    // A list of rectangle positions for every word in the current Japanese sentence. The list
    // is populated when the strip is first drawn with a new sentence or different display
    // mode. The rectangles refer to the current word positions and are updated when the strip
    // is scrolled.
    std::vector<QRect> wordrect;

    // Index of the word rectangle the mouse cursor is on. When the cursor is not in the strip
    // this value is -1. If it is over the strip but not over a rectangle, the value is
    // wordrect.size().
    int hovered;

    bool interactible;

    // Width of the Japanese text when drawn on the strip. Passed to scrollMax() when
    // requested.
    mutable int jpwidth;

    // Width of the translated text when drawn on the strip. Passed to scrollMax() when
    // requested.
    mutable int trwidth;

    friend class ZExamplePopup;

    typedef ZScrollArea base;
};

#endif // ZEXAMPLESTRIP_H

