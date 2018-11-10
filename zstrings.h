/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZSTRINGS_H
#define ZSTRINGS_H

#include <QCoreApplication>

enum class InfTypes;

namespace Strings
{
    QString priorities(int level);
    QString capitalize(const QString &str);

    QString wordType(uchar type);
    QString wordNote(uchar note);
    QString wordField(uchar field);
    QString wordDialect(uchar field);
    QString wordInfo(uchar inf);
    QString wordJLPTLevel(uchar lv);
    QString wordInflection(uchar infl);

    QString wordTypeLong(uchar type);
    QString wordNoteLong(uchar note);
    QString wordFieldLong(uchar field);
    QString wordDialectLong(uchar field);
    QString wordInfoLong(uchar inf);

    QString wordAttrib(uchar attr);

    QString wordInfoTags(uchar infs);
    QString wordTypeTags(uint types);
    QString wordNoteTags(uint notes);
    QString wordFieldTags(uint fields);
    QString wordDialectTags(ushort dials);

    // Returns the value of the word grammar type from a string. If ok is passed, it returns
    // false if the format is incorrect. Empty string is correct and returns 0. Non lower case
    // character or underscore counts as incorrect. Tags must be separated by semicolon. Space
    // is invalid anywhere. If a tag is not recognized but doesn't count as incorrect, it's
    // ignored and doesn't change the result.

    uchar wordTagInfo(const QString &str, bool *ok = nullptr);
    uint wordTagTypes(const QString &str, bool *ok = nullptr);
    uint wordTagNotes(const QString &str, bool *ok = nullptr);
    uint wordTagFields(const QString &str, bool *ok = nullptr);
    ushort wordTagDialects(const QString &str, bool *ok = nullptr);

    QString wordTypesText(int types);
    QString wordTypesTextLong(int types, QString separator = QStringLiteral(", "));
    QString wordNotesText(int notes);
    QString wordNotesTextLong(int types, QString separator = QStringLiteral(", "));
    QString wordFieldsText(int fields);
    QString wordFieldsTextLong(int types, QString separator = QStringLiteral(", "));
    QString wordDialectsText(int dials);
    QString wordDialectsTextLong(int types, QString separator = QStringLiteral(", "));
    QString wordInfoText(int info);
    QString wordInflectionText(const std::vector<InfTypes> &infl);

    //Q_DECLARE_TR_FUNCTIONS(Strings);
};

#endif
