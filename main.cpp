/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QStandardPaths>
#include <QLabel>
#include <QLayout>
#include <QSharedMemory>
#include <QStringBuilder>
#include <QTextStream>

//#include <QDesktopWidget>
//#include <QScreen>

#include <initializer_list>
#include <iostream>

#include "zui.h"
#include "zevents.h"
#include "zkanjimain.h"
#include "zkanjiform.h"
#include "grammar.h"
#include "zkanjimain.h"
#include "studydecks.h"
//#include "worddeckform.h"
#include "import.h"
#include "words.h"
#include "kanji.h"
#include "importreplaceform.h"
#include "globalui.h"
#include "sentences.h"
#include "kanjistrokes.h"

#include "grammar_enums.h"
#include "languages.h"
#include "languagesettings.h"
#include "dialogs.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <QLocalServer>
#include <QLocalSocket>
#endif

// Version of program as ansi-text. Must not contain non-latin characters.
//
// Versioning only changes between releases.
char ZKANJI_PROGRAM_VERSION[] = "v0.1.0 (beta)";

int showAndQuit(QString title, QString text)
{
    QTimer timer;
    timer.setSingleShot(true);
    timer.connect(&timer, &QTimer::timeout, [&]() {
        QMessageBox msg;
        msg.setWindowTitle(title);
        msg.setText(text);
        msg.exec();

        qApp->exit(-1);
    });
    timer.start(1000);

    exit(qApp->exec());
}

namespace
{
    void showSimpleDialog(QString title, QString text)
    {
        QMessageBox msg;
        msg.setWindowTitle(title);
        msg.setText(text);
        msg.exec();
    }

    // Returns whether the given path exists, contains previously saved user data, and can be
    // used for writing.
    bool existsAndWritable(QString path)
    {
        if (path.isEmpty())
            return false;

        NTFSPermissionGuard permissionguard;

        if (QFileInfo::exists(path + "/zkanji.ini"))
        {
            QFileInfo finf(path + "/zkanji.ini");
            if (!finf.isWritable() || !finf.isReadable())
                return false;

            finf.setFile(path + "/data/");
            if (finf.isWritable())
                return true;
        }

        if (QFileInfo::exists(path + "/data/English.zkdict") && QFileInfo::exists(path + "/data/English.zkuser"))
        {
            QFileInfo finf(path + "/data/English.zkdict");
            if (!finf.isWritable() || !finf.isReadable())
                return false;
            finf.setFile(path + "/data/English.zkuser");
            if (!finf.isWritable() || !finf.isReadable())
                return false;

            finf.setFile(path + "/data/");
            if (finf.isWritable())
                return true;
        }

        return false;
    }

    bool exists(QString path)
    {
        if (path.isEmpty())
            return false;

        return QFileInfo::exists(path + "/data/English.zkdict") && QFileInfo::exists(path + "/data/English.zkuser");
    }

    // Returns whether a data sub-dir (or the whole directory tree) can be created at the given location,
    // or it exists and is writable. We assume user data is not already there.
    bool accessibleDataPath(QString path)
    {
        NTFSPermissionGuard permissionguard;

        if (QFileInfo::exists(path % "/data"))
            path = path % "/data";
        if (QFileInfo::exists(path))
        {
            QFileInfo fi(path);

            if (!fi.isDir() || !fi.isWritable() || !fi.isReadable())
                return false;
            return true;
        }

        do
        {
            int ix = path.lastIndexOf('/');
            if (ix == -1)
                return false;
            path = path.left(ix);
            if (path.isEmpty())
                return false;
        } while (!QFileInfo::exists(path));
        QFileInfo fi(path);

        if (!fi.isDir() || !fi.isWritable() || !fi.isReadable())
            return false;
        return true;
    }

    void checkAppFolder()
    {
        NTFSPermissionGuard permissionguard;

        // No data in the app folder either means incorrect install or pending import. 
        if (!QFileInfo::exists(ZKanji::appFolder() + "/data/zdict.zkj"))
        {
            ZKanji::setNoData(true);

            QFileInfo adir = QFileInfo(ZKanji::appFolder() + "/data");
            if (!adir.exists() || !adir.isDir() || !adir.isWritable() || !adir.isReadable())
                showAndQuit(qApp->translate("", "Error starting zkanji"), qApp->translate("", "The file containing the dictionary and other data is not found at the program's location. Quitting... (2)"));
        }
    }

    // If a valid setting is found, the program uses that as UI language, otherwise shows a
    // language select dialog. Only executes once.
    void loadLanguageSetting(QString basedir)
    {
        static bool executed = false;
        if (executed)
            return;
        executed = true;

        QSettings ini(basedir + "/zkanji.ini", QSettings::IniFormat);

        Settings::language = ini.value("language", "INVALID LANGUAGE ID").toString();
        if (!zLang->setLanguage(Settings::language))
        {
            Settings::language = QString();

            zLang->setLanguage(QLocale::system().language());
            if (!languageSelect())
                exit(0);
        }

        ini.setValue("language", Settings::language);
    }

    // Finds the folder to save user data and returns whether such location was found.
    bool initUserDirectory()
    {
        bool portable = false;
        bool allowportable = false;
        bool allowappdata = false;
        bool appdata = false;
        QString appdatadir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        portable = existsAndWritable(ZKanji::appFolder());
        if (portable)
            ZKanji::setUserFolder(ZKanji::appFolder());
        else
        {
            if (exists(ZKanji::appFolder()))
            {
                // Try to get the error message in a language the user understands. If the
                // appFolder() has data, it's likely it'll have a read-only ini file too.
                loadLanguageSetting(ZKanji::appFolder());

                int result = showAndReturn("zkanji",
                    qApp->translate("", "Warning: data files of a portable installation detected at the application's location, but the location is read only!"),
                    qApp->translate("", "Choose \"%1\" to start the program in non-portable mode.").arg(qApp->translate("", "Continue")),
                    {
                        { qApp->translate("", "Continue"), QMessageBox::AcceptRole },
                        { qApp->translate("", "Quit"), QMessageBox::RejectRole }
                    });
                if (result == 1)
                    exit(0);
            }
            appdata = existsAndWritable(appdatadir);
        }
        if (appdata)
            ZKanji::setUserFolder(appdatadir);
#ifdef Q_OS_WIN
        else if (!portable && existsAndWritable(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)))
        {
            appdata = true;
            appdatadir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
            ZKanji::setUserFolder(appdatadir);
        }
#endif

        if (!portable && !appdata)
        {
            allowportable = accessibleDataPath(ZKanji::appFolder());
            allowappdata = accessibleDataPath(appdatadir);
#ifdef Q_OS_WIN
            if (!allowappdata && accessibleDataPath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)))
            {
                allowappdata = true;
                appdatadir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
            }
#endif

            // Second time we try to show the error message in a language the user
            // understands. The only chance for an ini file is at the appFolder() at this
            // point, but if it's not found, a dialog can still be shown.
            loadLanguageSetting(ZKanji::appFolder());

            if (!allowportable && !allowappdata)
                showAndQuit(qApp->translate("", "Error starting zkanji"), qApp->translate("", "Couldn't determine where to store data generated by the program. Quitting..."));

            if (!allowportable && allowappdata)
                ZKanji::setUserFolder(appdatadir);
            else if (allowportable && !allowappdata)
                ZKanji::setUserFolder(ZKanji::appFolder());
            else
            {
                int result = showAndReturn(qApp->translate("", "Running for the first time."),
                    qApp->translate("", "Would you like to use zkanji in normal mode or in portable mode?"),
                    qApp->translate("", "Choose \"%1\" to store user data at the system default location, and \"%2\" to store it next to the executable.").arg(qApp->translate("", "Normal")).arg(qApp->translate("", "Portable")),
                    {
                        { qApp->translate("", "Normal"), QMessageBox::AcceptRole },
                        { qApp->translate("", "Portable"), QMessageBox::AcceptRole },
                        { qApp->translate("", "Quit"), QMessageBox::RejectRole }
                    });
                if (result == 2)
                    exit(0);

                if (result == 0)
                {
                    appdata = true;
                    ZKanji::setUserFolder(appdatadir);
                }
                else
                {
                    portable = true;
                    ZKanji::setUserFolder(ZKanji::appFolder());
                }
            }
        }

        // Last time we try to find the language for the user if it failed previously. The
        // user folder is already established here.
        loadLanguageSetting(ZKanji::userFolder());

        return portable || appdata;
    }

    bool importolddata = false;

    void importOldData()
    {
        if (!ZKanji::noData())
        {
            // If the zkg and zkd files are found at the default user load location, old user
            // data will be imported automatically. Otherwise ask the user to browse there.

            bool found = false;
            if (QFileInfo::exists(ZKanji::userFolder() + "/data/English.zkd") || QFileInfo::exists(ZKanji::userFolder() + "/data/English.zkg"))
            {
                NTFSPermissionGuard permissionguard;

                found = true;
                QFileInfo finf(ZKanji::userFolder() + "/data/English.zkd");
                if (!finf.isReadable())
                    found = false;
                if (found)
                {
                    finf.setFile(ZKanji::userFolder() + "/data/English.zkg");
                    if (!finf.isReadable())
                        found = false;
                }

                if (found)
                    showSimpleDialog("zkanji", qApp->translate("", "Old dictionary and user data were found at the user data folder. zkanji will attempt to import them."));
            }

            if (!found)
            {
                int result = showAndReturn(qApp->translate("", "Running for the first time."),
                    qApp->translate("", "Zkanji can import user data written by the previous version of the program."),
                    qApp->translate("", "\"%1\" for the location of the old version (the folder containing the \"data\" folder), or \"%2\" without importing.").arg(qApp->translate("", "Browse...")).arg(qApp->translate("", "Continue")),
                    {
                        { qApp->translate("", "Browse..."), QMessageBox::AcceptRole },
                        { qApp->translate("", "Continue"), QMessageBox::AcceptRole },
                        { qApp->translate("", "Quit"), QMessageBox::RejectRole }
                    });
                if (result == 2)
                    exit(0);

                while (result == 0)
                {
                    result = 1;
                    QString str;
                    str = QFileDialog::getExistingDirectory(nullptr, (qApp->translate("", "Browse...")));
                    if (!str.isEmpty())
                    {
                        bool fail = false;
                        if (!QFileInfo::exists(str + "/data/English.zkd") || !QFileInfo::exists(str + "/data/English.zkg"))
                            fail = true;
                        else
                        {
                            NTFSPermissionGuard permissionguard;

                            QFileInfo finf(str + "/data/English.zkd");
                            if (!finf.isReadable())
                                fail = true;
                            finf.setFile(str + "/data/English.zkg");
                            if (!finf.isReadable())
                                fail = true;
                        }

                        if (!fail)
                        {
                            //str = str.left(str.length() - 5);
                            ZKanji::setLoadFolder(str);
                            found = true;
                        }
                        else
                        {
                            result = showAndReturn(qApp->translate("", "Error"),
                                qApp->translate("", "The selected folder is not valid."),
                                qApp->translate("", "Would you like to \"%1\" to select a different folder, \"%2\" without importing or \"%3\" zkanji?").arg(qApp->translate("", "Try again...")).arg(qApp->translate("", "Continue")).arg(qApp->translate("", "Quit")),
                                {
                                    { qApp->translate("", "Try again..."), QMessageBox::AcceptRole },
                                    { qApp->translate("", "Continue"), QMessageBox::AcceptRole },
                                    { qApp->translate("", "Quit"), QMessageBox::RejectRole }
                                });
                            if (result == 2)
                                exit(0);
                        }
                    }
                }
            }

            importolddata = found;
        }
    }

    void loadDictionaries()
    {
        // Creating and loading main dictionary.
        Dictionary *d = ZKanji::addDictionary();
        bool userdir = false;
        // Dictionary data file extension. Different when loading legacy data. Only used when
        // the file we look for is on the loadFolder() location. When loading data from the
        // userFolder() location, it must have the new file extension.
        QString exdict = "zkdict";
        // User data file extension. Different when loading legacy data. Only used when the
        // file we look for is on the loadFolder() location. When loading data from the
        // userFolder() location, it must have the new file extension.
        QString exuser = "zkuser";
        try
        {
            d->loadBaseFile(ZKanji::appFolder() + "/data/zdict.zkj");

            // pre2015 program version string is set here, but overwritten in loadFile() if
            // only the zkj is the old version. It must be checked at both places.
            bool oldver = d->pre2015();

            if (!oldver)
            {
                if (!importolddata && QFileInfo::exists(ZKanji::userFolder() + "/data/English.zkdict"))
                {
                    userdir = true;
                    d->loadFile(ZKanji::loadFolder() + "/data/English.zkdict", true, false);
                }
                else if (importolddata && QFileInfo::exists(ZKanji::loadFolder() + "/data/English.zkd"))
                {
                    userdir = true;

                    exdict = "zkd";
                    exuser = "zkg";

                    d->loadFile(ZKanji::loadFolder() + "/data/English.zkd", true, false);
                }
                else
                {
                    d->loadFile(ZKanji::appFolder() + "/data/English.zkj", true, false);
                    oldver = d->pre2015();
                }
            }

            // One of the zkj files are pre 2015.
            if (oldver)
                showAndQuit(qApp->translate("", "Dictionary version too old"), qApp->translate("", "The base dictionary in the program's data folder is outdated. Please replace it with a newer one.\n\nNote: you can use your old data with the new base dictionary."));
        }
        catch (const ZException &e)
        {
            showAndQuit(qApp->translate("", "Startup Error"), qApp->translate("", "Failed to load base dictionary data. \nError message: %1").arg(e.what()));
        }
        catch (...)
        {
            showAndQuit(qApp->translate("", "Startup Error"), qApp->translate("", "Failed to load base dictionary data."));
        }

        try
        {
            if (userdir && QFileInfo::exists(ZKanji::loadFolder() % "/data/English." % exuser))
                d->loadUserDataFile(ZKanji::loadFolder() % "/data/English." % exuser);
        }
        catch (const ZException &e)
        {
            showAndQuit(qApp->translate("", "Startup Error"), qApp->translate("", "Failed to load user groups for the main dictionary. \nError message: %1").arg(e.what()));
        }
        catch (...)
        {
            showAndQuit(qApp->translate("", "Startup Error"), qApp->translate("", "Failed to load user groups for the main dictionary."));
        }

        ZKanji::elements()->applyElements();


        // Main dictionary loaded. Checking whether the installed dictionary and the loaded
        // dictionaries are the same. If this is not the case, the new installed dictionary
        // replaces the old one.
        if (userdir)
        {
            QDateTime wdate = Dictionary::fileWriteDate(ZKanji::appFolder() + "/data/English.zkj");
            if (!wdate.isValid())
                showAndQuit(qApp->translate("", "Data error"), qApp->translate("", "Date of installed dictionary or the file itself is invalid."));

            if (wdate != d->lastWriteDate())
            {
                if (d->pre2015() && showAndReturn("zkanji", qApp->translate("", "Because your data was made for a different version of zkanji, it must be updated."),
                    qApp->translate("", "Press \"%1\" to update your groups and study data to use the new program version. This program does not support the outdated dictionary. If you wish to continue using it, press \"%2\" and install the last version of the program you were previously using.\nNote: suspended tests can be updated too, but their suspended status will be lost.").arg(qApp->translate("", "Continue")).arg(qApp->translate("", "Quit")),
                    {
                        { qApp->translate("", "Continue"), QMessageBox::AcceptRole },
                        { qApp->translate("", "Quit"), QMessageBox::RejectRole }
                    }) == 1)
                    exit(-1);

                // The installed zkj file is different.
                if (d->pre2015() ||
                    showAndReturn("zkanji", qApp->translate("", "A new base dictionary has been installed."),
                    qApp->translate("", "Press \"%1\" to update your groups and study data to use the new dictionary. If you \"%2\" zkanji will load normally with the old dictionary, and you'll be asked again when zkanji starts the next time.").arg(qApp->translate("", "Continue")).arg(qApp->translate("", "Cancel")),
                    {
                        { qApp->translate("", "Continue"), QMessageBox::AcceptRole },
                        { qApp->translate("", "Cancel"), QMessageBox::RejectRole }
                    }) == 0)
                {
                    ImportReplaceForm irf(d);
                    if (irf.exec())
                    {
                        ZKanji::originals.swap(irf.originals());
                        d->swapDictionaries(irf.dictionary(), irf.changes());

                        if (QFileInfo::exists(ZKanji::userFolder() + "/data/English.zkdict"))
                        {
                            if (!QFile::remove(ZKanji::userFolder() + "/data/English.zkdict"))
                            {
                                showAndQuit(qApp->translate("", "Update not finished"), qApp->translate("", "Couldn't remove old main dictionary data. Make sure its folder exists and is not read-only, and the old file is not write protected."));
                                return;
                            }
                        }

                        if (!QFile::copy(ZKanji::appFolder() + "/data/English.zkj", ZKanji::userFolder() + "/data/English.zkdict"))
                        {
                            showAndQuit(qApp->translate("", "Update not finished"), qApp->translate("", "Couldn't save main dictionary data. Make sure its folder exists and is not read-only, and the old file is not write protected."));
                            return;
                        }

                        if (!d->saveUserData(ZKanji::userFolder() + "/data/English.zkuser"))
                        {
                            showAndQuit(qApp->translate("", "Update not finished"), qApp->translate("", "The user data file for the main dictionary couldn't be saved. The file might be compromised.") % QString("\n\n%1").arg(Error::last()));
                        }
                    }
                    else if (d->pre2015())
                    {
                        showAndQuit(qApp->translate("", "Update not finished"), qApp->translate("", "The program can't run without updated data files. Please try again when you can finish the update."));
                    }
                }
            }
        }


        // Looking for other dictionaries.

        QDir dir(ZKanji::loadFolder() + "/data");
        dir.setNameFilters(QStringList(std::initializer_list<QString>({ "*." % exdict })));
        dir.setFilter(QDir::Files | QDir::Readable);

        QStringList files = dir.entryList();

        QString loaderrors;
        for (QString &filename : files)
        {
            if (filename == QStringLiteral("English.") % exdict)
                continue;

            d = ZKanji::addDictionary();
            QString dictname = filename.left(filename.size() - exdict.size() - 1);

            bool donedict = false;
            bool error = false;
            try
            {
                d->loadFile(ZKanji::loadFolder() + "/data/" % dictname % "." % exdict, false, true);
                donedict = true;
                d->loadUserDataFile(ZKanji::loadFolder() + "/data/" % dictname % "." % exuser);
            }
            catch (const ZException &e)
            {
                if (!loaderrors.isEmpty())
                    loaderrors += "\n";
                if (!donedict)
                    loaderrors += qApp->translate("", "Error loading dictionary data: %1").arg(dictname);
                else
                    loaderrors += qApp->translate("", "Error loading user data for dictionary: %1").arg(dictname);
                loaderrors += qApp->translate("", " Error message: %1").arg(e.what());
                error = true;
            }
            catch (...)
            {
                if (!loaderrors.isEmpty())
                    loaderrors += "\n";
                if (!donedict)
                    loaderrors += qApp->translate("", "Error loading dictionary data: %1").arg(dictname);
                else
                    loaderrors += qApp->translate("", "Error loading user data for dictionary: %1").arg(dictname);
                error = true;
            }

            if (error || (exdict != "zkdict" && (!d->save(ZKanji::userFolder() + QString("/data/%1.zkdict").arg(dictname)) || !d->saveUserData(ZKanji::userFolder() + QString("/data/%1.zkuser").arg(dictname)))))
            {
                if (!error)
                {

                    if (!loaderrors.isEmpty())
                        loaderrors += "\n";
                    loaderrors += qApp->translate("", "Error saving imported dictionary or user data: %1").arg(filename.left(filename.size() - 4));
                }
                ZKanji::deleteDictionary(ZKanji::dictionaryCount() - 1);
            }
        }

        try
        {
            if (QFileInfo::exists(ZKanji::loadFolder() + "/data/student.zkp"))
                ZKanji::profile().load(ZKanji::loadFolder() + "/data/student.zkp");
            else if (QFileInfo::exists(ZKanji::loadFolder() + "/data/student.zpf"))
                ZKanji::profile().load(ZKanji::loadFolder() + "/data/student.zpf");
            if (ZKanji::loadFolder() != ZKanji::userFolder() || !QFileInfo::exists(ZKanji::userFolder() + "/data/student.zkp"))
                ZKanji::profile().save(ZKanji::userFolder() + "/data/student.zkp");
        }
        catch (const ZException &e)
        {
            ZKanji::profile().clear();
            if (!loaderrors.isEmpty())
                loaderrors += "\n";
            loaderrors += qApp->translate("", "Error loading student profile. Error message: %1").arg(e.what());
        }
        catch (...)
        {
            ZKanji::profile().clear();
            if (!loaderrors.isEmpty())
                loaderrors += "\n";
            loaderrors += qApp->translate("", "Error loading student profile.");
        }

        if (!loaderrors.isEmpty())
        {
            showSimpleDialog("zkanji", qApp->translate("", "Errors occurred during startup. The program will start but without the data causing the problem.\n\n %1").arg(loaderrors));
        }
    }

    void loadRecognizerData()
    {
        if (!QFileInfo::exists(ZKanji::appFolder() + "/data/zdict.zks"))
            return;

        try
        {
            ZKanji::initElements(ZKanji::appFolder() + "/data/zdict.zks");
        }
        catch (const ZException &e)
        {
            showAndQuit("zkanji", qApp->translate("", "Error occured while loading the kanji stroke order diagrams. ZKanji cannot run. Quitting...\n\nError message: %1").arg(e.what()));
        }
        catch (...)
        {
            showAndQuit("zkanji", qApp->translate("", "Error occured while loading the kanji stroke order diagrams. ZKanji cannot run. Quitting..."));
        }
    }

    QString expath;

    void handleArguments(QStringList args)
    {
        if (args.isEmpty())
            return;

        bool ifound = false;
        bool efound = false;


        // Options to look for
        //
        //  import JMDict:
        //  -i [path]
        //
        // import example sentences:
        //  -e [path]
        //
        for (int ix = 0; ix != args.size() - 1; ++ix)
        {
            if ((args[ix] == "-i" || args[ix] == "-ie" || args[ix] == "-ei") && !ifound)
            {
                QString d = args[ix + 1];
#ifdef Q_OS_WIN
                d = d.replace(QChar('\\'), "/");
                if (d[d.size() - 1] == QChar('/'))
                    d = d.left(d.size() - 1);
#endif
                QDir dir(d);

                if (!dir.exists())
                {
                    showAndQuit("zkanji", qApp->translate("", "Import folder does not exist."));
                    continue;
                }

                NTFSPermissionGuard permissionguard;

                if ((QFileInfo(ZKanji::appFolder() + "/data/zdict.zkj").exists() && !QFileInfo(ZKanji::appFolder() + "/data/zdict.zkj").isWritable()) ||
                    (QFileInfo(ZKanji::appFolder() + "/data/English.zkj").exists() && !QFileInfo(ZKanji::appFolder() + "/data/English.zkj").isWritable()) ||
                    !QFileInfo(ZKanji::appFolder() + "/data").isWritable())
                {
                    showAndQuit("zkanji", qApp->translate("", "File permissions do not allow creating and writing data files in the program's data folder."));
                    continue;
                }

                DictImport diform;
                std::unique_ptr<Dictionary> dict(diform.importDict(dir.path(), true));
                if (dict.get() != nullptr)
                    dict->saveImport(ZKanji::appFolder() + "/data");

                ZKanji::cleanupImport();

                ifound = true;
            }

            if ((args[ix] == "-e" || args[ix] == "-ie" || args[ix] == "-ei") && !efound)
            {
                QString d = args[ix + 1];
#ifdef Q_OS_WIN
                d = d.replace(QChar('\\'), "/");
                if (d[d.size() - 1] == QChar('/'))
                    d = d.left(d.size() - 1);
#endif
                QDir dir(d);

                NTFSPermissionGuard permissionguard;

                if (!dir.exists() || !QFileInfo(d + "/examples.utf").exists() || !QFileInfo(d + "/examples.utf").isReadable())
                {
                    showAndQuit("zkanji", qApp->translate("", "Import folder or examples file not found or not accessible."));
                    continue;
                }

                if ((QFileInfo(ZKanji::appFolder() + "/data/examples.zkj").exists() && !QFileInfo(ZKanji::appFolder() + "/data/examples.zkj").isWritable()) ||
                    !QFileInfo(ZKanji::appFolder() + "/data").isWritable())
                {
                    showAndQuit("zkanji", qApp->translate("", "File permissions do not allow creating and writing data files in the program's data folder."));
                    continue;
                }

                // Saving the path to the examples file. It'll be imported later when the main
                // dictionary has already loaded.
                expath = d;
                efound = true;
            }
        }
    }

}

#ifdef _DEBUG
class ZApplication : public QApplication
{
private:
    typedef QApplication    base;
public:
    ZApplication(int &argc, char **argv) : base(argc, argv) { ; }

    virtual bool notify(QObject *receiver, QEvent *e) override
    {
        try
        {
            return base::notify(receiver, e);
        }
        catch (...)
        {
            //QEvent::Type t = e->type();
            return true;
        }
    }
};
#endif

int main(int argc, char **argv)
{
#ifdef _DEBUG
    ZApplication a(argc, argv);
#else
    QApplication a(argc, argv);
    //QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/qt5_plugins");
#endif

    QStringList args = a.arguments();

    if (args.contains("--help") || args.contains("-h") || args.contains("-?") || args.contains("/help") || args.contains("/h") || args.contains("/?"))
    {
        QTextStream out(stdout);
        out << "zkanji " << ZKANJI_PROGRAM_VERSION << endl;
        out << "USAGE: zkanji [option]" << endl;
        out << endl;
        out << "  --help, -h, -?, /help, /h, /?" << endl;
        out << "                  show command line flags without starting the program." << endl;
        out << endl;
        out << "  -i [path]       import JMdict dictionary data at startup from files at path." << endl;
        out << endl;
        out << "  Files needed for import are: JMdict in UTF-8 encoding," << endl;
        out << "                               kanjidic in EUC-JP encoding," << endl;
        out << "                               radkfile in EUC-JP encoding," << endl;
        out << "                               kanjiorder.txt," << endl;
        out << "                               JLPTNData.txt," << endl;
        out << "                               radkelement.txt," << endl;
        out << "                               zradfile.txt" << endl;
        out << endl;
        out << "  -e [path]       import the Tanaka Corpus example sentences data at startup" << endl;
        out << "                  from files at path. Only works if the dictionary data is" << endl;
        out << "                  already generated." << endl;
        out << endl;
        out << "  Files needed for import are: examples.utf the Tanaka Corpus in UTF-8" << endl;
        out << "                               encoding." << endl;
        out << endl;
        out << "  -ie [path]      can be used when the files are located at the same path." << endl;
        out.flush();
        exit(0);
    }

#ifdef Q_OS_WIN
    QIcon prgico(":/programico.ico");
    a.setWindowIcon(prgico);
#endif

    try
    {
        std::unique_ptr<QSharedMemory> singleappguard(new QSharedMemory("zkanjiSingleAppGuardSoNoMultipleZKanjiAppsGetOpened", &a));
#ifdef Q_OS_WIN
        if (singleappguard->attach(QSharedMemory::ReadOnly))
        {
            if (singleappguard->lock())
            {
                QByteArray bb = QByteArray((const char*)singleappguard->data(), singleappguard->size());
                singleappguard->unlock();
                // Send a message to the window to restore or raise the application.
                HWND wnd = FindWindow(nullptr, &QString::fromLatin1(bb).toStdWString()[0]);
                if (wnd != NULL)
                    PostMessage(wnd, WM_USER + 999, 0, 0);
            }

            singleappguard->detach();
            exit(0);
        }
        if (!singleappguard->create(7 + tosigned(strlen(ZKANJI_PROGRAM_VERSION))) || !singleappguard->lock())
        {
            // Some error occurred that prevents creating this shared memory.
            exit(0);
        }
        else
        {
            memcpy(singleappguard->data(), "zkanji ", 7);
            memcpy((byte*)singleappguard->data() + 7, ZKANJI_PROGRAM_VERSION, strlen(ZKANJI_PROGRAM_VERSION));
            singleappguard->unlock();
        }

#else
        QLocalServer server;
        if (singleappguard->attach(QSharedMemory::ReadOnly))
            singleappguard->detach();
        // Attach/detach done twice, because on Linux it seems this removes the shared memory
        // object if a previous instance of the program crashed and is not holding onto it.
        if (singleappguard->attach(QSharedMemory::ReadOnly))
        {
            QLocalSocket socket;
            socket.connectToServer("zkanjiSingleAppServer");
            socket.waitForConnected(1000);
            socket.disconnectFromServer();

            singleappguard->detach();
            exit(0);
        }
        if (!singleappguard->create(1))
        {
            // Some error occurred that prevents creating this shared memory.
            exit(0);
        }

        // TODO: add something specific to the server name for each user, so different users
        // can run their own instances. Better alternative to include the path where zkanji
        // will save its data, as that should limit the instances running.
        QLocalServer::removeServer("zkanjiSingleAppServer");
        //server.setSocketOptions(); - Use this if the specific user mode is implemented by
        // setting different name to the server for each user/data save location.
        server.listen("zkanjiSingleAppServer");
        gUI->connect(&server, &QLocalServer::newConnection, gUI, &GlobalUI::secondAppStarted);
#endif

        a.setApplicationName("zkanji");
        //a.setOrganizationName("zkanji");

#ifdef Q_OS_WIN
        UINT scrolllines;
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrolllines, 0);

        a.setWheelScrollLines(scrolllines);
#endif

        initializeDeinflecter();
        ZKanji::generateValidUnicode();

        ZKanji::setAppFolder(qApp->applicationDirPath());

        zLang->initialize();

        bool dirfound = initUserDirectory();

        loadRecognizerData();

        handleArguments(args);

        checkAppFolder();

        if (!dirfound)
            importOldData();

        gUI->loadScalingSetting();

        if (ZKanji::noData())
            showAndQuit(qApp->translate("", "Error starting zkanji"), qApp->translate("", "The file containing the dictionary and other data is not found at the program's location. Quitting... (1)"));

        loadDictionaries();
        ZKanji::loadSimilarKanji(ZKanji::appFolder() + "/data/similar.txt");

        if (!expath.isEmpty())
        {
            // Importing example sentences.
            DictImport diform;
            if (diform.importExamples(expath, ZKanji::appFolder() + "/data/examples.zkj2", ZKanji::dictionary(0)))
            {
                if (QFile::exists(ZKanji::appFolder() + "/data/examples.zkj"))
                    QFile::remove(ZKanji::appFolder() + "/data/examples.zkj");
                if (!QFile::rename(ZKanji::appFolder() + "/data/examples.zkj2", ZKanji::appFolder() + "/data/examples.zkj"))
                    showSimpleDialog("zkanji", qApp->translate("", "Error occurred while saving imported example sentences file. If exists, the old data will be loaded."));
            }
            else
                showSimpleDialog("zkanji", qApp->translate("", "Error occurred while importing example sentences file. If exists, the old data will be loaded."));
        }

        ZKanji::sentences.load(ZKanji::appFolder() + "/data/examples.zkj");

#define COUNT_WORD_DATA 0
#if (COUNT_WORD_DATA == 1)

        int hasfreq = 0;
        int hasinf = 0;
        bool infnotfit8bits = false;
        int typecnt = 0;
        int notecnt = 0;
        int fieldcnt = 0;
        int dialcnt = 0;

        int defcnt = 0;
        int inbytes = 0;

        for (int ix = 0, siz = ZKanji::dictionary(0)->wordCount(); ix != siz; ++ix)
        {
            WordEntry *e = ZKanji::dictionary(0)->wordEntry(ix);

            defcnt += e->defs.size();
            inbytes += 2 + 4 + e->defs.size() * (3 * 4 + 2);

            if (e->freq != 0)
                ++hasfreq;
            if ((e->inf /*& ~(1 << (int)WordInfo::InGroup)*/) != 0)
                ++hasinf;
            if ((e->inf /*& ~(1 << (int)WordInfo::InGroup)*/) > 0xff)
                infnotfit8bits = true;

            bool hastype = false;
            bool hasnote = false;
            bool hasfield = false;
            bool hasdial = false;
            for (int iy = 0, siz2 = e->defs.size(); iy != siz2; ++iy)
            {
                const WordDefinition &d = e->defs[iy];
                if (d.attrib.types != 0)
                    hastype = true;
                if (d.attrib.notes != 0)
                    hasnote = true;
                if (d.attrib.fields != 0)
                    hasfield = true;
                if (d.attrib.dialects != 0)
                    hasdial = true;
                if (hastype && hasnote && hasfield && hasdial)
                    break;
            }
            if (hastype)
                ++typecnt;
            if (hasnote)
                ++notecnt;
            if (hasfield)
                ++fieldcnt;
            if (hasdial)
                ++dialcnt;
        }

        QMessageBox::information(nullptr, "zkanji", QString("total words: %1\nHas freq: %2, Has inf: %3, Inf above 8: %4\nType cnt: %5\nNote cnt: %6\nField cnt: %7\nDial cnt: %8\n\nDef cnt: %9\n\nBytes: %10")
            .arg(ZKanji::dictionary(0)->wordCount()).arg(hasfreq).arg(hasinf).arg(infnotfit8bits).arg(typecnt).arg(notecnt).arg(fieldcnt).arg(dialcnt).arg(defcnt).arg(inbytes), QMessageBox::Ok);

#endif

        gUI->loadSettings();

        gUI->createWindow(true);
        gUI->applySettings();

        a.postEvent(gUI, new StartEvent, INT_MIN);

        //qreal dpi = QApplication::primaryScreen()->physicalDotsPerInch();
        //qreal dpi2 = QApplication::primaryScreen()->logicalDotsPerInchY();
        //qreal ratio = (qreal)QApplication::primaryScreen()->logicalDotsPerInchY() / QApplication::primaryScreen()->physicalDotsPerInchY();
        //QMessageBox::information(nullptr, "zkanji", QString("%1, %2, %3").arg(dpi).arg(dpi2).arg(ratio));

        int result = a.exec();
        return result;
    }
    catch (...)
    {
        showSimpleDialog("zkanji", qApp->translate("", "An unexpected error occurred. The program will terminate."));
        return 1;
    }

    return -1;
}

