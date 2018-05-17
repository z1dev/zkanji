/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include "languages.h"
#include "zkanjimain.h"

//-------------------------------------------------------------

// Static initializers:

Languages *Languages::instance = nullptr;
std::vector<std::pair<QString, QString>> Languages::list = {

    { "English (United States)", "" },

    // This "language" should be skipped when initializing the class. Added to test that it
    // works correctly. It's possible to create a matching language file for fun.
    { "Intentionally Wrong Language", "wrong" },

    // List of all other languages. The language names should be in the same language, and
    // placed in alphabetic order. The suffix (second) value should match the suffix in the
    // qt_XX.qm translation files in the lang folder.
    { "Magyar", "hu" }

};


//-------------------------------------------------------------


Languages* Languages::getInstance()
{
    if (instance == nullptr)
        instance = new Languages();
    return instance;
}

Languages::Languages() : base(), current(0), busy(false)
{
    qApp->installEventFilter(this);
}

Languages::~Languages()
{

}

void Languages::initialize()
{
    for (int ix = 1, siz = list.size(); ix != siz; ++ix)
    {
        if (!QFile::exists(ZKanji::appFolder() + QString("/lang/zkanji_%1.qm").arg(list[ix].second)))
            list[ix].first = "";
    }
    list.resize(std::remove_if(list.begin(), list.end(), [](const std::pair<QString, QString> &val) { return val.first.isEmpty(); }) - list.begin());
}

int Languages::count()
{
    return list.size();
}

QString Languages::names(int ix)
{
    return list[ix].first;
}

QString Languages::IDs(int ix)
{
    return list[ix].second;
}

int Languages::idIndex(QString str)
{
    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
        if (list[ix].second == str)
            return ix;
    return -1;
}

bool Languages::setLanguage(int ix)
{
    if (ix < 0 || ix >= list.size() || ix == current)
        return false;

    if (ix == 0)
    {
        busy = true;
        qApp->removeTranslator(qttranslator.get());
        qApp->removeTranslator(qtbasetranslator.get());
        busy = false;
        qApp->removeTranslator(translator.get());

        qttranslator.reset();
        qtbasetranslator.reset();
        translator.reset();
        current = ix;

        emit languageChanged();

        return true;
    }

    std::unique_ptr<QTranslator> dummy(new QTranslator);
    std::unique_ptr<QTranslator> qtdummy(new QTranslator);
    std::unique_ptr<QTranslator> qtbasedummy(new QTranslator);
    if (QFile::exists(ZKanji::appFolder() + QString("/lang/qt_%1.qm").arg(list[ix].second)) && !qtdummy->load(QString("qt_%1").arg(list[ix].second), ZKanji::appFolder() + QString("/lang")))
        return false;
    if (QFile::exists(ZKanji::appFolder() + QString("/lang/qtbase_%1.qm").arg(list[ix].second)) && !qtbasedummy->load(QString("qtbase_%1").arg(list[ix].second), ZKanji::appFolder() + QString("/lang")))
        return false;
    if (!dummy->load(QString("zkanji_%1").arg(list[ix].second), ZKanji::appFolder() + QString("/lang")))
        return false;

    busy = true;
    if (!qtdummy->isEmpty() && !qApp->installTranslator(qtdummy.get()))
    {
        busy = false;
        return false;
    }
    if (!qtbasedummy->isEmpty() && !qApp->installTranslator(qtbasedummy.get()))
    {
        if (!qtdummy->isEmpty())
            qApp->removeTranslator(qtdummy.get());
        busy = false;
        return false;
    }

    busy = false;
    if (!qApp->installTranslator(dummy.get()))
    {
        busy = true;
        if (!qtdummy->isEmpty())
            qApp->removeTranslator(qtdummy.get());
        if (!qtbasedummy->isEmpty())
            qApp->removeTranslator(qtbasedummy.get());
        busy = false;
        return false;
    }

    busy = true;
    qApp->removeTranslator(qttranslator.get());
    qApp->removeTranslator(qtbasetranslator.get());
    qApp->removeTranslator(translator.get());
    busy = false;

    qttranslator = std::move(qtdummy);
    qtbasetranslator = std::move(qtbasedummy);
    translator = std::move(dummy);
    current = ix;

    emit languageChanged();

    return true;
}

bool Languages::setLanguage(QString id)
{
    return setLanguage(idIndex(id));
}

int Languages::currentLanguage()
{
    return current;
}

QString Languages::currentID()
{
    return list[current].second;
}

QString Languages::translate(const char *context, const char *key, const char *disambiguation)
{
    if (current == 0 || translator == nullptr)
        return key;

    return qApp->translate(context, key, disambiguation);
}

QString Languages::translate(const char *context, const QString &key, const char *disambiguation)
{
    if (current == 0 || translator == nullptr)
        return key;

    return qApp->translate(context, key.toLatin1(), disambiguation);
}

QString Languages::translate(const QString &context, const QString &key, const char *disambiguation)
{
    if (current == 0 || translator == nullptr)
        return key;

    return qApp->translate(context.toLatin1(), key.toLatin1(), disambiguation);
}

bool Languages::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::LanguageChange && busy)
        return true;

    return base::eventFilter(o, e);
}


//-------------------------------------------------------------

