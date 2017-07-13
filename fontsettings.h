#ifndef FONTSETTINGS_H
#define FONTSETTINGS_H

#include <QString>

struct FontSettings
{
    enum FontStyle { Normal, Bold, Italic, BoldItalic };
    enum LineSize { Small, Medium, Large };

    // Kanji in a kanji grid.
    QString kanji;
    // Point size of the kanji font shown in kanji grids.
    int kanjifontsize = 35;
    // Whether to disable sub-pixel rendering of the kanji grid font.
    bool nokanjialias = true;
    // Written and kana parts of word in the dictionary.
    QString kana;
    // Definition text displayed in the dictionary.
    QString definition;
    // Small info text displayed in the dictionary.
    QString info;
    // Style of small info text displayed in the dictionary.
    FontStyle infostyle = Normal;
    // Inflection/conjugation text when displayed in the dictionary.
    QString extra;
    // Style of inflection/conjugation text when displayed in the dictionary.
    FontStyle extrastyle = Bold;

    // Written and kana parts of word in the dictionary.
    QString printkana;
    // Definition text displayed in the dictionary.
    QString printdefinition;
    // Small info text displayed in the dictionary.
    QString printinfo;
    // Style of small info text displayed in the dictionary.
    FontStyle printinfostyle = Italic;

    // Line size of dictionary entries in the main windows.
    LineSize mainsize = Medium;
    // Line size of dictionary entries in the popup dictionary.
    LineSize popsize = Medium;
};

namespace Settings
{
    extern FontSettings fonts;

    // Returns the name of the font used for drawing radicals.
    // TODO: replace this with SOD drawings?
    QString radicalFontName();

    QFont kanjiFont();
    QFont radicalFont();
    QFont kanaFont();
    QFont defFont();
    QFont notesFont();
    QFont extraFont();

    QFont printKanaFont();
    QFont printDefFont();
    QFont printInfoFont();
}


#endif // FONTSETTINGS_H
