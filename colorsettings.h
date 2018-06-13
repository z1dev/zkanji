/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef COLORSETTINGS_H
#define COLORSETTINGS_H

#include <QColor>
#include <QPalette>

struct ColorSettings
{
    enum SystemColorTypes { Bg, Text, SelBg, SelText, ScrollBg, ScrollHandle };

    bool useinactive = true;

    QColor bg;
    QColor text;
    QColor selbg;
    QColor seltext;
    QColor bgi;
    QColor texti;
    QColor selbgi;
    QColor seltexti;
    QColor scrollbg;
    QColor scrollbgi;
    QColor scrollh;
    QColor scrollhi;

    enum UIColorTypes {
        Grid, StudyCorrect, StudyWrong, StudyNew,
        Inf, Attrib, Types, Notes,
        Fields, Dialects, KanaOnly, SentenceWord, KanjiTestPos,
        Oku, N5, N4, N3, N2, N1, KanjiNoWords,
        KanjiUnsorted, KanjiNoTranslation, KataBg, HiraBg,
        SimilarText, /*SimilarBg, PartsBg, PartOfBg,*/
        StrokeDot, KanjiExBg, Stat1, Stat2, Stat3
    };

    QColor grid;

    QColor studycorrect;
    QColor studywrong;
    QColor studynew;

    QColor inf;
    QColor attrib;
    QColor types;
    QColor notes;
    QColor fields;
    QColor dialect;
    QColor kanaonly;
    QColor sentenceword;
    QColor kanjitestpos;

    QColor n5;
    QColor n4;
    QColor n3;
    QColor n2;
    QColor n1;

    QColor kanjinowords;
    QColor kanjiunsorted;
    QColor kanjinotranslation;

    QColor katabg;
    QColor hirabg;

    QColor similartext;
    //QColor similarbg;
    //QColor partsbg;
    //QColor partofbg;
    QColor strokedot;
    QColor kanjiexbg;

    QColor stat1;
    QColor stat2;
    QColor stat3;

    bool coloroku = true;
    QColor okucolor;

    // Which default colors to return, ldef for light or ddef for dark theme.
    bool lighttheme = true;
};

namespace Settings
{
    extern ColorSettings colors;

    // Returns the color selected in the settings for elements with active/inactive state,
    // like the text and backgrounds.
    QColor textColor(const QPalette &pal, bool active, ColorSettings::SystemColorTypes type);

    // Returns the color selected in the settings for elements with active/inactive state,
    // like the text and backgrounds, using the application palette.
    QColor textColor(bool active, ColorSettings::SystemColorTypes type);

    // Returns the color selected in the settings for elements with active/inactive state,
    // like the text and backgrounds, using the application palette. Whether the displayed
    // state is active or inactive depends on the passed widget.
    QColor textColor(QWidget *w, ColorSettings::SystemColorTypes type);

    // Returns the color selected in the settings for elements with active/inactive state,
    // like the text and backgrounds, using the application palette for active widgets.
    QColor textColor(ColorSettings::SystemColorTypes type);

    // Returns the color selected in the settings for a UI element. If the user did not set a
    // color for something, the default color matching the current color theme is returned.
    QColor uiColor(ColorSettings::UIColorTypes type);

    // Returns the default color for a UI element under the current color theme.
    QColor defUiColor(ColorSettings::UIColorTypes type);

    // Changes the palette of the widget to make the text and background fit the settings.
    void updatePalette(QWidget *w);
}

#endif // COLORSETTINGS_H

