/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DICTIONARYSTATSFORM_H
#define DICTIONARYSTATSFORM_H

#include <QBasicTimer>
#include <QThread>
#include <atomic>
#include <memory>
#include "dialogwindow.h"

namespace Ui {
    class DictionaryStatsForm;
}

class DictionaryStatsForm;
class Dictionary;
class StatsThread : public QRunnable
{
public:
    StatsThread(DictionaryStatsForm *owner, Dictionary *dict);
    virtual ~StatsThread();

    virtual void run() override;
    // Returns the calculated value. Only valid when the variable "done" is set to true.
    int entryResult() const;
    int definitionResult() const;
    int kanjiResult() const;

    std::atomic_bool terminate;
    std::atomic_bool done;
private:
    // Checks whether the thread should be terminated. If so, sets done to true and returns 
    // true.
    bool checkTerminate();

    bool calculateEntries();
    bool calculateDefinitions();
    bool calculateKanji();

    DictionaryStatsForm *owner;
    Dictionary *dict;

    int valentry;
    int valdef;
    int valkanji;

    typedef QRunnable base;
};

struct StatResult
{
    int entries;
    int defs;
    int kanji;
};

class Dictionary;
class DictionaryStatsForm : public DialogWindow
{
    Q_OBJECT
public:
    DictionaryStatsForm(int index, QWidget *parent = nullptr);
    virtual ~DictionaryStatsForm();
public slots:
    void on_dictCBox_currentIndexChanged(int newindex);
protected:
    virtual bool event(QEvent *e) override;
    virtual bool eventFilter(QObject *o, QEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;
private:
    void updateData();

    void stopThreads();
    void startThreads(Dictionary *d);

    void translateTexts();
    void updateLabels(const StatResult &result);

    void setHtmlInfoText(const QString &str);

    Ui::DictionaryStatsForm *ui;
    QBasicTimer timer;

    std::unique_ptr<StatsThread> thread;

    std::map<Dictionary *, StatResult> results;

    typedef DialogWindow    base;
};


#endif // DICTIONARYSTATSFORM_H
