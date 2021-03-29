/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QStylePainter>
#include <QApplication>
#include <QAbstractItemView>
#include <QColorDialog>

#include <cmath>

#include "zcolorcombobox.h"
#include "colorsettings.h"

#include "checked_cast.h"

//-------------------------------------------------------------


static std::vector<std::pair<std::string, QColor>> basecolors =
{ { QT_TR_NOOP("aliceblue"), QColor(240, 248, 255) },
{ QT_TR_NOOP("antiquewhite"), QColor(250, 235, 215) },
{ QT_TR_NOOP("aqua"), QColor( 0, 255, 255) },
{ QT_TR_NOOP("aquamarine"), QColor(127, 255, 212) },
{ QT_TR_NOOP("azure"), QColor(240, 255, 255) },
{ QT_TR_NOOP("beige"), QColor(245, 245, 220) },
{ QT_TR_NOOP("bisque"), QColor(255, 228, 196) },
{ QT_TR_NOOP("black"), QColor( 0, 0, 0) },
{ QT_TR_NOOP("blanchedalmond"), QColor(255, 235, 205) },
{ QT_TR_NOOP("blue"), QColor( 0, 0, 255) },
{ QT_TR_NOOP("blueviolet"), QColor(138, 43, 226) },
{ QT_TR_NOOP("brown"), QColor(165, 42, 42) },
{ QT_TR_NOOP("burlywood"), QColor(222, 184, 135) },
{ QT_TR_NOOP("cadetblue"), QColor( 95, 158, 160) },
{ QT_TR_NOOP("chartreuse"), QColor(127, 255, 0) },
{ QT_TR_NOOP("chocolate"), QColor(210, 105, 30) },
{ QT_TR_NOOP("coral"), QColor(255, 127, 80) },
{ QT_TR_NOOP("cornflowerblue"), QColor(100, 149, 237) },
{ QT_TR_NOOP("cornsilk"), QColor(255, 248, 220) },
{ QT_TR_NOOP("crimson"), QColor(220, 20, 60) },
{ QT_TR_NOOP("cyan"), QColor( 0, 255, 255) },
{ QT_TR_NOOP("darkblue"), QColor( 0, 0, 139) },
{ QT_TR_NOOP("darkcyan"), QColor( 0, 139, 139) },
{ QT_TR_NOOP("darkgoldenrod"), QColor(184, 134, 11) },
{ QT_TR_NOOP("darkgray"), QColor(169, 169, 169) },
{ QT_TR_NOOP("darkgreen"), QColor( 0, 100, 0) },
{ QT_TR_NOOP("darkgrey"), QColor(169, 169, 169) },
{ QT_TR_NOOP("darkkhaki"), QColor(189, 183, 107) },
{ QT_TR_NOOP("darkmagenta"), QColor(139, 0, 139) },
{ QT_TR_NOOP("darkolivegreen"), QColor( 85, 107, 47) },
{ QT_TR_NOOP("darkorange"), QColor(255, 140, 0) },
{ QT_TR_NOOP("darkorchid"), QColor(153, 50, 204) },
{ QT_TR_NOOP("darkred"), QColor(139, 0, 0) },
{ QT_TR_NOOP("darksalmon"), QColor(233, 150, 122) },
{ QT_TR_NOOP("darkseagreen"), QColor(143, 188, 143) },
{ QT_TR_NOOP("darkslateblue"), QColor( 72, 61, 139) },
{ QT_TR_NOOP("darkslategray"), QColor( 47, 79, 79) },
{ QT_TR_NOOP("darkslategrey"), QColor( 47, 79, 79) },
{ QT_TR_NOOP("darkturquoise"), QColor( 0, 206, 209) },
{ QT_TR_NOOP("darkviolet"), QColor(148, 0, 211) },
{ QT_TR_NOOP("deeppink"), QColor(255, 20, 147) },
{ QT_TR_NOOP("deepskyblue"), QColor( 0, 191, 255) },
{ QT_TR_NOOP("dimgray"), QColor(105, 105, 105) },
{ QT_TR_NOOP("dimgrey"), QColor(105, 105, 105) },
{ QT_TR_NOOP("dodgerblue"), QColor( 30, 144, 255) },
{ QT_TR_NOOP("firebrick"), QColor(178, 34, 34) },
{ QT_TR_NOOP("floralwhite"), QColor(255, 250, 240) },
{ QT_TR_NOOP("forestgreen"), QColor( 34, 139, 34) },
{ QT_TR_NOOP("fuchsia"), QColor(255, 0, 255) },
{ QT_TR_NOOP("gainsboro"), QColor(220, 220, 220) },
{ QT_TR_NOOP("ghostwhite"), QColor(248, 248, 255) },
{ QT_TR_NOOP("gold"), QColor(255, 215, 0) },
{ QT_TR_NOOP("goldenrod"), QColor(218, 165, 32) },
{ QT_TR_NOOP("gray"), QColor(128, 128, 128) },
{ QT_TR_NOOP("grey"), QColor(128, 128, 128) },
{ QT_TR_NOOP("green"), QColor( 0, 128, 0) },
{ QT_TR_NOOP("greenyellow"), QColor(173, 255, 47) },
{ QT_TR_NOOP("honeydew"), QColor(240, 255, 240) },
{ QT_TR_NOOP("hotpink"), QColor(255, 105, 180) },
{ QT_TR_NOOP("indianred"), QColor(205, 92, 92) },
{ QT_TR_NOOP("indigo"), QColor( 75, 0, 130) },
{ QT_TR_NOOP("ivory"), QColor(255, 255, 240) },
{ QT_TR_NOOP("khaki"), QColor(240, 230, 140) },
{ QT_TR_NOOP("lavender"), QColor(230, 230, 250) },
{ QT_TR_NOOP("lavenderblush"), QColor(255, 240, 245) },
{ QT_TR_NOOP("lawngreen"), QColor(124, 252, 0) },
{ QT_TR_NOOP("lemonchiffon"), QColor(255, 250, 205) },
{ QT_TR_NOOP("lightblue"), QColor(173, 216, 230) },
{ QT_TR_NOOP("lightcoral"), QColor(240, 128, 128) },
{ QT_TR_NOOP("lightcyan"), QColor(224, 255, 255) },
{ QT_TR_NOOP("lightgoldenrodyellow"), QColor(250, 250, 210) },
{ QT_TR_NOOP("lightgray"), QColor(211, 211, 211) },
{ QT_TR_NOOP("lightgreen"), QColor(144, 238, 144) },
{ QT_TR_NOOP("lightgrey"), QColor(211, 211, 211) },
{ QT_TR_NOOP("lightpink"), QColor(255, 182, 193) },
{ QT_TR_NOOP("lightsalmon"), QColor(255, 160, 122) },
{ QT_TR_NOOP("lightseagreen"), QColor( 32, 178, 170) },
{ QT_TR_NOOP("lightskyblue"), QColor(135, 206, 250) },
{ QT_TR_NOOP("lightslategray"), QColor(119, 136, 153) },
{ QT_TR_NOOP("lightslategrey"), QColor(119, 136, 153) },
{ QT_TR_NOOP("lightsteelblue"), QColor(176, 196, 222) },
{ QT_TR_NOOP("lightyellow"), QColor(255, 255, 224) },
{ QT_TR_NOOP("lime"), QColor( 0, 255, 0) },
{ QT_TR_NOOP("limegreen"), QColor( 50, 205, 50) },
{ QT_TR_NOOP("linen"), QColor(250, 240, 230) },
{ QT_TR_NOOP("magenta"), QColor(255, 0, 255) },
{ QT_TR_NOOP("maroon"), QColor(128, 0, 0) },
{ QT_TR_NOOP("mediumaquamarine"), QColor(102, 205, 170) },
{ QT_TR_NOOP("mediumblue"), QColor( 0, 0, 205) },
{ QT_TR_NOOP("mediumorchid"), QColor(186, 85, 211) },
{ QT_TR_NOOP("mediumpurple"), QColor(147, 112, 219) },
{ QT_TR_NOOP("mediumseagreen"), QColor( 60, 179, 113) },
{ QT_TR_NOOP("mediumslateblue"), QColor(123, 104, 238) },
{ QT_TR_NOOP("mediumspringgreen"), QColor( 0, 250, 154) },
{ QT_TR_NOOP("mediumturquoise"), QColor( 72, 209, 204) },
{ QT_TR_NOOP("mediumvioletred"), QColor(199, 21, 133) },
{ QT_TR_NOOP("midnightblue"), QColor( 25, 25, 112) },
{ QT_TR_NOOP("mintcream"), QColor(245, 255, 250) },
{ QT_TR_NOOP("mistyrose"), QColor(255, 228, 225) },
{ QT_TR_NOOP("moccasin"), QColor(255, 228, 181) },
{ QT_TR_NOOP("navajowhite"), QColor(255, 222, 173) },
{ QT_TR_NOOP("navy"), QColor( 0, 0, 128) },
{ QT_TR_NOOP("oldlace"), QColor(253, 245, 230) },
{ QT_TR_NOOP("olive"), QColor(128, 128, 0) },
{ QT_TR_NOOP("olivedrab"), QColor(107, 142, 35) },
{ QT_TR_NOOP("orange"), QColor(255, 165, 0) },
{ QT_TR_NOOP("orangered"), QColor(255, 69, 0) },
{ QT_TR_NOOP("orchid"), QColor(218, 112, 214) },
{ QT_TR_NOOP("palegoldenrod"), QColor(238, 232, 170) },
{ QT_TR_NOOP("palegreen"), QColor(152, 251, 152) },
{ QT_TR_NOOP("paleturquoise"), QColor(175, 238, 238) },
{ QT_TR_NOOP("palevioletred"), QColor(219, 112, 147) },
{ QT_TR_NOOP("papayawhip"), QColor(255, 239, 213) },
{ QT_TR_NOOP("peachpuff"), QColor(255, 218, 185) },
{ QT_TR_NOOP("peru"), QColor(205, 133, 63) },
{ QT_TR_NOOP("pink"), QColor(255, 192, 203) },
{ QT_TR_NOOP("plum"), QColor(221, 160, 221) },
{ QT_TR_NOOP("powderblue"), QColor(176, 224, 230) },
{ QT_TR_NOOP("purple"), QColor(128, 0, 128) },
{ QT_TR_NOOP("red"), QColor(255, 0, 0) },
{ QT_TR_NOOP("rosybrown"), QColor(188, 143, 143) },
{ QT_TR_NOOP("royalblue"), QColor( 65, 105, 225) },
{ QT_TR_NOOP("saddlebrown"), QColor(139, 69, 19) },
{ QT_TR_NOOP("salmon"), QColor(250, 128, 114) },
{ QT_TR_NOOP("sandybrown"), QColor(244, 164, 96) },
{ QT_TR_NOOP("seagreen"), QColor( 46, 139, 87) },
{ QT_TR_NOOP("seashell"), QColor(255, 245, 238) },
{ QT_TR_NOOP("sienna"), QColor(160, 82, 45) },
{ QT_TR_NOOP("silver"), QColor(192, 192, 192) },
{ QT_TR_NOOP("skyblue"), QColor(135, 206, 235) },
{ QT_TR_NOOP("slateblue"), QColor(106, 90, 205) },
{ QT_TR_NOOP("slategray"), QColor(112, 128, 144) },
{ QT_TR_NOOP("slategrey"), QColor(112, 128, 144) },
{ QT_TR_NOOP("snow"), QColor(255, 250, 250) },
{ QT_TR_NOOP("springgreen"), QColor( 0, 255, 127) },
{ QT_TR_NOOP("steelblue"), QColor( 70, 130, 180) },
{ QT_TR_NOOP("tan"), QColor(210, 180, 140) },
{ QT_TR_NOOP("teal"), QColor( 0, 128, 128) },
{ QT_TR_NOOP("thistle"), QColor(216, 191, 216) },
{ QT_TR_NOOP("tomato"), QColor(255, 99, 71) },
{ QT_TR_NOOP("turquoise"), QColor( 64, 224, 208) },
{ QT_TR_NOOP("violet"), QColor(238, 130, 238) },
{ QT_TR_NOOP("wheat"), QColor(245, 222, 179) },
{ QT_TR_NOOP("white"), QColor(255, 255, 255) },
{ QT_TR_NOOP("whitesmoke"), QColor(245, 245, 245) },
{ QT_TR_NOOP("yellow"), QColor(255, 255, 0) },
{ QT_TR_NOOP("yellowgreen"), QColor(154, 205, 50) } };

ZColorListModel::ZColorListModel(QObject *parent) : base(parent), listcustom(true)
{
    owner = (ZColorComboBox*)parent;
}

ZColorListModel::~ZColorListModel()
{

}

QColor ZColorListModel::defaultColor() const
{
    return defcol;
}

void ZColorListModel::setDefaultColor(QColor color)
{
    if (defcol == color)
        return;

    bool changed = (defcol.isValid() != color.isValid());
    if (changed && !defcol.isValid())
        beginInsertRows(QModelIndex(), 0, 0);
    else if (changed && defcol.isValid())
        beginRemoveRows(QModelIndex(), 0, 0);
    defcol = color;
    if (changed && defcol.isValid())
        endInsertRows();
    else if (changed && !defcol.isValid())
        endRemoveRows();

    if (!changed && defcol.isValid())
        emit dataChanged(index(0, 0), index(0, 0));
}

bool ZColorListModel::listCustom() const
{
    return listcustom;
}

void ZColorListModel::setListCustom(bool allow)
{
    if (listcustom == allow)
        return;

    if (allow)
        beginInsertRows(QModelIndex(), defcol.isValid() ? 1 : 0, defcol.isValid() ? 1 : 0);
    else
        beginRemoveRows(QModelIndex(), defcol.isValid() ? 1 : 0, defcol.isValid() ? 1 : 0);
    listcustom = allow;
    if (!listcustom)
        custom = QColor();
    if (allow)
        endInsertRows();
    else
        endRemoveRows();
}

QColor ZColorListModel::customColor()
{
    if (listcustom)
        return custom;
    return QColor();
}

void ZColorListModel::setCustomColor(QColor color)
{
    if (!listcustom || (custom.isValid() == color.isValid() && custom.rgb() == color.rgb()))
        return;

    custom = color;
    emit dataChanged(index(defcol.isValid() ? 1 : 0, 0), index(defcol.isValid() ? 1 : 0, 0));
}

int ZColorListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return tosigned(basecolors.size()) + (defcol.isValid() ? 1 : 0) + (listcustom ? 1 : 0);
}

static const int ColorRole = Qt::UserRole;
static const int ColorTextRole = Qt::UserRole + 1;

QVariant ZColorListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (role != ColorRole && role != ColorTextRole && role != Qt::SizeHintRole))
        return QVariant();

    
    int row = index.row();
    if (role == ColorTextRole)
    {
        if (row == 0 && defcol.isValid())
            return tr("Default");
        if (listcustom && row == (defcol.isValid() ? 1 : 0))
            return tr("Custom...");

        return tr(basecolors[row - (defcol.isValid() ? 1 : 0) - (listcustom ? 1 : 0)].first.c_str());
    }

    if (role == Qt::SizeHintRole)
        return owner->indexSizeHint(index);

    // ColorRole.
    if (row == 0 && defcol.isValid())
        return defcol;
    if (listcustom && row == (defcol.isValid() ? 1 : 0))
        return custom.isValid() ? custom : QVariant();
    return basecolors[row - (defcol.isValid() ? 1 : 0) - (listcustom ? 1 : 0)].second;
}


//-------------------------------------------------------------


ZColorListDelegate::ZColorListDelegate(ZColorComboBox *owner, QObject *parent) : base(parent), owner(owner)
{

}

ZColorListDelegate::~ZColorListDelegate()
{

}

void ZColorListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &op, const QModelIndex &index) const
{
    base::paint(painter, op, index);

    QStyleOptionComboBox opt;
    owner->initStyleOption(&opt);

    QColor cl;
    if (index.data(ColorRole).isValid())
        cl = index.data(ColorRole).value<QColor>();
    QString str = index.data(ColorTextRole).toString();

    painter->save();
    painter->setClipRect(op.rect);

    QStyle *s = owner->style();
    QRect r = op.rect;
    int h = std::ceil(QFontMetricsF(opt.fontMetrics).height()) - 2;

    QRect clrect = s->alignedRect(op.direction, Qt::AlignLeft | Qt::AlignVCenter, QSize(h, h), r);

    clrect.adjust(2, 0, 1, -1);
    clrect.adjust(0, -1, 1, 0);

    if (cl.isValid())
    {
        painter->setBrush(cl);
        painter->fillRect(clrect.adjusted(1, 1, 0, 0), cl);
    }
    else
    {
        painter->setPen(QPen(Settings::uiColor(ColorSettings::StudyWrong), 2) /*QPen(Qt::red, 2)*/ );
        painter->drawLine(clrect.left() + 1, clrect.bottom() + 1, clrect.right(), clrect.top() + 2);
    }

    painter->setPen(owner->palette().color(owner->hasFocus() ? QPalette::Active : QPalette::Inactive, QPalette::Text));
    painter->drawRect(clrect);

    if (op.direction == Qt::RightToLeft)
        r.adjust(0, 0, -h - 6, 0);
    else
        r.adjust(h + 6, 0, 0, 0);

    if (!str.isEmpty())
    {
        if (op.state & QStyle::State_Selected)
            painter->setPen(op.palette.color(QPalette::HighlightedText));

        //QFont f(owner->font());
        //f.setFamily(owner->font().family());
        //f.setPointSize(owner->font().pointSize());

        //painter->setFont(f);
        s->drawItemText(painter, r.adjusted(1, 0, -1, 0), s->visualAlignment(op.direction, Qt::AlignLeft | Qt::AlignVCenter), op.palette, op.state & QStyle::State_Enabled, str);
    }

    painter->restore();
}

//QSize ZColorListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
//{
//    bool hascolor = index.data(ColorRole).isValid();
//    QString str = index.data(ColorTextRole).toString();
//
//    int h = option.fontMetrics.height();
//    return QSize(option.fontMetrics.boundingRect(str).width() + (hascolor ? h + 6 : 0),  h);
//}


//-------------------------------------------------------------

#define C11SIGNAL(classnam, ret, arg, name) ( ret ( classnam ::*)( arg ))(& classnam :: name)
#define C11SLOT(classnam, ret, arg, name)   ( ret ( classnam ::*)( arg ))(& classnam :: name)

ZColorComboBox::ZColorComboBox(QWidget *parent) : base(parent), firstshown(true), oldindex(0)
{
    ZColorListModel *m = new ZColorListModel(this);
    setModel(m);

    connect(m, &ZColorListModel::dataChanged, this, &ZColorComboBox::dataChanged);
    connect(m, &ZColorListModel::rowsInserted, this, &ZColorComboBox::dataChanged);
    connect(m, &ZColorListModel::rowsRemoved, this, &ZColorComboBox::dataChanged);

    itemDelegate()->deleteLater();
    setItemDelegate(new ZColorListDelegate(this));

    QFont f = font();
    f.setFamily(font().family());
    f.setPointSize(font().pointSize());
    view()->setFont(f);

    connect(this, C11SIGNAL(QComboBox, void, int, activated), this, C11SLOT(ZColorComboBox, void, int, baseActivated));
}

ZColorComboBox::~ZColorComboBox()
{

}

QColor ZColorComboBox::defaultColor() const
{
    return ((const ZColorListModel*)model())->defaultColor();
}

void ZColorComboBox::setDefaultColor(QColor color)
{
    ((ZColorListModel*)model())->setDefaultColor(color);
}

bool ZColorComboBox::listCustom() const
{
    return ((const ZColorListModel*)model())->listCustom();
}

void ZColorComboBox::setListCustom(bool allow)
{
    ((ZColorListModel*)model())->setListCustom(allow);
}

void ZColorComboBox::setCurrentColor(QColor color)
{
    bool hasdef = defaultColor().isValid();
    bool listcustom = listCustom();

    if (!color.isValid())
    {
        if (hasdef)
            setCurrentIndex(0);
        return;
    }

    for (int ix = 0, siz = tosigned(basecolors.size()); ix != siz; ++ix)
    {
        if (color.rgb() == basecolors[ix].second.rgb())
        {
            setCurrentIndex(ix + (hasdef ? 1 : 0) + (listcustom ? 1 : 0));
            return;
        }
    }

    if (!listCustom())
        return;

    ((ZColorListModel*)model())->setCustomColor(color);
    setCurrentIndex(hasdef ? 1 : 0);
}

QColor ZColorComboBox::currentColor() const
{
    int row = currentIndex();
    if (row == -1 || (row == 0 && defaultColor().isValid()))
        return QColor();
    if (defaultColor().isValid())
        --row;
    if (row == 0 && listCustom())
        return ((ZColorListModel*)model())->customColor();
    if (listCustom())
        --row;

    return basecolors[row].second;
}

QSize ZColorComboBox::sizeHint() const
{
    if (!sizecache.isValid())
        computeSizeHint();
    return sizecache;
}

QSize ZColorComboBox::minimumSizeHint() const
{
    return sizeHint();
}

void ZColorComboBox::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    int cix = currentIndex();
    if (cix == -1)
        return;

    ZColorListModel *m = ((ZColorListModel*)model());

    QColor cl;
    if (m->data(m->index(cix, 0), ColorRole).isValid())
        cl = m->data(m->index(cix, 0), ColorRole).value<QColor>();

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QStylePainter painter(this);
    QStyle *s = style();

    QRect r = s->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    painter.setClipRect(r);

    int h = std::ceil(QFontMetricsF(opt.fontMetrics).height()) - 2;

    QRect clrect = s->alignedRect(opt.direction, Qt::AlignLeft | Qt::AlignVCenter, QSize(h, h), r);

    clrect.adjust(0, 0, -1, -1);
    clrect.adjust(0, -1, 1, 0);

    if (cl.isValid())
    {
        painter.setBrush(cl);
        painter.fillRect(clrect.adjusted(1, 1, 0, 0), cl);
    }
    else
    {
        painter.setPen(QPen(Settings::uiColor(ColorSettings::StudyWrong), 2) /*QPen(Qt::red, 2)*/);
        painter.drawLine(clrect.left() + 1, clrect.bottom() + 1, clrect.right(), clrect.top() + 2);
    }

    painter.setPen(palette().color(hasFocus() ? QPalette::Active : QPalette::Inactive, QPalette::Text));
    //painter.setPen(Qt::black);
    painter.drawRect(clrect);

    if (opt.direction == Qt::RightToLeft)
        r.adjust(0, 0, -h - 4, 0);
    else
        r.adjust(h + 4, 0, 0, 0);

    QString str = m->data(m->index(cix, 0), ColorTextRole).toString();

    if (!str.isEmpty())
    {
        //QFont f = painter.font();
        //f.setHintingPreference(QFont::PreferNoHinting);
        //f.setPixelSize(11);

        painter.setFont(font());
        s->drawItemText(&painter, r.adjusted(1, 0, -1, 0), s->visualAlignment(opt.direction, Qt::AlignLeft | Qt::AlignVCenter), opt.palette, opt.state & QStyle::State_Enabled, str);
    }
}

void ZColorComboBox::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::StyleChange:
#ifdef Q_OS_MAC
    case QEvent::MacSizeChange:
#endif
        sizecache = QSize();
        break;
    default:
        break;
    }

    base::changeEvent(e);
}

void ZColorComboBox::showEvent(QShowEvent *e)
{
    if (firstshown && sizeAdjustPolicy() == QComboBox::AdjustToContentsOnFirstShow)
    {
        sizecache = QSize();
        updateGeometry();
    }
    firstshown = false;

    base::showEvent(e);
}

void ZColorComboBox::dataChanged()
{
    if (sizeAdjustPolicy() == QComboBox::AdjustToContents)
    {
        sizecache = QSize();
        updateGeometry();
    }
}

void ZColorComboBox::baseActivated(int index)
{
    ZColorListModel *m = ((ZColorListModel*)model());

    bool colorchanged = index != oldindex;

    if (listCustom() && index == (defaultColor().isValid() ? 1 : 0))
    {
        QColor orig = m->customColor();
        QColor ret = QColorDialog::getColor(orig, parentWidget()->window(), tr("Select a color"), QColorDialog::DontUseNativeDialog);
        if (!ret.isValid())
        {
            if (!m->customColor().isValid() && oldindex != index)
            {
                setCurrentIndex(oldindex);
                return;
            }
        }
        else
        {
            m->setCustomColor(ret);

            if (ret != orig && index == oldindex)
                colorchanged = true;
        }
    }

    QString str = m->data(m->index(index, 0), ColorTextRole).toString();
    if (index != oldindex)
    {
        emit currentIndexChanged(index);
        emit currentIndexChanged(str);
    }
    oldindex = index;
    emit activated(index);
    emit activated(str);

    if (colorchanged)
    {
        if (defaultColor().isValid() && index == 0)
            emit currentColorChanged(QColor());
        else
            emit currentColorChanged(m->data(m->index(index, 0), ColorRole).value<QColor>());
    }
}

QSize ZColorComboBox::indexSizeHint(const QModelIndex &index) const
{
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    
    //bool hascolor = index.data(ColorRole).isValid();
    QString str = index.data(ColorTextRole).toString();

    int h = std::ceil(QFontMetricsF(opt.fontMetrics).height());
    return QSize(opt.fontMetrics.boundingRect(str).width() + /*(hascolor ? */h + 6/* : 0)*/, std::max(h, 14) + 2);

}

void ZColorComboBox::computeSizeHint() const
{
    ZColorListModel *m = ((ZColorListModel*)model());

    int cnt = m->rowCount();

    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QStyle *s = style();

    // Taken from Qt source for combo box size hint calculation.
    int h = std::ceil(QFontMetricsF(opt.fontMetrics).height());
    sizecache.setHeight(std::max(h, 14) + 2);

    h -= 2;

    if (cnt == 0)
        sizecache.setWidth(opt.fontMetrics.width(QLatin1Char('x')) * 7);
    else
    {
        for (int ix = 0; ix != cnt; ++ix)
        {
            //bool hascolor = m->data(m->index(ix, 0), ColorRole).isValid();
            QString str = m->data(m->index(ix, 0), ColorTextRole).toString();

            sizecache.setWidth(std::max<int>(sizecache.width(), opt.fontMetrics.boundingRect(str).width() + /*(hascolor ?*/ h + 6 /*: 0)*/));
        }
    }

    if (opt.rect.isValid())
        opt.rect.setWidth(sizecache.width());
    sizecache = s->sizeFromContents(QStyle::CT_ComboBox, &opt, sizecache, this);
    sizecache = sizecache.expandedTo(qApp->globalStrut());
}


//-------------------------------------------------------------

