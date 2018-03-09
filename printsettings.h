/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef PRINTSETTINGS_H
#define PRINTSETTINGS_H

struct PrintSettings
{
    // Show furigana for the printed kanji or not, and where to show it.
    // DontShow - No furigana.
    // ShowAbove - Furigana is placed above the kanji making the kanji line taller. Each kana
    //             is placed above the correct kanji.
    // ShowAfter - Reading of kanji is placed between parentheses after the kanji.
    enum Readings { DontShow, ShowAbove, ShowAfter };
    enum LineSizes { Size16, Size2, Size24, Size28, Size32, Size36, Size4 };

    // Use the fonts set for the dictionary instead of the local font settings.
    bool dictfonts = true;

    // Print the kanji/kana and definition on separate pages, making them take up the same
    // space.
    bool doublepage = false;

    // Separate the kanji/kana and definition into their own columns. Don't flow the text
    // around each other.
    bool separated = false;

    // Height of a printed line of text including spacing in inch.
    LineSizes linesize = Size24;

    // Number of columns printed on a single page.
    int columns = 2;

    // Whether to print kanji or only kana for words.
    bool usekanji = true;

    // Show the grammatical type of each printed word in front of the definition or not. This
    // is printed in italics when using base dictionary fonts.
    bool showtype = true;

    // User user definitions instead of dictionary definitions when specified.
    bool userdefs = true;

    // If false, the japanese version goes to the front for each word, when true, the
    // definition is placed in front.
    bool reversed = false;

    // Show furigana for the printed kanji or not, and where to show it. The ShowAbove value
    // is only valid if the kanji is printed separately from the definition. Either in a
    // doublepage setup, or when the kanji and definition are not separated in their column.
    Readings readings = ShowAfter;

    // Print darker background for every odd word.
    bool background = false;
    // Print separator line between words.
    bool separator = false;
    // Print page numbers.
    bool pagenum = true;

    // Name of printer last used. When this is the default printer, it's left empty.
    QString printername;
    // Page is printed in portrait mode or not.
    bool portrait = true;
    // Page margin in millimeters.
    int leftmargin = 16;
    // Page margin in millimeters.
    int rightmargin = 16;
    // Page margin in millimeters.
    int topmargin = 16;
    // Page margin in millimeters.
    int bottommargin = 16;
    // Numeric value of the last used page size.
    int pagesizeid = -1;
};

namespace Settings
{
    extern PrintSettings print;
}

#endif // PRINTSETTINGS_H
