/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef SITES_H
#define SITES_H

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include "zabstracttablemodel.h"
#include "zlistviewitemdelegate.h"

struct SiteItem
{
    // Display name of search site for users. Maximum length of 32 characters.
    QString name;
    // URL of site, excluding the search string to be added by zkanji. Maximum 2000 characters.
    QString url;
    // Position where the search string (kanji or kana) will be inserted in the url before
    // opening a web browser.
    int insertpos;
    // Whether changing the insert position is locked by the user so it won't be accidentally
    // changed by moving the cursor in the settings window.
    bool poslocked;
};

class SitesListModel : public ZAbstractTableModel
{
    Q_OBJECT
public:
    SitesListModel(QObject *parent = nullptr);
    virtual ~SitesListModel();

    // Copies the global site data to the model's vectors.
    void reset();

    // Copies the current site data to the global vectors.
    void apply();

    const SiteItem& items(int index) const;

    void deleteSite(int index);

    void setItemName(int index, const QString &str);
    void setItemUrl(int index, const QString &str);

    // Unlike the previous set functions, the following ones don't create a new item if it did not exist.

    void setItemInsertPos(int index, int pos);
    void lockItemInsertPos(int index, bool lock);
    bool itemInsertPosLocked(int index) const;

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;
    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent = QModelIndex()) override;
private:
    // List of sites for dictionary entries.
    std::vector<SiteItem> list;

    // Returns the item at row in list. If the row is above range, a new item is added and,
    // and the signals are emited.
    SiteItem& expandedItem(int row);

    typedef ZAbstractTableModel base;
};

class SitesItemDelegate : public ZListViewItemDelegate
{
    Q_OBJECT
public:
    SitesItemDelegate(ZListView *parent = nullptr);
    virtual ~SitesItemDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    typedef ZListViewItemDelegate    base;
};

//SitesListModel* sitesListModel();

namespace ZKanji
{
    extern std::vector<SiteItem> lookup_sites;

    void saveXMLLookupSites(QXmlStreamWriter &writer);
    void loadXMLLookupSites(QXmlStreamReader &reader);
}

#endif // SITES_H
