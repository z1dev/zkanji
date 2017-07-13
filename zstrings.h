#ifndef ZSTRINGS_H
#define ZSTRINGS_H

#include <QCoreApplication>

enum class InfTypes : int;

// Using a struct instead of namespace to make the Qt tr functions work (which require an
// object.)
struct Strings : QObject
{

    static QString capitalize(const QString &str);

    static QString wordType(uchar type);
    static QString wordNote(uchar note);
    static QString wordField(uchar field);
    static QString wordDialect(uchar field);
    static QString wordInfo(uchar inf);
    static QString wordJLPTLevel(uchar lv);
    static QString wordInflection(uchar infl);

    static QString wordTypeLong(uchar type);
    static QString wordNoteLong(uchar note);
    static QString wordFieldLong(uchar field);
    static QString wordDialectLong(uchar field);
    static QString wordInfoLong(uchar inf);

    static QString wordAttrib(uchar attr);

    static QString wordInfoTags(uchar infs);
    static QString wordTypeTags(uint types);
    static QString wordNoteTags(uint notes);
    static QString wordFieldTags(uint fields);
    static QString wordDialectTags(ushort dials);

    // Returns the value of the word grammar type from a string. If ok is passed, it returns
    // false if the format is incorrect. Empty string is correct and returns 0. Non lower case
    // character or underscore counts as incorrect. Tags must be separated by semicolon. Space
    // is invalid anywhere. If a tag is not recognized but doesn't count as incorrect, it's
    // ignored and doesn't change the result.

    static uchar wordTagInfo(const QString &str, bool *ok = nullptr);
    static uint wordTagTypes(const QString &str, bool *ok = nullptr);
    static uint wordTagNotes(const QString &str, bool *ok = nullptr);
    static uint wordTagFields(const QString &str, bool *ok = nullptr);
    static ushort wordTagDialects(const QString &str, bool *ok = nullptr);

    static QString wordTypesText(int types);
    static QString wordTypesTextLong(int types);
    static QString wordNotesText(int notes);
    static QString wordFieldsText(int fields);
    static QString wordDialectsText(int dials);
    static QString wordInfoText(int info);
    static QString wordInflectionText(std::vector<InfTypes> infl);

    //Q_DECLARE_TR_FUNCTIONS(Strings);
};

#endif
