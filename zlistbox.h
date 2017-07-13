#ifndef ZLISTBOX_H
#define ZLISTBOX_H

#include "zlistview.h"

class ZAbstractListBoxModel;
// Same as QListView where the sizeHint() can be specified to make the view resizable to a
// smaller size than originally.
class ZListBox : public ZListView
{
    Q_OBJECT
public:
    ZListBox(QWidget *parent = nullptr);
    virtual ~ZListBox();

    void setModel(ZAbstractListBoxModel *newmodel);
    ZAbstractListBoxModel* model() const;
private:

    typedef ZListView  base;
};

#endif // ZLISTBOX_H
