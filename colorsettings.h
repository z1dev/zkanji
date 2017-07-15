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
    const QColor infdef = QColor(255, 255, 234);
    const QColor attribdef = Qt::black;
    const QColor typesdef = QColor(64, 128, 48);
    const QColor notesdef = QColor(144, 64, 32);
    const QColor fieldsdef = QColor(48, 64, 128);
    const QColor dialectdef = QColor(128, 48, 64);
    const QColor kanaonlydef = QColor(144, 176, 160);
    const QColor kanjiexbgdef = QColor(210, 255, 210);

    const QColor okucolordef = QColor(48, 162, 255);

    const QColor n5def = QColor(255, 34, 34);
    const QColor n4def = QColor(50, 68, 220);
    const QColor n3def = QColor(7, 160, 153);
    const QColor n2def = QColor(145, 77, 200);
    const QColor n1def = QColor(Qt::black);

    const QColor nowordsdef = QColor(192, 192, 192);
    const QColor unsorteddef = QColor(247, 247, 240);

    const QColor katabgdef = QColor(157, 216, 243);
    const QColor hirabgdef = QColor(157, 243, 149);

    const QColor similartextdef = QColor(0, 145, 245);
    const QColor similarbgdef = QColor(214, 226, 228);
    const QColor partsbgdef = QColor(214, 228, 214);
    const QColor partofbgdef = QColor(227, 220, 214);

    enum ColorTypes { Bg, Text, SelBg, SelText };

    QColor grid;
    QColor bg;
    QColor text;
    QColor selbg;
    QColor seltext;
    QColor bgi;
    QColor texti;
    QColor selbgi;
    QColor seltexti;

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
};

namespace Settings
{
    extern ColorSettings colors;

    // Returns the color selected in the settings for the text and backgrounds.
    QColor color(const QPalette &pal, QPalette::ColorGroup group, ColorSettings::ColorTypes type);

    // Changes the palette of the widget to make the text and background fit the settings.
    void updatePalette(QWidget *w);
}

#endif // COLORSETTINGS_H

