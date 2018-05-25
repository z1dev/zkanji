/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DIALOGS_H
#define DIALOGS_H

// Functions that display dialogs. The implementations are found in the form source files.

class Dictionary;
class QWidget;
class WordToGroupForm;
class KanjiToGroupForm;
class WordToDictionaryForm;


// Shows a dialog to select program language on startup. Returns false if the user pressed
// close to prevent starting the program. If true is returned, the Settings::language string
// is set to the ID of the selected language.
bool languageSelect();

// Shows WordToGroupForm dialog. Set updatelastdiag to true, if the global UI should remember
// the last word dialog where the ok button was pressed. Calling gUI->wordToDestSelect() will
// then open the last dialog.
void wordToGroupSelect(Dictionary *d, int windex, bool showmodal = false, bool updatelastdiag = false /*, QWidget *dialogParent = nullptr*/);
// Shows WordToGroupForm dialog. Set updatelastdiag to true, if the global UI should remember
// the last word dialog where the ok button was pressed. Calling gUI->wordToDestSelect() will
// then open the last dialog.
void wordToGroupSelect(Dictionary *d, const std::vector<int> &indexes, bool showmodal = false, bool updatelastdiag = false/*, QWidget *dialogParent = nullptr*/);
// Shows kanjiToGroupForm dialog.
void kanjiToGroupSelect(Dictionary *d, ushort kindex, bool showmodal = false/*, QWidget *dialogParent = nullptr*/);
// Shows kanjiToGroupForm dialog.
void kanjiToGroupSelect(Dictionary *d, const std::vector<ushort> kindexes, bool showmodal = false/*, QWidget *dialogParent = nullptr*/);

// Shows WordToDictionaryForm dialog. Set updatelastdiag to true, if the global UI should
// remember the last word dialog where the ok button was pressed. Calling
// gUI->wordToDestSelect() will then open the last dialog.
void wordToDictionarySelect(Dictionary *d, int windex, bool showmodal = false, bool updatelastdiag = false);

// Opens the dictionary listing window where new dictionaries can be created or user
// dictionaries can be deleted.
void editDictionaries();

// Opens the dictionary information editor window.
void editDictionaryText(Dictionary *d);

// Opens the word editor window to edit a new word for the passed dictionary.
void editNewWord(Dictionary *d);

// Shows a modal window listing dictionary statistics for the passed dictionary. The
// dictionary can be changed by the user from a combo box.
void showDictionaryStats(int dictindex);

// Opens an editor for the word entry. The definition at defindex will be initially selected.
// Pass -1 to windex to start editing a new word.
void editWord(Dictionary *d, int windex, int defindex, QWidget *parent);
// Opens an editor for the word entry in dest at destwindex. The definitions from the source
// will be copied as new definitions to the end of the original word. Pass -1 in destwindex to
// start editing a new word. (The word is only created if the user accepts the edit.)
void editWord(Dictionary *srcd, int srcwindex, const std::vector<int> &srcdindexes, Dictionary *dest, int destwindex, QWidget *parent);

//// Opens a modal window above the main form for selecting a dictionary. Specify the text shown
//// to the user as the parameter. Returns the real index of the selected dictionary, or -1 on
//// cancel. Set okstr to some string to change the text of the ok button. Pass the real index
//// of the default selected dictionary. If it's -1, the first dictionary will be selected in
//// the user defined order.
//int selectDictionaryDialog(QString str, int firstselected = -1, QString okstr = QString());

// Opens the kanji definition editor form to edit the kanji passed in list.
void editKanjiDefinition(Dictionary *d, const std::vector<ushort> &list);

// Opens the kanji definition editor form to edit the passed kanji.
void editKanjiDefinition(Dictionary *d, ushort kindex);

class WordDeck;
// Shows the add-words-to-deck dialog window to select words to be added to a deck. The
// initial deck should be passed in deck, with the index of words to be listed. This window is
// always shown as a modal dialog.
void addWordsToDeck(WordDeck *deck, const std::vector<int> &indexes, QWidget *dialogParent = nullptr);
// Shows the add-words-to-deck dialog window to select words to be added to a deck. The
// destination dictionary should be passed in dictionary, with the index of words to be
// listed. Initially the last shown deck will be selected, or deck 1 if exists. When no decks
// are present for the dictionary, a new one will be created if the user accepts. The dialog
// is always shown as a modal dialog.
void addWordsToDeck(Dictionary *dictionary, const std::vector<int> &indexes, QWidget *dialogParent = nullptr);

// Shows the KanaPracticeSettingsForm to allow the user to select which kana to practice. From
// the form, the kana practice can be started.
void setupKanaPractice();

#endif // DIALOGS_H
