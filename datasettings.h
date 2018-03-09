/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DATASETTINGS_H
#define DATASETTINGS_H

struct DataSettings
{
    // Whether to have a timer checking if the data changed at given intervals and saving if
    // it did.
    bool autosave = false;
    // Interval of the autosave timer in minutes. Between 1 and 60.
    int interval = 2;

    // Whether to create backup files when the program runs and loads correctly.
    bool backup = false;
    // Number of backups of user data kept. The backup is created the first time the program
    // runs on a new day every backupskip days.
    int backupcnt = 4;
    // Number of days between two backups.
    int backupskip = 3;

    // Location of the backup folder. When left empty, the current user data folder will be
    // used. When set to a relative path, the path will be relative to the current user data
    // folder.
    QString location;
};

namespace Settings
{
    extern DataSettings data;
}

#endif // DATASETTINGS_H
