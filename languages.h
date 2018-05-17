/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef LANGUAGES_H
#define LANGUAGES_H

#include <QObject>
#include <QTranslator>
#include <vector>
#include <memory>

class Languages : public QObject
{
    Q_OBJECT
signals:
    // Signal for objects that can't use the LanguageChange notification event for a reason.
    void languageChanged();
public:
    static Languages* getInstance();
    ~Languages();

    // Checks the available translation files in the lang subfolder.
    void initialize();
    // Number of language translation files found in the lang folder.
    int count();
    // Name of the language at the position ix specified in list. Only languages found after
    // initialize are included
    QString names(int ix);
    // ID of the language replacing XX in the zkanji_XX.qm 
    QString IDs(int ix);
    // Returns the index of the passed language ID string.
    int idIndex(QString str);

    // Installs the language found at position ix to the application. This will automatically
    // emit a LanguageChange event to all toplevel widgets. Returns true on success.
    bool setLanguage(int ix);

    // Installs the language with the passed ID to the application. This will automatically
    // emit a LanguageChange event to all toplevel widgets. Returns true on success.
    bool setLanguage(QString id);

    // Language index currently installed in the application.
    int currentLanguage();
    // Language ID currently installed in the application.
    QString currentID();

    // Returns the translated text with the current language, or the original text if it's
    // either not found or the language is the default English at index 0.
    QString translate(const char *context, const char *key, const char *disambiguation = nullptr);
    // Returns the translated text with the current language, or the original text if it's
    // either not found or the language is the default English at index 0.
    // Warning: The key is converted to Latin1 string.
    QString translate(const char *context, const QString &key, const char *disambiguation = nullptr);
    // Returns the translated text with the current language, or the original text if it's
    // either not found or the language is the default English at index 0.
    // Warning: The context and key are converted to Latin1 string.
    QString translate(const QString &context, const QString &key, const char *disambiguation = nullptr);
protected:
    virtual bool eventFilter(QObject *o, QEvent *e) override;
private:
    Languages();
    Languages(const Languages &) = delete;
    Languages(Languages &&) = delete;
    Languages& operator=(const Languages &) = delete;
    Languages& operator=(Languages &&) = delete;

    static Languages *instance;

    // [Readable language name, Language ID] pairs. The language ID is XX in zkanji_XX.qm.
    // Initially filled with all possibilities. If a language file is not found, it's removed
    // from this list.
    static std::vector<std::pair<QString, QString>> list;

    // Index of the current installed UI translation.
    int current;

    // Translator object for all qt strings.
    std::unique_ptr<QTranslator> qttranslator;
    // Translator object for all qt base strings.
    std::unique_ptr<QTranslator> qtbasetranslator;
    // Translator object for custom strings.
    std::unique_ptr<QTranslator> translator;

    // Set to true if installing a translation is in progress, so we don't get multiple change
    // events when separately installing the qt translation. The event is filtered out at the
    // application level.
    bool busy;

    typedef QObject base;
};

#define zLang   Languages::getInstance()


#endif // LANGUAGES_H

