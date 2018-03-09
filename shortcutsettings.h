/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef SHORTCUTSETTINGS_H
#define SHORTCUTSETTINGS_H


struct ShortcutSettings
{
    enum Modifier { AltControl, Alt, Control };

    bool fromenable = false;
    Modifier frommodifier = AltControl;
    bool fromshift = false;
    QChar fromkey = 'J';

    bool toenable = false;
    Modifier tomodifier = AltControl;
    bool toshift = false;
    QChar tokey = 'E';

    bool kanjienable = false;
    Modifier kanjimodifier = AltControl;
    bool kanjishift = false;
    QChar kanjikey = 'K';
};

namespace Settings
{
    extern ShortcutSettings shortcuts;
}


#endif // SHORTCUTSETTINGS_H
