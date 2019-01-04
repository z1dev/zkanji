/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "qcharstring.h"
#include "smartvector.h"

struct KanjiEntry;

enum class InfTypes;
enum class WordInfo;
enum class WordTypes;
enum class WordNotes;
enum class WordFields;

// Must be called on startup before anything else zkanji related.
// Initializes the strings used for deinflecting words.
void initializeDeinflecter();

// Converst the old word types to the new word types from data pre-2015.
uint convertOldTypes(uint old);
// Converst the old word notes to the new word notes from data pre-2015.
uint convertOldNotes(uint old);
// Converst the old word fields to the new word fields from data pre-2015.
uint convertOldFields(uint old);

// deinflect: returns a list of possible inflections applied to str. Many inflections
// can be chained together that modify already inflected forms. 

// A single possible inflection and found by deinflect(). Use the form, type and
// other values to determine what kind of inflections the word might have had.
struct InflectionForm
{
    // The deinflected form of a string.
    QString form;
    // The number of characters at the end of form that were changed due to the inflections.
    int infsize;

    // The grammatical type of the word. After deinflection, a word must be a given type
    // for the inflections to apply. For example if the result is not an adjective, it
    // can't get the -kute inflection.
    WordTypes type;
    // List of inflections applied to the word before the current form is reached.
    std::vector<InfTypes> inf;

    InflectionForm();
    InflectionForm(const QString &form, int infsize, WordTypes type, const std::vector<InfTypes> &inf);
};

// Returns deinflected forms of possible words with the word type they should be,
// if they can take up the form in str. Inf contains a list of inflections
// removed.
void deinflect(QString str, smartvector<InflectionForm> &result);


#endif
