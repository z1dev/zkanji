#ifndef ZLISTBOXMODEL_H
#define ZLISTBOXMODEL_H

#include "zabstracttablemodel.h"

class ZAbstractListBoxModel : public ZAbstractTableModel
{
    Q_OBJECT
public:
    ZAbstractListBoxModel(QObject *parent = nullptr);
    virtual ~ZAbstractListBoxModel();

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
private:
    typedef ZAbstractTableModel base;
};

#endif // ZLISTBOXMODEL_H
