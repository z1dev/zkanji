/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "sentences.h"
#include "zkanjimain.h"
#include "zui.h"
#include "words.h"


//-------------------------------------------------------------


Sentences::Sentences() : usedsize(0), loaded(false)
{
    ;
}

Sentences::~Sentences()
{
    ;
}

void Sentences::reset()
{
    if (f.isOpen())
        f.close();

    blockpos.clear();
    blocks.clear();
    ids.clear();
    usedsize = 0;
    creation = QDateTime();
    loaded = false;

    ZKanji::commons.clearExamplesData();
    ZKanji::wordexamples.reset();
}

void Sentences::load(const QString &filename)
{
    reset();

    // File Format:
    // Header: 3 bytes id: "zex" + 3 bytes version number ('0' padded formatted string)
    // 8 bytes (quint64): UTC date of creation.
    // Program version string in UTF-8
    // 4 bytes: File position of the structure description. (Reading starts by seeking there
    //          right away.)
    // Example blocks until the structure description. (The structure description lists the
    //          number and starting position of these blocks.)
    // Each block is zlib compressed. The compressed blocks are prefixed by the size of the
    //          compressed data: 32 bit unsigned little endian integer.
    // (This block is passed to QByteArray:qUncompress, see Qt docs.)
    // Uncompressed block format:
    // There is no count of sentences present in a block. Reading must continue until the end
    //      of the uncompressed data is found.
    // 2 bytes: little endian unsigned sentence size
    // Japanese sentence in UTF-8
    // 2 bytes: little endian unsigned sentence size
    // English sentence in UTF-8
    // 2 bytes: little endian unsigned word data count in the Japanese sentence
    // As many word data as the previous value:
    // 2 bytes: little endian unsigned word position in the Japanese sentence
    // 2 bytes: little endian unsigned word length in the Japanese sentence
    // 2 bytes: little endian unsigned number of words matching word in the sentence.
    // As many word forms as the previous value:
    // 2 bytes: little endian unsigned word kanji size
    // Word kanji form in UTF-8
    // 2 bytes: little endian unsigned word kana size
    // Word kana form in UTF-8
    //
    // Structure description format:
    // The structure and word commons data is compressed like the blocks above. The first 4
    // bytes is the compressed size of the data. After it comes the compressed data.
    // The uncompressed format is:
    // 4 bytes: signed integer, number of blocks.
    // numblocks * 4 bytes signed integer: the starting position of each block.
    // (This structure starts at the end position of the last block.)
    // After the structure description is the listing of words with sentences:
    // zstr kanji of word.
    // zstr kana of word.
    // 2 bytes: little endian unsigned number of sentences the word is in.
    // sentences data * number of sentences for the word:
    // 2byte unsigned int: block number.
    // 1byte unsigned int: sentence number.
    // 1byte unsigned int: word index.
    // The sentences data ends at the end of the uncompress data.
    //
    // Sentence IDs list:
    // In the Tatoeba project, each sentence has two int sized IDs, one for the Japanese and
    // the other for the translated sentence. Both must be saved to identify an example.
    // The data is compressed in a single block and loaded when the examples data loads.
    // The uncompressed format is:
    // 4 bytes: little endian unsigned Japanese sentence ID
    // 4 bytes: little endian unsigned English sentence ID
    // This is repeated for every sentence until the end of the uncompressed data.
    //
    // 4 byte unsigned integer: the size of the sentences file. If this doesn't match the file
    // position after this is read the file was corrupted.

    try
    {
        f.setFileName(filename);
        if (!f.open(QIODevice::ReadOnly))
            return;

        stream.setDevice(&f);
        stream.setVersion(QDataStream::Qt_5_5);
        stream.setByteOrder(QDataStream::LittleEndian);

        char tmp[7];
        tmp[6] = 0;
        stream.readRawData(tmp, 6);

        if (strncmp(tmp, "zex", 3))
            return;

        int ver = atol(tmp + 3);
        if (ver != 2)
            return;

        stream >> make_zdate(creation);
        stream >> make_zstr(prgversion, ZStrFormat::Byte);

        qint32 stpos;
        stream >> stpos;

        f.seek(stpos);

        // Reading blocks structure data.

        qint32 i;
        stream >> i;

        QByteArray data;
        data.resize(i);
        stream.readRawData(data.data(), i);
        data = qUncompress(data);

        int pos = 0;
        blockpos.resize(getInt(data, pos) + 1);
        for (int ix = 0, siz = tosigned(blockpos.size()) - 1; ix != siz; ++ix)
            blockpos[ix] = getInt(data, pos);
        blockpos[blockpos.size() - 1] = stpos;

        QCharString kanji;
        QCharString kana;
        // TODO: check for premature end of data.
        while (pos != data.size())
        {
            kanji = getByteArrayString(data, pos);
            kana = getByteArrayString(data, pos);

            WordCommons *dat = ZKanji::commons.addWord(kanji.data(), kana.data());

            dat->examples.resize(getShort(data, pos));
            for (int ix = 0; ix != dat->examples.size(); ++ix)
            {
                WordCommonsExample &e = dat->examples[ix];
                e.block = getShort(data, pos);
                e.line = data.at(pos++);
                e.wordindex = data.at(pos++);
            }
        }

        ZKanji::commons.rebuild(true);

        // Reading ids.

        stream >> i;

        data.resize(i);
        stream.readRawData(data.data(), i);
        data = qUncompress(data);

        if ((data.size() % (sizeof(int) * 2)) != 0)
            throw ZException("Invalid example sentences data ID block size.");

        pos = 0;

        ids.resize(data.size() / (sizeof(int) * 2));
        for (int ix = 0, siz = tosigned(ids.size()); ix != siz; ++ix)
            ids[ix] = std::make_pair(getInt(data, pos), getInt(data, pos));

        quint32 ui;
        stream >> ui;
        if (f.pos() != ui)
            reset();
        else
            loaded = true;

        ZKanji::wordexamples.rebuild();
    }
    catch (...)
    {
        usedsize = 0;
        blockpos.clear();
        blocks.clear();
        ids.clear();
        creation = QDateTime();
        ZKanji::commons.clearExamplesData();
        ZKanji::wordexamples.reset();
        f.close();
        QMessageBox::warning(nullptr, "zkanji", qApp->translate("", "The example sentences data file is corrupted."));
    }
}

QDateTime Sentences::creationDate() const
{
    return creation;
}

QString Sentences::programVersion() const
{
    return prgversion;
}

ExampleSentenceData Sentences::getSentence(ushort block, uchar line)
{
#ifdef _DEBUG
    if (block >= blockpos.size() - 1)
        throw "Requesting not existing block.";
#endif

    // If the sentence block is loaded, move it forward and hand it to the
    // user.
    auto it = std::find_if(blocks.begin(), blocks.end(), [block](const ExampleBlock &b) { return b.block == block; });
    if (it != blocks.end())
    {
        if (it != blocks.begin())
            blocks.splice(blocks.begin(), blocks, it);
        return *it->lines[line];
    }

    ExampleBlock newblock;
    loadBlock(block, newblock);

    while (usedsize + newblock.size > 1024 * 1024)
    {
        usedsize -= blocks.back().size;
        blocks.pop_back();
    }

    usedsize += newblock.size;
    blocks.push_front(std::move(newblock));

    return *blocks.front().lines[line];
}

const std::vector<std::pair<int, int>>& Sentences::getIdList() const
{
    return ids;
}

bool Sentences::isLoaded() const
{
    return loaded;
}

void Sentences::loadBlock(ushort index, ExampleBlock &block)
{
    block.block = index;
    block.size = 0;

    f.seek(blockpos[index]);
    int compsize = blockpos[index + 1] - blockpos[index];

    QByteArray data;
    data.resize(compsize);
    stream.readRawData(data.data(), compsize);
    data = qUncompress(data);

    int pos = 0;

    while (pos != data.size())
    {
        ExampleSentenceData *sentence = new ExampleSentenceData;
        block.lines.push_back(sentence);
        sentence->japanese = getByteArrayString(data, pos);
        sentence->translated = getByteArrayString(data, pos);

        // Pointer size + 0 terminator * 2 + japanese and translated size.
        block.size += 4 + 2 + 2 + sentence->japanese.size() * 2 + sentence->translated.size() * 2;

        int wordcnt = getShort(data, pos);
        sentence->words.resize(wordcnt);

        // words array size + forms array size + ushort *2 (pos and len size)
        block.size += 2 + wordcnt * 6;

        for (int ix = 0; ix != wordcnt; ++ix)
        {
            ExampleWordsData &wdat = sentence->words[ix];

            wdat.pos = getShort(data, pos);
            wdat.len = getShort(data, pos);

            int defcnt = getShort(data, pos);
            wdat.forms.resize(defcnt);
            for (int iy = 0; iy != defcnt; ++iy)
            {
                wdat.forms[iy].kanji = getByteArrayString(data, pos);
                wdat.forms[iy].kana = getByteArrayString(data, pos);

                // kanji and kana 0 terminators and sizes.
                block.size += 2 + 2 + wdat.forms[iy].kanji.size() * 2 + wdat.forms[iy].kana.size() * 2;
            }
        }
    }

    block.lines.shrink_to_fit();
}

quint16 Sentences::getShort(const QByteArray &arr, int &pos)
{
    // TODO: notice if the data is corrupted.
    
    pos += 2;

    quint16 r = quint16(quint16((uchar)arr.at(pos - 2)) | (quint16((uchar)arr.at(pos - 1)) << 8));

    bool bigendian = (QSysInfo::ByteOrder == QSysInfo::BigEndian);
    if (bigendian)
        r = qbswap(r);
    return r;
}

qint32 Sentences::getInt(const QByteArray &arr, int &pos)
{
    // TODO: notice if the data is corrupted.

    pos += 4;

    qint32 r = qint32(quint32((uchar)arr.at(pos - 4)) | (quint32((uchar)arr.at(pos - 3)) << 8) | (quint32((uchar)arr.at(pos - 2)) << 16) | (quint32((uchar)arr.at(pos - 1)) << 24));

    bool bigendian = (QSysInfo::ByteOrder == QSysInfo::BigEndian);
    if (bigendian)
        r = qbswap(r);
    return r;
}

QCharString Sentences::getByteArrayString(const QByteArray &arr, int &pos)
{
    int siz = getShort(arr, pos);
    QCharString result;
    result.copy(QString::fromUtf8(arr.constData() + pos, siz).constData());
    pos += siz;
    return result;
}


//-------------------------------------------------------------


namespace ZKanji
{
    Sentences sentences;
}
