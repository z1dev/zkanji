#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H


struct GeneralSettings
{
    enum DateFormat { DayMonthYear, MonthDayYear, YearMonthDay };
    enum StartState { SaveState, AlwaysMinimize, AlwaysMaximize, ForgetState };

    DateFormat dateformat = DayMonthYear;

    // Whether to save and restore window positions and states.
    bool savewinstates = true;
    // Tool window states (i.e. kanji information window.)
    bool savetoolstates = false;

    StartState startstate = SaveState;

    bool minimizetotray = false;
};

namespace Settings
{
    extern GeneralSettings general;
}


#endif // GENERALSETTINGS_H


