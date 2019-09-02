/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef PRINTPREVIEWFORM_H
#define PRINTPREVIEWFORM_H

#include <QtCore>
#include <QMainWindow>
#include <QPrinter>
#include <QFont>
#include <QPrintPreviewWidget>
#include "dialogwindow.h"

namespace Ui {
    class PrintPreviewForm;
}

class Dictionary;
class QCharTokenizer;
class QPainter;
class QFontMetrics;
struct WordEntry;
class QCharString;


class PrintTextBlock
{
public:
    PrintTextBlock(int spacew, bool furitext);

    // Must be called before adding any text. When called on a built block, the position of
    // lines will be recalculated. Only the kanji block can handle a second call of
    // setMaxWidth() after the word to print has been set.
    void setMaxWidth(int width);

    // Should be called once before adding anything to the block.
    void setLineAttr(int lineheight, int descent);

    void setFuriWord(WordEntry *e, QFont &f, QFontMetrics &fm, QFont &ff, QFontMetrics &ffm, int lineheight, int descent, int furiheight, int furidescent);

    void addFuriWord(WordEntry *e, QFont &f, QFontMetrics &fm, QFont &ff, QFontMetrics &ffm, int furiheight, int furidescent);

    void addText(QCharTokenizer &tok, QFont &f, QFontMetrics &fm);
    void addText(const QString &str, QFont &f, QFontMetrics &fm);
    bool empty() const;
    int width() const;
    int maxWidth() const;
    int height() const;
    void setHeight(int h);
    void paint(QPainter &p, int x, int y, bool rightalign = false);
private:
    // Tokens and their leading delimiters added to the text block unchanged.
    struct Token
    {
        // String of the token.
        QString str;

        // Delim printed before the string if something comes in front of it.
        QString delim;
    };

    // Single word to be painted at a position and stretch value.
    struct Item
    {
        // Index of string the item represents in tokens.
        int tokenpos;
        // Substring position of item's text. -1 if the whole string is used.
        int pos;
        // Substring length of item's text. -1 if the whole string is used.
        int len;

        // Uses the delimiter string of the token.
        bool isdelim;

        int x;
        int y;
    };

    // Fonts settings passed to addText() or addFuriText() at a given item position.
    struct Fonts
    {
        QFont &f;
        QFontMetrics &fm;

        // Item position where the setting takes effect.
        int tokenpos;
    };

    // Helper for the addText() functions.
    void addString(int tokenpos, QFontMetrics &fm, bool addspace);

    void paintKanjiFuri(QPainter &p, int x, int y, bool rightalign);
    void paintFurigana(QPainter &p, int x, int y, int width, int pos, int len);

    // Tokens and their leading delimiters added to the text block unchanged.
    std::vector<Token> tokens;

    // Layout position of printed text items.
    std::vector<Item> list;

    // Font information for the tokens.
    std::vector<Fonts> setlist;

    // Unit width of each line starting at item position.
    struct Line
    {
        int itempos;
        int width;
    };
    std::vector<Line> lines;

    // Available width for the text.
    int maxwidth;

    // Padding width to leave before a previosu text when adding a new one.
    int spacew;

    // Line height.
    int lh;

    // Font descent.
    int desc;

    // Word used for furigana printing.
    WordEntry *word;

    // The word entry is printed at the front (or back) of the block in a flowing text.
    bool frontword;

    // Furigana line height.
    int furih;

    // Furigana font descent.
    int furidesc;

    // Furigana font.
    QFont furif;
    // Furigana font metrics.
    QFontMetrics furifm;

    // Width of the longest line.
    int w;

    // Current computed height of the text block. This can be changed with setHeight to an
    // arbitrary value. Don't add text after the change.
    int h;

    // Horizontal position in the current line.
    int left;

    // Set after the first call to addFuriText(). It's an error to mix calls to addText() and
    // addFuriText() and will throw an exception when debugging.
    bool furitext;
};


class PrintPreviewForm : public DialogWindow
{
    Q_OBJECT
public:
    PrintPreviewForm(QWidget *parent = nullptr);
    ~PrintPreviewForm();

    void exec(Dictionary *dict, const std::vector<int> &wordlist);
protected:
    virtual bool event(QEvent *e) override;
protected slots:
    void on_printButton_clicked();
    void on_setupButton_clicked();
    void on_settingsButton_clicked();
    void on_firstButton_clicked();
    void on_prevButton_clicked();
    void on_nextButton_clicked();
    void on_lastButton_clicked();
    void on_singleButton_clicked();
    void on_doubleButton_clicked();
    void on_fitButton_clicked();
    void on_stretchButton_clicked();
    void on_zoomoutButton_clicked();
    void on_zoominButton_clicked();
    void on_pageEdit_textEdited(QString newstr);
    void pageScrolled();

    void entryRemoved(int windex, int abcdeindex, int aiueoindex);
    void entryChanged(int windex, bool studydef);

    void paintPages(QPrinter *p);

    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);
private:
    // Saves the current printer settings in Settings::print. Returns true if anything was
    // changed.
    bool savePrinterSettings();

    Ui::PrintPreviewForm *ui;

    QPrinter printer;
    QPrintPreviewWidget *preview;

    // Finished previewing and started printing.
    bool printing;

    // Dictionary holding words to print.
    Dictionary *dict;

    // List of word indexes in dict.
    std::vector<int> list;

    // Number of pages.
    int pagecnt;

    typedef DialogWindow    base;
};


#endif // PRINTPREVIEWFORM_H
