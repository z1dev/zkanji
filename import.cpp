/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QtEndian>

#include <set>

#include "import.h"
#include "ui_import.h"
#include "treebuilder.h"
#include "words.h"
#include "sentences.h"
#include "kanji.h"
#include "grammar_enums.h"
#include "romajizer.h"
#include "zkanjimain.h"
//#include "zui.h"
#include "zevents.h"
#include "zstrings.h"
#include "groups.h"
#include "jlptreplaceform.h"

extern char ZKANJI_PROGRAM_VERSION[];
static char ZKANJI_EXAMPLES_FILE_VERSION[] = "001";

// When changed: also update exportDictionary() in words.cpp.
static const char JMDictInfoText[] = "This program uses a compilation of the <a href=\"http://www.edrdg.org/jmdict/j_jmdict.html\">JMdict</a> "
"and <a href=\"http://nihongo.monash.edu/kanjidic.html\">KANJIDIC</a> dictionary files "
"and <a href=\"http://users.monash.edu/~jwb/kradinf.html\">RADKFILE</a>, "
"which are the property of The Electronic Dictionary Research and Development Group, Monash University.\n"
"The files are made available under a Creative Commons Attribution-ShareAlike Licence (V3.0).\n"
"The group can be found at: <a href=\"http://www.edrdg.org/\">http://www.edrdg.org/</a>\n"
"\n"
"Additional conditions applying to KANJIDIC:\n"
"The following people have granted permission for material in KANJIDIC, for which they hold copyright "
"to be included in the file while retaining their copyright over that material:\n"
"Jack HALPERN: The SKIP codes. (More information below)\n"
"Christian WITTERN and Koichi YASUOKA: The Pinyin information.\n"
"Urs APP: the Four Corner codes and the Morohashi information.\n"
"Mark SPAHN and Wolfgang HADAMITZKY: the kanji descriptors from their dictionary.\n"
"Charles MULLER: the Korean readings.\n"
"Joseph DE ROO: the De Roo codes.\n"
"\n"
"The SKIP(System of Kanji Indexing by Patterns) system for ordering kanji was developed by Jack Halpern "
"(Kanji Dictionary Publishing Society at <a href=\"http://www.kanji.org/\">http://www.kanji.org/</a>), and is used with his permission. "
"The SKIP coding system and all established SKIP codes have been placed under a Creative Commons Attribution-ShareAlike 4.0 International.";


//-------------------------------------------------------------


ImportFileHandlerGuard::ImportFileHandlerGuard(ImportFileHandler &file) : file(&file)
{
    file.addGuard(this);
}

ImportFileHandlerGuard::~ImportFileHandlerGuard()
{
    if (file != nullptr)
        file->guardClose(this);
}

void ImportFileHandlerGuard::disable()
{
    if (file != nullptr)
        file->disableGuard(this);
    file = nullptr;
}

//-------------------------------------------------------------


ImportFileHandler::ImportFileHandler() : fail(false), f(nullptr), ownfile(true), linenum(0), skipread(false)
{

}

ImportFileHandler::ImportFileHandler(QString fname) : fail(false), f(nullptr), ownfile(true), linenum(0), skipread(false)
{
    open(fname);
}

ImportFileHandler::~ImportFileHandler()
{
    disableGuards();

    if (ownfile)
        close();
    f = nullptr;
    ownfile = true;
}

bool ImportFileHandler::open(QString fname, const char *codec)
{
    disableGuards();

    if (f != nullptr && ownfile)
        close();

    f = new QFile();
    ownfile = true;

    f->setFileName(fname);
    if (!f->exists() || !f->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        delete f;
        fail = true;
        return false;
    }

    stream.setDevice(f);
    if (codec == nullptr)
        stream.setCodec("UTF-8");
    else
        stream.setCodec(codec);

    return true;
}

void ImportFileHandler::setFile(QFile &file, const char *codec)
{
    if (f == &file)
        return;

    disableGuards();

    if (f != nullptr && ownfile)
        close();

    ownfile = false;
    f = &file;

    stream.setDevice(f);
    if (codec == nullptr)
        stream.setCodec("UTF-8");
    else
        stream.setCodec(codec);
}


void ImportFileHandler::close()
{
    disableGuards();

    if (f == nullptr)
        return;

    f->close();
    if (ownfile)
    {
        delete f;
        f = nullptr;
    }

    fail = false;
    linenum = 0;
    skipread = false;
    line = QString();
}

bool ImportFileHandler::isOpen() const
{
    return !fail && f != nullptr && f->isOpen();
}

QString ImportFileHandler::fileName() const
{
    return f == nullptr ? QString() : f->fileName();
}

int ImportFileHandler::lineNumber() const
{
    return linenum;
}

void ImportFileHandler::repeat()
{
    skipread = true;
}

bool ImportFileHandler::error() const
{
    return fail;
}

int ImportFileHandler::size() const
{
    return fail || f == nullptr ? 0 : f->size();
}

int ImportFileHandler::pos() const
{
    return fail || f == nullptr ? -1 : f->pos();
}

bool ImportFileHandler::getLine(QString &result)
{
    if (fail || f == nullptr)
        return false;

    if (skipread)
    {
        result = line;
        skipread = false;
        return true;
    }

    line = stream.readLine();
    if (line.isNull())
    {
        fail = true;
        result = QString();
        return false;
    }

    ++linenum;
    result = line;
    return true;
}

QString ImportFileHandler::lastLine() const
{
    return line;
}

//bool ImportFileHandler::atEnd() const
//{
//    return fail || f == nullptr || (!skipread && stream.atEnd());
//}

void ImportFileHandler::addGuard(ImportFileHandlerGuard *guard)
{
    guards.insert(guard);
}

void ImportFileHandler::disableGuard(ImportFileHandlerGuard *guard)
{
    auto it = guards.find(guard);
    if (it == guards.end())
        return;
    guards.erase(it);
}

void ImportFileHandler::disableGuards()
{
    QSet<ImportFileHandlerGuard*> tmp = guards;
    guards.clear();

    for (ImportFileHandlerGuard* g : tmp)
        g->disable();
}

void ImportFileHandler::guardClose(ImportFileHandlerGuard *guard)
{
    auto it = guards.find(guard);
    if (it == guards.end())
        return;
    guards.erase(it);

    if (guards.empty())
        close();
}



//-------------------------------------------------------------


DictImport::DictImport(QWidget *parent) : base(parent), ui(new Ui::DictImport), modified(false), stepcnt(0), step(1), kcurrent(nullptr), rcurrent(nullptr), scurrent(nullptr),
        /*entryr(0), entrys(0),*/ counter(0)
{
    ui->setupUi(this);

    ui->finishButton->setEnabled(false);
    connect(ui->finishButton, &QPushButton::clicked, this, &DictImport::closeAfterImport);
}

DictImport::~DictImport()
{
    delete ui;
}

Dictionary* DictImport::importDict(QString p, bool full)
{
    mode = Modes::Dictionary;

    stepcnt = full ? 8 : 6;
    path = p;
    //outpath = o;
    //lang = l;
    lang.clear();
    fullimport = full;
    dict = nullptr;

    setMainText(tr("Importing dictionary. This can take several minutes, please wait..."));

    show();
    adjustSize();
    setFixedSize(size());

    qApp->postEvent(this, new StartEvent());
    loop.exec();

    return dict;
}

Dictionary* DictImport::importFromExport(QString p)
{
    mode = Modes::Export;

    stepcnt = 5;
    path = p;
    //outpath = o;
    //lang = l;
    dict = nullptr;

    setMainText(tr("Importing dictionary. This can take several minutes, please wait..."));

    show();
    adjustSize();
    setFixedSize(size());

    qApp->postEvent(this, new StartEvent());
    loop.exec();

    return dict;
}

bool DictImport::importFromExportPartial(QString p, Dictionary *dest, WordGroup *worddest, KanjiGroup *kanjidest)
{
    mode = Modes::Partial;

    stepcnt = 3;
    path = p;
    dict = dest;
    wordgroup = worddest;
    kanjigroup = kanjidest;

    setMainText(tr("Importing partial dictionary. This can take several minutes, please wait..."));

    show();
    adjustSize();
    setFixedSize(size());

    qApp->postEvent(this, new StartEvent());
    loop.exec();

    return step != -1;
}

void DictImport::setMainText(const QString &str)
{
    QString secondary = !modified ? tr("You can stop the import if you close this window. No data will be lost or updated.") : tr("You can stop the import if you close this window. Some data has already been updated, and will remain.");
    ui->progressLabel->setText("<html><head/><body><p><span style=\"font-size:12pt;\">" + QString(str) + "</span></p><p><span style=\"font-size:9pt;\">" + secondary + "</span></p></body></html>");
}

void DictImport::setModifiedText(const QString &str)
{
    modified = true;
    setMainText(str);
}

bool DictImport::importExamples(QString p, QString o, Dictionary *d)
{
    mode = Modes::Examples;

    path = p;
    outpath = o;
    dict = d;

    setMainText(tr("Importing Example sentences. This can take several minutes, please wait..."));

    show();
    adjustSize();
    setFixedSize(size());

    qApp->postEvent(this, new StartEvent());
    loop.exec();

    return step != -1;
}

bool DictImport::importUserData(const QString &p, Dictionary *d, KanjiGroupCategory *kroot, WordGroupCategory *wroot, bool kanjiexamples, bool wordsmeanings)
{
    mode = Modes::User;

    stepcnt = 1;
    path = p;
    kanjiroot = kroot;
    wordsroot = wroot;
    dict = d;
    kanjiex = kanjiexamples;
    studymeanings = wordsmeanings;

    setMainText(tr("Importing Example sentences. This can take several minutes, please wait..."));

    show();
    adjustSize();
    setFixedSize(size());

    qApp->postEvent(this, new StartEvent());
    loop.exec();

    return step != -1;
}

bool DictImport::nextUpdate(int progress, bool forced)
{
    if (!forced && ++counter != 100 && (progress == -1 || progress == ui->progressBar->value() || (progress != ui->progressBar->maximum() && ui->progressBar->maximum() / (progress - ui->progressBar->value()) < 2)))
        return true;
    counter = 0;

    if (progress != -1)
        ui->progressBar->setValue(progress);

    loop.processEvents();
    return loop.isRunning();
}

void DictImport::closeEvent(QCloseEvent *e)
{
    if (loop.isRunning() && !ui->finishButton->isEnabled())
    {
        QString msg = !modified ? tr("Do you want to abort the import?")  : tr("Some data has been modified and it will be kept even if you abort the import.\n\nDo you want to abort?");
        if (QMessageBox::warning(nullptr, "zkanji", msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        {
            e->ignore();
            return;
        }
    }

    e->accept();

    base::closeEvent(e);
    if (!e->isAccepted())
        return;

    // Notifies the event loop that makes this window behave like a dialog.
    loop.quit();
}

bool DictImport::event(QEvent *e)
{
    if (e->type() == StartEvent::Type())
    {
        switch (mode)
        {
        case Modes::Dictionary:

            if (!doImportDict())
            {
                if (fullimport)
                {
                    ZKanji::kanjis.clear();
                    ZKanji::validkanji.clear();
                    ZKanji::radklist.clear();
                    ZKanji::radlist.clear();
                    ZKanji::radkcnt.clear();
                    ZKanji::radlist.clear();
                    ZKanji::commons.clearJLPTData();
                }

                step = -1;
                if (loop.isRunning())
                {
                    ui->finishButton->setText(tr("Abort"));
                    ui->finishButton->setEnabled(true);
                }
            }
            else
            {
                ZKanji::setNoData(false);

                ui->progressLabel->setText(tr("<html><head/><body><p><span style=\" font-size:12pt;\">Import finished.</span></p><p><span style=\" font-size:9pt;\">Press \"%1\" to close the importer and continue starting the program.</span></p></body></html>").arg(tr("Finish")));
                ui->infoEdit->appendPlainText(tr("Dictionary import done."));
                ui->finishButton->setText(tr("Finish"));
                ui->finishButton->setEnabled(true);

            }
            break;
        case Modes::Export:
            if (!doImportFromExport())
            {
                step = -1;
                if (loop.isRunning() && !ui->finishButton->isEnabled())
                {
                    ui->finishButton->setText(tr("Abort"));
                    ui->finishButton->setEnabled(true);
                }
            }
            else
            {
                ui->progressLabel->setText(tr("<html><head/><body><p><span style=\" font-size:12pt;\">Import finished.</span></p><p><span style=\" font-size:9pt;\">Press \"%1\" to close the importer.</span></p></body></html>").arg(tr("Finish")));
                ui->infoEdit->appendPlainText(tr("Dictionary import done."));
                ui->finishButton->setText(tr("Finish"));
                ui->finishButton->setEnabled(true);

            }
            break;
        case Modes::Partial:
            if (!doImportFromExportPartial())
            {
                step = -1;
                if (loop.isRunning() && !ui->finishButton->isEnabled())
                {
                    ui->finishButton->setText(tr("Abort"));
                    ui->finishButton->setEnabled(true);
                }
            }
            else
            {
                ui->progressLabel->setText(tr("<html><head/><body><p><span style=\" font-size:12pt;\">Import finished.</span></p><p><span style=\" font-size:9pt;\">Press \"%1\" to close the importer.</span></p></body></html>").arg(tr("Finish")));
                ui->infoEdit->appendPlainText(tr("Dictionary import done."));
                ui->finishButton->setText(tr("Finish"));
                ui->finishButton->setEnabled(true);
            }
            break;
        case Modes::Examples:
            if (!doImportExamples())
            {
                step = -1;
                if (loop.isRunning() && !ui->finishButton->isEnabled())
                {
                    ui->finishButton->setText(tr("Abort"));
                    ui->finishButton->setEnabled(true);
                }
            }
            else
            {
                ui->progressLabel->setText(tr("<html><head/><body><p><span style=\" font-size:12pt;\">Import finished.</span></p><p><span style=\" font-size:9pt;\">Press \"%1\" to close the importer and continue starting the program.</span></p></body></html>").arg(tr("Finish")));
                ui->infoEdit->appendPlainText(tr("Example database import done."));
                ui->finishButton->setText(tr("Finish"));
                ui->finishButton->setEnabled(true);

            }
            break;
        case Modes::User:
            if (!doImportUserData())
            {
                step = -1;
                if (loop.isRunning() && !ui->finishButton->isEnabled())
                {
                    ui->finishButton->setText(tr("Abort"));
                    ui->finishButton->setEnabled(true);
                }
            }
            else
            {
                ui->progressLabel->setText(tr("<html><head/><body><p><span style=\" font-size:12pt;\">Import finished.</span></p><p><span style=\" font-size:9pt;\">Press \"%1\" to close the importer.</span></p></body></html>").arg(tr("Finish")));
                ui->infoEdit->appendPlainText(tr("User data import done."));
                ui->finishButton->setText(tr("Finish"));
                ui->finishButton->setEnabled(true);
            }

            break;
        }

        return true;
    }
    return base::event(e);
}

void DictImport::closeAfterImport()
{
    setAttribute(Qt::WA_QuitOnClose, false);
    close();
}

bool DictImport::doImportExamples()
{
    // examples.utf file format:
    //
    // # is for comments.
    // There are A and B lines starting with the characters "A:" or "B:". A lines contain the
    // example sentences, Japanese first, followed by the TAB character and the English
    // sentence. At the end of the line the #ID=XXXX_XXXX string represents a unique id
    // (ignored by this import.)
    // B lines paired with the A lines contain the Japanese words found in the previous
    // sentence. Each word can be followed by some data in () or [] or {}. Data in [] is
    // ignored. Text inside () is the kana form of the word where necessary. Data in {} is the
    // form of the word as found in the previous sentence. When a word is followed by the ~
    // character, that means it's a checked and good example sentence.

    // examples.zkj notes:
    //
    // Warning: file versions before 2015 are not supported.
    //
    // The file starts with a short header describing the file version, and the time the file
    // was written. After this a 32 bit integer holding the number of bytes taken up by the
    // example sentences data, then the data. 
    // The example sentences data is written in blocks, each block holds at most 100
    // sentences. Each block of data is compressed separately with zLib via Qt's
    // QByteArray::qCompress. The resulting compressed data is prepended by a 32bit unsigned
    // integer in big-endian order which is the size in bytes of the data uncompressed. (This
    // is done by qCompress.) A sentence in a data block contains the Japanese and the English
    // version, and a list of words in the Japanese sentence. A single word in the sentence is
    // represented by the word's position, length as found in the sentence, paired with one or
    // more kanji and kana. After the sentences blocks is a list of the block positions and
    // their compressed sizes. This is followed by the word data written in the word commons
    // tree. These are compressed the same way.

    if (!setInfoText(tr("Opening examples.utf...")))
        return false;

    if (!file.open(path + "/examples.utf"))
    {
        setErrorText(tr("Couldn't open examples.utf."));
        return false;
    }

    ImportFileHandlerGuard fileguard(file);

    if (!setInfoText(tr("Opened file...")))
        return false;

    QFile of;
    of.setFileName(outpath);
    if (!of.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return false;

    ZKanji::commons.clearExamplesData();

    QDataStream ostream(&of);
    ostream.setVersion(QDataStream::Qt_5_5);
    ostream.setByteOrder(QDataStream::LittleEndian);

    ostream.writeRawData("zex", 3);
    ostream.writeRawData(ZKANJI_EXAMPLES_FILE_VERSION, 3);
    ostream << make_zdate(QDateTime::currentDateTimeUtc());
    ostream << make_zstr(QString::fromLatin1(ZKANJI_PROGRAM_VERSION), ZStrFormat::Byte);

    // Position of the next value saved, so we can seek back here later.
    int startpos = of.pos();

    // Writing empty 4 bytes which will be replaced by the size of blocks to skip. Will write
    // quint32 later.
    ostream.writeRawData("    ", 4);

    // Buffer that collects at most 100 sentences data. Its contents will be compressed and
    // written to file.
    std::vector<uchar> buff;

    qint64 s = file.size();
    ui->progressBar->setMaximum(s);

    QString line;
    bool Aline = true;

    // The Japanese sentence.
    QString jpn;
    // The English sentence.
    QString trans;

    // Index of the current block. Incremented at every 100 sentences.
    ushort blockix = 0;
    // Index of the current sentence in the current block.
    uchar sentenceix = 0;
    // Index of the current word in the current sentence.
    uchar wordix = 0;

    // A list of block positions written to file.
    std::vector<int> blockpos;

    if (!setInfoText(tr("Processing data...")))
        return false;

    while (file.getLine(line))
    {

        if (!nextUpdate(file.pos()))
            return false;
        if (line.isEmpty() || line.at(0) == '#')
            continue;

        int tabpos = -1;

        // Skip errors.
        if (line.size() < 4 || line.at(1) != ':' || (line.at(0) != 'A' && line.at(0) != 'B') || line.at(2) != ' ' ||
            (line.at(0) == 'A' && !Aline) || (line.at(0) == 'B' && Aline) || (Aline && (tabpos = line.indexOf('\t')) < 4))
        {
            // Back to looking for A line because the next B line might not be a good match.
            Aline = true;
            continue;
        }

        line.remove(0, 3);
        if (Aline)
        {
            tabpos -= 3;
            jpn = line.left(tabpos);
            int idpos = line.indexOf("#ID=");
            trans = line.mid(tabpos + 1, idpos - tabpos - 1);

            if (!jpn.isEmpty() && !trans.isEmpty())
                Aline = false;
            continue;
        }

        Aline = true;

        int jpnpos = 0;
        int jpnsiz = jpn.size();

        QCharTokenizer tokens(line.constData(), line.size(), qcharisspace);

        std::vector<ExampleWordsData> words;

        while (tokens.next() && jpnpos < jpnsiz)
        {
            // Check every word in the line for an equivalent in the Japanese sentence.
            const QChar *tok = tokens.token();
            int tsiz = tokens.tokenSize();

            int wsiz = -1;

            // The length of the actual word might be less than tsiz, when the extra data is
            // removed.
            for (int ix = 0; ix != tsiz && wsiz == -1; ++ix)
                if (tok[ix] == '[' || tok[ix] == '{' || tok[ix] == '~' || tok[ix] == '(')
                    wsiz = ix;
            if (wsiz == -1)
                wsiz = tsiz;

            // Current word.
            ushort wordpos = 0;
            ushort wordlen = 0;
            std::vector<ExampleWordsData::Form> wordforms;

            // The written form of the word as specified.
            const QChar *kanjiform = nullptr;
            // The kana form of the word if specified or if the word consist
            // only of kana.
            const QChar *kanaform = nullptr;
            // The form of the word as found in the example sentence.
            const QChar *exform = nullptr;
            int kanjisiz = 0;
            int kanasiz = 0;
            int exsiz = 0;
            kanjiform = tok;
            kanjisiz = wsiz;

            // Position of the word in the example sentence.
            int expos = -1;

            // Look for kanji in the word. If not found, the kana and kanji
            // form is the same. If found and there's a kana representation
            // between (), that's the hiragana form.
            bool haskanji = false;
            for (int ix = 0; ix != kanjisiz && !haskanji; ++ix)
                haskanji = !KANA(kanjiform[ix].unicode());

            if (!haskanji)
            {
                kanaform = kanjiform;
                kanasiz = kanjisiz;
            }

            // Kana representation and form in the example sentence when specified.
            for (int ix = wsiz; ix != tsiz && (kanaform == nullptr || exform == nullptr); ++ix)
            {
                if (tok[ix] == '(' || tok[ix] == '{')
                {
                    for (int iy = ix + 1; iy != tsiz; ++iy)
                    {
                        if (tok[iy] == ')' && tok[ix] == '(')
                        {
                            if (kanaform == nullptr)
                            {
                                kanaform = tok + ix + 1;
                                kanasiz = iy - (ix + 1);
                            }
                            ix = iy;
                            break;
                        }
                        if (tok[iy] == '}' && tok[ix] == '{')
                        {
                            if (exform == nullptr)
                            {
                                exform = tok + ix + 1;
                                exsiz = iy - (ix + 1);
                            }
                            ix = iy;
                            break;
                        }
                    }
                }
            } // For-loop end of kana representation or example form.

            if (exform == nullptr)
            {
                exform = kanjiform;
                exsiz = kanjisiz;
            }
            wordlen = exsiz;

            const QChar *jpndat = jpn.constData();
            // Find the position of the word in the Japanese sentence. Note: Using < instead
            // of != because ix might be over the size.
            for (int ix = jpnpos; ix < jpnsiz - exsiz + 1 && expos == -1; ++ix)
            {
                if (qcharncmp(exform, jpndat + ix, exsiz) == 0)
                {
                    expos = ix;
                    wordpos = expos;
                    jpnpos = expos + exsiz;
                }
            }

            if (expos == -1)
                continue;

            // Found everything needed from the sentences. Look in the dictionary for matching
            // words. Words are matching if:
            // - Both kanji and kana form are specified. A word is written even if nothing is
            // found in the dictionary.
            // - Both kanji and kana form are specified, the matching word has a different
            // kanji form but the kana form is the same, and the word definition exactly
            // matches the specified word.
            // - Only kanji form is found. Every word matches that has the same kanji.

            // Look for a word entry in the dictionary having the same kanji and kana.

            std::vector<int> wordsfound;
            if (kanaform != nullptr)
            {
                int wix = dict->findKanjiKanaWord(kanjiform, kanaform, nullptr, kanjisiz, kanasiz, -1);
                if (wix != -1)
                {
                    WordEntry *e = dict->wordEntry(wix);
                    dict->findKanaWords(wordsfound, QString(kanaform, kanasiz), 0, true, nullptr, nullptr);
                    // Only use words that have the exact same definition that e has.
                    for (int ix = wordsfound.size() - 1; ix != -1; --ix)
                    {
                        WordEntry *we = dict->wordEntry(wordsfound[ix]);
                        if (!definitionsMatch(e, we))
                            wordsfound.erase(wordsfound.begin() + ix);
                    }
                }
            }
            else
            {
                dict->findKanjiWords(wordsfound, QString(kanjiform, kanjisiz), 0, true, nullptr, nullptr);

                // No words found and no kana form is specified.
                if (wordsfound.empty())
                    continue;
            }

            QCharString kanjidat;
            QCharString kanadat;

            if (wordsfound.empty())
            {
                kanjidat.copy(kanjiform, kanjisiz);
                kanadat.copy(kanaform, kanasiz);

                if (ZKanji::commons.addExample(kanjidat.data(), kanadat.data(), { blockix, sentenceix, wordix }) != -1)
                    wordforms.push_back({ std::move(kanjidat), std::move(kanadat) });

            }
            else
            {
                for (int ix = 0; ix != wordsfound.size(); ++ix)
                {
                    WordEntry *we = dict->wordEntry(wordsfound[ix]);
                    kanjidat = we->kanji;
                    kanadat = we->kana;

                    if (ZKanji::commons.addExample(kanjidat.data(), kanadat.data(), { blockix, sentenceix, wordix }) != -1)
                        wordforms.push_back({ std::move(kanjidat), std::move(kanadat) });
                }
            }

            // Unlikely but if no word data were added to the commons, skip the word.
            // Otherwise add it.
            if (!wordforms.empty() && wordix < UCHAR_MAX)
            {
                ExampleWordsData wdata;
                wdata.pos = wordpos;
                wdata.len = wordlen;
                wdata.forms.resize(wordforms.size());
                for (int ix = 0; ix != wordforms.size(); ++ix)
                    wdata.forms[ix] = std::move(wordforms[ix]);
                words.push_back(std::move(wdata));
                ++wordix;
            }
        }

        wordix = 0;

        // At most 255 words are supported.
        if (words.empty() || words.size() > 255)
            continue;

        doImportExamplesSentenceHelper(buff, jpn, trans, words);
        words.clear();

        ++sentenceix;

        if (sentenceix == 100)
        {
            // Compress the buffer and empty it.
            QByteArray data = qCompress(buff.data(), buff.size());

            blockpos.push_back(of.pos());
            ostream.writeRawData(data.constData(), data.size());

            buff.clear();

            sentenceix = 0;
            wordix = 0;

			if (blockix == USHRT_MAX)
				throw ZException("More than 6 million sentences in the example database.");

            ++blockix;
        }
    }

    file.close();

    // There's remaining data to write. Do the same as above.
    if (sentenceix != 0)
    {
        // Compress the buffer and empty it.
        QByteArray data = qCompress(buff.data(), buff.size());

        blockpos.push_back(of.pos());
        ostream.writeRawData(data.constData(), data.size());
    }
    buff.clear();

    quint32 filepos = of.pos();
    // Seek back to the front of the file to write the position where the
    // data structure will be saved.
    // File starts with 6 byte id and a quint64 sized date.
    of.seek(startpos);
    ostream << filepos;

    // Continue with the file at the end.
    of.seek(filepos);

    // Write the data block structure.
    int bpos = 0;
    buff.resize(buff.size() + 4 + 4 * blockpos.size());
    addInt(buff.data(), bpos, blockpos.size());
    for (int ix = 0; ix != blockpos.size(); ++ix)
        addInt(buff.data(), bpos, blockpos[ix]);
    //ostream << make_zvec<qint32, qint32>(blockpos);

    // Write anything from the word commons list which has example data.

    const smartvector<WordCommons> &commons = ZKanji::commons.getItems();
    for (int ix = 0; ix != commons.size(); ++ix)
    {
        const WordCommons *item = commons[ix];
        if (item->examples.empty())
            continue;

        QByteArray kanjiarr = item->kanji.toUtf8();
        QByteArray kanaarr = item->kana.toUtf8();
        buff.resize(buff.size() + 2 + kanjiarr.size() + 2 + kanaarr.size() + 2 + item->examples.size() * 4);
        addByteArrayString(buff.data(), bpos, kanjiarr);
    //    ostream << make_zstr(item->kanji, ZStrFormat::Byte);
        addByteArrayString(buff.data(), bpos, kanaarr);
        //    ostream << make_zstr(item->kana, ZStrFormat::Byte);
        addShort(buff.data(), bpos, item->examples.size());
        //    ostream << (qint32)item->examples.size();

        for (int iy = 0; iy != item->examples.size(); ++iy)
        {
            const WordCommonsExample &ex = item->examples[iy];
            addShort(buff.data(), bpos, ex.block);
    //        ostream << (quint16)ex.block;
            buff[bpos++] = ex.line;
    //        ostream << (quint8)ex.line;
            buff[bpos++] = ex.wordindex;
    //        ostream << (quint8)ex.wordindex;
        }
    }

    QByteArray data = qCompress(buff.data(), buff.size());
    ostream << (qint32)data.size();
    ostream.writeRawData(data.constData(), data.size());

    //ostream << (qint32)0;

    ostream << (quint32)(of.pos() + 4);

    return true;
}

void DictImport::doImportExamplesSentenceHelper(std::vector<uchar> &buff, const QString &jpn, const QString &trans, const std::vector<ExampleWordsData> &words)
{
    QByteArray jpnstr = jpn.toUtf8();
    QByteArray transstr = trans.toUtf8();

    // Every word's kanji and kana converted to utf8 for writing.
    std::vector<QByteArray> utf8data;
    int utf8datapos = 0;
    // First determine the number of bytes to be written in buff.
    int writesize = 2 + jpnstr.size() + 2 + transstr.size() + 2;
    for (int ix = 0; ix != words.size(); ++ix)
    {
        const ExampleWordsData &wd = words[ix];
        writesize += 2 + 2 + 2;
        for (int iy = 0; iy != wd.forms.size(); ++iy)
        {
            auto &p = wd.forms[iy];
            QByteArray arr = p.kanji.toQStringRaw().toUtf8();
            utf8data.push_back(arr);
            writesize += 2 + arr.size();
            arr = p.kana.toQStringRaw().toUtf8();
            utf8data.push_back(arr);
            writesize += 2 + arr.size();
        }
    }

    int pos = buff.size();
    buff.resize(buff.size() + writesize);
    uchar *dat = buff.data();

    // Write the Japanese and English sentence with their size first in Little-Endian byte order.
    addByteArrayString(dat, pos, jpnstr);
    addByteArrayString(dat, pos, transstr);

    // Write the words data, starting with the number of items in words.
    addShort(dat, pos, words.size());

    for (int ix = 0; ix != words.size(); ++ix)
    {
        const ExampleWordsData &wd = words[ix];
        addShort(dat, pos, wd.pos);
        addShort(dat, pos, wd.len);

        addShort(dat, pos, wd.forms.size());
        for (int iy = 0; iy != wd.forms.size(); ++iy, utf8datapos += 2)
        {
            auto &p = wd.forms[iy];
            addByteArrayString(dat, pos, utf8data[utf8datapos]);
            addByteArrayString(dat, pos, utf8data[utf8datapos + 1]);
        }
    }
}

void DictImport::addShort(uchar *dat, int &pos, quint16 w)
{
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        w = qbswap(w);
    dat[pos] = (0xff & w);
    dat[pos + 1] = (0xff00 & w) >> 8;
    pos += 2;
}

void DictImport::addInt(uchar *dat, int &pos, quint32 i)
{
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        i = qbswap(i);
    dat[pos] = (0xff & i);
    dat[pos + 1] = (0xff00 & i) >> 8;
    dat[pos + 2] = (0xff0000 & i) >> 16;
    dat[pos + 3] = (0xff000000 & i) >> 24;
    pos += 4;
}

void DictImport::addByteArrayString(uchar *dat, int &pos, const QByteArray &str)
{
    quint16 siz = str.size();
    addShort(dat, pos, siz);
    memcpy(dat + pos, str.constData(), siz);
    pos += siz;
}

bool DictImport::doImportDict()
{
    if (fullimport)
    {
        if (!QFileInfo::exists(path + "/JLPTNData.txt") || !QFileInfo::exists(path + "/JMdict") || !QFileInfo::exists(path + "/kanjidic") || !QFileInfo::exists(path + "/kanjiorder.txt") ||
            !QFileInfo::exists(path + "/radkelement.txt") || !QFileInfo::exists(path + "/radkfile") || !QFileInfo::exists(path + "/zradfile.txt"))
        {
            setErrorText("Required file missing to complete full dictionary import. Make sure you have each of the following:\n\n"
                "JLPTNData.txt\n" "JMdict\n" "kanjidic\n" "kanjiorder.txt\n" "radkelement.txt\n" "radkfile\n" "zradfile.txt");
            return false;
        }

        if (!importKanjidic())
            return false;

        ZKanji::generateValidKanji();

        if (!importRadFiles())
            return false;
    }

    std::unique_ptr<Dictionary> dp;
    dp.reset(importJMdict());

    if (!dp)
        return false;

    dp->setName("English");
    //ZKanji::addImportDictionary(dp.get());

    if (fullimport && !importJLPTN(dp.get()))
    {
        //ZKanji::removeImportDictionary(dp.get());
        return false;
    }

    //ZKanji::removeImportDictionary(dp.get());


    //if (lang.isEmpty())
    dp->setProgramVersion();
    dp->setInfoText(JMDictInfoText);

    //else
    //{
    //    bool ok = false;
    //    while (!ok)
    //    {
    //        QString answer = QInputDialog::getText(this, tr("zkanji importer"), tr("Enter a name for the imported dictionary:"), QLineEdit::Normal, QString(), &ok);
    //        if (!ok)
    //            return false;
    //        if (answer.contains(QChar('/')) || answer.contains(QChar('\\')) || answer.contains(QChar('*')) ||
    //            answer.contains(QChar('.')) || answer.contains(QChar('?')) || answer.contains(QChar('+')) ||
    //            answer.contains(QChar(':')) || answer.contains(QChar('|')) || answer.contains(QChar('"')) ||
    //            answer.contains(QChar('<')) || answer.contains(QChar('>')))
    //        {
    //            ok = false;
    //            QMessageBox::warning(this, tr("zkanji importer"), tr("The dictionary name cannot contain the following characters:\n/ \\ * . ? + : \" < > |"), QMessageBox::Ok);
    //        }
    //        else
    //            dp->setName(answer);
    //    }
    //}

    if (fullimport && !setInfoText(tr("%1/%2 - Saving data and cleaning up...").arg(step).arg(stepcnt)))
        return false;

    dict = dp.release();

    return true;
}

bool DictImport::letterAndNumber(const QString &str, QChar &ch, QChar &ch2, int &num, int base)
{
    if (str.size() < 2)
        return false;
    if (str.at(0).unicode() > 'Z' || str.at(0).unicode() < 'A')
        return false;
    ch = str.at(0);
    int start = 1;

    if (str.at(1).unicode() >= 'A' && str.at(1).unicode() <= 'Z')
    {
        start = 2;
        if (str.size() < 3)
            return false;
        ch2 = str.at(1);
    }
    else
        ch2 = QChar(0);

    bool ok = false;
    num = str.rightRef(str.size() - start).toInt(&ok, base);
    return ok;
}

bool DictImport::importKanjidic()
{
    // Instead of kanjidic2, we only use kanjidic which is in an oldschool 1 line for 1 kanji
    // format. It's much easier and faster to work with than the fancy XML. It's clear there
    // were new features planned for kanjidic2, but in the end they never got to it. Thus
    // kanjidic contains everything we will need.

    // The parts are either info fields that start with 1-2 alphabet letters, hold meanings
    // between {} or they are katakana/hiragana (with some extra characters.)
    // Katakana are generally for ON reading, hiragana for KUN reading and they might start or
    // end with - and have . in the middle for okurigana.

    // Valid characters for the info fields: (characers that zkanji won't recognize are not
    // listed.)
    // B - radical from classical Nelson
    // C - classical radical
    // F - frequency of use for the top 2501 kanji
    // G - school grade between 1-6 or 8 for secondary school, 9 for name kanji, 10 for kanji
    //     variants used in names
    // J - old JLPT (this will be overwritten with the self-made grade guesses)
    // H - Halpern J-E dict
    // N - Nelson J-E dict
    // V - new Nelson
    // D - Index numbers in many different books. The second character tells which.
    // P - SKIP code
    // S - stroke count (might not be used)
    // I - has 2 forms from books by SnH
    // M - MN or MP from Morohashi Daikanwajiten
    // E - Remembering Japanese Characters (Henshall)
    // K - Gakken kanji dic
    // L - Heisig Remembering the Kanji
    // O - Japanese Names index (some can end in the letter A)
    // X - Only the XN numbers are used by zkanji for kanji that don't have an N index. They
    //     will be put in the Nelson index

    // In the middle of the readings there can be a T1 code, after it there can be a list of
    // only-in-names readings and T2 if the kanji is a radical and after that a radical's name
    // in kana.

    if (!setInfoText(tr("Opening kanjidic...")))
        return false;

    if (!file.open(path + "/kanjidic", "EUC-JP"))
    {
        setErrorText(tr("Couldn't open file for reading."));
        return false;
    }

    ImportFileHandlerGuard fileguard(file);

    if (!setInfoText(tr("Opened file, reading...")))
        return false;

    qint64 s = file.size();
    ui->progressBar->setMaximum(s);

    QFile orderfile(path + "/kanjiorder.txt");
    if (!orderfile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        setErrorText(tr("Couldn't find kanjiorder.txt file for kanji ordering."));
        return false;
    }
    QTextStream orderstream(&orderfile);
    orderstream.setCodec("UTF-8");
    QString kanjiorder = orderstream.readAll().trimmed();
    if (kanjiorder.size() != ZKanji::kanjicount)
    {
        setErrorText(tr("Couldn't read kanjiorder.txt or the file is corrupted."));
        return false;
    }

    if (!setInfoText(tr("Processing Kanjidic...")))
        return false;

    QString str;
    while (file.getLine(str))
    {
        if (!nextUpdate(file.pos()))
            return false;
        if (str.isEmpty() || str.at(0) == '#')
            continue;

        //0x4e00 <= int(t) && int(t) <= 0x9faf
        QStringList parts = str.split(' ', QString::SkipEmptyParts);

        // Not a real error checking so the file might still be corrupt. First string should
        // be a kanji, the second should be a 4 character representation of the JIS0208 in
        // readable hexa. The characters must be ordered the same way as previous data files
        // were, but there's no way to check this.
        if (parts.size() < 5 || parts.at(0).size() != 1 || parts.at(0).at(0).unicode() < 0x4e00 || parts.at(0).at(0).unicode() > 0x9faf)
            continue;

        int kindex = kanjiorder.indexOf(str.at(0));
        if (kindex == -1)
            continue;

        bool ok;
        int val = parts.at(1).toInt(&ok, 16);
        if (!ok)
            continue;

        KanjiEntry *k = ZKanji::addKanji(str.at(0), kindex);
        k->jis = val;

        enum class Modes { Normal, NameKana, RadicalName, Meaning };
        Modes mode = Modes::Normal;

        QStringList onr;
        QStringList kunr;
        QStringList namer;

        QString mstr;
        QStringList meanings;

        for (int pos = 2; pos != parts.size(); ++pos)
        {
            if (mode != Modes::Meaning)
                mstr.clear();

            QChar ch, ch2;
            int num;
            if (mode != Modes::Meaning && letterAndNumber(parts.at(pos), ch, ch2, num))
            {
                if (ch == 'T' && ch2 == 0)
                {
                    // T code of kana type found.
                    if (num == 1)
                        mode = Modes::NameKana;
                    else if (num == 2)
                        mode = Modes::RadicalName;
                }
                else if (ch == 'B' && ch2 == 0 && num > 0 && num < 215)
                    k->rad = num;
                else if (ch == 'C' && ch2 == 0 && num > 0 && num < 215)
                    k->crad = num;
                else if (ch == 'F' && ch2 == 0 && num > 0 && num < 2502)
                    k->frequency = num;
                else if (ch == 'G' && ch2 == 0 && num > 0 && num < 11 && num != 7)
                    k->jouyou = num;
                else if (ch == 'J' && ch2 == 0 && num > 0 && num < 5)
                    k->jlpt = num;
                else if (ch == 'H' && ch2 == 0 && num > 0 && num < 9999) // In reality 3574 was the highest I found at the time I was writing this.
                    k->halpern = num;
                else if (ch == 'N' && ch2 == 0 && num > 0 && num < 9999)
                    k->nelson = num;
                else if (ch == 'V' && ch2 == 0 && num > 0 && num < 9999)
                    k->newnelson = num;
                else if (ch == 'S' && ch2 == 0 && num > 0 && num < 9999)
                {
                    // There can be multiple stroke counts, the latter ones are miscounts.
                    if (k->strokes == 0)
                        k->strokes = num;
                }
                else if (ch == 'E' && ch2 == 0 && num > 0 && num < 9999)
                    k->henshall = num;
                else if (ch == 'K' && ch2 == 0 && num > 0 && num < 9999)
                    k->gakken = num;
                else if (ch == 'L' && ch2 == 0 && num > 0 && num < 9999)
                    k->heisig = num;
                else if (ch == 'X' && ch2 == 'N' && num > 0 && num < 9999 && k->nelson == 0)
                    k->nelson = num;
                else if (ch == 'D' && ch2 != 0)
                {
                    // DB - DBx.yy or DB1.A Not found by letterAndNumber
                    if (ch2 == 'C' && num > 0 && num < 9999)
                        k->crowley = num;
                    else if (ch2 == 'F' && num > 0 && num < 9999)
                        k->flashc = num;
                    else if (ch2 == 'G' && num > 0 && num < 9999)
                        k->kguide = num;
                    else if (ch2 == 'H' && num > 0 && num < 9999)
                        k->henshallg = num;
                    else if (ch2 == 'J' && num > 0 && num < 9999)
                        k->context = num;
                    else if (ch2 == 'K' && num > 0 && num < 9999)
                        k->halpernk = num;
                    else if (ch2 == 'L' && num > 0 && num < 9999)
                        k->halpernl = num;
                    else if (ch2 == 'M' && num > 0 && num < 9999)
                        k->heisigf = num;
                    else if (ch2 == 'N' && num > 0 && num < 9999)
                        k->heisign = num;
                    else if (ch2 == 'O' && num > 0 && num < 9999)
                        k->oneil = num;
                    else if (ch2 == 'P' && num > 0 && num < 9999)
                        k->halpernn = num;
                    else if (ch2 == 'R' && num > 0 && num < 9999)
                        k->deroo = num;
                    else if (ch2 == 'S' && num > 0 && num < 9999)
                        k->sakade = num;
                    else if (ch2 == 'T' && num > 0 && num < 9999)
                        k->tuttle = num;

                }
                else if (ch == 'I' && ch2 == 'N' && num > 0 && num < 9999)
                    k->knk = num;
            }
            else if (parts.at(pos).at(0) == QChar('{') || mode == Modes::Meaning)
            {
                // Processing meaning strings.

                if (mstr.isEmpty())
                    mstr = parts.at(pos);
                else
                    mstr += QChar(' ') + parts.at(pos);

                if (*std::prev(parts.at(pos).end()) == QChar('}'))
                {
                    mode = Modes::Normal;
                    meanings.push_back(mstr.mid(1, mstr.size() - 2));
                }
                else
                    mode = Modes::Meaning;
            }
            else if (KANA(parts.at(pos).at(parts.at(pos).at(0) == QChar('-') && parts.at(pos).size() > 1 ? 1 : 0).unicode()))
            {
                // Readings.

                // Radical names are ignored because the zradfile has everything.
                if (mode == Modes::RadicalName)
                    continue;

                bool error = false;
                bool onreading = KATAKANA(parts.at(pos).at(0).unicode());
                bool dotfound = false;

                // Check if the reading is correct. Incorrect readings will be ignored.
                for (auto it = parts.at(pos).begin() + 1; it != parts.at(pos).end(); ++it)
                {
                    if (!KANA(it->unicode()) && it->unicode() != KDASH)
                    {
                        if (*it == QChar('.'))
                        {
                            if (dotfound || onreading)
                                error = true;
                            dotfound = true;
                        }
                        else if (*it == QChar('-') && it != std::prev(parts.at(pos).end()))
                            error = true;
                    }
                    else if (onreading != (it->unicode() == KDASH || KATAKANA(it->unicode())))
                        error = true;
                }

                if (error)
                    continue;

                if (onreading)
                {
                    // Name kana should be hiragana only.
                    if (mode == Modes::NameKana)
                        continue;
                    if (onr.indexOf(parts.at(pos)) == -1)
                        onr.push_back(parts.at(pos));
                }
                else if (mode == Modes::NameKana)
                {
                    if (dotfound || parts.at(pos).at(0) == QChar('-') || *std::prev(parts.at(pos).end()) == QChar('-'))
                        continue;
                    if (namer.indexOf(parts.at(pos)) == -1)
                        namer.push_back(parts.at(pos));
                }
                else
                {
                    int start = 0;
                    int len = parts.at(pos).size();
                    if (parts.at(pos).at(0) == QChar('-'))
                        ++start, --len;
                    if (*std::prev(parts.at(pos).end()) == QChar('-'))
                        --len;
                    str = parts.at(pos).mid(start, len);

                    if (kunr.indexOf(str) == -1)
                        kunr.push_back(str);
                }

                // Readings End
            }
            else
            {
                // SKIP code in the form of Pn-n-n where n is a number.
                if (parts.at(pos).at(0) == 'P')
                {
                    QStringList skips = parts.at(pos).right(parts.at(pos).size() - 1).split('-');
                    if (skips.size() != 3)
                        continue;

                    bool ok = true;
                    k->skips[0] = skips.at(0).toInt(&ok);
                    if (k->skips[0] >= 1 && k->skips[0] <= 4)
                        for (int ix = 1; ok && ix != 3; ++ix)
                            k->skips[ix] = skips.at(ix).toInt(&ok);

                    if (!ok)
                        memset(k->skips, 0, 3 * sizeof(uchar));
                }
                else if (parts.at(pos).at(0) == 'I' && parts.at(pos).size() < 10 && parts.at(pos).size() > 5)
                {
                    // SnH index in the form nxnn.n with at most 8 characters.
                    memcpy(k->snh, parts.at(pos).toLatin1().constData() + 1, sizeof(char) * parts.at(pos).size() - 1);
                    if (parts.at(pos).size() < 9)
                        k->snh[parts.at(pos).size() - 2] = 0;
                }
                else if (parts.at(pos).size() >= 5 && parts.at(pos).size() <= 6 && parts.at(pos).at(0) == 'D' && parts.at(pos).at(1) == 'B')
                {
                    memcpy(k->busy, parts.at(pos).toLatin1().constData() + 2, sizeof(char) * parts.at(pos).size() - 2);
                    if (parts.at(pos).size() == 5)
                        k->busy[3] = 0;
                }
            }
        }

        k->on.copy(onr);

        std::sort(kunr.begin(), kunr.end(), [](const QString &a, const QString &b) {
            int adot = a.indexOf('.');
            int bdot = b.indexOf('.');

            int alen = adot == -1 ? a.size() : adot;
            int blen = bdot == -1 ? b.size() : bdot;
            int cmp = qcharncmp(a.constData(), b.constData(), std::min(alen, blen));
            if (cmp != 0)
                return cmp < 0;
            if (alen != blen)
                return alen < blen;
            if (a.size() != b.size())
                return a.size() < b.size();

            return qcharncmp(a.constData(), b.constData(), a.size()) < 0;

        });

        k->kun.copy(kunr);
        k->nam.copy(namer);
        k->meanings.copy(meanings);
    }

    if (ZKanji::isKanjiMissing())
    {
        setErrorText(tr("The kanjidic file is missing kanji which compromises the integrity of the zkanji user data."));
        return false;
    }

    return true;
}

Dictionary* DictImport::importJMdict()
{
    // JMdict is in a large XML format which takes a long time to properly process. Using a
    // class for XML reading would work but it'd be too slow. We use the fact that each field
    // in JMDict is on a new line and in a given order, making the reading and processing much
    // faster.


    // Tags recognized by the import:
    // entry - Each entry corresponds to a Japanese word with many glosses. The entries might
    //      contain alternative writing forms. Those are all added as separate words to the
    //      zkanji dictionary. Every other tag is inside <entry> tags.
    // k_ele / keb - Kanji written form of word. Multiple forms can be present.
    // k_ele / ke_inf - Information about the kanji form. I.e. irregular.
    // k_ele / ke_pri - Popularity of the word with this written form. See the documentation
    //      inside the XML for more information.
    // r_ele / reb - Kana reading of a word. If the <keb> is not present, this is used for the
    //      written form as well.
    // ** r_ele / re_nokanji - In the form of <re_nokanji/>. The reading form is not true
    //      reading of kanji. Probably won't be used, but it's noted here just in case.
    // r_ele / re_restr - The reading is only valid for the kanji form listed here. Multiple
    //      of these can be for a single <r_ele>
    // r_ele / re_inf - Information about the written form. I.e. irregular.
    // r_ele / re_pri - same as <ke_pri> for written forms.
    // sense - After the kanji and kana forms, <sense> contains definitions of the word in
    //      other languages. If more senses are listed, each one is a different meaning.
    //      Glosses under a single sense are different translations of the same Japanese
    //      meaning.
    // sense / stagk - The sense is only valid for the given written form.
    // sense / stagr - The sense is only valid for the given reading.
    // sense / pos - Part Of Speech information. If not provided, uses the same information
    //      given to a previous sense of the same word.
    // sense / field - Field usage of word.
    // sense / misc - Other information for meaning. When not provided uses one specified for
    //      a previous sense of the same word.
    // sense / dial - Dialect where the form is used.
    // sense / gloss - Single meaning or definition. If more are listed in a single sense,
    //      they are different translations of the same sense. If the gloss is in the form
    //      <gloss xml:lang="***">, the word is a translation in the given language.


    if (!setInfoText(tr("Opening JMdict...")))
        return nullptr;


    // Only a path is provided, not the filename. Look for the file named JMdict, or JMdict_e.
    if (!file.open(path + "/JMdict") && (!lang.isEmpty() || !file.open(path + "/JMdict_e")))
    {
        setErrorText(tr("Couldn't open JMdict."));
        return nullptr;
    }

    ImportFileHandlerGuard fileguard(file);

    qint64 s = file.size();
    ui->progressBar->setMaximum(s);

    QString str;

    bool linefound = false;
    // Skip till the start of data.
    while (file.getLine(str))
    {
        if (!nextUpdate(file.pos()))
            return nullptr;

        if (str == "<JMdict>")
        {
            linefound = true;
            break;
        }
    }

    if (!linefound)
    {
        setErrorText(tr("Premature end of file."));
        return nullptr;
    }

    // Data import start.

    if (!setInfoText(tr("%1/%2 - Processing JMdict...").arg(step).arg(stepcnt)))
        return nullptr;
    ++step;

    // Current line.

    // Inside kanji element.
    bool kele = false;
    // Inside reading element.
    bool rele = false;
    // Inside sense element.
    bool sense = false;

    // Skipping word because of an error.
    bool skip = false;

#ifdef COLLECT_WORD_DATA
    // Word data in JMdict from 2015.08.26:
    // Types 84: adj-f adj-i adj-ix adj-ku adj-na adj-nari adj-no adj-pn
    // adj-shiku adj-t adv adv-to aux aux-adj aux-v conj cop-da ctr exp int n
    // n-adv n-pr n-pref n-suf n-t num pn pref prt suf unc v1 v1-s v2a-s v2b-k
    // v2d-s v2g-k v2g-s v2h-k v2h-s v2k-k v2k-s v2m-s v2n-s v2r-k v2r-s v2s-s
    // v2t-k v2t-s v2w-s v2y-k v2y-s v2z-s v4b v4h v4k v4m v4r v4s v4t v5aru
    // v5b v5g v5k v5k-s v5m v5n v5r v5r-i v5s v5t v5u v5u-s vi vk vn vr vs
    // vs-c vs-i vs-s vt vz 

    // Fields 30: Buddh MA Shinto anat archit astron baseb biol bot bus chem
    // comp econ engr finc food geol geom law ling mahj math med mil music
    // physics shogi sports sumo zool 

    // Notes 32 : abbr arch ateji chn col derog fam fem gikun hon hum iK id ik
    // io joc m-sl male oK obs obsc ok on-mim poet pol proverb rare sens
    // sl uk vulg yoji

    std::set<QString> fields;
    std::set<QString> types;
    std::set<QString> notes;
#endif

    while (file.getLine(str))
    {
        if (str != "<entry>")
        {
            if (!nextUpdate(file.pos()))
                return nullptr;
            continue;
        }

#ifdef COLLECT_WORD_DATA
        if (str.startsWith("<field>") && str.endsWith("</field>"))
            fields.insert(str.mid(8, str.size() - 8 - 9));
        else if (str.startsWith("<ke_inf>") && str.endsWith("</ke_inf>"))
            notes.insert(str.mid(9, str.size() - 9 - 10));
        else if (str.startsWith("<re_inf>") && str.endsWith("</re_inf>"))
            notes.insert(str.mid(9, str.size() - 9 - 10));
        else if (str.startsWith("<misc>") && str.endsWith("</misc>"))
            notes.insert(str.mid(7, str.size() - 7 - 8));
        else if (str.startsWith("<pos>") && str.endsWith("</pos>"))
            types.insert(str.mid(6, str.size() - 6 - 7));
#endif

        if (skip == true)
            skip = false;
        newEntry();
        skip = false;

        kele = false;
        rele = false;
        sense = false;

        linefound = false;
        while (!skip && file.getLine(str))
        {
            // Inside an entry. Look for the possible kanji and kana pairs.
            if (str != "</entry>" && str != "<k_ele>" && str != "<r_ele>" && str != "<sense>")
            {
                if (!nextUpdate(file.pos()))
                    return nullptr;
                continue;
            }

            if (str == "</entry>")
            {
                linefound = true;
                saveEntry();
                break;
            }

            kele = (str == "<k_ele>");
            // Writing tag is not valid after a reading or a sense part.
            if (kele && (rele || sense))
                skip = true;
            rele = (str == "<r_ele>");
            // Reading tag is not valid after a sense part.
            if (rele && sense)
                skip = true;
            sense = (str == "<sense>");

            if (!skip && kele)
                skip = !newKElement();
            if (!skip && rele)
                skip = !newRElement();
            if (!skip && sense)
                skip = !newSElement();

            while (kele && !skip && file.getLine(str))
            {
                if (!nextUpdate(file.pos()))
                    return nullptr;

                if (str == "</k_ele>")
                {
                    saveKElement();
                    break;
                }
                else if (str.startsWith("<keb>"))
                    skip = !addKeb(str);
                else if (str.startsWith("<ke_inf>&"))
                    skip = !addKInf(str);
                else if (str.startsWith("<ke_pri>"))
                    skip = !addKPri(str);
                // Possible error in file format, skip the whole word.
                else if (!str.startsWith("<ke") && !str.startsWith("</ke"))
                    skip = true;
            }

            while (rele && !skip && file.getLine(str))
            {
                if (!nextUpdate(file.pos()))
                    return nullptr;

                if (str == "</r_ele>")
                {
                    saveRElement();
                    break;
                }
                else if (str.startsWith("<reb>"))
                    skip = !addReb(str);
                else if (str.startsWith("<re_restr"))
                    skip = !addRRestr(str);
                else if (str.startsWith("<re_inf>&"))
                    skip = !addRInf(str);
                else if (str.startsWith("<re_pri>"))
                    skip = !addRPri(str);
                // Possible error in file format, skip the whole word.
                else if (!str.startsWith("<re") && !str.startsWith("</re"))
                    skip = true;
            }

            while (sense && !skip && file.getLine(str))
            {
                if (!nextUpdate(file.pos()))
                    return nullptr;
                if (str == "</sense>")
                {
                    saveSElement();
                    break;
                }
                else if (str.startsWith("<stagk>"))
                    skip = !addSTagK(str);
                else if (str.startsWith("<stagr>"))
                    skip = !addSTagR(str);
                else if (str.startsWith("<pos>&"))
                    skip = !addSPos(str);
                else if (str.startsWith("<field>&"))
                    skip = !addSField(str);
                else if (str.startsWith("<misc>&"))
                    skip = !addSMisc(str);
                else if (str.startsWith("<dial>&"))
                    skip = !addSDial(str);
                else if ((lang.isEmpty() && str.startsWith("<gloss>")) || (!lang.isEmpty() && str.startsWith(QStringLiteral("<gloss xml:lang=\"%1\">").arg(lang))))
                    skip = !addGloss(str);
                // Possible error in file format, skip the whole word.
                else if (!str.startsWith("<") || str.startsWith("<ke_") || str.startsWith("<k_") || str.startsWith("<re_") || str.startsWith("<entry>") ||
                        str.startsWith("</ke_") || str.startsWith("</k_") || str.startsWith("</re_") || str.startsWith("</entry>"))
                    skip = true;
            }
        }

        if (!linefound)
            saveEntry();
    }

#ifdef COLLECT_WORD_DATA
    str.clear();
    str += "Types " + QString::number(types.size()) + ": ";
    for (const QString &s : types)
        str += s + " ";

    str += "\n\n";

    str += "Fields " + QString::number(fields.size()) + ": ";
    for (const QString &s : fields)
        str += s + " ";

    str += "\n\n";

    str += "Notes " + QString::number(notes.size()) + ": ";
    for (const QString &s : notes)
        str += s + " ";


    str = str.trimmed();

    QMessageBox::information(this, "Types, notes, fields:", str, QMessageBox::Ok);
#endif

    file.close();

    TextSearchTree ktree(nullptr, true, false);
    TextSearchTree btree(nullptr, true, true);
    TextSearchTree dtree(nullptr, false, false);

    // The dictionary and its indexes that will be built in this function.
    TreeBuilder idtree(dtree, words.size(),
        [this](int wix, QStringList& texts) { for (int ix = 0; ix != words[wix]->defs.size(); ++ix)
        {
            QCharString str;
            str.copy(words[wix]->defs[ix].def.toLower().constData());
            QCharTokenizer tok(str.data());

            while (tok.next())
                texts << QString(tok.token(), tok.tokenSize());
        }},
        [this]() { return nextUpdate(); });
    TreeBuilder iktree(ktree, words.size(),
        [this](int wix, QStringList& texts) { texts << words[wix]->romaji.toQStringRaw(); },
        [this]() { return nextUpdate(); });
    TreeBuilder ibtree(btree, words.size(),
        [this](int wix, QStringList& texts) { texts << words[wix]->romaji.toQStringRaw(); },
        [this]() { return nextUpdate(); });
    smartvector<KanjiDictData> kanjidata;
    std::map<ushort, std::vector<int>> symdata;
    std::map<ushort, std::vector<int>> kanadata;
    std::vector<int> abcde;
    std::vector<int> aiueo;

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(6);

    if (!setInfoText(tr("%1/%2 - Initializing search trees...").arg(step).arg(stepcnt)))
        return nullptr;
    ++step;

    ui->progressBar->setMaximum(iktree.initSize() + ibtree.initSize() + idtree.initSize());

    while (iktree.initNext() || ibtree.initNext() || idtree.initNext())
    {
        if (!nextUpdate(iktree.initPos() + ibtree.initPos() + idtree.initPos(), true))
            return nullptr;
    }

    if (!setInfoText(tr("%1/%2 - Building search trees...").arg(step).arg(stepcnt)))
        return nullptr;
    ++step;

    // Filled the words vector with the entries from the dictionary. The
    // JMdict file is no longer needed. Everything else is generated here.

    ui->progressBar->setMaximum(iktree.importSize() + ibtree.importSize() + idtree.importSize());

    while (iktree.sortNext() || ibtree.sortNext() || idtree.sortNext())
    {
        if (!nextUpdate(iktree.importPos() + ibtree.importPos() + idtree.importPos(), true))
            return nullptr;
    }


    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(words.size() * 2);

    if (!setInfoText(tr("%1/%2 - Building character indexes...").arg(step).arg(stepcnt)))
        return nullptr;
    ++step;

    kanjidata.resize(ZKanji::kanjicount, KanjiDictData());

    abcde.resize(words.size());
    aiueo.resize(words.size());
    for (int ix = 0; ix != words.size(); ++ix)
    {
        abcde[ix] = ix;
        aiueo[ix] = ix;

        WordEntry *w = words[ix];
        QChar *kanji = w->kanji.data();
        int len = w->kanji.size();
        for (int iy = 0; iy != len; ++iy)
        {
            if (KANJI(kanji[iy].unicode()))
            {
                int kix = ZKanji::kanjiIndex(kanji[iy]);

                std::vector<int> &wvec = kanjidata[kix]->words;
                if (!wvec.empty() && wvec.back() == ix)
                    continue;
                wvec.push_back(ix);
                
                ZKanji::kanjis[kix]->word_freq += w->freq;
            }
            else if (!KANA(kanji[iy].unicode()) && UNICODE_J(kanji[iy].unicode()))
            {
                std::vector<int> &svec = symdata[kanji[iy].unicode()];
                if (!svec.empty() && svec.back() == ix)
                    continue;
                svec.push_back(ix);
            }
        }
        if (!nextUpdate(ix * 2))
            return nullptr;

        QChar *romaji = words[ix]->romaji.data();
        len = words[ix]->romaji.size();

        for (int iy = 0; iy != len; ++iy)
        {
            ushort ch;

            int dummy;
            if (!kanavowelize(ch, dummy, romaji + iy, len - iy))
                continue;

            std::vector<int> &kvec = kanadata[ch];
            if (!kvec.empty() && kvec.back() == ix)
                continue;
            kvec.push_back(ix);
        }

        QChar *kana = words[ix]->kana.data();
        len = words[ix]->kana.size();

        for (int iy = 0; iy != len; ++iy)
        {
            ushort ch = kana[iy].unicode();
            if (!KANA(ch))
                continue;
            if (KATAKANA(ch) && ch <= 0x30F4)
                ch -= 0x60;

            std::vector<int> &kvec = kanadata[ch];
            if (!kvec.empty() && kvec.back() == ix)
                continue;
            kvec.push_back(ix);
        }

        if (!nextUpdate(ix * 2 + 1))
            return nullptr;
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(3);
    if (!setInfoText(tr("%1/%2 - Alphabetical and AIUEO ordering...").arg(step).arg(stepcnt)))
        return nullptr;
    ++step;

    std::map<QCharString, QString> hira;
    ui->progressBar->setValue(1);
    if (interruptSort(abcde.begin(), abcde.end(), [this, &hira](int a, int b, bool &stop) {
        if (!nextUpdate())
        {
            stop = false;
            return false;
        }

        int val = qcharcmp(words[a]->romaji.data(), words[b]->romaji.data());
        if (val != 0)
            return val < 0;

        QString ah;
        QString bh;
        auto it = hira.find(words[a]->kana);
        if (it != hira.end())
            ah = it->second;
        else
            hira[words[a]->kana] = ah = hiraganize(words[a]->kana);
        it = hira.find(words[b]->kana);
        if (it != hira.end())
            bh = it->second;
        else
            hira[words[b]->kana] = bh = hiraganize(words[b]->kana);
        val = qcharcmp(ah.constData(), bh.constData());
        if (val != 0)
            return val < 0;

        val = qcharcmp(words[a]->kana.data(), words[b]->kana.data());
        if (val != 0)
            return val < 0;

        return qcharcmp(words[a]->kanji.data(), words[b]->kanji.data()) < 0;
    }))
        return nullptr;

        
    ui->progressBar->setValue(2);
    if (interruptSort(aiueo.begin(), aiueo.end(), [this, &hira](int a, int b, bool &stop) {
        if (!nextUpdate())
        {
            stop = false;
            return false;
        }

        QString ah;
        QString bh;
        auto it = hira.find(words[a]->kana);
        if (it != hira.end())
            ah = it->second;
        else
            hira[words[a]->kana] = ah = hiraganize(words[a]->kana);
        it = hira.find(words[b]->kana);
        if (it != hira.end())
            bh = it->second;
        else
            hira[words[b]->kana] = bh = hiraganize(words[b]->kana);
        int val = qcharcmp(ah.constData(), bh.constData());
        if (val != 0)
            return val < 0;

        val = qcharcmp(words[a]->kana.data(), words[b]->kana.data());
        if (val != 0)
            return val < 0;

        return qcharcmp(words[a]->kanji.data(), words[b]->kanji.data()) < 0;
    }))
        return nullptr;

    if (!nextUpdate(3, true))
        return nullptr;

    return new Dictionary(std::move(words), std::move(dtree), std::move(ktree), std::move(btree), std::move(kanjidata), std::move(symdata), std::move(kanadata), std::move(abcde), std::move(aiueo));
}

bool DictImport::importJLPTN(Dictionary *dict)
{
    if (!setInfoText(tr("Opening JLPT N data...")))
        return false;

    // JLPTNData format (tab delimiter):
    // word_kanji word_kana N [obsolete_field]
    //
    // after an empty line: (no delimiter)
    // kanji_character N

    if (!file.open(path + "/JLPTNData.txt"))
        return false;

    ImportFileHandlerGuard fileguard(file);

    qint64 s = file.size();
    ui->progressBar->setMaximum(s);

    // <Word kanji, word kana, word kana romanized>, JLPT N level
    std::vector<std::pair<std::tuple<QString, QString, QString>, int>> words;
    // Missing words: <Word kanji, word kana, JLPT N level>
    std::vector<std::tuple<QString, QString, int>> missing;

    enum class Modes { Words, Kanji };
    Modes mode = Modes::Words;
    QString str;
    while (file.getLine(str))
    {
        if (!nextUpdate(file.pos()))
            return false;

        if (str.isEmpty())
        {
            mode = Modes::Kanji;
            continue;
        }

        if (str.at(0) == '#')
            continue;

        if (mode == Modes::Words)
        {
            QStringList parts = str.split('\t', QString::SkipEmptyParts);

            if (parts.size() != 3 && parts.size() != 4)
            {
                setErrorText("Invalid file format.");
                return false;
            }

            QString kanji = parts.at(0);
            int s = kanji.size();
            for (int ix = 0; ix != s; ++ix)
            {
                ushort c = kanji.at(ix).unicode();
                if (!UNICODE_J(c) && !KANJI(c))
                {
                    setErrorText("Invalid file format.");
                    return false;
                }
            }

            QString kana = parts.at(1);
            s = kana.size();
            bool haskana = false;
            for (int ix = 0; ix != s; ++ix)
            {
                ushort c = kana.at(ix).unicode();
                haskana = haskana || KANA(c);

                if ((!VALIDKANA(c) && !MIDDOT(c)) || (ix != 0 && DASH(c) && !KANA(kana.at(ix - 1).unicode())))
                {
                    setErrorText("Invalid file format.");
                    return false;
                }
            }

            if (!haskana)
            {
                setErrorText("Invalid file format.");
                return false;
            }

            int N;
            bool ok = false;
            N = parts.at(2).toInt(&ok);

            if (!ok || N < 1 || N > 5)
            {
                setErrorText("Invalid file format.");
                return false;
            }

            int ix = dict->findKanjiKanaWord(kanji, kana);
            if (ix != -1)
                words.push_back(std::make_pair(std::make_tuple(kanji, kana, romanize(kana)), N));
            else
                missing.push_back(std::make_tuple(kanji, kana, N));
        }
        else
        {
            if (str.size() != 2 || !KANJI(str.at(0).unicode()) || str.at(1).unicode() - '0' < 0 || str.at(1).unicode() - '0' > 5)
            {
                setErrorText("Invalid file format.");
                return false;
            }

            if (str.at(1).unicode() - '0' != 0)
                ZKanji::kanjis[ZKanji::kanjiIndex(str.at(0))]->jlpt = str.at(1).unicode() - '0';
        }
    }

    file.close();

    if (!setInfoText(tr("Sorting JLPT N data...")))
        return false;

    if (interruptSort(words.begin(), words.end(), [this](const std::pair<std::tuple<QString, QString, QString>, int> &a, const std::pair<std::tuple<QString, QString, QString>, int> &b, bool &stop)
    {
        if (!nextUpdate())
        {
            stop = true;
            return false;
        }
        return wordcompare(std::get<0>(a.first).constData(), std::get<1>(a.first).constData(), std::get<0>(b.first).constData(), std::get<1>(b.first).constData()) < 0;
    }))
        return false;

    TreeBuilder tree(ZKanji::commons, words.size(),
        [&words](int wix, QStringList& texts) { texts << std::get<2>(words[wix].first); },
        [this]() { return nextUpdate(); });

    for (int ix = 0; ix != words.size(); ++ix)
        ZKanji::commons.addJLPTN(std::get<0>(words[ix].first).constData(), std::get<1>(words[ix].first).constData(), words[ix].second);

    ui->progressBar->setValue(0);

    if (!setInfoText(tr("%1/%2 - Initializing commons tree...").arg(step).arg(stepcnt)))
        return false;

    ++step;

    ui->progressBar->setMaximum(tree.initSize());

    while (tree.initNext())
    {
        if (!nextUpdate(tree.initPos(), true))
            return false;
    }

    if (!setInfoText(tr("%1/%2 - Building commons tree...").arg(step).arg(stepcnt)))
        return false;

    ++step;

    // Filled the words vector with the entries from the dictionary. The JMdict file is no
    // longer needed. Everything else is generated here.

    ui->progressBar->setMaximum(tree.importSize());

    while (tree.sortNext())
    {
        if (!nextUpdate(tree.importPos(), true))
            return false;
    }
    ui->progressBar->setValue(ui->progressBar->maximum());

    if (!missing.empty())
    {
        JLPTReplaceForm *frm = new JLPTReplaceForm(this);
        if (frm->exec(path, dict, missing) != ModalResult::Ok)
            return false;
    }

    return true;
}

bool DictImport::doImportFromExport()
{
    if (!setInfoText(tr("Opening export file...")))
        return false;

    if (!file.open(path))
    {
        setErrorText(tr("Couldn't open file for reading."));
        return false;
    }

    ImportFileHandlerGuard fileguard(file);

    if (!setInfoText(tr("Opened file, reading...")))
        return false;

    qint64 s = file.size();
    ui->progressBar->setMaximum(s);

    if (!setInfoText(tr("%1/%2 - Processing file...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    // Current line.
    QString str;

    QString infotext;
    smartvector<KanjiDictData> kanjidata;
    kanjidata.resize(ZKanji::kanjicount, KanjiDictData());
    std::map<ushort, std::vector<int>> symdata;
    std::map<ushort, std::vector<int>> kanadata;
    std::vector<int> abcde;
    std::vector<int> aiueo;

    while (file.getLine(str))
    {
        if (!nextUpdate(file.pos()))
            return false;

        if (str.isEmpty() || str.at(0) == '#')
            continue;

        QString sname = sectionName(str);
        if (sname.isEmpty())
        {
            setErrorText(tr("Invalid line. Expected section start."));
            return false;
        }

        if (sname == "Base")
        {
            if (!infotext.isEmpty())
            {
                setErrorText(tr("Duplicate About or Base section found."));
                return false;
            }

            infotext = JMDictInfoText;
            continue;
        }
        if (sname == "About")
        {
            if (!infotext.isEmpty())
            {
                setErrorText(tr("Duplicate About or Base section found."));
                return false;
            }

            while (file.getLine(str))
            {
                if (!nextUpdate(file.pos()))
                    return false;

                if (str.isEmpty() || str.at(0) == '#')
                    continue;

                if (str.at(0) == '[')
                {
                    file.repeat();
                    break;
                }

                if (str.at(0) == '*')
                    infotext += "\n" % str.mid(1);
                else if (str.at(0) == '-')
                    infotext += str.mid(1);
                else
                {
                    setErrorText(tr("Invalid character in the About section."));
                    return false;
                }
            }

            continue;
        }

        if (sname == "Words")
        {
            WordEntry *e;
            while ((e = importWord()) != nullptr)
            {
                if (!nextUpdate(file.pos()))
                    return false;

                words.push_back(e);
            }

            if (isErrorSet() || !nextUpdate(file.pos()))
                return false;
            
            continue;
        }

        if (sname == "KanjiDefinitions")
        {
            std::pair<ushort, QStringList> def;
            while (importKanjiDefinition(def))
            {
                if (!nextUpdate(file.pos()))
                    return false;

                kanjidata[def.first]->meanings.copy(def.second);
            }

            if (isErrorSet() || !nextUpdate(file.pos()))
                return false;

            continue;
        }

        if (!skipSection())
            return false;
    }

    TextSearchTree ktree(nullptr, true, false);
    TextSearchTree btree(nullptr, true, true);
    TextSearchTree dtree(nullptr, false, false);

    // The dictionary and its indexes that will be built in this function.
    TreeBuilder idtree(dtree, words.size(),
        [this](int wix, QStringList& texts) { for (int ix = 0; ix != words[wix]->defs.size(); ++ix)
    {
        QCharString str;
        str.copy(words[wix]->defs[ix].def.toLower().constData());
        QCharTokenizer tok(str.data());

        while (tok.next())
            texts << QString(tok.token(), tok.tokenSize());
    }},
        [this]() { return nextUpdate(); });
    TreeBuilder iktree(ktree, words.size(),
        [this](int wix, QStringList& texts) { texts << words[wix]->romaji.toQStringRaw(); },
        [this]() { return nextUpdate(); });
    TreeBuilder ibtree(btree, words.size(),
        [this](int wix, QStringList& texts) { texts << words[wix]->romaji.toQStringRaw(); },
        [this]() { return nextUpdate(); });

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(6);

    if (!setInfoText(tr("%1/%2 - Initializing search trees...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    ui->progressBar->setMaximum(iktree.initSize() + ibtree.initSize() + idtree.initSize());

    while (iktree.initNext() || ibtree.initNext() || idtree.initNext())
    {
        if (!nextUpdate(iktree.initPos() + ibtree.initPos() + idtree.initPos(), true))
            return false;
    }

    if (!setInfoText(tr("%1/%2 - Building search trees...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    // Filled the words vector with the entries from the dictionary. The
    // JMdict file is no longer needed. Everything else is generated here.

    ui->progressBar->setMaximum(iktree.importSize() + ibtree.importSize() + idtree.importSize());

    while (iktree.sortNext() || ibtree.sortNext() || idtree.sortNext())
    {
        if (!nextUpdate(iktree.importPos() + ibtree.importPos() + idtree.importPos(), true))
            return false;
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(words.size() * 2);

    if (!setInfoText(tr("%1/%2 - Building character indexes...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    abcde.resize(words.size());
    aiueo.resize(words.size());
    for (int ix = 0; ix != words.size(); ++ix)
    {
        abcde[ix] = ix;
        aiueo[ix] = ix;

        WordEntry *w = words[ix];
        QChar *kanji = w->kanji.data();
        int len = w->kanji.size();
        for (int iy = 0; iy != len; ++iy)
        {
            if (KANJI(kanji[iy].unicode()))
            {
                int kix = ZKanji::kanjiIndex(kanji[iy]);

                std::vector<int> &wvec = kanjidata[kix]->words;
                if (!wvec.empty() && wvec.back() == ix)
                    continue;
                wvec.push_back(ix);

                // Only for main dictionary:
                //ZKanji::kanjis[kix]->word_freq += w->freq;
            }
            else if (!KANA(kanji[iy].unicode()) && UNICODE_J(kanji[iy].unicode()))
            {
                std::vector<int> &svec = symdata[kanji[iy].unicode()];
                if (!svec.empty() && svec.back() == ix)
                    continue;
                svec.push_back(ix);
            }
        }
        if (!nextUpdate(ix * 2))
            return false;

        QChar *romaji = words[ix]->romaji.data();
        len = words[ix]->romaji.size();

        for (int iy = 0; iy != len; ++iy)
        {
            ushort ch;

            int dummy;
            if (!kanavowelize(ch, dummy, romaji + iy, len - iy))
                continue;

            std::vector<int> &kvec = kanadata[ch];
            if (!kvec.empty() && kvec.back() == ix)
                continue;
            kvec.push_back(ix);
        }

        QChar *kana = words[ix]->kana.data();
        len = words[ix]->kana.size();

        for (int iy = 0; iy != len; ++iy)
        {
            ushort ch = kana[iy].unicode();
            if (!KANA(ch))
                continue;
            if (KATAKANA(ch) && ch <= 0x30F4)
                ch -= 0x60;

            std::vector<int> &kvec = kanadata[ch];
            if (!kvec.empty() && kvec.back() == ix)
                continue;
            kvec.push_back(ix);
        }

        if (!nextUpdate(ix * 2 + 1))
            return false;
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(3);
    if (!setInfoText(tr("%1/%2 - Alphabetical and AIUEO ordering...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    std::map<QCharString, QString> hira;
    ui->progressBar->setValue(1);
    if (interruptSort(abcde.begin(), abcde.end(), [this, &hira](int a, int b, bool &stop) {
        if (!nextUpdate())
        {
            stop = false;
            return false;
        }

        int val = qcharcmp(words[a]->romaji.data(), words[b]->romaji.data());
        if (val != 0)
            return val < 0;

        QString ah;
        QString bh;
        auto it = hira.find(words[a]->kana);
        if (it != hira.end())
            ah = it->second;
        else
            hira[words[a]->kana] = ah = hiraganize(words[a]->kana);
        it = hira.find(words[b]->kana);
        if (it != hira.end())
            bh = it->second;
        else
            hira[words[b]->kana] = bh = hiraganize(words[b]->kana);
        val = qcharcmp(ah.constData(), bh.constData());
        if (val != 0)
            return val < 0;

        val = qcharcmp(words[a]->kana.data(), words[b]->kana.data());
        if (val != 0)
            return val < 0;

        return qcharcmp(words[a]->kanji.data(), words[b]->kanji.data()) < 0;
    }))
        return false;


    ui->progressBar->setValue(2);
    if (interruptSort(aiueo.begin(), aiueo.end(), [this, &hira](int a, int b, bool &stop) {
        if (!nextUpdate())
        {
            stop = false;
            return false;
        }

        QString ah;
        QString bh;
        auto it = hira.find(words[a]->kana);
        if (it != hira.end())
            ah = it->second;
        else
            hira[words[a]->kana] = ah = hiraganize(words[a]->kana);
        it = hira.find(words[b]->kana);
        if (it != hira.end())
            bh = it->second;
        else
            hira[words[b]->kana] = bh = hiraganize(words[b]->kana);
        int val = qcharcmp(ah.constData(), bh.constData());
        if (val != 0)
            return val < 0;

        val = qcharcmp(words[a]->kana.data(), words[b]->kana.data());
        if (val != 0)
            return val < 0;

        return qcharcmp(words[a]->kanji.data(), words[b]->kanji.data()) < 0;
    }))
        return false;

    if (!nextUpdate(3, true))
        return false;

    dict = new Dictionary(std::move(words), std::move(dtree), std::move(ktree), std::move(btree), std::move(kanjidata), std::move(symdata), std::move(kanadata), std::move(abcde), std::move(aiueo));
    dict->setInfoText(infotext);
    return true;
}

bool DictImport::doImportFromExportPartial()
{
    if (!setInfoText(tr("Opening export file...")))
        return false;

    if (!file.open(path))
    {
        setErrorText(tr("Couldn't open file for reading."));
        return false;
    }

    ImportFileHandlerGuard fileguard(file);

    if (!setInfoText(tr("Opened file, reading...")))
        return false;

    qint64 s = file.size();
    ui->progressBar->setMaximum(s);

    if (!setInfoText(tr("%1/%2 - Processing file...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    QString str;

    std::vector<std::pair<ushort, QStringList>> kanjis;

    while (file.getLine(str))
    {
        if (!nextUpdate(file.pos()))
            return false;

        if (str.isEmpty() || str.at(0) == '#')
            continue;

        QString sname = sectionName(str);
        if (sname.isEmpty())
        {
            setErrorText(tr("Invalid line. Expected section start."));
            return false;
        }

        if (sname == "Words")
        {
            WordEntry *e;
            while ((e = importWord()) != nullptr)
            {
                if (!nextUpdate(file.pos()))
                    return false;
                words.push_back(e);
            }

            if (isErrorSet() || !nextUpdate(file.pos()))
                return false;

            continue;
        }

        if (sname == "KanjiDefinitions")
        {
            std::pair<ushort, QStringList> def;
            while (importKanjiDefinition(def))
            {
                if (!nextUpdate(file.pos()))
                    return false;

                kanjis.push_back(def);
            }

            if (isErrorSet() || !nextUpdate(file.pos()))
                return false;

            continue;
        }

        if (!skipSection())
            return false;
    }

    setModifiedText(tr("Importing partial dictionary. This can take several minutes, please wait..."));
    if (!setInfoText(tr("%1/%2 - Updating words. User data will be modified...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    for (int ix = 0, siz = words.size(); ix != siz; ++ix)
    {
        WordEntry *e = words[ix];
        int windex = dict->findKanjiKanaWord(e->kanji.data(), e->kana.data(), e->romaji.data());
        if (windex != -1)
            dict->cloneWordData(windex, words[ix], false, false);
        else
            windex = dict->addWordCopy(e, false);

        if (wordgroup != nullptr)
            wordgroup->add(windex);
    }

    if (!setInfoText(tr("%1/%2 - Updating kanji meanings. User data will be modified...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    for (int ix = 0, siz = kanjis.size(); ix != siz; ++ix)
    {
        dict->setKanjiMeaning(kanjis[ix].first, kanjis[ix].second);
        if (kanjigroup != nullptr)
            kanjigroup->add(kanjis[ix].first);
    }

    return true;
}

WordEntry* DictImport::importWord()
{
    // "# written_form(kana_reading)(SPACE)frequency_number(SPACE)information_field\n"
    // "# meaning_lines\n"
    // "#\n"
    // "# The format of each meaning line:\n"
    // "# D:(SPACE)types_notes_fields_dialects(TAB)definition_text\n"

    QString str;

    while (file.getLine(str))
    {
        if (str.isEmpty() || str.at(0) == '#')
            continue;

        if (str.at(0) == '[')
        {
            file.repeat();
            return nullptr;
        }

        if (!nextUpdate(file.pos()))
            return nullptr;

        break;
    }

    if (str.isEmpty())
        return nullptr;


    QString kanji;
    QString kana;

    int pos = 0;
    if (!kanjiKana(str, 0, kanji, kana, pos))
        return nullptr;
    
    if (str.size() <= pos || str.at(pos) != ' ')
    {
        setErrorText(tr("Invalid line format. Expected space after word."));
        return false;
    }

    ++pos;

    int freq = 0;
    bool ok;
    int p2 = str.indexOf(' ', pos);
    if (p2 == -1)
        p2 = str.size();
    freq = str.mid(pos, p2 - pos).toInt(&ok);
    if(!ok)
    {
        setErrorText(tr("Word frequency not found or not a valid number."));
        return nullptr;
    }

    uchar inf = Strings::wordTagInfo(str.mid(p2 + 1), &ok);
    if (!ok)
    {
        setErrorText(tr("Word information field is invalid."));
        return nullptr;
    }

    std::unique_ptr<WordEntry> e(new WordEntry);
    e->kanji.copy(kanji.constData());
    e->kana.copy(kana.constData());
    e->freq = freq;
    e->dat = 0;
    e->romaji.copy(romanize(kana.constData()).constData());
    e->inf = inf;

    std::vector<WordDefinition> defs;

    while (file.getLine(str))
    {
        if (str.isEmpty() || str.at(0) == '#')
            continue;

        if (str.left(3) != "D: ")
        {
            file.repeat();
            break;
        }

        if (!nextUpdate(file.pos()))
            return nullptr;

        //# D:(SPACE)types_notes_fields_dialects(TAB)definition_text\n
        int tabpos = str.indexOf('\t', 3);
        if (tabpos == -1 || str.indexOf('\t', tabpos + 1) != -1)
        {
            setErrorText(tr("Word definition line is invalid."));
            return nullptr;
        }

        QString tags = str.mid(3, tabpos - 3);

        WordDefinition def;
        def.attrib.types = Strings::wordTagTypes(tags, &ok);
        if (ok)
            def.attrib.notes = Strings::wordTagNotes(tags, &ok);
        if (ok)
            def.attrib.fields = Strings::wordTagFields(tags, &ok);
        if (ok)
            def.attrib.dialects = Strings::wordTagDialects(tags, &ok);

        if (!ok)
        {
            setErrorText(tr("Word definition's tag field is invalid."));
            return nullptr;
        }

        def.def.copy(str.mid(tabpos + 1).trimmed().constData());
        if (def.def.size() > 9999)
        {
            setErrorText(tr("Word definition too long. Possibly corrupt data."));
            return nullptr;
        }

        defs.push_back(def);
    }

    if (defs.empty())
    {
        setErrorText(tr("Word definition line missing."));
        return nullptr;
    }

    e->defs = defs;

    return e.release();
}

bool DictImport::importKanjiDefinition(std::pair<ushort, QStringList> &result)
{
    QString str;
    while (file.getLine(str) && (str.isEmpty() || str.at(0) == '#'))
    {
        if (!nextUpdate(file.pos()))
            return false;
    }

    if (str.isEmpty() || !nextUpdate(file.pos()))
        return false;

    QChar kch = str.at(0);
    if (!KANJI(kch.unicode()))
    {
        file.repeat();
        return false;
    }

    int kix = ZKanji::kanjiIndex(kch);
    if (kix == -1)
    {
        file.repeat();
        return false;
    }
    if (str.size() < 3 || str.at(1) != '\t')
    {
        setErrorText(tr("Invalid line in KanjiDefinitions section."));
        return false;
    }

    result = std::pair<ushort, QStringList>(kix, str.mid(2).split(GLOSS_SEP_CHAR, QString::SkipEmptyParts));

    return true;
}

bool DictImport::doImportUserData()
{
    if (!setInfoText(tr("Opening export file...")))
        return false;

    if (!file.open(path))
    {
        setErrorText(tr("Couldn't open file for reading."));
        return false;
    }

    ImportFileHandlerGuard fileguard(file);

    if (!setInfoText(tr("Opened file, reading...")))
        return false;

    qint64 s = file.size();
    ui->progressBar->setMaximum(s);

    if (!setInfoText(tr("%1/%2 - Processing file...").arg(step).arg(stepcnt)))
        return false;
    ++step;

    QString str;

    while (file.getLine(str))
    {
        if (!nextUpdate(file.pos()))
            return false;

        if (str.isEmpty() || str.at(0) == '#')
            continue;

        QString sname = sectionName(str);
        if (sname.isEmpty())
        {
            setErrorText(tr("Invalid line. Expected section start."));
            return false;
        }

        if (sname == "StudyDefinitions")
        {
            if (!studymeanings)
            {
                if (!skipSection())
                    return false;
                continue;
            }

            while (file.getLine(str))
            {
                if (!nextUpdate(file.pos()))
                    return false;

                if (str.isEmpty() || str.at(0) == '#')
                    continue;

                if (str.at(0) == '[')
                {
                    file.repeat();
                    break;
                }

                int tabpos = str.indexOf('\t');
                if (tabpos < 3)
                {
                    setErrorText("Invalid line format. Tab character expected.");
                    return false;
                }

                int pos = 0;
                QString kanji;
                QString kana;
                if (!kanjiKana(str, 0, kanji, kana, pos))
                    return false;

                if (pos != tabpos)
                {
                    setErrorText(tr("Invalid line format. Expected TAB after word."));
                    return false;
                }

                int windex = dict->findKanjiKanaWord(kanji, kana);
                if (windex == -1)
                {
                    setInfoText(tr("Word %1(%2) missing from dictionary for studied word definition.").arg(kanji).arg(kana));
                    continue;
                }

                QString def = str.mid(tabpos + 1);
                dict->setWordStudyDefinition(windex, def);
            }

            continue;
        }
        if (sname == "KanjiWordExamples")
        {
            if (!kanjiex)
            {
                if (!skipSection())
                    return false;
                continue;
            }

            while (file.getLine(str))
            {
                if (!nextUpdate(file.pos()))
                    return false;

                if (str.isEmpty() || str.at(0) == '#')
                    continue;

                if (str.at(0) == '[')
                {
                    file.repeat();
                    break;
                }

                if (str.size() < 6 || str.at(1) != ' ')
                {
                    setErrorText("Incorrect line format in kanji word examples listing.");
                    return false;
                }

                int kindex = ZKanji::kanjiIndex(str.at(0));

                if (kindex == -1)
                {
                    setErrorText("Unrecognized kanji in kanji word examples listing.");
                    return false;
                }

                int pos = 3;
                while (pos < str.size())
                {
                    QString kanji;
                    QString kana;
                    if (!kanjiKana(str, pos, kanji, kana, pos))
                        return false;

                    if (str.size() > pos && str.at(pos) != ' ')
                    {
                        setErrorText(tr("Invalid line format. Expected space after word."));
                        return false;
                    }

                    int windex = dict->findKanjiKanaWord(kanji, kana);

                    if (windex == -1)
                        setInfoText(tr("Word %1(%2) missing from dictionary, as example for kanji: %3").arg(kanji).arg(kana).arg(str.at(0)));
                    else
                    {
                        if (dict->wordEntry(windex)->kanji.find(str.at(0)) == -1)
                        {
                            setErrorText("Word listed as example for kanji doesn't contain the kanji.");
                            return false;
                        }
                        dict->addKanjiExample(kindex, windex);
                    }
                    pos = pos + 1;
                }
            }

            continue;
        }
        if (sname == "KanjiGroups")
        {
            if (kanjiroot == nullptr)
            {
                if (!skipSection())
                    return false;
                continue;
            }

            while (file.getLine(str))
            {
                if (!nextUpdate(file.pos()))
                    return false;

                if (str.isEmpty() || str.at(0) == '#')
                    continue;

                if (str.at(0) == '[')
                {
                    file.repeat();
                    break;
                }

                int tabpos = str.indexOf('\t');
                if (str.size() < 4 || tabpos < 4 || str.left(3) != "G: ")
                {
                    setErrorText(tr("Invalid line format in group line."));
                    return false;
                }

                KanjiGroup *grp = kanjiroot->groupFromEncodedName(str, 3, tabpos - 3, true);
                if (grp == nullptr)
                {
                    setErrorText(tr("Invalid or missing group name."));
                    return false;
                }

                for (int ix = tabpos + 1, siz = str.size(); ix != siz; ++ix)
                {
                    int kindex = ZKanji::kanjiIndex(str.at(ix));

                    if (kindex == -1)
                    {
                        setErrorText("Unrecognized kanji or invalid character in kanji listing.");
                        return false;
                    }

                    grp->add(kindex);
                }
            }

            continue;
        }
        if (sname == "WordGroups")
        {
            if (wordsroot == nullptr)
            {
                if (!skipSection())
                    return false;
                continue;
            }

            while (file.getLine(str))
            {
                if (!nextUpdate(file.pos()))
                    return false;

                if (str.isEmpty() || str.at(0) == '#')
                    continue;

                if (str.at(0) == '[')
                {
                    file.repeat();
                    break;
                }

                int tabpos = str.indexOf('\t');
                if (str.size() < 4 || tabpos < 4 || str.left(3) != "G: ")
                {
                    setErrorText(tr("Invalid line format in group line."));
                    return false;
                }

                WordGroup *grp = wordsroot->groupFromEncodedName(str, 3, tabpos - 3, true);
                if (grp == nullptr)
                {
                    setErrorText(tr("Invalid or missing group name."));
                    return false;
                }

                QStringList words = str.mid(tabpos + 1).split(' ');
                int pos = tabpos + 1;
                while (pos < str.size())
                {
                    QString kanji;
                    QString kana;
                    if (!kanjiKana(str, pos, kanji, kana, pos))
                        return false;

                    if (str.size() > pos && str.at(pos) != ' ')
                    {
                        setErrorText(tr("Invalid line format. Expected space after word."));
                        return false;
                    }

                    ++pos;

                    int windex = dict->findKanjiKanaWord(kanji, kana);

                    if (windex == -1)
                    {
                        setInfoText(tr("Word %1(%2) missing from dictionary in group %3").arg(kanji).arg(kana).arg(grp->fullName(wordsroot)));
                        continue;
                    }

                    grp->add(windex);
                }
            }

            continue;
        }

        if (!skipSection())
            return false;
    }

    return true;
}

bool DictImport::importRadFiles()
{
    ui->progressBar->setValue(0);
    if (!setInfoText(tr("Reading radicals files...")))
        return false;

    QFile f1;
    f1.setFileName(path + "/radkelement.txt");
    QFile f2;
    f2.setFileName(path + "/radkfile");
    QFile f3;
    f3.setFileName(path + "/zradfile.txt");

    if (!f1.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    if (!f2.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    if (!f3.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    qint64 s1 = f1.size();
    qint64 s2 = f2.size();
    qint64 s3 = f3.size();
    ui->progressBar->setMaximum(s1 + s2 + s3);

    file.setFile(f1);

    //QTextStream stream(&f1);
    //stream.setCodec("UTF-8");

    // Maps unicode character codes to <element index, element variant> pairs. The unicode
    // character is what is found in radkfile. Elements refer to the stroke order database
    // loaded later.
    // TODO: modify the stroke order data file to hold the characters to radkfile instead,
    // so we don't rely on the stroke order data staying unchanged.

    QString str;
    while (file.getLine(str))
    {
        if (!nextUpdate(file.pos()))
            return false;

        if (str.isEmpty() || str.at(0) == '#')
            continue;

        QStringList parts = str.split(' ', QString::SkipEmptyParts);

        if ((parts.size() != 2 && parts.size() != 3) || parts.at(0).size() != 1 || parts.at(0).at(0).unicode() < 0x30ce || parts.at(0).at(0).unicode() > 0xff5c)
        {
            setErrorText("Invalid radkelement file.");
            return false;
        }

        bool ok = false;
        int val1 = parts.at(1).toInt(&ok);
        int val2 = 0;
        if (parts.size() == 3 && ok)
            val2 = parts.at(2).toInt(&ok);

        if (!ok)
        {
            setErrorText("Invalid element index number. It's not a number.");
            return false;
        }

        ZKanji::radkmap[parts.at(0).at(0).unicode()] = std::make_pair(val1, val2);
    }
    file.close();

    file.setFile(f2, "EUC-JP");

    //stream.setDevice(&f2);
    //stream.setCodec("EUC-JP");

    enum class Modes { Seeking, Reading };
    Modes mode = Modes::Seeking;

    ZKanji::radklist.reserve(253);
    // Temporary storage before filling the fastarray in radklist.
    std::vector<ushort> tmplist;

    while (file.getLine(str))
    {
        if (!nextUpdate(s1 + file.pos()))
            return false;

        if (str.isEmpty() || (mode == Modes::Seeking && str.at(0) != '$'))
            continue;

        mode = Modes::Reading;

        // Each line starting with the dollar sign has the format:
        // $ radical-symbol stroke-count [optional image file name on some server somewhere]
        if (str.at(0) == '$')
        {
            if (!ZKanji::radklist.empty())
                ZKanji::radklist.back().second = tmplist;
            tmplist.clear();

            QStringList parts = str.split(' ', QString::SkipEmptyParts);

            // Something went wrong if there are not 3 parts.
            // TODO: check the radical and see if it matches an array of radicals (to be created).
            if ((parts.size() != 3 && parts.size() != 4) || parts.at(0).size() != 1 || parts.at(1).size() != 1)
            {
                setErrorText("Bad file format.");
                return false;
            }

            bool ok = false;
            int scnt = parts.at(2).toInt(&ok);

            if (!ok || scnt < 1 || scnt > 20)
            {
                setErrorText("Bad file format.");
                return false;
            }

            if (ZKanji::radkcnt.size() < scnt + 1)
                ZKanji::radkcnt.resize(scnt + 1, 0);
            ++ZKanji::radkcnt[scnt];

            ZKanji::radklist.push_back(std::make_pair(parts.at(1).at(0).unicode(), fastarray<ushort>()));
            continue;
        }

        //std::vector<ushort> &list = ZKanji::radklist.back().second;

        for (int ix = 0; ix != str.size(); ++ix)
        {
            int kix = ZKanji::kanjiIndex(str.at(ix));
            if (kix == -1)
                continue;
            KanjiEntry *k = ZKanji::kanjis[kix];
            if (!k->radks.empty() && k->radks.back() == ZKanji::radklist.size() - 1)
                continue;
            k->radks.push_back(ZKanji::radklist.size() - 1);
            tmplist.push_back(kix);
        }
    }
    if (!ZKanji::radklist.empty())
        ZKanji::radklist.back().second = tmplist;// .shrink_to_fit();
    tmplist.clear();

    file.close();

    // Fix radkcnt, which now contains number of radicals for a given stroke count. It should
    // contain the sum of radicals with less than or equal stroke count.

    for (int ix = 1, siz = ZKanji::radkcnt.size(); siz != 0 && ix != siz; ++ix)
        ZKanji::radkcnt[ix] += ZKanji::radkcnt[ix - 1];

    file.setFile(f3);

    //stream.setDevice(&f3);
    //stream.setCodec("UTF-8");
    
    int partix = -1;
    uchar radnum;
    uchar radstrokes;
    QChar rad;
    QCharStringList radnames;
    std::vector<ushort> radkanji;
    while (file.getLine(str))
    {
        if (!nextUpdate(s1 + s2 + file.pos()))
            return false;

        if (str.isEmpty() || str.at(0) == '#')
            continue;

        if (str.at(0) == '$')
        {
            if (!radkanji.empty())
                ZKanji::radlist.addRadical(rad, std::move(radkanji), radstrokes, radnum, std::move(radnames));
            QStringList parts = str.split(' ', QString::SkipEmptyParts);

            if (parts.size() < 4)
            {
                setErrorText("Bad line format.");
                return false;
            }

            bool ok = false;
            int val = parts.at(0).rightRef(parts.at(0).size() - 1).toInt(&ok);

            if (!ok || val < 1 || val > 214)
            {
                setErrorText("Invalid data.");
                return false;
            }

            radnum = val;

            rad = parts.at(1).at(0);

            val = parts.at(2).toInt(&ok);

            if (!ok || val < 1 || val > 17)
            {
                setErrorText("Invalid data.");
                return false;
            }

            radstrokes = val;

            radkanji.clear();

            radnames.clear(); 
            radnames.reserve(parts.size() - 3);
            for (int ix = 3; ix != parts.size(); ++ix)
            {
                for (int iy = 0; iy != parts.at(ix).size(); ++iy)
                {
                    if (!VALIDKANA(parts.at(ix).at(iy).unicode()))
                    {
                        setErrorText("Radical name invalid.");
                        return false;
                    }
                }

                radnames.add(parts.at(ix).constData());
            }


            ++partix;
            continue;
        }
        else if (partix == -1)
            continue;

        for (int ix = 0; ix != str.size(); ++ix)
        {
            int kix = ZKanji::kanjiIndex(str.at(ix));
            if (kix == -1)
                continue;
            KanjiEntry *k = ZKanji::kanjis[kix];
            if (!k->rads.empty() && k->rads.back() == partix)
                continue;
            k->rads.push_back(partix);
            radkanji.push_back(kix);
        }

    }
    file.close();

    if (!radkanji.empty())
        ZKanji::radlist.addRadical(rad, std::move(radkanji), radstrokes, radnum, std::move(radnames));

    return true;
}

bool DictImport::setInfoText(const QString &str)
{
    ui->infoEdit->appendPlainText(str);
    return nextUpdate(-1, true);
}

void DictImport::setErrorText(const QString &str)
{
    ui->progressLabel->setText("<html><head/><body><p><span style=\"font-size:12pt;\">" % tr("An error occured during import") % "</span></p><p><span style=\"font-size:9pt;\">" % tr("The operation will be aborted.") % "</span></p></body></html>");
    if (file.isOpen())
    {
        QString fname = QFileInfo(file.fileName()).fileName();
        ui->infoEdit->appendPlainText(fname % " Line number: " % QString::number(file.lineNumber()) % " Error:" % str);

        file.close();
    }
    else
        ui->infoEdit->appendPlainText("Error: " % str);

    step = -1;

    ui->finishButton->setEnabled(true);
    ui->finishButton->setText(tr("Abort"));
}

bool DictImport::isErrorSet()
{
    return step == -1;
}

QString DictImport::sectionName(const QString &str) const
{
    if (str.isEmpty() || str.at(0) != '[' || str.at(str.size() - 1) != ']')
        return QString();
    return str.mid(1, str.size() - 2);
}

bool DictImport::skipSection()
{
    QString str;

    while (file.getLine(str) && (str.isEmpty() || str.at(0) != '['))
    {
        if (!nextUpdate(file.pos()))
            return false;
    } 

    if (!str.isEmpty())
        file.repeat();

    return true;
}

bool DictImport::kanjiKana(const QString &str, int p, QString &kanji, QString &kana, int &pos)
{
    pos = p;

    pos = str.indexOf('(', pos);
    if (pos < 1)
    {
        setErrorText(tr("Invalid line format. Expected word kanji."));
        return false;
    }

    kanji = str.mid(p, pos - p);
    for (int ix = 0, siz = kanji.size(); ix != siz; ++ix)
    {
        if (!JAPAN(kanji.at(ix).unicode()))
        {
            setErrorText(tr("Invalid character in word written form."));
            return false;
        }
    }

    ++pos;
    int p2 = str.indexOf(')', pos);
    if (p2 < pos + 1)
    {
        setErrorText(tr("Invalid line format. Expected word kana."));
        return false;
    }

    kana = str.mid(pos, p2 - pos);

    // First position after ).
    pos = p2 + 1;

    bool haskana = false;
    for (int ix = 0, siz = kana.size(); ix != siz; ++ix)
    {
        ushort c = kana.at(ix).unicode();
        haskana = haskana || KANA(c);
        if ((!VALIDKANA(c) && !MIDDOT(c)) || (ix != 0 && DASH(c) && !KANA(kana.at(ix - 1).unicode())))
        {
            setErrorText(tr("Invalid character in word kana form."));
            return false;
        }
    }
    if (!haskana)
    {
        setErrorText(tr("Word kana form doesn't contain kana characters."));
        return false;
    }

    return true;
}

void DictImport::newEntry()
{
    entry.saved = false;
    entry.kusage = 0;
    entry.rusage = 0;
    entry.susage = 0;

    kcurrent = nullptr;
    rcurrent = nullptr;
    scurrent = nullptr;

    //clearEntryCache();

    //lasttypes = 0;
    //lastnotes = 0;
    //entryr = 0;
    //entrys = 0;

    //current = new ImportEntry();
    //current->saved = false;
    //current->kempty = true;
    //list.push_back(current);
}

namespace {
    bool inRange(int val, int min, int max)
    {
        return val >= min && val <= max;
    }
}

void fixDefTypes(const QChar *kanjiform, fastarray<WordDefinition> &defs);
void DictImport::saveEntry()
{
    // Ignore saved and invalid entries.
    if (entry.saved || entry.rusage == 0 || entry.susage == 0)
    {
        entry.saved = true;
        return;
    }

    // Create a word entry only if there are valid kanji/reading/sense
    // combinations for it. 

    for (int ix = 0; ix != (entry.kusage == 0 ? 1 : entry.kusage); ++ix)
    {
        // Kanji entries are not necesserily present if the readings also
        // represent the kanji forms.
        ImportKElement *k = entry.kusage == 0 ? nullptr : entry.klist[ix];
        if (k != nullptr && k->str.isEmpty())
            continue;
        for (int iy = 0; iy != entry.rusage; ++iy)
        {
            ImportRElement *r = entry.rlist[iy];
            if (r->str.isEmpty() || (r->kusage && !inRange(r->krestr.indexOf(k != nullptr ? k->str : r->str), 0, r->kusage - 1)))
                continue;

            // We have a kanji and a reading. Find the first sense which is
            // good for this combination.
            int firsts = -1;
            int spos = 0;
            // Number of good senses
            int scnt = 0;
            for ( ; spos != entry.susage; ++spos)
            {
                ImportSElement *s = entry.slist[spos];
                if (s->gusage != 0 && (!s->kusage || inRange(s->krestr.indexOf(k != nullptr ? k->str : r->str), 0, s->kusage - 1)) && (!s->rusage || inRange(s->rrestr.indexOf(r->str), 0, s->rusage - 1)))
                {
                    ++scnt;
                    if (firsts == -1)
                        firsts = spos;
                }
            }
            // No good sense found, skip the reading.
            if (firsts == -1)
                continue;

            // Kanji, reading and sense are all good. Add a new word entry
            WordEntry *w = new WordEntry;
            words.push_back(w);

            // Frequency set below only after the definitions.
            w->freq = 0;

            w->inf = (k != nullptr ? k->inf : 0) | r->inf;
            w->kanji.copy(k != nullptr ? k->str.constData() : r->str.constData());
            w->kana.copy(r->str.constData());
            w->romaji.copy(romanize(r->str.constData()).constData());
            w->defs.resize(scnt);

            spos = firsts;
            int dpos = 0;

            bool onlykana = true;
            for (; spos != entry.susage; ++spos)
            {
                ImportSElement *s = entry.slist[spos];
                if (s->gusage == 0 || (s->kusage && !inRange(s->krestr.indexOf(k != nullptr ? k->str : r->str), 0, s->kusage - 1)) || (s->rusage && !inRange(s->rrestr.indexOf(r->str), 0, s->rusage - 1)))
                    continue;

                WordDefinition &def = w->defs[dpos++];
                def.attrib = s->attrib;

                if (k != nullptr && onlykana && (def.attrib.notes & (1 << (int)WordNotes::KanaOnly)) == 0)
                    onlykana = false;

                // Find the required length by the glosses. Initialized with
                // the space for the separator characters.
                int gsiz = s->gusage - 1;
                for (int gix = 0; gix != s->gusage; ++gix)
                    gsiz += s->glosses.at(gix).size();

                def.def.setSize(gsiz);
                QChar *data = def.def.data();
                for (int gix = 0; gix != s->gusage; ++gix)
                {
                    const QString &str = s->glosses.at(gix);
                    int strsiz = str.size();
                    memcpy(data, str.constData(), sizeof(QChar) * strsiz);

                    if (gix != s->gusage - 1)
                    {
                        data[strsiz] = GLOSS_SEP_CHAR;
                        data = data + strsiz + 1;
                    }
                }
            }

            // Copy the pos and misc tags to definitions which don't have them
            // and fix the types of verbs and adjectives which have Aux.
            fixDefTypes(k != nullptr ? k->str.constData() : r->str.constData(), w->defs);

            // Words can have a number for both the kanji and the kana form.
            // Both can be used to compute the word frequency, but only, if
            // the current kanji/kana usage is not outdated, and there's no
            // kana-only set at every reading.
            // The frequency also gains the value of the least popular kanji
            // in the kanji form (if the word is not written merely with
            // kana).

            // If a word didn't have kanji written forms, they will use the
            // popularity of the readings.
            // Kana only words use the reading priority as is.
            // 0 kanji and reading popularity has inf of 0.
            // 0 kanji popularity will use reading popularity / 2 + worst kanji freq / 6.
            // 0 reading popularity will use kanji popularity + worst kanji freq / 4.
            // Otherwise add kanji+reading popularity / 2 + worst kanji freq.
            // Kanji freq is used directly from the kanji file. The frequency used is the
            // 2502 - the stored data.

            // Figure out the frequency of the word. Find the least frequent
            // kanji first then add them all together.
            int kanjifreq = 0;
            if (k != nullptr && !onlykana)
            {
                int ks = w->kanji.size();
                for (int ix = 0; ix != ks; ++ix)
                {
                    if (KANJI(w->kanji[ix].unicode()))
                    {
                        int kix = ZKanji::kanjiIndex(w->kanji[ix]);
                        if (ZKanji::kanjis[kix]->frequency > kanjifreq)
                            kanjifreq = ZKanji::kanjis[kix]->frequency;
                    }
                }
            }

            if (kanjifreq != 0)
                kanjifreq = 2502 - kanjifreq;

            if (k != nullptr && k->freq != 0 && !onlykana)
            {
                if (r->freq == 0)
                    w->freq = k->freq + kanjifreq / 4;
                else
                    w->freq = (k->freq + r->freq) / 2 + kanjifreq;
            }
            else if (r->freq != 0)
                w->freq = r->freq / 2 + kanjifreq / 6;
        }
    }

    entry.saved = true;
}

bool DictImport::newKElement()
{
    if (entry.kusage == 100)
        return false;

    if (kcurrent == nullptr || !kcurrent->str.isEmpty())
    {
        if (entry.kusage != entry.klist.size())
        {
            kcurrent = entry.klist[entry.kusage];
            kcurrent->str.clear();
        }
        else
        {
            kcurrent = new ImportKElement;
            entry.klist.push_back(kcurrent);
        }
        ++entry.kusage;
    }

    kcurrent->freq = 0;
    kcurrent->inf = 0;

    return true;
}

bool DictImport::newRElement()
{
    if (entry.rusage == 100)
        return false;

    if (rcurrent == nullptr || !rcurrent->str.isEmpty())
    {
        if (entry.rusage != entry.rlist.size())
        {
            rcurrent = entry.rlist[entry.rusage];
            rcurrent->str.clear();
        }
        else
        {
            rcurrent = new ImportRElement;
            entry.rlist.push_back(rcurrent);
        }
        ++entry.rusage;
    }

    rcurrent->inf = 0;
    rcurrent->freq = 0;
    rcurrent->kusage = 0;

    return true;
}

bool DictImport::newSElement()
{
    if (entry.susage == 255)
        return false;

    if (scurrent == nullptr || scurrent->gusage != 0)
    {
        if (entry.susage != entry.slist.size())
            scurrent = entry.slist[entry.susage];
        else
        {
            scurrent = new ImportSElement;
            entry.slist.push_back(scurrent);
        }
        ++entry.susage;
    }

    scurrent->gusage = 0;
    scurrent->attrib = WordDefAttrib();
    scurrent->kusage = 0;
    scurrent->rusage = 0;

    return true;
}

void DictImport::saveKElement()
{
    if (kcurrent != nullptr && kcurrent->str.isEmpty())
    {
        kcurrent = nullptr;
        --entry.kusage;
    }
}

void DictImport::saveRElement()
{
    if (rcurrent != nullptr && rcurrent->str.isEmpty())
    {
        rcurrent = nullptr;
        --entry.rusage;
    }
}

void DictImport::saveSElement()
{
    if (scurrent != nullptr && scurrent->glosses.isEmpty())
    {
        scurrent = nullptr;
        --entry.susage;
    }
}

bool DictImport::addKeb(QString str)
{
    if (!kcurrent || !kcurrent->str.isEmpty() || !str.endsWith("</keb>"))
        return false;

    kcurrent->str = str.mid(5, str.size() - 6 - 5);
    int s = kcurrent->str.size();
    if (s > 255)
        return false;
    for (int ix = 0; ix != s; ++ix)
    {
        ushort c = kcurrent->str.at(ix).unicode();
        if (!UNICODE_J(c) && !KANJI(c))
        {
            kcurrent->str.clear();
            return true;
        }
    }

    return true;
}

bool DictImport::addKInf(QString str)
{
    if (!kcurrent || !str.endsWith(";</ke_inf>"))
        return false;

    QStringRef s = str.midRef(9, str.size() - 9 - 10);

    if (s == "ateji")
        kcurrent->inf |= (1 << (int)WordInfo::Ateji);
    else if (s == "gikun")
        kcurrent->inf |= (1 << (int)WordInfo::Gikun);
    else if (s == "iK")
        kcurrent->inf |= (1 << (int)WordInfo::IrregKanji);
    else if (s == "ik")
        kcurrent->inf |= (1 << (int)WordInfo::IrregKana);
    else if (s == "io")
        kcurrent->inf |= (1 << (int)WordInfo::IrregOku);
    else if (s == "oK")
        kcurrent->inf |= (1 << (int)WordInfo::OutdatedKanji);
    else if (s == "ok")
        kcurrent->inf |= (1 << (int)WordInfo::OutdatedKana);

    return true;
}

bool DictImport::addKPri(QString str)
{
    if (!kcurrent || !str.endsWith("</ke_pri>"))
        return false;

    QString pri = str.mid(8, str.size() - 8 - 9);
    // Valid values:
    // news1, news2, ichi1, ichi2, spec1, spec2, gai1, gai2
    // Also valid values nfxx, where the xx is a number. The lower number
    // indicates more frequent words.

    // The points assigned to these were decided arbitrary by me, based my own
    // experience with a lot of testing and adjusting.

    if (pri == "news1" || pri == "spec1")
        kcurrent->freq += 1000;
    else if (pri == "news2" || pri == "spec2")
        kcurrent->freq += 500;
    else if (pri == "ichi1")
        kcurrent->freq += 2000;
    else if (pri == "ichi2")
        kcurrent->freq += 1000;
    else if (pri.size() == 4 && pri.startsWith("nf"))
    {
        bool ok;
        int val = pri.right(2).toInt(&ok);
        if (ok)
            kcurrent->freq += (50 - val) * 25;
    }

    return true;
}

bool DictImport::addReb(QString str)
{
    if (!rcurrent || !rcurrent->str.isEmpty() || !str.endsWith("</reb>"))
        return false;
    bool haskana = false;
    for (int ix = 5; ix != str.size() - 6; ++ix)
    {
        haskana = haskana || KANA(str.at(ix).unicode());
        ushort c = str.at(ix).unicode();
        if ((!VALIDKANA(c) && !MIDDOT(c)) || (ix != 5 && DASH(c) && !KANA(str.at(ix - 1).unicode())))
            return true;
    }
    if (!haskana)
        return true;
    rcurrent->str = str.mid(5, str.size() - 6 - 5);
    if (rcurrent->str.size() > 100)
        return false;
    return true;
}

bool DictImport::addRRestr(QString str)
{
    if (!rcurrent || rcurrent->kusage == 255 || !str.endsWith("</re_restr>"))
        return false;
    QString k = str.mid(10, str.size() - 10 - 11);
    if (rcurrent->kusage == rcurrent->krestr.size())
        rcurrent->krestr.push_back(k);
    else
        rcurrent->krestr[rcurrent->kusage] = k;
    ++rcurrent->kusage;
    return true;
}

bool DictImport::addRInf(QString str)
{
    if (!rcurrent || !str.endsWith(";</re_inf>"))
        return false;

    QStringRef s = str.midRef(9, str.size() - 9 - 10);

    if (s == "ateji")
        rcurrent->inf |= (1 << (int)WordInfo::Ateji);
    else if (s == "gikun")
        rcurrent->inf |= (1 << (int)WordInfo::Gikun);
    else if (s == "iK")
        rcurrent->inf |= (1 << (int)WordInfo::IrregKanji);
    else if (s == "ik")
        rcurrent->inf |= (1 << (int)WordInfo::IrregKana);
    else if (s == "io")
        rcurrent->inf |= (1 << (int)WordInfo::IrregOku);
    else if (s == "oK")
        rcurrent->inf |= (1 << (int)WordInfo::OutdatedKanji);
    else if (s == "ok")
        rcurrent->inf |= (1 << (int)WordInfo::OutdatedKana);

    return true;
}

bool DictImport::addRPri(QString str)
{
    if (!rcurrent || !str.endsWith("</re_pri>"))
        return false;


    QString pri = str.mid(8, str.size() - 8 - 9);
    // Valid values:
    // news1, news2, ichi1, ichi2, spec1, spec2, gai1, gai2
    // Also valid values nfxx, where the xx is a number. The lower number
    // indicates more frequent words.

    // The points assigned to these were decided arbitrary by me, based my own
    // experience with a lot of testing and adjusting.

    if (pri == "news1" || pri == "spec1")
        rcurrent->freq += 500;
    else if (pri == "news2" || pri == "spec2")
        rcurrent->freq += 250;
    else if (pri == "ichi1")
        rcurrent->freq += 2000;
    else if (pri == "ichi2")
        rcurrent->freq += 1000;
    else if (pri.size() == 4 && pri.startsWith("nf"))
    {
        bool ok;
        int val = pri.right(2).toInt(&ok);
        if (ok)
            rcurrent->freq += (50 - val) * 25;
    }

    return true;
}

bool DictImport::addSTagK(QString str)
{
    if (!scurrent || scurrent->kusage == 255 || !str.endsWith("</stagk>"))
        return false;
    QString k = str.mid(7, str.size() - 7 - 8);
    if (scurrent->kusage == scurrent->krestr.size())
        scurrent->krestr.push_back(k);
    else
        scurrent->krestr[scurrent->kusage] = k;
    ++scurrent->kusage;
    return true;
}

bool DictImport::addSTagR(QString str)
{
    if (!scurrent || scurrent->rusage == 255 || !str.endsWith("</stagr>"))
        return false;
    QString r = str.mid(7, str.size() - 7 - 8);
    if (scurrent->rusage == scurrent->rrestr.size())
        scurrent->rrestr.push_back(r);
    else
        scurrent->rrestr[scurrent->rusage] = r;
    ++scurrent->rusage;
    return true;
}

bool DictImport::addSPos(QString str)
{
    if (!scurrent || !str.endsWith(";</pos>"))
        return false;
    QStringRef s = str.midRef(6, str.size() - 6 - 7);

    if (s == "adj-f")
        scurrent->attrib.types |= (1 << (int)WordTypes::PrenounAdj);
    else if (s == "adj-i" || s == "adj-ix")
        scurrent->attrib.types |= (1 << (int)WordTypes::TrueAdj);
    else if (s == "adj-ku" || s == "adj-shiku")
        scurrent->attrib.types |= (1 << (int)WordTypes::ArchaicAdj);
    else if (s == "adj-na")
        scurrent->attrib.types |= (1 << (int)WordTypes::NaAdj);
    else if (s == "adj-nari")
        scurrent->attrib.types |= (1 << (int)WordTypes::ArchaicNa);
    else if (s == "adj-no")
        scurrent->attrib.types |= (1 << (int)WordTypes::MayTakeNo);
    else if (s == "adj-pn")
        scurrent->attrib.types |= (1 << (int)WordTypes::PrenounAdj);
    else if (s == "adj-t")
        scurrent->attrib.types |= (1 << (int)WordTypes::Taru);
    else if (s == "adv")
        scurrent->attrib.types |= (1 << (int)WordTypes::Adverb);
    else if (s == "adv-to")
        scurrent->attrib.types |= ((1 << (int)WordTypes::Adverb) | (1 << (int)WordTypes::Taru));
    else if (s == "aux" || s == "aux-v")
        scurrent->attrib.types |= (1 << (int)WordTypes::Aux);
    else if (s == "aux-adj")
        scurrent->attrib.types |= (1 << (int)WordTypes::Aux);
    else if (s == "conj")
        scurrent->attrib.types |= ((1 << (int)WordTypes::Aux) | (1 << (int)WordTypes::Conjunction));
    else if (s == "cop-da")
        scurrent->attrib.types |= (1 << (int)WordTypes::CopulaDa);
    else if (s == "ctr")
        scurrent->attrib.types |= (1 << (int)WordTypes::Counter);
    else if (s == "exp")
        scurrent->attrib.types |= (1 << (int)WordTypes::Expression);
    else if (s == "int")
        scurrent->attrib.types |= (1 << (int)WordTypes::Interjection);
    else if (s == "n" || s == "n-pr" || s == "n-t")
        scurrent->attrib.types |= (1 << (int)WordTypes::Noun);
    else if (s == "n-adv")
        scurrent->attrib.types |= ((1 << (int)WordTypes::Noun) | (1 << (int)WordTypes::Adverb));
    else if (s == "n-pref")
        scurrent->attrib.types |= ((1 << (int)WordTypes::Noun) | (1 << (int)WordTypes::Prefix));
    else if (s == "n-suf")
        scurrent->attrib.types |= ((1 << (int)WordTypes::Noun) | (1 << (int)WordTypes::Suffix));
    else if (s == "pn")
        scurrent->attrib.types |= (1 << (int)WordTypes::Pronoun);
    else if (s == "pref")
        scurrent->attrib.types |= (1 << (int)WordTypes::Prefix);
    else if (s == "prt")
        scurrent->attrib.types |= (1 << (int)WordTypes::Particle);
    else if (s == "suf")
        scurrent->attrib.types |= (1 << (int)WordTypes::Suffix);
    else if (s == "v1" || s == "v1-s")
        scurrent->attrib.types |= (1 << (int)WordTypes::IchidanVerb);
    else if (s == "v2a-s" || (s.size() == 5 && s.startsWith("v2") && (s.endsWith("-k") || s.endsWith("-s"))))
    {
        //scurrent->attrib.types |= (1 << (int)WordTypes::NidanVerb);
        scurrent->attrib.types |= (1 << (int)WordTypes::ArchaicVerb);
        scurrent->attrib.notes |= (1 << (int)WordNotes::Archaic);
    }
    else if (s.size() == 3 && s.startsWith("v4"))
    {
        //scurrent->attrib.types |= (1 << (int)WordTypes::YodanVerb);
        scurrent->attrib.types |= (1 << (int)WordTypes::ArchaicVerb);
        scurrent->attrib.notes |= (1 << (int)WordNotes::Archaic);
    }
    else if (s == "v5aru")
        scurrent->attrib.types |= (1 << (int)WordTypes::AruVerb);
    else if ((s.size() == 3 && s.startsWith("v5")) || s == "v5r-i" || s == "v5u-s" || s == "vn")
        scurrent->attrib.types |= (1 << (int)WordTypes::GodanVerb);
    else if (s == "v5k-s")
        scurrent->attrib.types |= (1 << (int)WordTypes::IkuVerb);
    else if (s == "vi")
        scurrent->attrib.types |= (1 << (int)WordTypes::Intransitive);
    else if (s == "vk")
        scurrent->attrib.types |= (1 << (int)WordTypes::KuruVerb);
    else if (s == "vr")
        scurrent->attrib.types |= (1 << (int)WordTypes::RiVerb);
    else if (s == "vs")
        scurrent->attrib.types |= (1 << (int)WordTypes::TakesSuru);
    else if (s == "vs-c")
        // Ancestor of suru
        scurrent->attrib.types |= (1 << (int)WordTypes::GodanVerb);
    else if (s == "vs-i" || s == "vs-s")
        scurrent->attrib.types |= (1 << (int)WordTypes::SuruVerb);
    else if (s == "vt")
        scurrent->attrib.types |= (1 << (int)WordTypes::Transitive);
    else if (s == "vz")
        scurrent->attrib.types |= (1 << (int)WordTypes::ZuruVerb);

    return true;
}

bool DictImport::addSField(QString str)
{
    if (!scurrent || !str.endsWith(";</field>"))
        return false;

    QStringRef s = str.midRef(8, str.size() - 8 - 9);

    if (s == "Buddh")
        scurrent->attrib.fields |= (1 << (int)WordFields::Buddhism);
    else if (s == "MA")
        scurrent->attrib.fields |= (1 << (int)WordFields::MartialArts);
    else if (s == "Shinto")
        scurrent->attrib.fields |= (1 << (int)WordFields::Shinto);
    else if (s == "anat")
        scurrent->attrib.fields |= (1 << (int)WordFields::Anatomy);
    else if (s == "archit")
        scurrent->attrib.fields |= (1 << (int)WordFields::Architecture);
    else if (s == "astron")
        scurrent->attrib.fields |= (1 << (int)WordFields::Astronomy);
    else if (s == "baseb")
        scurrent->attrib.fields |= (1 << (int)WordFields::Baseball);
    else if (s == "biol")
        scurrent->attrib.fields |= (1 << (int)WordFields::Biology);
    else if (s == "bot")
        scurrent->attrib.fields |= (1 << (int)WordFields::Botany);
    else if (s == "bus")
        scurrent->attrib.fields |= (1 << (int)WordFields::Business);
    else if (s == "chem")
        scurrent->attrib.fields |= (1 << (int)WordFields::Chemistry);
    else if (s == "comp")
        scurrent->attrib.fields |= (1 << (int)WordFields::Computing);
    else if (s == "econ")
        scurrent->attrib.fields |= (1 << (int)WordFields::Economics);
    else if (s == "engr")
        scurrent->attrib.fields |= (1 << (int)WordFields::Engineering);
    else if (s == "finc")
        scurrent->attrib.fields |= (1 << (int)WordFields::Finance);
    else if (s == "food")
        scurrent->attrib.fields |= (1 << (int)WordFields::Food);
    else if (s == "geol")
        scurrent->attrib.fields |= (1 << (int)WordFields::Geology);
    else if (s == "geom")
        scurrent->attrib.fields |= (1 << (int)WordFields::Geometry);
    else if (s == "law")
        scurrent->attrib.fields |= (1 << (int)WordFields::Law);
    else if (s == "ling")
        scurrent->attrib.fields |= (1 << (int)WordFields::Linguistics);
    else if (s == "mahj")
        scurrent->attrib.fields |= (1 << (int)WordFields::Mahjong);
    else if (s == "math")
        scurrent->attrib.fields |= (1 << (int)WordFields::Math);
    else if (s == "med")
        scurrent->attrib.fields |= (1 << (int)WordFields::Medicine);
    else if (s == "mil")
        scurrent->attrib.fields |= (1 << (int)WordFields::Military);
    else if (s == "music")
        scurrent->attrib.fields |= (1 << (int)WordFields::Music);
    else if (s == "physics")
        scurrent->attrib.fields |= (1 << (int)WordFields::Physics);
    else if (s == "shogi")
        scurrent->attrib.fields |= (1 << (int)WordFields::Shogi);
    else if (s == "sports")
        scurrent->attrib.fields |= (1 << (int)WordFields::Sports);
    else if (s == "sumo")
        scurrent->attrib.fields |= (1 << (int)WordFields::Sumo);
    else if (s == "zool")
        scurrent->attrib.fields |= (1 << (int)WordFields::Zoology);

    return true;
}

bool DictImport::addSMisc(QString str)
{
    if (!scurrent || !str.endsWith(";</misc>"))
        return false;
    QStringRef s = str.midRef(7, str.size() - 7 - 8);

    if (s == "abbr")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Abbrev);
    else if (s == "abbr")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Archaic);
    //else if (s == "ateji")
    //    scurrent->attrib.notes |= (1 << (int)WordNotes::Ateji);
    else if (s == "chn")
        scurrent->attrib.notes |= (1 << (int)WordNotes::ChildLang);
    else if (s == "col")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Colloquial);
    else if (s == "derog")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Derogatory);
    else if (s == "fam")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Familiar);
    else if (s == "fem")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Female);
    //else if (s == "gikun")
    //    scurrent->attrib.notes |= (1 << (int)WordNotes::Gikun);
    else if (s == "hon")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Honorific);
    else if (s == "hum")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Humble);
    //else if (s == "iK")
    //    scurrent->attrib.notes |= (1 << (int)WordNotes::IrregKanji);
    else if (s == "id")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Idiomatic);
    //else if (s == "ik")
    //    scurrent->attrib.notes |= (1 << (int)WordNotes::IrregKana);
    //else if (s == "io")
    //    scurrent->attrib.notes |= (1 << (int)WordNotes::IrregOku);
    else if (s == "joc")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Joking); /* jocular */
    else if (s == "m-sl")
        scurrent->attrib.notes |= (1 << (int)WordNotes::MangaSlang);
    else if (s == "male")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Male);
    //else if (s == "oK")
    //    scurrent->attrib.notes |= (1 << (int)WordNotes::OutdatedKanji);
    else if (s == "obs")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Obsolete);
    else if (s == "obsc")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Obscure);
    //else if (s == "ok")
    //    scurrent->attrib.notes |= (1 << (int)WordNotes::OutdatedKana);
    else if (s == "on-mim")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Onomatopoeia);
    else if (s == "poet")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Poetical);
    else if (s == "pol")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Polite);
    else if (s == "proverb")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Proverb);
    else if (s == "rare")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Rare);
    else if (s == "sens")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Sensitive);
    else if (s == "sl")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Slang);
    else if (s == "uk")
        scurrent->attrib.notes |= (1 << (int)WordNotes::KanaOnly);
    else if (s == "vulg")
        scurrent->attrib.notes |= (1 << (int)WordNotes::Vulgar);
    else if (s == "yoji")
        scurrent->attrib.notes |= (1 << (int)WordNotes::FourCharIdiom);
    else if (s == "male-sl") // Not used in current JMdict but listed in the file - 20150824
        scurrent->attrib.notes |= (1 << (int)WordNotes::Male) | (1 << (int)WordNotes::Slang);

    return true;
}

bool DictImport::addSDial(QString str)
{
    if (!scurrent || !str.endsWith(";</dial>"))
        return false;
    QStringRef s = str.midRef(7, str.size() - 7 - 8);

    if (s == "kyb")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::KyotoBen);
    else if (s == "osb")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::OsakaBen);
    else if (s == "ksb")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::KansaiBen);
    else if (s == "ktb")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::KantouBen);
    else if (s == "tsb")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::TosaBen);
    else if (s == "thb")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::TouhokuBen);
    else if (s == "tsug")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::TsugaruBen);
    else if (s == "kyu")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::KyuushuuBen);
    else if (s == "rkb")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::RyuukyuuBen);
    else if (s == "nab")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::NaganoBen);
    else if (s == "hob")
        scurrent->attrib.dialects |= (1 << (int)WordDialects::HokkaidouBen);

    return true;
}

bool DictImport::addGloss(QString str)
{
    if (!scurrent || scurrent->gusage == 511 || !str.endsWith("</gloss>"))
        return false;
    // The gloss tag is longer if a lang is provided. Looking for the first
    // closing > is safe because we checked the string before coming here.
    int p = str.indexOf(">");
    QString g = str.mid(p + 1, str.size() - 8 - (p + 1));
    if (scurrent->gusage == scurrent->glosses.size())
        scurrent->glosses.push_back(g);
    else
        scurrent->glosses[scurrent->gusage] = g;
    ++scurrent->gusage;
    return true;
}


//-------------------------------------------------------------
