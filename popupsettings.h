/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef POPUPSETTINGS_H
#define POPUPSETTINGS_H

#include <QRect>

// Saved state or settings of the popup dictionary.
struct PopupSettings
{
    enum Activation { Unchanged, Clipboard, Clear };

    // Whether to close the popup dictionary when it loses the input focus.
    bool autohide = true;

    // Whether to show the popup dictionary with full screen width when not floating.
    bool widescreen = false;

    // Whether to show the status bar below the dictionary word list.
    bool statusbar = false;

    // What to do with the search input field when the popup window is shown.
    Activation activation = Unchanged;

    // Value between 0-10. The highest makes the popup window the most transparent.
    int transparency = 0;

    // Name of the dictionary to be shown in the popup dictionary.
    QString dict;

    // Whether to close the kanji search window when it loses the input focus.
    bool kanjiautohide = false;

    // Name of the dictionary to be shown in the kanji search window.
    QString kanjidict;
};

namespace Settings
{
    extern PopupSettings popup;
}


#endif // POPUPSETTINGS_H
