#include <QBoxLayout>
#include <QLabel>
#include <QStyle>

#include <cmath>

#include "zstatusbar.h"
#include "zui.h"
#include "globalui.h"
#include "generalsettings.h"

//-------------------------------------------------------------


ZStatusLayout::ZStatusLayout(QWidget *parent) : base(parent)
{
    int margin = Settings::scaled(2);
    setContentsMargins(margin, margin, margin, margin);
}

ZStatusLayout::~ZStatusLayout()
{
    ;
}

void ZStatusLayout::addItem(QLayoutItem *item)
{
    list.push_back(item);
    cacheheight = -1;
}

QLayoutItem* ZStatusLayout::itemAt(int index) const
{
    if (index < 0 || index >= list.size())
        return nullptr;
    return list.at(index);
}

QLayoutItem* ZStatusLayout::takeAt(int index)
{
    if (index < 0 || index >= list.size())
        return nullptr;

    cacheheight = -1;
    return list.takeAt(index);
}

int ZStatusLayout::count() const
{
    return list.size();
}

QSize ZStatusLayout::sizeHint() const
{
    return minimumSize();
}

QSize ZStatusLayout::minimumSize() const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    if (cacheheight == -1)
    {
        cacheheight = dynamic_cast<QWidget*>(parent()) != nullptr ? ((QWidget*)parent())->fontMetrics().height() : 0;
        for (const QLayoutItem *item : list)
        {
            if (!item->isEmpty())
                cacheheight = std::max(cacheheight, item->minimumSize().height());
        }

    }

    return QSize(std::max(1, left + right), cacheheight + top + bottom);
}

void ZStatusLayout::invalidate()
{
    cacheheight = -1;
    base::invalidate();
}

void ZStatusLayout::setGeometry(const QRect &r)
{
    cacheheight = -1;
    if (parentWidget()->isVisibleTo(parentWidget()->window()))
        realign(r);
    QRect r2 = r;
    r2.setHeight(minimumSize().height());
    base::setGeometry(r2);

    parentWidget()->setMinimumHeight(r2.height());
    parentWidget()->setMaximumHeight(r2.height());
}

void ZStatusLayout::realign(const QRect &r)
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    cacheheight = dynamic_cast<QWidget*>(parent()) != nullptr ? ((QWidget*)parent())->fontMetrics().height() : 0;

    QStyle *style = nullptr;
    int hs = Settings::scaled(6);
    if (hs < 0 && parentWidget() != nullptr)
        style = parentWidget()->style();
    int spacex = 0;

    std::vector<int> sizes;

    // Width remaining for expanding items.
    int width = r.width() - left - right;

    // Number of items that don't have a fixed width in the current row.
    //int expnum = 0;

    // Sum of width of the expanding items.
    int expwidth = 0;

    for (auto lit = list.begin(); lit != list.end();)
    {
        // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
        if ((*lit)->isEmpty())
        {
            ++lit;
            continue;
        }

        QLayoutItem *item = *lit;

        cacheheight = std::max(cacheheight, item->sizeHint().height());

        int itemw = item->sizeHint().width();

        QWidget *widget = item->widget();
        int whs = hs;

        // Negative for items that cannot grow from their current size.
        int limitprefix = -1;

        if (widget != nullptr && widget->sizePolicy().horizontalPolicy() != QSizePolicy::Fixed)
        {
            // Making sure something is saved in sizes even if sizeHint() returned a bad
            // value.
            if (itemw <= 0)
                itemw = 1;

            limitprefix = 1;
            //++expnum;
            expwidth += itemw;
        }

        auto nextit = std::next(lit);
        while (nextit != list.end() && (*nextit)->isEmpty())
            ++nextit;

        if (nextit != list.end())
            computeSpacing(item, *nextit, spacex, whs, style);

        sizes.push_back(limitprefix * itemw);

        width -= itemw + (nextit != list.end() ? spacex : 0);

        lit = nextit;
    }

    int x = left;
    int sizespos = 0;

    for (auto lit = list.begin(); lit != list.end();)
    {
        // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
        if ((*lit)->isEmpty())
        {
            ++lit;
            continue;
        }

        QLayoutItem *item = *lit;

        int itemh = item->sizeHint().height();

        QWidget *widget = item->widget();
        int whs = hs;

        auto nextit = std::next(lit);
        while (nextit != list.end() && (*nextit)->isEmpty())
            ++nextit;

        if (nextit != list.end())
            computeSpacing(item, *nextit, spacex, whs, style);

        int itemw = sizes[sizespos];
        if (itemw < 0)
        {
            itemw = std::abs(itemw);
            if (width > 0)
            {
                int extraw = (width * ((double)itemw / expwidth));
                //--expnum;
                expwidth -= itemw;
                itemw += extraw;
            }
        }

        item->setGeometry(QRect(x, top + (cacheheight - itemh) / 2, itemw, itemh));

        x += itemw + spacex;

        lit = nextit;
        ++sizespos;
    }
}

QSize ZStatusLayout::minimumSize()
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    if (cacheheight == -1)
    {
        cacheheight = dynamic_cast<QWidget*>(parent()) != nullptr ? ((QWidget*)parent())->fontMetrics().height() : 0;
        for (const QLayoutItem *item : list)
        {
            if (!item->isEmpty())
                cacheheight = std::max(cacheheight, item->minimumSize().height());
        }
    }

    return QSize(std::max(1, left + right), cacheheight + top + bottom);
}

void ZStatusLayout::computeSpacing(QLayoutItem *first, QLayoutItem *next, int &spacex, int hs, const QStyle *style)
{
    QSizePolicy::ControlTypes c1 = QSizePolicy::DefaultType;
    QSizePolicy::ControlTypes c2 = QSizePolicy::DefaultType;

    spacex = hs;

    QWidget *p = parentWidget();

    if (hs < 0)
    {
        c1 = first->controlTypes();

        if (next != nullptr)
            c2 = next->controlTypes();

        if (style != nullptr)
            spacex = style->combinedLayoutSpacing(c1, c2, Qt::Horizontal, 0, p);
        else
            spacex = 0;
    }
}


//-------------------------------------------------------------


ZStatusBar::ZStatusBar(QWidget *parent) : base(parent)
{
    QSizePolicy pol = sizePolicy();
    pol.setVerticalPolicy(QSizePolicy::Fixed);
    pol.setHorizontalPolicy(QSizePolicy::Ignored);
    setSizePolicy(pol);

    if (layout() != nullptr)
        delete layout();

    ZStatusLayout *l = new ZStatusLayout(this);
    setLayout(l);
}

ZStatusBar::~ZStatusBar()
{

}

int ZStatusBar::size() const
{
    return list.size();
}

bool ZStatusBar::empty() const
{
    return list.empty();
}

void ZStatusBar::clear()
{
    for (int ix = list.size() - 1; ix != -1; --ix)
        delete list[ix].second;
    list.clear();
}

int ZStatusBar::add(QString value, int vsiz)
{
    QWidget *w = new QWidget();

    QBoxLayout *l = new QBoxLayout(QBoxLayout::LeftToRight, nullptr);
    l->setContentsMargins(0, 0, 0, 0);

    QLabel *lb = new QLabel(nullptr);
    lb->setText(value);
    if (vsiz == 0)
    {
        QSizePolicy pol = lb->sizePolicy();
        pol.setHorizontalPolicy(QSizePolicy::Expanding);
        lb->setSizePolicy(pol);
    }
    else
        restrictWidgetSize(lb, vsiz);
    l->addWidget(lb);

    w->setLayout(l);

    addWidget(w);

    list.push_back(std::make_pair(StatusTypes::SingleValue, w));

    gUI->scaleWidget(w);

    return list.size() - 1;
}

int ZStatusBar::add(QString title, int lsiz, QString value, int vsiz, bool alignright)
{
    QWidget *w = new QWidget();

    QBoxLayout *l = new QBoxLayout(QBoxLayout::LeftToRight, nullptr);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(w->fontMetrics().averageCharWidth());

    QLabel *lb = new QLabel(nullptr);
    lb->setText(title);
    if (lsiz == 0)
    {
        QSizePolicy pol = lb->sizePolicy();
        pol.setHorizontalPolicy(QSizePolicy::Fixed);
        lb->setSizePolicy(pol);
    }
    else
        restrictWidgetSize(lb, lsiz);
    l->addWidget(lb);
        
    lb = new QLabel(nullptr);
    lb->setText(value);
    if (vsiz == 0)
    {
        QSizePolicy pol = lb->sizePolicy();
        pol.setHorizontalPolicy(QSizePolicy::Fixed);
        lb->setSizePolicy(pol);
    }
    else
        restrictWidgetSize(lb, vsiz);
    if (alignright)
        lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    l->addWidget(lb);

    w->setLayout(l);

    addWidget(w);

    list.push_back(std::make_pair(StatusTypes::TitleValue, w));

    gUI->scaleWidget(w);

    return list.size() - 1;
}

void ZStatusBar::setValue(int index, QString str)
{
    if (index < 0 || index >= list.size() || (list[index].first != StatusTypes::TitleValue && list[index].first != StatusTypes::SingleValue))
        return;

    ((QLabel*)((QBoxLayout*)list[index].second->layout())->itemAt(list[index].first == StatusTypes::TitleValue ? 1 : 0)->widget())->setText(str);
}

QString ZStatusBar::value(int index) const
{
    if (index < 0 || index >= list.size() || (list[index].first != StatusTypes::TitleValue && list[index].first != StatusTypes::SingleValue))
        return QString();

    return ((QLabel*)((QBoxLayout*)list[index].second->layout())->itemAt(list[index].first == StatusTypes::TitleValue ? 1 : 0)->widget())->text();
}

int ZStatusBar::add(QString title, int lsiz, QString value1, int vsiz1, QString value2, int vsiz2)
{
    QWidget *w = new QWidget();

    QBoxLayout *l = new QBoxLayout(QBoxLayout::LeftToRight, nullptr);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(w->fontMetrics().averageCharWidth());

    QBoxLayout *lv = new QBoxLayout(QBoxLayout::LeftToRight, nullptr);
    lv->setContentsMargins(0, 0, 0, 0);
    lv->setSpacing(std::max(2, w->fontMetrics().averageCharWidth() / 3));

    QLabel *lb = nullptr;
    
    if (!title.isEmpty())
    {
        lb = new QLabel(nullptr);
        lb->setText(title);
        if (lsiz == 0)
        {
            QSizePolicy pol = lb->sizePolicy();
            pol.setHorizontalPolicy(QSizePolicy::Fixed);
            lb->setSizePolicy(pol);
        }
        else
            restrictWidgetSize(lb, lsiz);
        l->addWidget(lb);
    }

    lb = new QLabel(nullptr);
    lb->setText(value1);
    if (vsiz1 == 0)
    {
        QSizePolicy pol = lb->sizePolicy();
        pol.setHorizontalPolicy(QSizePolicy::Fixed);
        lb->setSizePolicy(pol);
    }
    else
        restrictWidgetSize(lb, vsiz1);
    lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lv->addWidget(lb);
    lb = new QLabel(nullptr);
    lb->setText(value2);
    if (vsiz2 == 0)
    {
        QSizePolicy pol = lb->sizePolicy();
        pol.setHorizontalPolicy(QSizePolicy::Fixed);
        lb->setSizePolicy(pol);
    }
    else
        restrictWidgetSize(lb, vsiz2);
    lb->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lv->addWidget(lb);
    l->addLayout(lv);

    w->setLayout(l);

    addWidget(w);

    list.push_back(std::make_pair(title.isEmpty() ? StatusTypes::DoubleValue : StatusTypes::TitleDouble, w));

    gUI->scaleWidget(w);

    return list.size() - 1;
}

void ZStatusBar::setValues(int index, QString val1, QString val2)
{
    if (index < 0 || index >= list.size() || (list[index].first != StatusTypes::TitleDouble && list[index].first != StatusTypes::DoubleValue))
        return;

    if (list[index].first == StatusTypes::TitleDouble)
    {
        ((QLabel*)list[index].second->layout()->itemAt(1)->layout()->itemAt(0)->widget())->setText(val1);
        ((QLabel*)list[index].second->layout()->itemAt(1)->layout()->itemAt(1)->widget())->setText(val2);
    }
    else
    {
        ((QLabel*)list[index].second->layout()->itemAt(0)->layout()->itemAt(0)->widget())->setText(val1);
        ((QLabel*)list[index].second->layout()->itemAt(0)->layout()->itemAt(1)->widget())->setText(val2);
    }
}

void ZStatusBar::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
}

void ZStatusBar::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
}

void ZStatusBar::addWidget(QWidget *w)
{
    ((ZStatusLayout*)layout())->addWidget(w);
}


//-------------------------------------------------------------

