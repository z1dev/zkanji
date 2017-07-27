/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef COLORSETTINGS_H
#define COLORSETTINGS_H

#include <QColor>
#include <QPalette>

struct ColorSettings
{
    // Default colors for the light user interface.
    const QColor infldef = QColor(255, 255, 234);
    const QColor attribldef = Qt::black;
    const QColor typesldef = QColor(64, 128, 48);
    const QColor notesldef = QColor(144, 64, 32);
    const QColor fieldsldef = QColor(48, 64, 128);
    const QColor dialectldef = QColor(128, 48, 64);
    const QColor kanaonlyldef = QColor(144, 176, 160);
    const QColor kanjiexbgldef = QColor(210, 255, 210);

    const QColor okucolorldef = QColor(48, 162, 255);

    const QColor n5ldef = QColor(255, 34, 34);
    const QColor n4ldef = QColor(50, 68, 220);
    const QColor n3ldef = QColor(7, 160, 153);
    const QColor n2ldef = QColor(145, 77, 200);
    const QColor n1ldef = QColor(Qt::black);

    const QColor nowordsldef = QColor(192, 192, 192);
    const QColor unsortedldef = QColor(247, 247, 240);

    const QColor katabgldef = QColor(157, 216, 243);
    const QColor hirabgldef = QColor(157, 243, 149);

    const QColor similartextldef = QColor(0, 145, 245);
    const QColor similarbgldef = QColor(214, 226, 228);
    const QColor partsbgldef = QColor(214, 228, 214);
    const QColor partofbgldef = QColor(227, 220, 214);

    // Default colors for the dark user interface.
    const QColor infddef = QColor(255, 255, 234);
    const QColor attribddef = Qt::black;
    const QColor typesddef = QColor(64, 128, 48);
    const QColor notesddef = QColor(144, 64, 32);
    const QColor fieldsddef = QColor(48, 64, 128);
    const QColor dialectddef = QColor(128, 48, 64);
    const QColor kanaonlyddef = QColor(144, 176, 160);
    const QColor kanjiexbgddef = QColor(210, 255, 210);

    const QColor okucolorddef = QColor(48, 162, 255);

    const QColor n5ddef = QColor(255, 34, 34);
    const QColor n4ddef = QColor(50, 68, 220);
    const QColor n3ddef = QColor(7, 160, 153);
    const QColor n2ddef = QColor(145, 77, 200);
    const QColor n1ddef = QColor(Qt::black);

    const QColor nowordsddef = QColor(192, 192, 192);
    const QColor unsortedddef = QColor(247, 247, 240);

    const QColor katabgddef = QColor(157, 216, 243);
    const QColor hirabgddef = QColor(157, 243, 149);

    const QColor similartextddef = QColor(0, 145, 245);
    const QColor similarbgddef = QColor(214, 226, 228);
    const QColor partsbgddef = QColor(214, 228, 214);
    const QColor partofbgddef = QColor(227, 220, 214);

    enum TextColorTypes { Bg, Text, SelBg, SelText };

    QColor grid;
    QColor bg;
    QColor text;
    QColor selbg;
    QColor seltext;
    QColor bgi;
    QColor texti;
    QColor selbgi;
    QColor seltexti;

    enum UIColorTypes {
        Inf, Attrib, Types, Notes,
        Fields, Dialects, KanaOnly, KanjiExBg,
        Oku, N5, N4, N3, N2, N1, KanjiNoWords,
        KanjiUnsorted, KataBg, HiraBg,
        SimilarText, SimilarBg, PartsBg,
        PartOfBg
    };

    QColor inf;
    QColor attrib;
    QColor types;
    QColor notes;
    QColor fields;
    QColor dialect;
    QColor kanaonly;
    QColor kanjiexbg;

    QColor n5;
    QColor n4;
    QColor n3;
    QColor n2;
    QColor n1;

    QColor kanjinowords;
    QColor kanjiunsorted;

    QColor katabg;
    QColor hirabg;

    QColor similartext;
    QColor similarbg;
    QColor partsbg;
    QColor partofbg;

    bool coloroku = true;
    QColor okucolor;

    // Which default colors to return, ldef for light or ddef for dark theme.
    bool lighttheme = true;
};

namespace Settings
{
    extern ColorSettings colors;

    // Returns the color selected in the settings for the text and backgrounds.
    QColor textColor(const QPalette &pal, QPalette::ColorGroup group, ColorSettings::TextColorTypes type);

    // Returns the color selected in the settings for a UI element. If the user did not set a
    // color for something, the default color matching the current color theme is returned.
    QColor uiColor(ColorSettings::UIColorTypes type);

    // Returns the default color for a UI element under the current color theme.
    QColor defUiColor(ColorSettings::UIColorTypes type);

    // Changes the palette of the widget to make the text and background fit the settings.
    void updatePalette(QWidget *w);
}

#endif // COLORSETTINGS_H

