//#include <QLayout>
//#include <QFontMetrics>
#include <QHeaderView>
#include <QApplication>
#include "zlistbox.h"
#include "zlistboxmodel.h"


//-------------------------------------------------------------


ZListBox::ZListBox(QWidget *parent) : base(parent)
{
    setShowGrid(false);
    horizontalHeader()->hide();
    verticalHeader()->hide();
    setFont(QApplication::font("QListViewFont"));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

ZListBox::~ZListBox()
{
    ;
}

void ZListBox::setModel(ZAbstractListBoxModel *newmodel)
{
    base::setModel(newmodel);
}

ZAbstractListBoxModel* ZListBox::model() const
{
    return (ZAbstractListBoxModel*)base::model();
}


//-------------------------------------------------------------
