/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef IMPORT_H
#define IMPORT_H

#include <QMainWindow>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QSet>

#include <qeventloop.h>

#include "smartvector.h"
#include "words.h"
#include "dialogwindow.h"

namespace Ui {
    class DictImport;
}

// The pointers inside the structures' vectors are managed by the importer in a smartvector.


struct ImportSElement
{
    // Number of glosses in this sense for the current entry.
    int gusage;
    // Sense glosses. They are never deleted during import. Use kusage instead
    // of the size of this list.
    QStringList glosses;

    WordDefAttrib attrib;

    // Counter to see how many readings have this sense added. Used when
    // deleting unwanted senses after an error, so only those get deleted
    // which have no parent.
    //uchar parentcnt;

    // Number of items in krestr usable for the current entry.
    int kusage;
    // Written forms the sense is valid for. They are never deleted
    // during import. Use kusage instead of the size of this list.
    QStringList krestr;
    // Number of items in rrestr usable for the current entry.
    int rusage;
    // Reading forms the sense is valid for. They are never deleted
    // during import. Use rusage instead of the size of this list.
    QStringList rrestr;

};

struct ImportRElement
{
    QString str;
    uchar inf;
    int freq;

    // Number of elements in krestr that were added for the current reading.
    int kusage;
    // Written forms the reading is valid for. The readings are never deleted
    // during import. Use usage instead of the size of this list.
    QStringList krestr;

};

struct ImportKElement
{
    QString str;
    uchar inf;
    int freq;
};

struct ImportEntry
{
    // Set to true after saving the entry. Unsaved entries and all the
    // inner data are deleted if newEntry is called and the entry is not
    // saved.
    bool saved;

    // Number of elements in klist that were added for this entry.
    int kusage;
    // List of kanji elements. The elements are never deleted, just updated
    // when reading the next entry, so the size of klist is not meaningful
    // for the current entry. Use kusage instead.
    smartvector<ImportKElement> klist;

    // Number of elements in rlist that were added for this entry.
    int rusage;
    // List of reading elements. The elements are never deleted, just updated
    // when reading the next entry, so the size of rlist is not meaningful
    // for the current entry. Use rusage instead.
    smartvector<ImportRElement> rlist;

    // Number of elements in slist that were added for this entry.
    int susage;
    // Sense elements added to the entry. The senses are never deleted
    // during import. Use usage instead of the size of this list.
    smartvector<ImportSElement> slist;
};

class ImportFileHandler;
// Closes the file opened by ImportFileHandler when it's destroyed. It's safe to use multiple
// guards, only the last one will close the file. If the file is manually closed, the guards
// will stop functioning on it and new ones should be created.
class ImportFileHandlerGuard final
{
public:
    ImportFileHandlerGuard() = delete;
    ImportFileHandlerGuard(const ImportFileHandlerGuard&) = delete;
    ImportFileHandlerGuard(ImportFileHandlerGuard&&) = delete;
    ImportFileHandlerGuard& operator=(const ImportFileHandlerGuard&) = delete;
    ImportFileHandlerGuard& operator=(ImportFileHandlerGuard&&) = delete;

    ImportFileHandlerGuard(ImportFileHandler &file);
    ~ImportFileHandlerGuard();

    // Call to disable the guard. When destroyed, the file won't be closed.
    void disable();
private:
    // The file to close on destruction. When the file is manually closed, this is set to null
    // to avoid closing it when a new file is opened.
    ImportFileHandler *file;
};

// Class used for opening and reading from a file. Keeps track of the current line number,
// that can be used in error reporting. Opens files in ReadOnly and Text modes.
class ImportFileHandler
{
public:
    ImportFileHandler();
    // Opens the file with the passed name. Call error() to check whether an error occurred.
    ImportFileHandler(QString fname);

    ~ImportFileHandler();

    // Opens the file with the passed name and format (if codec is set). Returns false on
    // error. Calling error() to check whether an error occurred is valid after this call.
    // By default the codec is UTF-8.
    bool open(QString fname, const char *codec = nullptr);

    // Sets an already open file to be used for reading. The file must be opened with Read and
    // Text modes. To use the inner file again, use open().
    // Warning: the file won't be closed when the ImportFileHandler is destroyed or a new file
    // is opened or set. Call close() either on this object or on the passed file.
    void setFile(QFile &file, const char *codec = nullptr);

    // Closes the file if it's open, resetting the object. Call open() afterwards in case the
    // file object should be used again.
    void close();

    // Returns whether a file is currently open and either ready to read or at the end.
    bool isOpen() const;
    // Returns the name of the currently open file.
    QString fileName() const;
    // Returns the number of the last line read. Numbering starts at 1.
    int lineNumber() const;

    // Call when a new line has been read already, but it should be returned by the next
    // getLine() call instead of reading the next line. Calling getLine() again will read
    // normally.
    void repeat();

    // Returns whether opening the file didn't succeed.
    bool error() const;

    // File size of the opened file.
    qint64 size() const;
    // File position in the opened file.
    int pos() const;
    // Reads the next line into result and returns true if a line was read. Sets result to an
    // empty string and returns false when there was nothing more to read, because we reached
    // the end of the file.
    bool getLine(QString &result);
    // Returns the last line read with getLine().
    QString lastLine() const;
    //// Returns whether there are no more lines to read.
    //bool atEnd() const;
private:
    // Puts the guard in the list of guards.
    void addGuard(ImportFileHandlerGuard *guard);
    // Removes the guard from the list of guards, but doesn't close the file.
    void disableGuard(ImportFileHandlerGuard *guard);
    // Disables all guards without closing the file.
    void disableGuards();
    // Removes the guard from the list of guards. When the last one is removed, the file is
    // closed.
    void guardClose(ImportFileHandlerGuard *guard);

    // True when error occured.
    bool fail;

    QFile *f;
    bool ownfile;

    QTextStream stream;

    // Current line number.
    int linenum;
    // Last read line.
    QString line;
    // Should return line instead of reading from the file again.
    bool skipread;

    QSet<ImportFileHandlerGuard*> guards;

    friend class ImportFileHandlerGuard;
};

struct WordEntry;
struct ExampleWordsData;
class Dictionary;
class WordGroup;
class KanjiGroup;
template<typename T>
class GroupCategory;
typedef GroupCategory<WordGroup>    WordGroupCategory;
typedef GroupCategory<KanjiGroup>   KanjiGroupCategory;
class DictImport : public DialogWindow
{
    Q_OBJECT
public:
    DictImport(QWidget *parent = nullptr);
    ~DictImport();

    // Reads the importable files (JMdict, radkfile etc.) from path, and returns the
    // dictionary. Set full to true to import kanji or commons data as well. Returns null on
    // failure.
    Dictionary* importDict(QString path, bool full);

    // Reads zkanji exported dictionary file from path, and returns a dictionary made from the
    // file contents. Returns null on failure.
    Dictionary* importFromExport(QString path);

    // Reads zkanji exported dictionary file from path, and returns a dictionary made from the
    // file contents. Returns null on failure.
    bool importFromExportPartial(QString path, Dictionary *dest, WordGroup *worddest, KanjiGroup *kanjidest);

    // Reads examples.utf at path and creates an example sentences database file examples.zkj
    // at outpath. Returns true on success and false on error.
    bool importExamples(QString path, QString outpath, Dictionary *d);

    // Reads user groups and study meanings or kanji example words from file at path, creating
    // the groups in kanjiroot and wordsroot as root with same group path as in the file.
    // Existing groups don't get recreated.
    bool importUserData(const QString &path, Dictionary *dict, KanjiGroupCategory *kanjiroot, WordGroupCategory *wordsroot, bool kanjiexamples, bool wordsmeanings);

    // Lets the application handle user input and painting at every given
    // number of calls. Set forced to true to force the update.
    // Returns true if the user hasn't clicked the close button of the window
    // during its update.
    bool nextUpdate(int progress = -1, bool forced = false);
signals:
    void closed();
protected:
    virtual void closeEvent(QCloseEvent *e) override;
    virtual bool event(QEvent *e) override;
private slots:
    void closeAfterImport();
private:
    // Imports the examples file. Returns false if the import was interrupted.
    bool doImportExamples();
    // Adds a single sentence to the buffer.
    void doImportExamplesSentenceHelper(std::vector<uchar> &buff, const QString &jpn, const QString &trans, const std::vector<ExampleWordsData> &words);

    // Helper function for doImportExamplesSentence() for writing ushorts in
    // little endian byte order in dat at pos. Increments pos by 2.
    // WARNING: when passing data from a vector, make sure it has enough 
    // space.
    void addShort(uchar *dat, int &pos, quint16 w);
    // Helper function for doImportExamplesSentence() for writing int in
    // little endian byte order in dat at pos. Increments pos by 4.
    // WARNING: when passing data from a vector, make sure it has enough 
    // space.
    void addInt(uchar *dat, int &pos, quint32 w);
    // Helper function for doImportExamplesSentence() for writing QByteArrays
    // with their size in dat at pos. Pos is incremented by the bytes written.
    // WARNING: when passing data from a vector, make sure it has enough 
    // space.
    void addByteArrayString(uchar *dat, int &pos, const QByteArray &str);

    // Imports the files by calling the functions below. Returns false if the
    // import was interrupted by the user.
    bool doImportDict();

    // Used by importKanjidic() internally. If str starts with 1 or 2
    // alphabet characters and ends with a number, sets ch and ch2 to the
    // characters and num to the number, returning true. Otherwise returns
    // false. ch2 is set to a 0 character if str only had 1.
    bool letterAndNumber(const QString &str, QChar &ch, QChar &ch2, int &num, int base = 10);
    // Imports kanjidic. (At this point the old data is not loaded yet.)
    bool importKanjidic();
    // Imports radkfile and zradfile.
    bool importRadFiles();
    // Imports JMdict into a new dictionary.
    Dictionary* importJMdict();
    // Imports the jlpt N data from JLPTNData.txt
    bool importJLPTN(Dictionary *dict);

    // Imports the exported dictionary from the path saved previously. Returns whether the
    // import was a success.
    bool doImportFromExport();
    // Imports the exported dictionary from the path saved previously. Returns whether the
    // import was a success. Sets the passed dictionary to null on abort or failure.
    bool doImportFromExportPartial();
    // Helper for doImportFromExport() and doImportFromExportPartial(). Reads in a single word
    // paragraph in the [Words] section, and builds a word from it.
    // Sets the error text when the line format is wrong, which can be checked by calling
    // isErrorSet().
    // Returns nullptr if the next line doesn't start with word data or an error occured.
    // Wordindex should be the index of the word in the future dictionary's words list.
    WordEntry* importWord();
    // Helper for doImportFromExport() and doImportFromExportPartial(). Reads in a line from
    // the kanjis section, setting [kanji index, kanji definitions] in result for the line.
    // Sets the error text when the line format is wrong, which can be checked by calling
    // isErrorSet().
    // Returns false if an error occured or the next line will be the start of a new section.
    bool importKanjiDefinition(std::pair<ushort, QStringList> &result);

    // Imports the exported user data from path and creates groups in the specified roots.
    bool doImportUserData();

    // Changes the large font text informing the user about the operation taking place.
    void setMainText(const QString &str);

    // Changes the large font text informing the user about the operation taking place, while
    // also setting modified state to true. This informs the user that aborting the import
    // will not restore the original data.
    void setModifiedText(const QString &str);

    // Updates the info text and returns true if the user hasn't clicked the close button of
    // the window during this update.
    bool setInfoText(const QString &str);

    // Updates the info text with an error message, including the current file name and line
    // number, and allowing the user to close the window by pressing the button. 
    void setErrorText(const QString &str);
    // Returns true if setErrorText() has been called once.
    bool isErrorSet();

    // Returns the string between [] characters in str. If the string doesn't start and end
    // with those characters, an empty string is returned instead.
    QString sectionName(const QString &str) const;

    // Reads from file until the current import section (marked by []) is skipped. Returns
    // false if the import was aborted during the read.
    bool skipSection();

    // Reads the strings of "kanji(kana)" from str starting at pos position. Sets the results
    // in kanji and kana respectively. Endpos is the first character position in str coming
    // after the closing ) of the kana part. Returns false if an error occurred or on user
    // abort.
    bool kanjiKana(const QString &str, int pos, QString &kanji, QString &kana, int &endpos);

    // Clears any cached word entry data. Call saveEntry() before this if the
    // word entry had no errors before the closing tag. Otherwise the unsaved
    // entry will be lost.
    void newEntry();
    // Saves the entry if enough information is found to insert it in the
    // dictionary.
    void saveEntry();
    // Deletes any data of the current entry if it's unsaved.
    //void clearEntryCache();
    // Creates a new element for the written word part.
    bool newKElement();
    // Creates a new temporary element for the reading part. If a previous
    // temporary reading element exists it is deleted.
    bool newRElement();
    // Creates a new temporary element for the sense part. If a previous
    // temporary sense element exists it is deleted.
    bool newSElement();
    // Cleanup after the kanji element <keb> is closed.
    void saveKElement();
    // Cleanup after the kana element <reb> is closed.
    void saveRElement();
    // Cleanup after the sense element is closed.
    void saveSElement();


    // Functions called inside kanji, reading or sense parts to add new data.

    bool addKeb(QString str);
    bool addKInf(QString str);
    bool addKPri(QString str);
    bool addReb(QString str);
    bool addRRestr(QString str);
    bool addRInf(QString str);
    bool addRPri(QString str);
    bool addSTagK(QString str);
    bool addSTagR(QString str);
    bool addSPos(QString str);
    bool addSField(QString str);
    bool addSMisc(QString str);
    bool addSDial(QString str);
    bool addGloss(QString str);

    Ui::DictImport *ui;

    ImportFileHandler file;

    //QEventLoop loop;

    // Whether the import is at a point where the dictionary or user data is already modified,
    // and it can't be restored when closing the import midway.
    bool modified;

    QString lang;
    QString path;
    QString outpath;
    bool fullimport;

    int stepcnt;
    int step;

    // Current entry.
    ImportEntry entry;
    // Current written part.
    ImportKElement *kcurrent;

    ImportRElement *rcurrent;
    ImportSElement *scurrent;

    smartvector<WordEntry> words;

    Dictionary *dict;
    WordGroup *wordgroup;
    KanjiGroup *kanjigroup;

    KanjiGroupCategory *kanjiroot;
    WordGroupCategory *wordsroot;
    bool kanjiex;
    bool studymeanings;

    enum class Modes { Dictionary, Export, Partial, Examples, User };
    Modes mode;

    // Counts the time nextUpdate was called. Updates only happen when the
    // counter reaches a limit.
    int counter;
     
    typedef DialogWindow base;
};



#endif // IMPORT_H
