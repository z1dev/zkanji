/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJISETTINGS_H
#define KANJISETTINGS_H

#include <QColor>

struct KanjiSettings
{
    bool savefilters = true;
    bool resetpopupfilters = false;

    int mainref1 = 3;
    int mainref2 = 4;
    int mainref3 = 0;
    int mainref4 = 0;

    bool listparts = false;

    bool tooltip = true;
    bool hidetooltip = true;
    int tooltipdelay = 5;

    bool showref[33];
    int reforder[33];
};

namespace Settings
{
    extern KanjiSettings kanji;
}



#endif // KANJISEARCHSETTINGS_H

