/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef FONTSETTINGS_H
#define FONTSETTINGS_H

#include <QString>

enum class FontStyle { Normal, Bold, Italic, BoldItalic };
enum class LineSize { Small, Medium, Large, VeryLarge };

struct FontSettings
{
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
    FontStyle infostyle = FontStyle::Normal;

    // Written and kana parts of word in the dictionary.
    QString printkana;
    // Printed word definition text font name.
    QString printdefinition;
    // Printed small info text font name.
    QString printinfo;
    // Printed small info text font stgyle.
    FontStyle printinfostyle = FontStyle::Italic;

    // Line size of dictionary entries in the main windows.
    LineSize mainsize = LineSize::Medium;
    // Line size of dictionary entries in the popup dictionary.
    LineSize popsize = LineSize::Medium;
};

namespace Settings
{
    extern FontSettings fonts;

    // Returns the name of the font used for drawing radicals.
    // TODO: replace this with SOD drawings?
    QString radicalFontName();

    // Default font used for kanji grid. Set scaled to false to return a size without
    // interface scaling, and true to get the interface scaled value.
    QFont kanjiFont(bool scaled = true);
    // Default font used for radical grid. Set scaled to false to return a size without
    // interface scaling, and true to get the interface scaled value.
    QFont radicalFont(bool scaled = true);
    // Default font used for kana line edits. Set scaled to false to return a size without
    // interface scaling, and true to get the interface scaled value.
    QFont kanaFont(bool scaled = true);
    // Default font used for untranslated text. Set scaled to false to return a size without
    // interface scaling, and true to get the interface scaled value.
    QFont mainFont(bool scaled = true);
    // Default font used for dictionary notes. Set scaled to false to return a size without
    // interface scaling, and true to get the interface scaled value.
    QFont notesFont(bool scaled = true);
    // Default font used for special text, like inflections. Set scaled to false to return a
    // size without interface scaling, and true to get the interface scaled value.
    QFont extraFont(bool scaled = true);

    QFont printKanaFont();
    QFont printDefFont();
    QFont printInfoFont();
}


#endif // FONTSETTINGS_H
