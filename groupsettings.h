#ifndef GROUPSETTINGS_H
#define GROUPSETTINGS_H

struct GroupSettings
{
    bool hidesuspended = true;
};

namespace Settings
{
    extern GroupSettings group;
}

#endif // GROUPSETTINGS_H
