#ifndef RECOGNIZERSETTINGS_H
#define RECOGNIZERSETTINGS_H

#include <QRect>

struct RecognizerSettings
{
    // Save and restore the last set size of the recognizer window.
    bool savesize = true;
    // Save and restore the last set position of the recognizer window.
    bool saveposition = false;

    // Lasted saved geometry rectangle of the handwriting recognizer. When the recognizer is
    // shown near the button, only the size of the rectangle is used.
    QRect rect = QRect(0, 0, 220, 220);
};

namespace Settings
{
    extern RecognizerSettings recognizer;
}

#endif // RECOGNIZERSETTINGS_H

