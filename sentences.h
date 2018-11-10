/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef SENTENCES_H
#define SENTENCES_H

#include <QtCore>
#include "qcharstring.h"
#include "fastarray.h"
#include "smartvector.h"

// Structure storing word data for a single sentence in an examples data block.
struct ExampleWordsData
{
    struct Form
    {
        QCharString kanji;
        QCharString kana;
    };

    // Word position in sentence.
    ushort pos;
    // Word length in sentence.
    ushort len;
    // Kanji and kana forms of words.
    fastarray<Form, quint16> forms;
};

// Data for a single example sentence in an examples data block.
struct ExampleSentenceData
{
    fastarray<ExampleWordsData, quint16> words;
    QCharString japanese;
    QCharString translated;
};

struct ExampleBlock
{
    ushort block;
    // Size of block with sentences and word forms in bytes.
    int size;
    smartvector<ExampleSentenceData> lines;
};

// Class for loading and managing the example sentences data. At most 1 MB of example
// sentences are kept in memory at a time. When a new sentence is requested, the block of the
// last accessed sentence can be unloaded to stay below that limit.
class Sentences final
{
public:
    Sentences();
    ~Sentences();

    void reset();
    void load(const QString &filename);

    // The date when the sentences data file was built.
    QDateTime creationDate() const;
    // The version string of the program the sentences database was built with.
    QString programVersion() const;

    // Returns a copy of a sentence data from the passed block and line. Sentences can be
    // unloaded at any time, so a direct pointer can't be returned here.
    ExampleSentenceData getSentence(ushort block, uchar line);

    // Returns the list of example sentence ids.
    const std::vector<std::pair<int, int>> &getIdList() const;

    bool isLoaded() const;
private:
    void loadBlock(ushort index, ExampleBlock &block);

    // Helper function for loadBlock. Takes two bytes from arr at pos and returns them as a
    // short value. Pos is incremented by 2. The bytes should be in little endian order in the
    // array.
    quint16 getShort(const QByteArray &arr, int &pos);

    // Helper function for loadBlock. Takes four bytes from arr at pos and returns them as an
    // int value. Pos is incremented by 4. The bytes should be in little endian order in the
    // array.
    qint32 getInt(const QByteArray &arr, int &pos);

    // Helper function for loadBlock. Reads the length of a UTF-8 sring and the string itself,
    // converts it to 2 byte unicode and returns it as a QCharString. Updates pos to point
    // after the last used character.
    QCharString getByteArrayString(const QByteArray &arr, int &pos);

    QFile f;
    QDataStream stream;

    // Number of bytes all the loaded blocks take.
    int usedsize;

    // Whether the sentences data file has been correctly loaded.
    bool loaded;

    // Date and time when the sentences data was built and saved.
    QDateTime creation;
    // Program version used to build this data.
    QString prgversion;

    // Position of each block in the examples file.
    std::vector<int> blockpos;

    std::list<ExampleBlock> blocks;

    // Sentence ids in order.
    std::vector<std::pair<int, int>> ids;
};

namespace ZKanji
{
    extern Sentences sentences;
}


#endif // SENTENCES_H
