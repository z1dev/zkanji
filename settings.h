/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef SETTINGS_H
#define SETTINGS_H


namespace Settings
{
    void saveIniFile();
    void saveStatesFile();

    void loadScaleSettingFromFile();

    void saveSettingsToFile();
    void loadSettingsFromFile();
    void loadStatesFromFile();
}

#endif // SETTINGS_H
