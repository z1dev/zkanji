/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDTESTSETTINGSFORM_H
#define WORDTESTSETTINGSFORM_H

#include <QMainWindow>
#include <functional>

#include "zdictionarymodel.h"
#include "dialogwindow.h"
#include "groupstudy.h"

namespace Ui {
    class WordTestSettingsForm;
}

class WordGroup;

enum class TestWordsDisplay { All, WrittenConflict, KanaConflict, DefinitionConflict };
enum class TestWordsColumnTypes { Order = (int)DictColumnTypes::Last, Score };

class TestWordsItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    TestWordsItemModel(WordGroup *group, QObject *parent = nullptr);
    virtual ~TestWordsItemModel();

    // Gets the current items from the group, updating the listed data. The studied items list
    // is not normally refreshed from the group. In case the group changes the study data will
    // hold the original items unless refresh is called and the group is updated with the
    // refreshed list.
    void refresh();

    TestWordsDisplay displayed() const;
    void setDisplayed(TestWordsDisplay disp);

    // Fills the items displayed by the model. If dorefresh is false, the previously displayed
    // items are kept, otherwise the items are queried from the group.
    void reset(bool dorefresh = false);

    // Returns whether excluded items should be shown.
    bool showExcluded() const;
    // Changes whether the excluded items should be shown.
    void setShowExcluded(bool shown);

    // Changes the excluded status of the passed items to the value of exclude.
    void setExcluded(const std::vector<int> &rows, bool exclude);

    // Resets the stored score of the items at rows.
    void resetScore(const std::vector<int> &rows);

    // The items in the model, which can be distinct from the items in the study group if the
    // user has changed anything before accepting the settings dialog.
    const std::vector<WordStudyItem>& getItems() const;

    // Returns whether the word at the displayed row index is excluded or not.
    bool isRowExcluded(int row) const;

    // Returns whether the item at row has previous study data stored.
    bool rowScored(int row) const;

    // Sets the words are to be provided by this model.
    //void setData(std::vector<int> indexes);

    // Returns the index of a word in its dictionary at the passed position.
    virtual int indexes(int pos) const override;
    // Returns the dictionary the model shows items for.
    virtual Dictionary* dictionary() const override;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index = QModelIndex()) const override;

    // Removes rows from the model, emitting the necessary signals.
    //void removeRows(int from, int to);

    bool sortOrder(int c, int a, int b) const;
protected slots:
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;
private:
    // Returns the string used for comparing listed item meanings when display mode is
    // MeaningConflict. The cache map is used to save the meaning if it's not in it already
    // and to return it if it is stored.
    const QString& meaningString(int windex, std::map<int, QString> &cache) const;
    // Returns whether two words compare equal when checking for conflicts with the passed
    // display mode. The cache map is used to cache meaning strings in case the dislay mode is
    // MeaningConflict.
    bool comparedEqual(TestWordsDisplay disp, int windex1, int windex2, std::map<int, QString> &cache) const;

    WordGroup *group;

    TestWordsDisplay display;
    bool showexcluded;

    // Item indexes in the study group.
    std::vector<WordStudyItem> items;
    // A filtered ordering of items. Contains indexes to items in the current order and
    // exluded items either in the list or not depending on user choice.
    std::vector<int> list;

    typedef DictionaryItemModel base;
};

class WordGroup;
struct WordStudySettings;
enum class ModalResult;
class WordTestSettingsForm : public DialogWindow
{
    Q_OBJECT
public:
    WordTestSettingsForm(WordGroup *group, QWidget *parent = nullptr);
    ~WordTestSettingsForm();

    void exec();
protected:
    virtual bool event(QEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;
private:
    void setWidgetTexts();
    // Enables or disables the start and save buttons depending on the current selection.
    void updateStartSave();

    // Returns true if the "Exclude" button is shown for words instead of "Include" when index
    // is -1. Otherwise returns whether the exclude button would show for the given
    // displayCBox index.
    //bool hasExclude(int index = -1);

    Ui::WordTestSettingsForm *ui;

    WordGroup *group;

    TestWordsItemModel *model;

    // Saved sort column.
    int scolumn;
    // Saved sort order.
    Qt::SortOrder sorder;

    typedef DialogWindow    base;
private slots:
    void on_methodCBox_currentIndexChanged(int index);
    void on_timeCBox_currentIndexChanged(int index);
    void on_timeLimitEdit_textChanged(QString t);
    void on_wordsEdit_textChanged(QString t);
    void on_saveButton_clicked();
    void on_startButton_clicked();
    void on_displayBox_currentIndexChanged(int index);
    void on_excludedButton_clicked();
    void on_excludeButton_clicked();
    void on_includeButton_clicked();
    void on_resetButton_clicked();
    void on_refreshButton_clicked();
    void on_answerCBox_currentIndexChanged(int index);
    void selectionChanged();
    void headerSortChanged(int index, Qt::SortOrder order);
};

#endif // WORDTESTSETTINGSFORM_H
