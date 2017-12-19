/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPainter>
#include <QPageSetupDialog>
#include <QPrintDialog>
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QPrinterInfo>

#include "printpreviewform.h"
#include "ui_printpreviewform.h"

#include "printsettings.h"
#include "fontsettings.h"
#include "zui.h"
#include "words.h"
#include "zkanjimain.h"
#include "zstrings.h"
#include "furigana.h"

#include "settingsform.h"
#include "globalui.h"

// TODO: add manifest so the system dialogs don't have the ancient win95 look.


//-------------------------------------------------------------


PrintTextBlock::PrintTextBlock(int spacew, bool furitext) : maxwidth(-1), spacew(spacew), lh(0), desc(0), word(nullptr), frontword(true), furih(0), furidesc(0), furifm(furif), w(0), h(0), left(0), furitext(furitext)
{

}

void PrintTextBlock::setMaxWidth(int width)
{
    if (maxwidth == width)
        return;

    maxwidth = width;

    if (lines.empty())
        return;

    // Recompute the layout.

    if (!furitext)
    {
        list.clear();
        lines.clear();

        w = 0;
        h = 0;
        left = 0;

        int setpos = 0;

        bool addspace = false;
        for (int ix = 0; ix != tokens.size(); ++ix)
        {

            if (setpos < setlist.size() - 1 && setlist[setpos + 1].tokenpos == ix)
            {
                ++setpos;
                addspace = true;
            }
            addString(ix, setlist[setpos].fm, addspace);
        }
    }
    else
    {
        lines.clear();
        QFont &f = setlist[0].f;
        QFontMetrics &fm = setlist[0].fm;
        //QFont &ff = furif;// setlist[1].f;
        //QFontMetrics &ffm = furifm; // setlist[1].fm;
        setlist.clear();
        w = 0;
        h = 0;
        setFuriWord(word, f, fm, furif, furifm, lh, desc, furih, furidesc);
        //furitext = false;
    }
}

void PrintTextBlock::setLineAttr(int lineheight, int descent)
{
    lh = lineheight;
    desc = descent;
}

void PrintTextBlock::setFuriWord(WordEntry *e, QFont &f, QFontMetrics &fm, QFont &ff, QFontMetrics &ffm, int lineheight, int descent, int furiheight, int furidescent)
{
#ifdef _DEBUG
    if (!furitext || !lines.empty())
        throw "Can't change the contents of a used block to furigana.";
#endif

    word = e;
    lh = lineheight;
    desc = descent;
    furih = furiheight;
    furidesc = furidescent;
    furif = ff;
    furifm = ffm;

    setlist.push_back({ f, fm, 0 });
    //setlist.push_back({ ff, ffm, 0 });

    // Compute the width, height and lines for the layouting.

    w = fm.width(e->kanji.toQStringRaw());
    if (w <= maxwidth)
    {
        // If the whole line fits simply save it.

        h = furih + lh;
        lines.push_back({ 0, w });
        return;
    }
    else
    {
        // Try to break up the word on furigana boundaries. Only the kanji of the data counts
        // as this is only for measuring.
        std::vector<FuriganaData> fdat;
        findFurigana(word->kanji, word->kana, fdat);

        int kanjisiz = word->kanji.size();
        int kanasiz = word->kana.size();


        h = furih + lh;
        w = 0;

        uint siz = e->kanji.size();
        int strw;
        int linew = 0;

        lines.push_back({ 0, 0 });

        int datpos = 0;

        for (int pos = 0; pos != siz; ++pos)
        {
            strw = 0;

            bool datfit = false;

            // See if one block of the data fits.
            if (datpos != fdat.size() && fdat[datpos].kanji.pos == pos)
            {
                strw = fm.width(word->kanji.toQString(fdat[datpos].kanji.pos, fdat[datpos].kanji.len));
                datfit = true;
            }

            // No furigana or no fit, use one character.
            if (strw == 0 || strw > maxwidth)
            {
                strw = fm.width(e->kanji[pos]);
                datfit = false;
            }

            if (linew != 0 && linew + strw > maxwidth)
            {
                // The word must be broken at the boundary of the block.
                lines.push_back({ pos, 0 });
                h += furih + lh;
                linew = 0;
            }

            linew += strw;
            lines.back().width = linew;
            w = std::max(w, linew);

            if (datfit)
            {
                pos += fdat[datpos].kanji.len - 1;
                ++datpos;
            }
        }

    }
}

void PrintTextBlock::addFuriWord(WordEntry *e, QFont &f, QFontMetrics &fm, QFont &ff, QFontMetrics &ffm, int furiheight, int furidescent)
{
#ifdef _DEBUG
    if (furitext)
        throw "Can't add text in furigana block.";
#endif

    word = e;
    frontword = tokens.empty();
    furih = furiheight;
    furidesc = furidescent;
    furif = ff;
    furifm = ffm;

    setlist.push_back({ f, fm, (int)tokens.size() });
    tokens.push_back({ e->kanji.toQString(), QString() });

    addString(tokens.size() - 1, fm, true);
}

void PrintTextBlock::addText(QCharTokenizer &tok, QFont &f, QFontMetrics &fm)
{
#ifdef _DEBUG
    if (furitext)
        throw "Can't add text in furigana block.";
#endif
    setlist.push_back({ f, fm, (int)tokens.size() });

    bool first = true;
    while (tok.next())
    {
        QString str = QString(tok.token(), tok.tokenSize());
        QString dstr = QString(tok.delimiters(), tok.delimSize());

        tokens.push_back({ str, dstr });
        addString(tokens.size() - 1, fm, first);
        first = false;
    }

    // Nothing was added.
    if (first)
        setlist.pop_back();
}

void PrintTextBlock::addText(const QString &str, QFont &f, QFontMetrics &fm)
{
    if (str.isEmpty())
        return;

#ifdef _DEBUG
    if (furitext)
        throw "Can't add text in furigana block.";
#endif

    setlist.push_back({ f, fm, (int)tokens.size() });
    tokens.push_back({ str, QString() });
    addString(tokens.size() - 1, fm, true);
}

void PrintTextBlock::addString(int tokenpos, QFontMetrics &fm, bool addspace)
{
    const QString &str = tokens[tokenpos].str;
    const QString &delim = tokens[tokenpos].delim;

    if (str.isEmpty())
        return;

    int space = addspace ? spacew : 0;

    int dw = left == 0 || delim.isEmpty() ? 0 : fm.width(delim);
    int strw = fm.width(str);

    if (lines.empty())
    {
        // First string to add.

        lines.push_back({ 0, 0 });
        h = lh;

        space = 0;
    }

    // If the string fits the full line it won't be cut up.
    if (strw <= maxwidth)
    {
        if (left + space + strw + dw > maxwidth)
        {
            // The word doesn't fit the line if the delimiter is included.

            lines.push_back({ (int)list.size(), 0 });

            left = 0;
            dw = 0;
            h += lh;
            space = 0;
        }

        if (dw != 0)
            list.push_back({ tokenpos, -1, -1, true, left + space, h - desc });
        list.push_back({ tokenpos, -1, -1, false, left + dw + space, h - desc });

        left += dw + strw + space;
        lines.back().width = left;

        w = std::max(w, left);

        return;
    }


    // Word does not fit the line even with no indentation. The word will be measured
    // character by character. If the delimiter fits before the first character on an
    // existing line, it's included first.

    int pos = 0;
    strw = 0;

    for (int ix = 0; ix != str.size() + 1; ++ix)
    {
        bool finish = (ix == str.size());
        int chw = !finish ? fm.width(str.at(ix)) : 0;

        if (ix == 0 && dw != 0 && left != 0 && left + chw + dw + space <= maxwidth)
        {
            // First character fits. Include the delimiter.
            list.push_back({ tokenpos, -1, -1, true, left + space, h - desc });

            left += dw + space;

            space = 0;
        }


        if (!finish && (left + strw == 0 || left + strw + chw + space <= maxwidth))
            strw += chw + space;
        else
        {
            // Character at ix does not fit. Include everything before it.

            if (strw != 0)
            {
                // Add whatever still fits before the current character.

                list.push_back({ tokenpos, pos, ix - pos, false, left + space, h - desc });
                left += strw;
                space = 0;
                lines.back().width = left;
                w = std::max(w, left);
            }

            pos = ix;

            if (!finish)
            {
                h += lh;
                left = 0;
                space = 0;
                strw = chw;

                lines.push_back({ (int)list.size(), 0 });
            }
        }
    }
    
}

bool PrintTextBlock::empty() const
{
    return lines.empty();
}

int PrintTextBlock::width() const
{
    return w;
}

int PrintTextBlock::maxWidth() const
{
    return maxwidth;
}

int PrintTextBlock::height() const
{
    if (furitext || (!furitext && word == nullptr))
        return h;

    // Adds height of furigana to the height of the block if the kanji and the definition are
    // printed together. If the kanji comes front but it takes up more than one line, or it
    // comes at the end of the string, every line must be higher.

    if (frontword && (list.size() <= 1 || list[1].tokenpos != 0))
        return h + furih;
    return h + lines.size() * furih;
}

void PrintTextBlock::setHeight(int newh)
{
    h = newh;
}

void PrintTextBlock::paint(QPainter &p, int x, int y, bool rightalign)
{
    if (empty())
        return;

    int linepos = 0;
    int setpos = 0;
    int padding = 0;

    // Every line should be higher by furih when drawing furigana over the kanji part and it's
    // not on the first line.
    bool addfurispace = word != nullptr && (!frontword || (list.size() > 1 && list[1].tokenpos == 0));
    int furiextra = (!addfurispace && word == nullptr) ? 0 : furih;

    std::vector<FuriganaData> fdat;
    

    if (!furitext)
    {
        p.setFont(setlist[0].f);

        int nextfpos = -1;

        for (int ix = 0; ix != list.size(); ++ix)
        {
            if (linepos < lines.size() - 1 && lines[linepos + 1].itempos == ix)
            {
                if (addfurispace)
                    furiextra += furih;
                ++linepos;
            }
            if (setpos < setlist.size() - 1 && setlist[setpos + 1].tokenpos == list[ix].tokenpos)
            {
                ++setpos;
                p.setFont(setlist[setpos].f);
            }

            if (rightalign && lines[linepos].itempos == ix)
                padding = w - lines[linepos].width;

            const QString &str = list[ix].isdelim ? tokens[list[ix].tokenpos].delim : tokens[list[ix].tokenpos].str;

            p.drawText(x + list[ix].x + padding, y + list[ix].y + furiextra, list[ix].pos == -1 && list[ix].len == -1 ? str : str.mid(list[ix].pos, list[ix].len));

            if (word != nullptr && ((frontword && list[ix].tokenpos == 0) || (!frontword && list[ix].tokenpos == tokens.size() - 1)))
            {
                // Draw furigana above kanji.

                if (fdat.empty())
                    findFurigana(word->kanji, word->kana, fdat);

                int left = 0;
                int strpos = list[ix].pos == -1 ? 0 : list[ix].pos;

                int datpos = 0;
                // Find the data pos in furigana data of the painted text part.
                for (int siz = fdat.size(); datpos != siz; ++datpos)
                    if (fdat[datpos].kanji.pos + fdat[datpos].kanji.len > list[ix].pos)
                        break;

                while (datpos < fdat.size() && (list[ix].pos == -1 || fdat[datpos].kanji.pos < list[ix].pos + list[ix].len))
                {
                    // Skip part that doesn't contain kanji.
                    if (fdat[datpos].kanji.pos > strpos)
                    {
                        left += setlist[setpos].fm.width(str.mid(strpos, fdat[datpos].kanji.pos - strpos));
                        strpos = fdat[datpos].kanji.pos;
                    }

                    // The whole furigana part fits.
                    if (list[ix].pos == -1 || (fdat[datpos].kanji.pos >= list[ix].pos && fdat[datpos].kanji.pos + fdat[datpos].kanji.len <= list[ix].pos + list[ix].len))
                    {
                        int strw = setlist[setpos].fm.width(str.mid(fdat[datpos].kanji.pos, fdat[datpos].kanji.len));
                        paintFurigana(p, x + list[ix].x + padding + left, y + list[ix].y + furiextra - (furih + lh - desc), strw, fdat[datpos].kana.pos, fdat[datpos].kana.len);
                        left += strw;
                        strpos += fdat[datpos].kanji.len;
                        ++datpos;
                        continue;
                    }

                    int kpos = fdat[datpos].kanji.pos - strpos;
                    int klen = std::min(list[ix].pos + list[ix].len, fdat[datpos].kanji.pos + fdat[datpos].kanji.len) - strpos;

                    QString strpart = str.mid(strpos, klen);
                    int strw = setlist[setpos].fm.width(strpart);

                    int fpos = std::max(nextfpos, fdat[datpos].kana.pos + (int)std::round((double(kpos) / fdat[datpos].kanji.len) * fdat[datpos].kana.len));
                    int flen = (int)std::round((double(klen) / fdat[datpos].kanji.len) * fdat[datpos].kana.len);
                    if (fpos + flen > fdat[datpos].kana.pos + fdat[datpos].kana.len)
                        flen = fdat[datpos].kana.pos + fdat[datpos].kana.len - fpos;
                    nextfpos = fpos + flen;

                    if (flen > 0)
                        paintFurigana(p, x + left + padding, y, strw, fdat[datpos].kana.pos + fpos, flen);

                    strpos = std::min(fdat[datpos].kanji.pos + fdat[datpos].kanji.len, list[ix].pos + list[ix].len);
                    if (strpos == fdat[datpos].kanji.pos + fdat[datpos].kanji.len)
                        ++datpos;
                    left += strw;
                }

            }
        }
    }
    else
        paintKanjiFuri(p, x, y, false/*rightalign*/);
}

void PrintTextBlock::paintKanjiFuri(QPainter &p, int x, int y, bool rightalign)
{
    std::vector<FuriganaData> fdat;
    findFurigana(word->kanji, word->kana, fdat);

    uint kanjisiz = word->kanji.size();
    uint kanasiz = word->kana.size();

    // Position in fdat.
    int datpos = 0;

    // Position in lines.
    uint linepos = 0;

    // Position to print the next character relative to x.
    int left = 0;

    // Position in the kana/furigana when breaking up word parts.
    int fpos = 0;

    // Left padding of the current line to make it right aligned.
    int padding = rightalign ? maxwidth - lines[0].width : 0;

    // Kanji font and metrics.
    QFont &f = setlist[0].f;
    QFontMetrics &fm = setlist[0].fm;

    for (uint pos = 0; pos != kanjisiz; ++pos)
    {
        p.setFont(f);

        // Newline starts.
        if (linepos != lines.size() - 1 && lines[linepos + 1].itempos == pos)
        {
            y += lh + furih;
            left = 0;
            ++linepos;
            if (rightalign)
                padding = maxwidth - lines[linepos].width;
        }

        if (datpos == fdat.size() || fdat[datpos].kanji.pos > pos)
        {
            // No furigana data for this (presumably kana) character. Print it separately as a
            // single character.
            p.drawText(x + left + padding, y + furih + lh - desc, word->kanji[pos]);
            left += fm.width(word->kanji[pos]);
            continue;
        }

        if (fdat[datpos].kanji.pos == pos && (linepos == lines.size() - 1 || lines[linepos + 1].itempos >= pos + fdat[datpos].kanji.len))
        {
            // If the whole data part fits, print it unchanged with furigana.

            QString str = word->kanji.toQString(fdat[datpos].kanji.pos, fdat[datpos].kanji.len);
            p.drawText(x + left + padding, y + furih + lh - desc, str);
            int strw = fm.width(str);
            paintFurigana(p, x + left + padding, y, strw, fdat[datpos].kana.pos, fdat[datpos].kana.len);

            pos += fdat[datpos].kanji.len - 1;
            left += strw;
            ++datpos;
            continue;
        }

        if (fdat[datpos].kanji.pos == pos)
            fpos = 0;

        // Length of the rest of the furigana part not painted yet.
        int flen = fdat[datpos].kana.len - fpos;

        // See how much of the kanji fits and paint it. Either to the end of the line or the
        // end of the kanji part, whichever comes first.
        int klen = std::min((linepos == lines.size() - 1 ? kanjisiz : lines[linepos + 1].itempos) - pos, fdat[datpos].kanji.len - (pos - fdat[datpos].kanji.pos));
        QString str = word->kanji.toQString(pos, klen);
        int strw = fm.width(str);

        p.drawText(x + left + padding, y + furih + lh - desc, str);

        // Change furigana part length to the number of characters that should be painted
        // above the current kanji.
        double kdiv = double(klen) / (fdat[datpos].kanji.len - (pos - fdat[datpos].kanji.pos));
        flen *= kdiv;

        paintFurigana(p, x + left + padding, y, strw, fpos, flen);
        fpos += flen;

        left += strw;
        pos += klen - 1;

        // End of the current data part.
        if (fdat[datpos].kanji.pos + fdat[datpos].kanji.len - 1 == pos)
            ++datpos;
    }
}

void PrintTextBlock::paintFurigana(QPainter &p, int x, int y, int width, int pos, int len)
{
    // Furigana font and metrics.
    //QFont &f = setlist[1].f;
    //QFontMetrics &fm = setlist[1].fm;

    p.setFont(furif);

    QString str = word->kana.toQString(pos, len);
    int kanaw = furifm.width(str);

    // The furigana can be printed as a single string if it takes up enough of the available
    // space.
    if (kanaw >= width || len == 1 || width - kanaw <= furih * 0.6)
    {
        if (kanaw > width)
        {
            // If the text to print does not fit the available space, it'll be stretched.
            double stretch = double(width) / kanaw;
            furif.setStretch(stretch * 100);

            kanaw *= stretch;

            p.setFont(furif);
        }

        p.drawText(x + (width - kanaw) / 2, y + furih - furidesc / 2, str);

        // Resetting stretch that was set above.
        furif.setStretch(100);
        furifm = QFontMetrics(furif);
        return;
    }

    // The furigana takes up too little space. Print each character separately
    // leaving some space between them.

    // Average space between each character and padding on both sides.
    double charspace = (width - kanaw) / (len + 1);
    x += charspace;
    width -= charspace * 2;

    // Sum of all space between the characters.
    double space = width - kanaw;

    for (int ix = 0; ix != len; ++ix)
    {
        //p.drawText(x, y, furichw, furilineheight, Qt::AlignLeft | Qt::AlignBottom | Qt::TextSingleLine, kana[pos + ix]);
        p.drawText(x, y + furih - furidesc / 2, str.at(ix));

        if (ix != len - 1)
        {
            int chspace = ix % 2 == 0 ? std::ceil(space / (len - ix - 1)) : std::floor(space / (len - ix - 1));
            x += furifm.width(str.at(ix)) + chspace;
            space = std::max(0.0, space - chspace);
        }
    }

}


//-------------------------------------------------------------


PrintPreviewForm::PrintPreviewForm(QWidget *parent) : base(parent), ui(new Ui::PrintPreviewForm), printer(QPrinter::HighResolution), printing(false), pagecnt(0)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    scaleWidget(this);

    // Because of a usual bad design decision in Qt, there's no way to get the internal
    // printer from a QPrintPreviewWidget, and the designer does not allow setting custom
    // arguments in a promoted widget's constructor. Replace the print preview widget
    // placeholder with a real preview widget which gets our own printer device.
    delete ui->previewWidget;
    ui->previewWidget = preview = new QPrintPreviewWidget(&printer, ui->centralwidget);
    preview->setObjectName(QStringLiteral("previewWidget"));

    //int pres = printer.resolution();


    if (!Settings::print.printername.isEmpty())
        printer.setPrinterName(Settings::print.printername);

    printer.setPageOrientation(Settings::print.portrait ? QPageLayout::Portrait : QPageLayout::Landscape);

    if (Settings::print.pagesizeid != -1)
    {
        QPrinterInfo inf = QPrinterInfo::printerInfo(printer.printerName());
        if (!inf.isNull())
        {
            auto sizes = inf.supportedPageSizes();
            for (const QPageSize &siz : sizes)
            {
                if (siz.id() == (QPageSize::PageSizeId)Settings::print.pagesizeid)
                {
                    printer.setPageSize(siz);
                    break;
                }
            }
        }
    }

    QPageLayout layout = printer.pageLayout();
    layout.setUnits(QPageLayout::Millimeter);
    QMarginsF margins = layout.minimumMargins();
    margins.setLeft(std::max<int>(Settings::print.leftmargin, margins.left()));
    margins.setTop(std::max<int>(Settings::print.topmargin, margins.top()));
    margins.setBottom(std::max<int>(Settings::print.bottommargin, margins.bottom()));
    margins.setRight(std::max<int>(Settings::print.rightmargin, margins.right()));
    printer.setPageMargins(margins, QPageLayout::Millimeter);

    //QStringList printers = QPrinterInfo::availablePrinterNames();
    //ui->printersCBox->addItems(printers);
    //QPrinterInfo def = QPrinterInfo::defaultPrinter();
    //if (!def.isNull())
    //    ui->printersCBox->setEditText(def.printerName());
    //else
    //    ui->printersCBox->setCurrentText(printer.printerName());

    ui->verticalLayout->addWidget(preview);

    connect(preview, &QPrintPreviewWidget::paintRequested, this, &PrintPreviewForm::paintPages);
    connect(preview->findChild<QAbstractScrollArea*>()->verticalScrollBar(), &QScrollBar::valueChanged, this, &PrintPreviewForm::pageScrolled);
    connect(gUI, &GlobalUI::settingsChanged, preview, &QPrintPreviewWidget::updatePreview);
    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &PrintPreviewForm::dictionaryToBeRemoved);
}

PrintPreviewForm::~PrintPreviewForm()
{
    delete ui;
}

void PrintPreviewForm::exec(Dictionary *d, const std::vector<int> &wordlist)
{
    dict = d;
    list = wordlist;

    connect(d, &Dictionary::entryRemoved, this, &PrintPreviewForm::entryRemoved);
    connect(d, &Dictionary::entryChanged, this, &PrintPreviewForm::entryChanged);
    connect(d, &Dictionary::dictionaryReset, this, &PrintPreviewForm::close);

    preview->updatePreview();

    show();
}

void PrintPreviewForm::on_printButton_clicked()
{
    QPrintDialog dlg(&printer, this);
    dlg.setOptions(QAbstractPrintDialog::PrintToFile | QAbstractPrintDialog::PrintPageRange | QAbstractPrintDialog::PrintCollateCopies);
    if (dlg.exec() == QDialog::Accepted)
    {
        savePrinterSettings();

        printing = true;
        //paintPages(&printer);
        preview->print();
        close();
    }
    else if (savePrinterSettings())
        preview->updatePreview();
}

void PrintPreviewForm::on_setupButton_clicked()
{
    QPageSetupDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        savePrinterSettings();
        preview->updatePreview();
    }
}

void PrintPreviewForm::on_settingsButton_clicked()
{
    SettingsForm *form = new SettingsForm(this);
    form->show(SettingsPage::Printing);
}

void PrintPreviewForm::on_firstButton_clicked()
{
    preview->setCurrentPage(1 + (Settings::print.doublepage ? 1 : 0));
}

void PrintPreviewForm::on_prevButton_clicked()
{
    int p = preview->currentPage();
    if (p <= 1)
        return;

    if (ui->doubleButton->isChecked())
        --p;
    preview->setCurrentPage(p - 1);
}

void PrintPreviewForm::on_nextButton_clicked()
{
    int p = preview->currentPage();
    if (p >= pagecnt + (Settings::print.doublepage ? 1 : 0))
        return;

    if (ui->doubleButton->isChecked())
        ++p;
    preview->setCurrentPage(p + 1);
}

void PrintPreviewForm::on_lastButton_clicked()
{
    preview->setCurrentPage(pagecnt + (Settings::print.doublepage ? 1 : 0));
}

void PrintPreviewForm::on_singleButton_clicked()
{
    preview->setSinglePageViewMode();
    if (ui->fitButton->isChecked())
        preview->setZoomMode(QPrintPreviewWidget::FitInView);
    if (ui->stretchButton->isChecked())
        preview->setZoomMode(QPrintPreviewWidget::FitToWidth);

    ui->fitButton->setEnabled(true);
    ui->stretchButton->setEnabled(true);
    ui->zoominButton->setEnabled(true);
    ui->zoomoutButton->setEnabled(true);
}

void PrintPreviewForm::on_doubleButton_clicked()
{
    preview->setFacingPagesViewMode();

    //if (ui->fitButton->isChecked())
    //    preview->setZoomMode(QPrintPreviewWidget::FitInView);
    //if (ui->stretchButton->isChecked())
    //preview->setZoomMode(QPrintPreviewWidget::FitToWidth);

    ui->fitButton->setEnabled(false);
    ui->stretchButton->setEnabled(false);
    ui->zoominButton->setEnabled(false);
    ui->zoomoutButton->setEnabled(false);
}

void PrintPreviewForm::on_fitButton_clicked()
{
    preview->setZoomMode(QPrintPreviewWidget::FitInView);
}

void PrintPreviewForm::on_stretchButton_clicked()
{
    preview->setZoomMode(QPrintPreviewWidget::FitToWidth);
}

void PrintPreviewForm::on_zoomoutButton_clicked()
{
    preview->zoomOut();
    ui->fitButton->setCheckable(false);
    ui->stretchButton->setCheckable(false);
    ui->fitButton->update();
    ui->stretchButton->update();
    ui->fitButton->setCheckable(true);
    ui->stretchButton->setCheckable(true);
}

void PrintPreviewForm::on_zoominButton_clicked()
{
    preview->zoomIn();
    ui->fitButton->setCheckable(false);
    ui->stretchButton->setCheckable(false);
    ui->fitButton->update();
    ui->stretchButton->update();
    ui->fitButton->setCheckable(true);
    ui->stretchButton->setCheckable(true);
}

void PrintPreviewForm::on_pageEdit_textEdited(QString newstr)
{
    bool ok;
    int val = newstr.toInt(&ok);
    if (ok)
        preview->setCurrentPage(val + (Settings::print.doublepage ? 1 : 0));
}

void PrintPreviewForm::pageScrolled()
{
    ui->pageEdit->setText(QString::number(std::max(1, preview->currentPage() - (Settings::print.doublepage ? 1 : 0))));
}

//void PrintPreviewForm::on_printersCBox_currentIndexChanged(int index)
//{
//    if (index != -1)
//    {
//        if (printer.printerName() == ui->printersCBox->itemText(index))
//            return;
//
//        printer.setPrinterName(ui->printersCBox->itemText(index));
//
//        QPageLayout layout = printer.pageLayout();
//        layout.setUnits(QPageLayout::Millimeter);
//        QMarginsF margins = layout.minimumMargins();
//        margins.setLeft(std::max<int>(Settings::print.leftmargin, margins.left()));
//        margins.setTop(std::max<int>(Settings::print.topmargin, margins.top()));
//        margins.setBottom(std::max<int>(Settings::print.bottommargin, margins.bottom()));
//        margins.setRight(std::max<int>(Settings::print.rightmargin, margins.right()));
//        printer.setPageMargins(margins, QPageLayout::Millimeter);
//
//        preview->updatePreview();
//    }
//}

void PrintPreviewForm::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    int wpos = -1;
    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
    {
        if (list[ix] > windex)
            --list[ix];
        else if (list[ix] == windex)
            wpos = ix;
    }

    if (wpos != -1)
    {
        list.erase(list.begin() + wpos);
        preview->updatePreview();
    }
}

void PrintPreviewForm::entryChanged(int windex, bool studydef)
{
    if (std::find(list.begin(), list.end(), windex) != list.end())
        preview->updatePreview();
}

void PrintPreviewForm::paintPages(QPrinter *pr)
{
    // Current printed page. Indexed from 1 for display.
    pagecnt = 1;

    // Current word being printed.
    int wordpos = 0;

    // First word on the current page.
    int pagepos = 0;

    // Printing the second page of a double page layout.
    bool secondpage = false;

    // Saved line sizes for double page printing.
    smartvector<PrintTextBlock> blocks;

    QRect pagerect = pr->pageRect();

    int pres = pr->resolution();

    double sizes[] = { 0.16, 0.2, 0.24, 0.28, 0.32, 0.36, 0.4 };
    double linesize = sizes[(int)Settings::print.linesize] * pres;
    int h = std::ceil(linesize);

    int m = 0.4 * pres;
    if (Settings::print.pagenum)
        pagerect.setBottom(pagerect.bottom() - m);
    //pagerect.adjust(m, m, -m, -m * (Settings::print.pagenum ? 2 : 1));

    int pagebottom = pagerect.height();

    // Space between printed columns in print units.
    double colspacing = linesize;

    int linespacing = linesize * 0.8;

    // Width of a single printed column in print units.
    double colwidth = (pagerect.width() - (Settings::print.columns - 1) * colspacing) / Settings::print.columns;

    // Column currently printed in.
    int column = 0;

    // Kana font.
    QFont kf = QFont(Settings::printKanaFont(), pr);
    // Font used for furigana.
    QFont ff = QFont(Settings::printKanaFont(), pr);
    // Definition font.
    QFont df = QFont(Settings::printDefFont(), pr);
    // Type font.
    QFont tf = QFont(Settings::printInfoFont(), pr);

    // Page number font
    QFont pf = QFont(Settings::printDefFont(), pr);

    // Determine the size of the fonts from the line size.
    adjustFontSize(kf, linesize * 0.9/*, QString(QChar(0x4e80))*/);
    adjustFontSize(ff, linesize * 0.6);
    adjustFontSize(df, linesize/*, QStringLiteral("mgMG")*/);
    adjustFontSize(tf, linesize);
    adjustFontSize(pf, m * 0.6);

    QFontMetrics kfm(kf);
    QFontMetrics ffm(ff);
    QFontMetrics dfm(df);
    QFontMetrics tfm(tf);

    WordEntry *e;

    int spacewidth = dfm.width(' ');

    int fdesc = mmin(kfm.descent(), dfm.descent(), tfm.descent());
    int furidesc = ffm.descent();

    // Height of the furigana string above the kanji. The linesize at the kanji side includes
    // this, while the other side is printed lower by this much.
    int furilinesize = std::ceil(linesize * 0.65);

    // The top and left position of the rectangle, where the current word is printed.
    int basetop = linespacing / 2;
    int baseleft = colspacing / 4;
    int top = basetop; //pagerect.top();
    int left = baseleft; //pagerect.left();

    // Used for every printed string at every step.
    QString str;

    QPainter p;
    p.begin(pr);

    p.setPen(QPen(p.pen().color(), std::max(0.1, 0.009 * pres)));

    if (!printing && Settings::print.doublepage)
    {
        QFont tmpf = QFont(Settings::printDefFont(), pr);
        adjustFontSize(tmpf, pres * 0.65/*, QStringLiteral("mgMG")*/);

        p.setFont(tmpf);
        p.drawText(0, 0, pagerect.width(), pagerect.height(), Qt::AlignCenter | Qt::TextWordWrap, tr("This page won't be printed. It's only included here to allow the pages to face each other in the preview."));
        pr->newPage();
    }

    std::unique_ptr<PrintTextBlock> block;

    // Whether to show furigana when the kanji is separately printed from the definition.
    bool blockfuri = (Settings::print.doublepage || Settings::print.separated) && Settings::print.usekanji && Settings::print.readings == PrintSettings::ShowAbove;

    // Whether to show furigana when the kanji and definition are printed together.
    bool inlinefuri = !blockfuri && Settings::print.usekanji && Settings::print.readings == PrintSettings::ShowAbove;

    // Text block for the separate kanji/kana side. Only used for separate printing.
    std::unique_ptr<PrintTextBlock> kblock;

    // Maximum width available for the kanji/kana text is around one third of the whole width
    // of the column. If it takes up less space, the rest will go to the definition. This
    // space will be extended during painting if the definition doesn't need all of the rest
    // of the width.
    int kmaxwidth = Settings::print.doublepage ? (colwidth - colspacing / 2) : std::ceil((colwidth - colspacing / 2) * 0.35);

    while (wordpos != list.size())
    {
        e = dict->wordEntry(list[wordpos]);

        // Measuring the space needed for the current word and printing if it fits on the
        // current column. If it doesn't fit, it should go on the next one.

        if (!secondpage)
        {
            block.reset(new PrintTextBlock(spacewidth, false));
            kblock.reset(new PrintTextBlock(0, blockfuri));

            // Measuring the space needed for the current word.

            if (Settings::print.doublepage || Settings::print.separated)
            {
                // Separate printing of kanji and definition if it's in its own column or on a
                // separate page.

                kblock->setMaxWidth(kmaxwidth);

                // In case furigana is shown after the kanji, it must be included in the
                // printed string. Otherwise use the kanji, and leave space for optional
                // furigana above it.
                if (blockfuri)
                    kblock->setFuriWord(e, kf, kfm, ff, ffm, h, fdesc, furilinesize, furidesc);
                else
                {
                    kblock->setLineAttr(h, fdesc);

                    if (Settings::print.readings == PrintSettings::ShowAfter && Settings::print.usekanji && e->kanji != e->kana)
                    {
                        str = e->kanji.toQString() + QStringLiteral("(%1)").arg(e->kana.toQStringRaw());
                        QCharTokenizer ktok(str.constData(), str.size(), [](QChar ch) { if (ch == '(') return QCharKind::BreakBefore; return QCharKind::Normal; });
                        kblock->addText(ktok, kf, kfm);
                    }
                    else
                    {
                        str = Settings::print.usekanji ? e->kanji.toQString() : e->kana.toQString();
                        kblock->addText(str, kf, kfm);
                    }
                }
            }

            // Definition and the rest of the word if it's printed on the same side.

            // Width available for the definition side.
            int maxwidth = colwidth - colspacing / 2;
            if (!kblock->empty())
                maxwidth -= colspacing + kblock->width();

            block->setMaxWidth(maxwidth);
            block->setLineAttr(h, fdesc);

            // Measure the kanji/kana + word types + definition in some order.

            QString kanjistr;

            // Build the string for the kanji/kana part.

            if (!Settings::print.separated && !Settings::print.doublepage)
            {
                if (inlinefuri)
                {
                    if (!Settings::print.reversed)
                        block->addFuriWord(e, kf, kfm, ff, ffm, furilinesize, furidesc);
                }
                else if (Settings::print.readings == PrintSettings::ShowAfter && Settings::print.usekanji && e->kanji != e->kana)
                {
                    kanjistr = e->kanji.toQString() + QStringLiteral("(%1)").arg(e->kana.toQStringRaw());

                    if (!Settings::print.reversed)
                    {
                        // Kanji/kana when it comes before the definition.
                        QCharTokenizer ktok(kanjistr.constData(), kanjistr.size(), [](QChar ch) { if (ch == '(') return QCharKind::BreakBefore; return QCharKind::Normal; });
                        block->addText(ktok, kf, kfm);
                    }
                }
                else
                {
                    kanjistr = !Settings::print.usekanji ? e->kana.toQString() : e->kanji.toQString();
                    if (!Settings::print.reversed)
                        block->addText(kanjistr, kf, kfm);
                }

                if (!Settings::print.reversed)
                    block->addText("-", kf, kfm);
            }

            // Definition has several parts:
            // definition number, word type, definition text. If the word has a user defined
            // definition, it's used together with all the word types specified for each
            // definition.

            const QCharString *sdef = Settings::print.userdefs ? dict->studyDefinition(list[wordpos]) : nullptr;
            if (sdef != nullptr)
            {
                // There was a user given definition for the word.

                if (Settings::print.showtype)
                {
                    QString str;
                    for (int ix = 0; ix != e->defs.size(); ++ix)
                    {
                        str = Strings::wordTypesText(e->defs[ix].attrib.types);
                        if (ix != e->defs.size() - 1)
                            str += "; ";
                    }
                    QCharTokenizer pttok(str.constData(), str.size(), qcharisspace);
                    block->addText(pttok, tf, tfm);
                }

                str = sdef->toQStringRaw();
                QCharTokenizer stok(str.constData(), str.size(), qcharisspace);

                block->addText(stok, df, dfm);
            }
            else
            {
                // No user definition. Use the word from the dictionary.
                // Print each definition separately.
                for (int ix = 0; ix != e->defs.size(); ++ix)
                {
                    if (e->defs.size() != 1)
                        block->addText(QStringLiteral("%1.").arg(ix + 1), df, dfm);

                    if (Settings::print.showtype)
                    {
                        QString str = Strings::wordTypesText(e->defs[ix].attrib.types);

                        QCharTokenizer pttok(str.constData(), str.size(), qcharisspace);
                        block->addText(pttok, tf, tfm);
                    }

                    str = e->defs[ix].def.toQStringRaw();

                    QCharTokenizer stok(str.constData(), str.size(), qcharisspace);
                    block->addText(stok, df, dfm);
                }
            }

            if (!Settings::print.doublepage && Settings::print.reversed && !Settings::print.separated)
            {
                // Kanji/kana when printed after the definition.

                block->addText("-", kf, kfm);

                if (inlinefuri)
                    block->addFuriWord(e, kf, kfm, ff, ffm, furilinesize, furidesc);
                else
                {
                    QCharTokenizer ktok(kanjistr.constData(), kanjistr.size(), [](QChar ch) { if (ch == '(') return QCharKind::BreakBefore; return QCharKind::Normal; });
                    block->addText(ktok, kf, kfm);
                }
            }

            // Update the available width of the separate kanji text after the
            // definition has been completed.
            if (!Settings::print.doublepage && Settings::print.separated)
                kblock->setMaxWidth(colwidth - colspacing / 2 - colspacing - block->width());

            // Print after measurments are done.

            // The definition block should be printed a few pixels below
            // to line up with the kanji under the furigana.
            int blockskip = 0;
            if (!Settings::print.doublepage && Settings::print.separated && Settings::print.usekanji && Settings::print.readings == PrintSettings::ShowAbove)
                blockskip = furilinesize;

            int blockh = std::max(kblock->height(), block->height() + blockskip);

            if (top != basetop && top + blockh /*+ linespacing*/ > pagebottom)
            {
                top = basetop;
                left += colwidth + colspacing;
                ++column;

                if (column >= Settings::print.columns)
                {
                    if (Settings::print.doublepage)
                    {
                        wordpos = pagepos;
                        secondpage = true;
                        continue;
                    }

                    left = baseleft;
                    column = 0;

                    if (Settings::print.pagenum)
                    {
                        p.setFont(pf);
                        p.drawText(QRect(0, pagebottom, pagerect.width(), m), Qt::AlignHCenter | Qt::AlignBottom, QString("%1").arg(pagecnt));
                    }

                    pr->newPage();
                    ++pagecnt;

                    pagepos = wordpos + 1;
                }

            }
            if (Settings::print.separator || Settings::print.background)
            {
                if (Settings::print.background && (wordpos % 2) == 1)
                    p.fillRect(QRect(left - colspacing / 4, top - linespacing / 2, colwidth, blockh + linespacing), qRgb(230, 230, 230));
                if (top != basetop && Settings::print.separator)
                    p.drawLine(left - colspacing / 4, top - linespacing / 2, left + colwidth - colspacing / 4, top - linespacing / 2);
            }

            if (!Settings::print.separated && !Settings::print.doublepage)
                block->paint(p, left, top);
            else if (!Settings::print.reversed)
            {
                // Kanji first.

                kblock->paint(p, left, top);
                if (!Settings::print.doublepage)
                    block->paint(p, left + colwidth - block->width() - colspacing / 2, top + blockskip, true);
                else
                {
                    block->setHeight(blockh);
                    blocks.push_back(block.release());
                }
            }
            else
            {
                // Definition first.

                block->paint(p, left, top + blockskip);
                if (!Settings::print.doublepage)
                    kblock->paint(p, left + colwidth - kblock->width() - colspacing / 2, top, true);
                else
                {
                    kblock->setHeight(blockh);
                    blocks.push_back(kblock.release());
                }
            }

            top += linespacing + blockh;
            ++wordpos;

            if (Settings::print.doublepage && !secondpage && wordpos == list.size())
            {
                wordpos = pagepos;
                secondpage = true;
            }
        }
        else
        {
            left = baseleft;
            top = basetop;
            column = 0;

            if (Settings::print.pagenum)
            {
                p.setFont(pf);
                p.drawText(QRect(0, pagebottom, pagerect.width(), m), Qt::AlignHCenter | Qt::AlignBottom, QString("%1").arg(pagecnt));
            }

            pr->newPage();
            ++pagecnt;

            // Second page.
            for (int ix = 0; ix != blocks.size(); ++ix, ++wordpos)
            {
                PrintTextBlock *bl = blocks[ix];
                int blockh = bl->height();

                if (top != basetop && top + bl->height() /*+ linespacing*/ > pagebottom)
                {
                    top = basetop;
                    left += colwidth + colspacing;
                }
                if (Settings::print.separator || Settings::print.background)
                {
                    if (Settings::print.background && (wordpos % 2) == 1)
                        p.fillRect(QRect(left - colspacing / 4, top - linespacing / 2, colwidth, blockh + linespacing), qRgb(230, 230, 230));
                    if (top != basetop && Settings::print.separator)
                        p.drawLine(left - colspacing / 4, top - linespacing / 2, left + colwidth - colspacing / 4, top - linespacing / 2);
                }

                bl->paint(p, left, top);
                top += linespacing + bl->height();
            }

            column = 0;
            top = basetop;
            left = baseleft;

            blocks.clear();

            secondpage = false;
            pagepos = wordpos;

            if (wordpos != list.size())
            {
                if (Settings::print.pagenum)
                {
                    p.setFont(pf);
                    p.drawText(QRect(0, pagebottom, pagerect.width(), m), Qt::AlignHCenter | Qt::AlignBottom, QString("%1").arg(pagecnt));
                }

                pr->newPage();
                ++pagecnt;
            }
        }
    }

    if (!list.empty() && Settings::print.pagenum)
    {
        p.setFont(pf);
        p.drawText(QRect(0, pagebottom, pagerect.width(), m), Qt::AlignHCenter | Qt::AlignBottom, QString("%1").arg(pagecnt));
    }

    p.end();

    ui->pageLabel->setText(QStringLiteral("/ %1").arg(pagecnt));
}

void PrintPreviewForm::dictionaryToBeRemoved(int index, int orderindex, Dictionary *d)
{
    if (d == dict)
        close();
}

bool PrintPreviewForm::savePrinterSettings()
{
    bool changed = false;

    QString oldname = Settings::print.printername;
    Settings::print.printername = printer.printerName();
    if (Settings::print.printername == QPrinterInfo::defaultPrinterName())
        Settings::print.printername = QString();

    if (Settings::print.printername != oldname)
        changed = true;

    int oldsize = Settings::print.pagesizeid;
    Settings::print.pagesizeid = (int)printer.pageLayout().pageSize().id();
    if (oldsize != Settings::print.pagesizeid)
        changed = true;

    bool oldportrait = Settings::print.portrait;
    Settings::print.portrait = printer.pageLayout().orientation() == QPageLayout::Portrait;
    if (oldportrait != Settings::print.portrait)
        changed = true;

    QPageLayout layout = printer.pageLayout();
    layout.setUnits(QPageLayout::Millimeter);

    QMarginsF margins = layout.margins();

    int oldleft = Settings::print.leftmargin;
    int oldtop = Settings::print.topmargin;
    int oldright = Settings::print.rightmargin;
    int oldbottom = Settings::print.bottommargin;
    Settings::print.leftmargin = margins.left();
    Settings::print.topmargin = margins.top();
    Settings::print.rightmargin = margins.right();
    Settings::print.bottommargin = margins.bottom();
    if (oldleft != Settings::print.leftmargin || oldtop != Settings::print.topmargin || oldright != Settings::print.rightmargin || oldbottom != Settings::print.bottommargin)
        changed = true;

    return changed;
}

//-------------------------------------------------------------
