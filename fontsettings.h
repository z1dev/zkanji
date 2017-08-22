/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef FONTSETTINGS_H
#define FONTSETTINGS_H

#include <QString>

struct FontSettings
{
    enum FontStyle { Normal, Bold, Italic, BoldItalic };
    enum LineSize { Small, Medium, Large };

    // Kanji in a kanji grid.
    QString kanji;
    // Point size of the kanji font shown in kanji grids.
    int kanjifontsize = 35;
    // Whether to disable sub-pixel rendering of the kanji grid font.
    bool nokanjialias = true;
    // Widget and dictionary definition text font.
    QString main;
    // Written and kana parts of word in the dictionary.
    QString kana;
    // Small info text font displayed in the dictionary.
    QString info;
    // Style of small info text font displayed in the dictionary.
    FontStyle infostyle = Normal;

    // Written and kana parts of word in the dictionary.
    QString printkana;
    // Printed word definition text font name.
    QString printdefinition;
    // Printed small info text font name.
    QString printinfo;
    // Printed small info text font stgyle.
    FontStyle printinfostyle = Italic;

    // Line size of dictionary entries in the main windows.
    LineSize mainsize = Medium;
    // Line size of dictionary entries in the popup dictionary.
    LineSize popsize = Medium;
};

namespace Settings
{
    extern FontSettings fonts;

    // Returns the name of the font used for drawing radicals.
    // TODO: replace this with SOD drawings?
    QString radicalFontName();

    QFont kanjiFont();
    QFont radicalFont();
    QFont kanaFont();
    QFont defFont();
    QFont notesFont();
    QFont extraFont();

    QFont printKanaFont();
    QFont printDefFont();
    QFont printInfoFont();
}


#endif // FONTSETTINGS_H
