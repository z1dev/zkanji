/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QPainter>
#include <QMimeData>
#include "sites.h"
#include "fontsettings.h"
#include "ranges.h"
#include "colorsettings.h"
#include "zlistview.h"
#include "zui.h"

namespace ZKanji
{
    std::vector<SiteItem> lookup_sites;

    void saveXMLSite(const SiteItem &item, QXmlStreamWriter &writer)
    {
        writer.writeEmptyElement("Site");
        writer.writeAttribute("name", item.name);
        writer.writeAttribute("url", item.url);
        writer.writeAttribute("insertpos", QString::number(item.insertpos));
        writer.writeAttribute("locked", item.poslocked ? "1" : "0");
    }

    void loadXMLSite(std::vector<SiteItem> &list, QXmlStreamReader &reader)
    {
        QStringRef nstr = reader.attributes().value("name").left(32);
        QStringRef ustr = reader.attributes().value("url").left(2000);

        if (nstr.isEmpty() && ustr.isEmpty())
            return;

        SiteItem i;
        i.name = nstr.toString();
        i.url = ustr.toString();

        bool ok;
        int val = reader.attributes().value("insertpos").toInt(&ok);
        if (ok && val >= 0 && val <= i.url.size())
            i.insertpos = val;
        else
            i.insertpos = i.url.size();

        i.poslocked = reader.attributes().value("locked") == "1";

        list.push_back(i);
        reader.skipCurrentElement();
    }

    void saveXMLLookupSites(QXmlStreamWriter &writer)
    {
        for (SiteItem &item : lookup_sites)
            saveXMLSite(item, writer);
    }

    void loadXMLLookupSites(QXmlStreamReader &reader)
    {
        lookup_sites.clear();
        while (reader.readNextStartElement())
        {
            if (reader.name() == "Site")
                loadXMLSite(lookup_sites, reader);
            else
                reader.skipCurrentElement();
        }
    }

}

//-------------------------------------------------------------


SitesListModel::SitesListModel(QObject *parent) : base(parent), list(ZKanji::lookup_sites)
{

}

SitesListModel::~SitesListModel()
{

}

void SitesListModel::reset()
{
    beginResetModel();
    list = ZKanji::lookup_sites;
    endResetModel();
}

void SitesListModel::apply()
{
    ZKanji::lookup_sites = list;
}

const SiteItem& SitesListModel::items(int index) const
{
    return list[index];
}

void SitesListModel::deleteSite(int ix)
{
#ifdef _DEBUG
    if (ix < 0 || ix >= list.size())
        return;
#endif

    list.erase(list.begin() + ix);
    signalRowsRemoved({ { ix, ix } });
}

void SitesListModel::setItemName(int ix, const QString &str)
{
    SiteItem &item = expandedItem(ix);

    if (item.name == str.left(32))
        return;
    item.name = str.left(32);
    emit dataChanged(index(ix, 0), index(ix, 0));
}

void SitesListModel::setItemUrl(int ix, const QString &str)
{
    SiteItem &item = expandedItem(ix);
    if (item.url == str.left(2000))
        return;
    item.url = str.left(2000);
    emit dataChanged(index(ix, 2), index(ix, 2));
}

void SitesListModel::setItemInsertPos(int ix, int pos)
{
#ifdef _DEBUG
    if (ix < 0)
        throw "out of bounds";
#endif
    if (list.size() <= ix)
        return;

    SiteItem &item = list[ix];

    if (item.poslocked || item.insertpos == pos)
        return;

    item.insertpos = pos;
    emit dataChanged(index(ix, 2), index(ix, 2));
}

void SitesListModel::lockItemInsertPos(int ix, bool lock)
{
#ifdef _DEBUG
    if (ix < 0)
        throw "out of bounds";
#endif
    if (list.size() <= ix)
        return;

    SiteItem &item = list[ix];
    item.poslocked = lock;
}

bool SitesListModel::itemInsertPosLocked(int ix) const
{
#ifdef _DEBUG
    if (ix < 0 || list.size() <= ix)
        throw "out of bounds";
#endif
    const SiteItem &item = list[ix];
    return item.poslocked;
}

int SitesListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 2;
}

int SitesListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return list.size() + 1;
}

QVariant SitesListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == (int)ColumnRoles::FreeSized && section == 0)
        return true;

    if (role == (int)ColumnRoles::AutoSized && section == 1)
        return (int)ColumnAutoSize::StretchedAuto;

    if (role != Qt::DisplayRole)
        return QVariant();


    if (section == 0)
        return tr("Name");
    else
        return tr("Web address");
}

QVariant SitesListModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    int row = index.row();
    int col = index.column();
    if (row < list.size())
    {
        const SiteItem &item = list[row];

        if (col == 0)
            return item.name;

        return item.url.left(item.insertpos) + QChar(0x30fb) + item.url.mid(item.insertpos);
    }

    return QVariant();
}

QMimeData* SitesListModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty() || !indexes.front().isValid() || indexes.front().row() >= list.size() )
        return nullptr;

    QMimeData *mime = new QMimeData();
    QByteArray arr;
    arr.resize(sizeof(int));
    int *dat = (int*)arr.data();
    *dat = indexes.front().row();

    mime->setData("zkanji/siterow", arr);

    return mime;
}

Qt::DropActions SitesListModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions SitesListModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (!samesource)
        return 0;
    return Qt::MoveAction;
}

bool SitesListModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row == -1 || row > list.size() || !data->hasFormat("zkanji/siterow") || data->data("zkanji/siterow").size() != sizeof(int) || action != Qt::MoveAction ||
        *((int*)data->data("zkanji/siterow").data()) >= list.size())
        return false;

    return true;
}

bool SitesListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    int index = *((int*)data->data("zkanji/siterow").data());

    SiteItem item = list[index];
    list.erase(list.begin() + index);

    int oldrow = row;
    if (row > index)
        --row;
    list.insert(list.begin() + row, item);

    signalRowsMoved({ { index, index } }, oldrow);

    return true;
}

SiteItem& SitesListModel::expandedItem(int row)
{
#ifdef _DEBUG
    if (row < 0)
        throw "out of bounds";
#endif

    if (row < list.size())
        return list[row];

    list.push_back(SiteItem());
    SiteItem &item = list.back();
    item.url = "http://";
    item.insertpos = item.url.size();
    item.poslocked = false;
    signalRowsInserted({ { (int)list.size() - 1, 1 } });

    return item;
}


//-------------------------------------------------------------


SitesItemDelegate::SitesItemDelegate(ZListView *parent) : base(parent)
{

}

SitesItemDelegate::~SitesItemDelegate()
{

}

void SitesItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    base::paint(painter, option, index);

    bool selected = owner()->rowSelected(index.row());

    QPalette::ColorGroup colorgrp = !owner()->isActiveWindow()/*(option.state & QStyle::State_Active)*/ ? QPalette::Inactive : QPalette::Active;

    QColor celltextcol = index.data((int)CellRoles::TextColor).value<QColor>();
    QColor basetextcol = option.palette.color(colorgrp, QPalette::Text);
    QColor seltextcol = option.palette.color(colorgrp, QPalette::HighlightedText);
    QColor textcol = selected ? seltextcol : basetextcol;
    if (celltextcol.isValid())
        textcol = colorFromBase(basetextcol, textcol, celltextcol);

    QColor cellcol = index.data((int)CellRoles::CellColor).value<QColor>();
    QColor basecol = option.palette.color(colorgrp, QPalette::Base);
    QColor selcol = option.palette.color(colorgrp, QPalette::Highlight);
    QColor bgcol = selected ? selcol : basecol;
    if (cellcol.isValid())
        bgcol = colorFromBase(basecol, bgcol, cellcol);

    int row = index.row();
    int col = index.column();

    SitesListModel *m = (SitesListModel*)owner()->model();
    if (m == nullptr || row < m->rowCount() - 1)
        return;

    QFont f = Settings::mainFont();
    f.setItalic(true);

    QString str;
    if (col == 0)
        str = tr("-");
    else
        str = tr("Select this and edit above to add a new site.");

    QColor pc = textcol;
    QColor bc = bgcol;

    QColor pencol{ (pc.red() + bc.red()) / 2, (pc.green() + bc.green()) / 2, (pc.blue() + bc.blue()) / 2 };

    painter->setPen(pencol);
    painter->setFont(f);

    painter->drawText(option.rect.left() + 4, option.rect.top(), option.rect.width() - 8, option.rect.height(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, str);
}


//-------------------------------------------------------------
