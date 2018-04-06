/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJISETTINGS_H
#define KANJISETTINGS_H

#include <QColor>
#include "fastarray.h"

struct KanjiSettings
{
    enum ShowPosition { NearCursor, RestoreLast, SystemDefault };

    bool savefilters = true;
    bool resetpopupfilters = false;

    int mainref1 = 3;
    int mainref2 = 4;
    int mainref3 = 0;
    int mainref4 = 0;

    bool listparts = false;

    ShowPosition showpos;

    bool tooltip = true;
    bool hidetooltip = true;
    int tooltipdelay = 5;

    // Show/hide each reference in kanji information.
    std::vector<char> showref;
    // Display position of each reference in kanji information.
    std::vector<int> reforder;
};

namespace Settings
{
    extern KanjiSettings kanji;
}



#endif // KANJISEARCHSETTINGS_H

