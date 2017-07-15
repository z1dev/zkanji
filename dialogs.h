/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DIALOGS_H
#define DIALOGS_H

// Functions that display dialogs. The implementations are found in the form source files.

class Dictionary;
class QWidget;
void wordToGroupSelect(Dictionary *d, int windex, QWidget *dialogParent = nullptr);
void wordToGroupSelect(Dictionary *d, const std::vector<int> &indexes, QWidget *dialogParent = nullptr);
void kanjiToGroupSelect(Dictionary *d, ushort kindex, QWidget *dialogParent = nullptr);
void kanjiToGroupSelect(Dictionary *d, const std::vector<ushort> kindexes, QWidget *dialogParent = nullptr);
// Opens a word filter editor window with the passed parent. Set filterindex to -1 to create a
// new filter.
void editWordFilter(int filterindex, QWidget *parent);

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

// Opens a modal window above the main form for selecting a dictionary. Specify the text shown
// to the user as the parameter. Returns the real index of the selected dictionary, or -1 on
// cancel. Set okstr to some string to change the text of the ok button. Pass the real index
// of the default selected dictionary. If it's -1, the first dictionary will be selected in
// the user defined order.
int selectDictionaryDialog(QString str, int firstselected = -1, QString okstr = QString());

// Opens the kanji definition editor form to edit the kanji passed in list.
void editKanjiDefinition(Dictionary *d, const std::vector<ushort> &list);

// Opens the kanji definition editor form to edit the passed kanji.
void editKanjiDefinition(Dictionary *d, ushort kindex);

class WordDeck;
// Shows the Add words to deck dialog window to select words to be added to a deck. The
// initial deck should be passed in deck, with the index of words to be listed.
void addWordsToDeck(WordDeck *deck, const std::vector<int> &indexes, QWidget *dialogParent = nullptr);

#endif // DIALOGS_H
