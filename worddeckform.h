/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDDECKFORM_H
#define WORDDECKFORM_H

#include <QItemDelegate>
#include "dialogwindow.h"
#include "zlistview.h"
#include "zabstracttablemodel.h"

namespace Ui {
    class WordDeckForm;
}


class WordDeckList;
class Dictionary;

class DeckListModel : public ZAbstractTableModel
{
    Q_OBJECT
public:
    DeckListModel(WordDeckList *decks, QObject *parent = nullptr);

    // Creates a temporary item if it's not added already, at the end of the model. The item
    // should be edited to select a name for it. 
    void addTempItem();
    // Removes the temporary item's row and signals it.
    void removeTempItem();

    bool hasTempItem() const;

    // Virtual functions derived from model.

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex &index = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;

    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
    //virtual QStringList mimeTypes() const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;
private slots:
    // QAbstractItemModel::endMoveRows() directly handles the end move.
    //void beginMove(int first, int last, int pos);
    void decksMoved(const smartvector <Range> &ranges, int pos);

    void deckRemoved(int index);
private:
    WordDeckList *decks;
    // True if a temporary row should be listed after the decks to prepare the creation a new
    // deck.
    bool tempitem;

    typedef ZAbstractTableModel base;
};

class WordDeckForm;
class WordDeckListView : public ZListView
{
    Q_OBJECT
signals:
    void editorClosed();
public:
    WordDeckListView(QWidget *parent = nullptr);
protected slots:
    virtual void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override;
private:

    typedef ZListView   base;
};

class WordDeckForm : public DialogWindow
{
    Q_OBJECT
signals:
    void activated(QObject*);
public:
    // Returns the word deck form if it has been created and shown. pass true in createshow to
    // create and show the form if it does not exist. Returns null if the form has not been
    // created and createshow is false. When the form is created it's always visible, and when
    // it's hidden it's destroyed.
    // Pass index of the dictionary in dict that should be selected by default.
    static WordDeckForm* Instance(bool createshow, int dict = -1);

    /* constructor is private */

    virtual ~WordDeckForm();

    void setDictionary(int index);
public slots:
    void on_delButton_clicked(bool checked);
    void on_addButton_clicked(bool checked);
    void on_statsButton_clicked(bool checked);
    void on_deckTable_rowSelectionChanged();
    void on_dictCBox_currentIndexChanged(int index);
    void editorClosed();

    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);
protected:
    //virtual void showEvent(QShowEvent *event) override;
    virtual bool event(QEvent *e) override;
    virtual void focusInEvent(QFocusEvent *e) override;
    // Prevent accepting default buttons while the table is editing.
    virtual void keyPressEvent(QKeyEvent *e) override;
private:
    WordDeckForm(QWidget *parent = nullptr);
    // Lists the decks of the current dictionary in the decks combobox.
    void fillDeckList();

    void viewClicked();
    void studyClicked();
    void currentDeckChanged(int current, int previous);
    void deckDoubleClicked(int index);


    static WordDeckForm *inst;
    Ui::WordDeckForm *ui;

    WordDeckList *decks;

    DeckListModel *model;
    // The row that was selected before the deck creation was started.
    int cacherow;

    typedef DialogWindow base;
};


#endif // WORDDECKFORM_H
