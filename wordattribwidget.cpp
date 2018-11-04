/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QSet>
#include "wordattribwidget.h"
#include "ui_wordattribwidget.h"
#include "grammar_enums.h"
#include "zstrings.h"
#include "qcharstring.h"

#include "checked_cast.h"

//-------------------------------------------------------------


namespace
{
    // Mapping for strings when calling wordTypeLong (0xL0NN), wordNoteLong (0xL1NN),
    // wordFieldLong (0xL2NN), wordDialectLong (0xL3NN), wordInfoLong (0xL4NN), 
    // wordJLPTLevel (0xL5NN) and wordAttrib (0xLFNN) (Where NN is the index in the strings, L is
    // the level in the tree.)
    const int attribMap[] = {
        0x0F00 + 0, // -WordType
        0x1000 + 0, //     Noun
        0x1F00 + 1, //     -Verb
        0x2000 + 1, //         IchidanVerb
        0x2000 + 2, //         GodanVerb
        0x2000 + 3, //         TakesSuru
        0x2000 + 4, //         SuruVerb
        0x2000 + 5, //         AruVerb
        0x2000 + 6, //         KuruVerb
        0x2000 + 7, //         IkuVerb
        0x2000 + 8, //         RiVerb
        0x2000 + 9, //         ZuruVerb
        0x2000 + 10, //         Transitive
        0x2000 + 11, //         Intransitive
        0x1F00 + 2, //     -Adjective
        0x2000 + 12, //         TrueAdj
        0x2000 + 13, //         NaAdj
        0x2000 + 14, //         Taru
        0x2000 + 15, //         PrenounAdj
        0x1000 + 16, //     MayTakeNo
        0x1000 + 17, //     Adverb
        0x1000 + 18, //     Aux
        0x1000 + 19, //     Prefix
        0x1000 + 20, //     Suffix
        0x1000 + 21, //     Conjunction
        0x1000 + 22, //     Interjection
        0x1000 + 23, //     Expression
        0x1000 + 24, //     Pronoun
        0x1000 + 25, //     Particle
        0x1000 + 26, //     Counter
        0x1000 + 27, //     CopulaDa
        0x1F00 + 3, //     -Archaic
        0x2000 + 28, //         ArchaicVerb
        0x2000 + 29, //         ArchaicAdj
        0x2000 + 30, //         ArchaicNa
        0x0F00 + 4, // -WordNote
        0x1100 + 0, //     KanaOnly
        0x1100 + 1, //     Abbrev
        0x1100 + 2, //     FourCharIdiom
        0x1100 + 3, //     Idiomatic
        0x1100 + 4, //     Obscure
        0x1100 + 5, //     Obsolete
        0x1100 + 6, //     Onomatopoeia
        0x1100 + 7, //     Proverb
        0x1100 + 8, //     Rare
        0x1100 + 9, //     Sensitive
        0x1F00 + 5, //     -Relation
        0x2100 + 10, //         Colloquial
        0x2100 + 11, //         Familiar
        0x2100 + 12, //         Honorific
        0x2100 + 13, //         Humble
        0x2100 + 14, //         Polite
        0x1F00 + 6, //     -Style
        0x2100 + 15, //         Archaic
        0x2100 + 16, //         Poetical
        0x2100 + 17, //         ChildLang
        0x2100 + 18, //         Male
        0x2100 + 19, //         Female
        0x2100 + 20, //         Joking
        0x2100 + 21, //         Slang
        0x2100 + 22, //         MangaSlang
        0x2100 + 23, //         Derogatory
        0x2100 + 24, //         Vulgar
        0x0F00 + 7, // -WordField
        0x1200 + 0, //     Architecture
        0x1200 + 1, //     Business
        0x1200 + 2, //     Computing
        0x1200 + 3, //     Economics
        0x1200 + 4, //     Engineering
        0x1200 + 5, //     Finance
        0x1200 + 6, //     Food
        0x1200 + 7, //     Law
        0x1200 + 8, //     Military
        0x1200 + 9, //     Music
        0x1F00 + 8, //     -Science
        0x2200 + 10, //         Anatomy
        0x2200 + 11, //         Astronomy
        0x2200 + 12, //         Biology
        0x2200 + 13, //         Botany
        0x2200 + 14, //         Chemistry
        0x2200 + 15, //         Geology
        0x2200 + 16, //         Geometry
        0x2200 + 17, //         Linguistics
        0x2200 + 18, //         Math
        0x2200 + 19, //         Medicine
        0x2200 + 20, //         Physics
        0x2200 + 21, //         Zoology
        0x1F00 + 9, //     -Religion
        0x2200 + 22, //         Buddhism
        0x2200 + 23, //         Shinto
        0x1F00 + 10, //     -Sport
        0x2200 + 24, //         Baseball
        0x2200 + 25, //         Mahjong
        0x2200 + 26, //         MartialArts
        0x2200 + 27, //         Shogi
        0x2200 + 28, //         Sports
        0x2200 + 29, //         Sumo
        0x0F00 + 11, // -WordDialect
        0x1300 + 0, //     HokkaidouBen
        0x1300 + 1, //     KansaiBen
        0x1300 + 2, //     KantouBen
        0x1300 + 3, //     KyotoBen
        0x1300 + 4, //     KyuushuuBen
        0x1300 + 5, //     NaganoBen
        0x1300 + 6, //     OsakaBen
        0x1300 + 7, //     RyuukyuuBen
        0x1300 + 8, //     TosaBen
        0x1300 + 9, //     TouhokuBen
        0x1300 + 10, //     TsugaruBen
        0x0F00 + 12, // -WordInfo
        0x1400 + 0, //     Ateji
        0x1400 + 1, //     Gikun
        0x1400 + 2, //     IrregKanji
        0x1400 + 3, //     IrregKana
        0x1400 + 4, //     IrregOku
        0x1400 + 5, //     OutdatedKanji
        0x1400 + 6, //     OutdatedKana
        0x0F00 + 13, // -WordJLPT
        0x1500 + 0, //     N5
        0x1500 + 1, //     N4
        0x1500 + 2, //     N3
        0x1500 + 3, //     N2
        0x1500 + 4  //     N1
    };

    QString attribStringAt(int index)
    {
        switch (attribMap[index] & 0x0f00)
        {
        case 0x0000:
            return Strings::wordTypeLong(attribMap[index] & 0xff);
        case 0x0100:
            return Strings::wordNoteLong(attribMap[index] & 0xff);
        case 0x0200:
            return Strings::wordFieldLong(attribMap[index] & 0xff);
        case 0x0300:
            return Strings::wordDialectLong(attribMap[index] & 0xff);
        case 0x0400:
            return Strings::wordInfoLong(attribMap[index] & 0xff);
        case 0x0500:
            return Strings::wordJLPTLevel(attribMap[index] & 0xff);
        case 0x0F00:
            return Strings::wordAttrib(attribMap[index] & 0xff);
        default:
            return QString();
        }
    }
}


//-------------------------------------------------------------


WordAttribModel::WordAttribModel(WordAttribWidget *owner, bool showglobal) : base(owner), owner(owner), global(showglobal), attribSize(sizeof(attribMap) / sizeof(int))
{
    if (!global)
        attribSize -= 6 + 8;

    reset();
}

bool WordAttribModel::showGlobal() const
{
    return global;
}

void WordAttribModel::setShowGlobal(bool show)
{
    if (global == show)
        return;

    beginResetModel();
    if (global)
        attribSize -= 6 + 8;

    global = show;

    if (global)
        attribSize += 6 + 8;
    endResetModel();
}

void WordAttribModel::ownerDataChanged()
{
    beginResetModel();
    endResetModel();

    //emitCheckedChanged(nullptr);
}

//QModelIndex WordAttribModel::index(int row, int column, const TreeItem *parent) const
//{
//    if (row < 0 || row >= attribSize || column != 0)
//        return QModelIndex();
//
//    if (!parent.isValid())
//    {
//        // Root index.
//
//        int index = -1;
//        for (int ix = 0; ix != attribSize; ++ix)
//        {
//            if ((attribMap[ix] & 0xf000) == 0)
//                ++index;
//            if (index == row)
//                return createIndex(row, 0, ix);
//        }
//        return QModelIndex();
//    }
//
//    int id = parent.internalId();
//    int plevel = (attribMap[id] & 0xf000);
//
//    int index = -1;
//
//    // Check if row is valid.
//    for (int ix = id + 1; ix != attribSize; ++ix)
//    {
//        // Item is direct child of parent if its level is greater by 1.
//        if ((attribMap[ix] & 0xf000) == plevel + 0x1000)
//            ++index;
//        // Found enough child items in parent to make row valid.
//        if (index == row)
//            return createIndex(row, 0, ix);
//        // Stepped out of parent, no more rows.
//        if ((attribMap[ix] & 0xf000) <= plevel)
//            break;
//    }
//
//    return QModelIndex();
//}
//
//QModelIndex WordAttribModel::parent(const QModelIndex &child) const
//{
//    if (!child.isValid())
//        return QModelIndex();
//
//    int id = child.internalId();
//
//    // Parent of top level items is invalid.
//    if ((attribMap[id] & 0xf000) == 0)
//        return QModelIndex();
//
//    // Get the position of parent (the item at the level above.)
//    int level = attribMap[id] & 0xf000;
//
//    // Index of parent in attribMap[].
//    int pindex = -1;
//
//    for (int ix = id - 1; ix != -1 && pindex == -1; --ix)
//        if ((attribMap[ix] & 0xf000) == level - 0x1000)
//            pindex = ix;
//
//    // Find the row of parent.
//    int row = 0;
//
//    for (int ix = pindex - 1; ix != -1; --ix)
//        if ((attribMap[pindex] & 0xf000) == (attribMap[ix] & 0xf000))
//            ++row;
//        else if ((attribMap[pindex] & 0xf000) > (attribMap[ix] & 0xf000))
//            break;
//
//    return createIndex(row, 0, pindex);
//
//}

intptr_t WordAttribModel::treeRowData(int row, const TreeItem *parent) const
{
    // Returns the index of the tree item at parent's row in attribMap[].

    if (row < 0 || row >= attribSize)
        return 0;

    // Level of items we are looking for.
    int level = 0;

    int ix = 0;
    if (parent != nullptr)
    {
        ix = (int)parent->userData() + 1;
        level = ((attribMap[ix - 1] & 0xf000) >> 12) + 1;
    }

    for ( ; ix != attribSize && row != -1; ++ix)
    {
        if (((attribMap[ix] & 0xf000) >> 12) == level)
            --row;

#ifdef _DEBUG
        if (((attribMap[ix] & 0xf000) >> 12) < level)
            throw "Invalid row requested.";
#endif
    }

    return (ix - 1);
}

int WordAttribModel::rowCount(const TreeItem *parent) const
{
    if (!hasChildren(parent))
        return 0;

    int ix = 0;
    int level = 0;
    if (parent != nullptr)
    {
        ix = (int)parent->userData() + 1;
        level = ((attribMap[ix - 1] & 0xf000) >> 12) + 1;
    }

    int cnt = 0;

    for ( ; ix != attribSize; ++ix)
    {
        if (((attribMap[ix] & 0xf000) >> 12) < level)
            break;
        if (((attribMap[ix] & 0xf000) >> 12) == level)
            ++cnt;
    }


    return cnt;
}

bool WordAttribModel::hasChildren(const TreeItem *parent) const
{
    if (parent == nullptr)
        return true;

    int ix = (int)parent->userData();
    return (ix < attribSize) && (attribMap[ix] & 0x0f00) == 0x0f00;
}

QVariant WordAttribModel::data(const TreeItem *index, int role) const
{
    if (index == nullptr)
        return QVariant();

    int id = (int)index->userData();
    if (role == Qt::DisplayRole)
        return Strings::capitalize(attribStringAt(id));

    if (role == Qt::CheckStateRole)
    {
        switch (attribMap[id] & 0x0f00)
        {
        case 0x0000:
        case 0x0100:
        case 0x0200:
        case 0x0300:
        case 0x0400:
        case 0x0500:
            return attribChecked(id) ? Qt::Checked : Qt::Unchecked;
        case 0x0F00:
        {
            // See whether all sub items are checked or not.

            //int level = (attribMap[id] & 0xf000) >> 12;
            int firstchild = nextSubItem(id);
            bool ischecked = attribChecked(firstchild);
            for (int ix = nextSubItem(id, firstchild); ix != -1; ix = nextSubItem(id, ix))
                if (attribChecked(ix) != ischecked)
                    return Qt::PartiallyChecked;

            return ischecked ? Qt::Checked : Qt::Unchecked;
        }
        default:
            return QVariant();
        }
    }

    return QVariant();
}

bool WordAttribModel::setData(TreeItem *item, const QVariant &value, int role)
{
    if (item == nullptr)
        return false;

    if (role == Qt::CheckStateRole)
    {
        bool check = (Qt::CheckState)value.toInt() == Qt::Checked;

        if (data(item, role).toInt() == value.toInt())
            return false;

        QSet<TreeItem*> list;
        ZTreeView *view = dynamic_cast<ZTreeView*>(qApp->focusWidget());
        if (view != nullptr && view->isSelected(item))
            view->selection(list);
        else
            list.insert(item);
        QSet<TreeItem*> tmp = list;
        for (TreeItem *i : tmp)
            collectChildren(i, list);

        QSet<TreeItem*> parents;

        for (TreeItem *i : list)
        {
            TreeItem *p = i;
            while (p->parent() != nullptr)
            {
                p = p->parent();
                parents.insert(p);
            }

            if (!i->empty())
                continue;

            int ix = (int)i->userData();
            int localrow = attribMap[ix] & 0xff;
            switch (attribMap[ix] & 0x0f00)
            {
            case 0x0000:
                owner->setTypeSelected(localrow, check);
                break;
            case 0x0100:
                owner->setNoteSelected(localrow, check);
                break;
            case 0x0200:
                owner->setFieldSelected(localrow, check);
                break;
            case 0x0300:
                owner->setDialectSelected(localrow, check);
                break;
            case 0x0400:
                owner->setInfoSelected(localrow, check);
                break;
            case 0x0500:
                owner->setJlptSelected(localrow, check);
                break;
            case 0x0F00:
                break;
//            {
//                // Check or uncheck everything below the given parent index.
//                int subix = -1;
//                int bits = 0;
//                while ((subix = nextSubItem(ix, subix)) != -1)
//                    bits |= (1 << (attribMap[subix] & 0xff));
//                switch (attribMap[nextSubItem(ix)] & 0x0f00)
//                {
//                case 0x0000:
//                    owner->setTypeBits(bits, check);
//                    break;
//                case 0x0100:
//                    owner->setNoteBits(bits, check);
//                    break;
//                case 0x0200:
//                    owner->setFieldBits(bits, check);
//                    break;
//                case 0x0300:
//                    owner->setDialectBits(bits, check);
//                    break;
//                case 0x0400:
//                    owner->setInfoBits(bits, check);
//                    break;
//                case 0x0500:
//                    owner->setJlptBits(bits, check);
//                    break;
//#ifdef _DEBUG
//                default:
//                    throw "Invalix subitem.";
//#endif
//                }
//
//                // Emit data changed for subitems.
//                emitCheckedChanged(item);
//
//                break;
//            }
#ifdef _DEBUG
            default:
                throw "Invalix index or item";
#endif
            }

            // Emit data changed for item and parents.
            emit dataChanged(i, i, { Qt::CheckStateRole });

            //TreeItem *pindex = item;
            //while ((pindex = pindex->parent()) != nullptr)
            //    emit dataChanged(pindex, pindex, { Qt::CheckStateRole });

            emit checked(i, check);
        }

        for (TreeItem *p : parents)
            emit dataChanged(p, p, { Qt::CheckStateRole }); 
    }

    return base::setData(item, value, role);
}

Qt::ItemFlags WordAttribModel::flags(const TreeItem *index) const
{
    if (index == nullptr)
        return base::flags(index);

    int id = (int)index->userData();

    // Non checkable category for display only.
    if ((attribMap[id] & 0x0f00) == 0x0f00)
        return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren;
}

void WordAttribModel::collectChildren(TreeItem *parent, QSet<TreeItem*> &result)
{
    if (parent->empty())
        return;

    for (int ix = 0, siz = tosigned(parent->size()); ix != siz; ++ix)
    {
        result.insert(parent->items(ix));
        collectChildren(parent->items(ix), result);
    }
}

bool WordAttribModel::attribChecked(int index) const
{
#ifdef _DEBUG
    if (index < 0 || index >= attribSize)
        throw "Index out of range.";
#endif

    int localrow = attribMap[index] & 0xff;
    switch (attribMap[index] & 0x0f00)
    {
    case 0x0000:
        return owner->typeSelected(localrow);
    case 0x0100:
        return owner->noteSelected(localrow);
    case 0x0200:
        return owner->fieldSelected(localrow);
    case 0x0300:
        return owner->dialectSelected(localrow);
    case 0x0400:
        return owner->infoSelected(localrow);
    case 0x0500:
        return owner->jlptSelected(localrow);
    default:
        throw "Don't call this for invalid items";
    }
}

int WordAttribModel::nextSubItem(int parentindex, int index) const
{
    int parentlevel = (attribMap[parentindex] & 0xf000) >> 12;
    if (index == -1)
        index = parentindex;
    for (int ix = index + 1; ix != attribSize; ++ix)
    {
        // Outside of parent.
        if (((attribMap[ix] & 0xf000) >> 12) <= parentlevel)
            break;
        // A category.
        if ((attribMap[ix] & 0x0f00) == 0x0f00)
            continue;
        return ix;
    }
    return -1;
}

//void WordAttribModel::emitCheckedChanged(const TreeItem *parent)
//{
//#ifdef _DEBUG
//    if (!hasChildren(parent))
//        throw "Invalid call.";
//#endif
//
//    int siz = parent == nullptr ? size() : parent->size();
//
//    if (parent != nullptr)
//        emit dataChanged(parent->items(0), parent->items(siz - 1), { Qt::CheckStateRole });
//
//    for (int ix = 0; ix != siz; ++ix)
//    {
//        const TreeItem *item = parent == nullptr ? items(ix) : parent->items(ix);
//        if (!item->empty())
//            emitCheckedChanged(item);
//    }
//}


//-------------------------------------------------------------


WordAttribProxyModel::WordAttribProxyModel(QObject *parent) : base(parent)
{

}

WordAttribProxyModel::~WordAttribProxyModel()
{

}

void WordAttribProxyModel::setSourceModel(WordAttribModel *m)
{
    WordAttribModel *tmp = dynamic_cast<WordAttribModel*>(sourceModel());
    if (tmp != nullptr)
        disconnect(tmp, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int> &)), this, 0);

    base::setSourceModel(m);

    connect(m, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int> &)), this, SLOT(sourceDataChanged(const QModelIndex&, const QModelIndex&, const QVector<int> &)));
}

void WordAttribProxyModel::setFilterText(QString str)
{
    beginResetModel();
    list.clear();

    WordAttribModel *model = (WordAttribModel*)sourceModel();

    TreeItem *parent = nullptr;

    // For walking the tree, holds the item positions we are walking in.
    std::list<int> stack;

    str = str.toLower();
    if (!str.isEmpty())
    {
        int cnt = model->rowCount();

        for (int ix = 0; ix != cnt; ++ix)
        {
            TreeItem *item = parent == nullptr ? model->items(ix) : parent->items(ix);
            if (model->hasChildren(item))
            {
                stack.push_front(ix);
                parent = item;
                ix = -1;
                cnt = tosigned(parent->size());
                continue;
            }

            if (model->data(item, Qt::DisplayRole).toString().toLower().contains(str))
                list.push_back(item);

            while (ix == cnt - 1 && !stack.empty())
            {
                ix = stack.front();
                parent = parent->parent();
                cnt = parent == nullptr ? model->rowCount() : tosigned(parent->size());
                stack.pop_front();
            }
        }

        std::sort(list.begin(), list.end(), [model](const TreeItem *a, const TreeItem *b) {
            QString astr = model->data(a, Qt::DisplayRole).toString().toLower();
            QString bstr = model->data(b, Qt::DisplayRole).toString().toLower();
            return qcharcmp(astr.constData(), bstr.constData()) < 0;
        });
    }

    endResetModel();
}

QModelIndex WordAttribProxyModel::mapToSource(const QModelIndex &index) const
{
    if (!index.isValid() || index.model() != this)
        return QModelIndex();

    WordAttribModel *m = (WordAttribModel*)sourceModel();

    return m->index(list[index.row()]);
}

QModelIndex WordAttribProxyModel::mapFromSource(const QModelIndex &index) const
{
    if (!index.isValid() || index.model() != sourceModel())
        return QModelIndex();

    TreeItem *item = (TreeItem*)index.internalId();
    auto it = std::find(list.begin(), list.end(), item);
    if (it == list.end())
        return QModelIndex();

    return createIndex(it - list.begin(), 0, nullptr);
}

QModelIndex WordAttribProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= tosigned(list.size()) || column != 0)
        return QModelIndex();
    return createIndex(row, 0, nullptr);
}

QModelIndex WordAttribProxyModel::parent(const QModelIndex &/*child*/) const
{
    return QModelIndex();
}

int WordAttribProxyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return tosigned(list.size());
}

int WordAttribProxyModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

void WordAttribProxyModel::sourceDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright, const QVector<int> &roles)
{
    if (!topleft.isValid() || !bottomright.isValid() || topleft.parent() != bottomright.parent())
        return;

    QModelIndex index = topleft;
    while (index != bottomright)
    {
        auto it = std::find_if(list.begin(), list.end(), [&index](TreeItem *a) {
            return (TreeItem*)index.internalId() == a;
        });
        if (it != list.end())
        {
            QModelIndex myindex = createIndex(it - list.begin(), 0, (quintptr)0);
            emit dataChanged(myindex, myindex, roles);
        }
        index = index.sibling(index.row() + 1, index.column());
    }

    auto it = std::find_if(list.begin(), list.end(), [&index](TreeItem *a) {
        return (TreeItem*)index.internalId() == a;
    });

    if (it != list.end())
    {
        QModelIndex myindex = createIndex(it - list.begin(), 0, (quintptr)0);
        emit dataChanged(myindex, myindex, roles);
    }
}


//-------------------------------------------------------------


WordAttribWidget::WordAttribWidget(QWidget *parent) : base(parent), ui(new Ui::WordAttribWidget), jlpt(0), treemodel(new WordAttribModel(this, true)), listmodel(new WordAttribProxyModel(this))
{
    ui->setupUi(this);

    ui->treeView->setHeaderHidden(true);

    listmodel->setSourceModel(treemodel);

    ui->treeView->setModel(treemodel);

    auto *oldselmodel = ui->listView->selectionModel();
    ui->listView->setModel(listmodel);
    if (oldselmodel != ui->listView->selectionModel())
        oldselmodel->deleteLater();

    connect(ui->filterEdit, &QLineEdit::textChanged, this, &WordAttribWidget::textChanged);
    connect(treemodel, &WordAttribModel::checked, this, &WordAttribWidget::changed);

    clearChecked();
}

WordAttribWidget::~WordAttribWidget()
{
    delete ui;
}

void WordAttribWidget::setChecked(const WordDefAttrib &a, uchar i, uchar j)
{
    attrib = a;
    inf = i;
    jlpt = j;
    treemodel->ownerDataChanged();
}

void WordAttribWidget::clearChecked()
{
    attrib.types = 0;
    attrib.fields = 0;
    attrib.notes = 0;
    attrib.dialects = 0;
    inf = 0;
    jlpt = 0;
    treemodel->ownerDataChanged();
}

const WordDefAttrib& WordAttribWidget::checkedTypes() const
{
    return attrib;
}

uchar WordAttribWidget::checkedInfo() const
{
    return inf;
}

uchar WordAttribWidget::checkedJLPT() const
{
    return jlpt;
}

bool WordAttribWidget::showGlobal() const
{
    return treemodel->showGlobal();
}

void WordAttribWidget::setShowGlobal(bool show)
{
    treemodel->setShowGlobal(show);
}

bool WordAttribWidget::typeSelected(uchar index) const
{
    return (attrib.types & (1 << index)) != 0;
}

bool WordAttribWidget::noteSelected(uchar index) const
{
    return (attrib.notes & (1 << index)) != 0;
}

bool WordAttribWidget::fieldSelected(uchar index) const
{
    return (attrib.fields & (1 << index)) != 0;
}

bool WordAttribWidget::dialectSelected(uchar index) const
{
    return (attrib.dialects & (1 << index)) != 0;
}

bool WordAttribWidget::infoSelected(uchar index) const
{
    return (inf & (1 << index)) != 0;
}

bool WordAttribWidget::jlptSelected(uchar index) const
{
    return (jlpt & (1 << index)) != 0;
}

void WordAttribWidget::setTypeSelected(uchar index, bool select)
{
    if (select)
        attrib.types |= (1 << index);
    else
        attrib.types &= ~(1 << index);
}

void WordAttribWidget::setNoteSelected(uchar index, bool select)
{
    if (select)
        attrib.notes |= (1 << index);
    else
        attrib.notes &= ~(1 << index);
}

void WordAttribWidget::setFieldSelected(uchar index, bool select)
{
    if (select)
        attrib.fields |= (1 << index);
    else
        attrib.fields &= ~(1 << index);
}

void WordAttribWidget::setDialectSelected(uchar index, bool select)
{
    if (select)
        attrib.dialects |= (1 << index);
    else
        attrib.dialects &= ~(1 << index);
}

void WordAttribWidget::setInfoSelected(uchar index, bool select)
{
    if (select)
        inf |= (1 << index);
    else
        inf &= ~(1 << index);
}

void WordAttribWidget::setJlptSelected(uchar index, bool select)
{
    if (select)
        jlpt |= (1 << index);
    else
        jlpt &= ~(1 << index);
}

void WordAttribWidget::setTypeBits(uint bits, bool select)
{
    if (select)
        attrib.types |= bits;
    else
        attrib.types &= ~bits;
}

void WordAttribWidget::setNoteBits(uint bits, bool select)
{
    if (select)
        attrib.notes |= bits;
    else
        attrib.notes &= ~bits;
}

void WordAttribWidget::setFieldBits(uint bits, bool select)
{
    if (select)
        attrib.fields |= bits;
    else
        attrib.fields &= ~bits;
}

void WordAttribWidget::setDialectBits(ushort bits, bool select)
{
    if (select)
        attrib.dialects |= bits;
    else
        attrib.dialects &= ~bits;
}

void WordAttribWidget::setInfoBits(uchar bits, bool select)
{
    if (select)
        inf |= bits;
    else
        inf &= ~bits;
}

void WordAttribWidget::setJlptBits(uchar bits, bool select)
{
    if (select)
        jlpt |= bits;
    else
        jlpt &= ~bits;
}

bool WordAttribWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    return base::event(e);
}

void WordAttribWidget::textChanged(const QString &str)
{
    listmodel->setFilterText(str);
    ui->viewStack->setCurrentIndex(str.trimmed().isEmpty() ? 0 : 1);
}


//-------------------------------------------------------------
