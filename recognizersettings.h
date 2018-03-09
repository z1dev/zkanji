/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef RECOGNIZERSETTINGS_H
#define RECOGNIZERSETTINGS_H

#include <QRect>

struct RecognizerSettings
{
    // Save and restore the last set size of the recognizer window.
    bool savesize = true;
    // Save and restore the last set position of the recognizer window.
    bool saveposition = false;
};

namespace Settings
{
    extern RecognizerSettings recognizer;
}

#endif // RECOGNIZERSETTINGS_H

