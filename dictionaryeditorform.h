/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DICTIONARYEDITORFORM_H
#define DICTIONARYEDITORFORM_H

#include "dialogwindow.h"
#include "zlistboxmodel.h"


namespace Ui {
    class DictionaryEditorForm;
}

class ZListView;
class DictionaryEditorModel : public ZAbstractListBoxModel
{
    Q_OBJECT
public:
    DictionaryEditorModel(ZListView *view, QWidget *parent = nullptr);
    virtual ~DictionaryEditorModel();

    bool orderChanged() const;

    virtual int rowCount(const QModelIndex &index = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent = QModelIndex()) override;
    virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;
protected slots:
    void handleAdded();
    void handleRemoved(int index, int order);
    void handleMoved(int from, int to);
    void handleRenamed(const QString &oldname, int index, int order);
    void handleFlagChanged(int index, int order);
private:
    ZListView *view;
    // Set to true if the dictionary order has changed since the form was shown.
    bool orderchanged;

    typedef ZAbstractListBoxModel base;
};

class DictionaryEditorForm : public DialogWindow
{
    Q_OBJECT
public:
    virtual ~DictionaryEditorForm();

    // Shows the single instance of the form, or brings the existing instance to the
    // foreground.
    static void execute();
public slots:
    void on_upButton_clicked(bool checked);
    void on_downButton_clicked(bool checked);
    void on_createButton_clicked(bool checked);
    void on_editButton_clicked(bool checked);
    void on_delButton_clicked(bool checked);
    void on_dictView_currentRowChanged(int curr, int prev);
protected:
    virtual bool event(QEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;
private:
    DictionaryEditorForm(QWidget *parent = nullptr);

    void translateTexts();

    DictionaryEditorForm(const DictionaryEditorForm &) = delete;
    DictionaryEditorForm(DictionaryEditorForm &&) = delete;
    DictionaryEditorForm& operator=(const DictionaryEditorForm &) = delete;
    DictionaryEditorForm& operator=(DictionaryEditorForm &&) = delete;

    static DictionaryEditorForm *instance;

    Ui::DictionaryEditorForm *ui;

    DictionaryEditorModel *model;

    typedef DialogWindow    base;
};


#endif // DICTIONARYEDITORFORM_H

