/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef SETTINGSFORM_H
#define SETTINGSFORM_H

#include <QtCore>
#include <QMainWindow>
#include <QFrame>
#include "fastarray.h"
#include "dialogwindow.h"
#include "zlistboxmodel.h"

namespace Ui {
    class SettingsForm;
}

enum class FontStyle;
enum class LineSize;

class fontPreviewWidget : public QFrame
{
    Q_OBJECT
public:
    fontPreviewWidget(QWidget *parent = nullptr);

    // Changes the fonts displayed in the preview.
    void setFonts(const QString &kf, const QString &df, const QString &nf, FontStyle nfs, LineSize siz);

    virtual QSize minimumSizeHint() const override;
    virtual QSize sizeHint() const override;
protected:
    virtual void paintEvent(QPaintEvent *e) override;
private:
    QString kanafont;
    QString deffont;
    QString notesfont;
    FontStyle notestyle;
    LineSize sizes;

    // Cached size hint height.
    mutable int sizeheight;

    typedef QFrame  base;
};

class KanjiRefListModel : public ZAbstractListBoxModel
{
    Q_OBJECT
public:
    KanjiRefListModel(QObject *parent = nullptr);
    virtual ~KanjiRefListModel();

    // Copies settings to model.
    void reset();
    // Applies model settings to the kanji settings.
    void apply();

    void moveUp(int currpos, const std::vector<int> &indexes);
    void moveDown(int currpos, const std::vector<int> &indexes);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index = QModelIndex()) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent = QModelIndex()) override;

    virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;
private:
    std::vector<char> showref;
    std::vector<int> reforder;

    typedef ZAbstractListBoxModel   base;
};

enum class SettingsPage { Default, Printing };

class QTreeWidgetItem;
class SettingsForm : public DialogWindow
{
    Q_OBJECT
public:
    SettingsForm(QWidget *parent = nullptr);
    virtual ~SettingsForm();

    void show(SettingsPage page = SettingsPage::Default);
public slots:
    void reset();

    void on_pagesTree_currentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *prev);

    void okClicked();
    void applyClicked();

    void on_backupFolderButton_clicked();

    void on_savePosBox_toggled();
    void on_jlptBox_toggled();
    void on_okuColorBox_toggled();

    void refSelChanged();
    void on_refUpButton_clicked();
    void on_refDownButton_clicked();

    void on_studyIncludeEdit_textChanged(const QString &text);

    void sitesSelChanged(int curr, int prev);
    void on_siteNameEdit_textEdited(const QString &text);
    void on_siteUrlEdit_textEdited(const QString &text);
    void on_siteUrlEdit_cursorPositionChanged(int oldp, int newp);
    void on_siteUrlEdit_selectionChanged();
    void on_siteLockButton_toggled(bool checked);
    void on_siteDelButton_clicked();
    void on_kanjiSizeCBox_currentIndexChanged(int index);

    void fontCBoxChanged();
    void fontSizeCBoxChanged();
    void popupSizeCBoxChanged();
    void printBoxToggled();
    void printCBoxChanged();
protected:
    virtual bool event(QEvent *e) override;
    virtual bool eventFilter(QObject *o, QEvent *e) override;
private:
    void translateTexts();

    // Updates se and ss to contain the selection start and selection end. The end of the
    // selection can be in front of the start, which is the cursor position.
    void getSiteUrlSel(int &ss, int &se);

    // Returns the item in the pages tree from the page index of the pages stack. There's no
    // range checking in the function.
    QTreeWidgetItem* pageTreeItem(int pageindex) const;

    Ui::SettingsForm *ui;

    bool ignoresitechange;
    // Start of last selection in the siteUrlEdit.
    int sitesels;
    // End of last selection in the siteUrlEdit.
    int sitesele;

    typedef DialogWindow base;
};


#endif // SETTINGSFORM_H
