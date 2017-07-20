/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZCOLORCOMBOBOX_H
#define ZCOLORCOMBOBOX_H

#include <QComboBox>
#include <QItemDelegate>

class ZColorComboBox;

class ZColorListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ZColorListModel(QObject *parent);
    virtual ~ZColorListModel();

    // Returns the color set with setDefaultColor(). The default color is shown at index 1 of
    // the combo box with the "Default" label.
    QColor defaultColor() const;
    // Sets the color to be shown at index 1 of the combo box with the "Default" label.
    void setDefaultColor(QColor color);

    // Returns whether the "Custom..." option is inserted at the front of the color list.
    bool listCustom() const;
    // Set whether the "Custom..." option should be inserted at the front of the color list.
    void setListCustom(bool allow);

    // Returns the color selected by the user in the color dialog with the "Custom..." item.
    QColor customColor();
    // Changes the color displayed next to the "Custom..." label. If listcustom is false,
    // returns without changing it.
    void setCustomColor(QColor color);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const override;
private:
    // When valid, the default color that appears at the first index of the combo box.
    QColor defcol;

    // Whether to list an item with the "Custom..." label. Selecting that item will open the
    // built in color selector dialog.
    bool listcustom;
    // The color selected by the user in the color selector dialog when listcustom is true.
    QColor custom;

    ZColorComboBox *owner;

    typedef QAbstractListModel  base;
};

class ZColorListDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    ZColorListDelegate(ZColorComboBox *owner, QObject *parent = nullptr);
    virtual ~ZColorListDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index = QModelIndex()) const override;
    //virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index = QModelIndex()) const override;
private:
    ZColorComboBox *owner;

    typedef QItemDelegate base;
};

class ZColorComboBox : public QComboBox
{
    Q_OBJECT
signals :
    void activated(int index);
    void activated(const QString &text);
    void currentIndexChanged(int index);
    void currentIndexChanged(const QString &text);
    void currentTextChanged(const QString &text);
    void currentColorChanged(QColor color);
public:
    ZColorComboBox(QWidget *parent = nullptr);
    virtual ~ZColorComboBox();

    QColor defaultColor() const;
    void setDefaultColor(QColor color);
    bool listCustom() const;
    void setListCustom(bool allow);

    // Sets the color to be the selected item in the combo box. If the passed color matches an
    // item in the list, it'll become the selected item. If the color is not found, and
    // listCustom() is true, the item with the "Custom..." label will be selected, displaying
    // this color. If the passed color is invalid and the box has a default color, the item
    // with the "Default" label will get selected. Otherwise the current color won't change.
    void setCurrentColor(QColor color);
    // Returns the color of the currently selected item, or an invalid color if the default is
    // selected.
    QColor currentColor() const;

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

    // The size of the item at index.
    QSize indexSizeHint(const QModelIndex &index) const;
protected:
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;
    virtual void changeEvent(QEvent *e) override;
private slots:
    void dataChanged();

    // Notification when the user selected an item in the combo box. The signal overrides are
    // all sent out by this single function.
    void baseActivated(int index);
private:
    // Recomputes the siz sizeHint().
    void computeSizeHint() const;

    // Cached size to be returned by sizeHint() and minimumSizeHint().
    mutable QSize siz;
    bool firstshown;

    // Saved previously selected index to be restored if user cancels the custom color
    // selection.
    int oldindex;

    using QComboBox::activated;
    using QComboBox::activated;
    using QComboBox::currentIndexChanged;
    using QComboBox::currentIndexChanged;
    using QComboBox::currentTextChanged;

    typedef QComboBox   base;
    friend class ZColorListDelegate;
};

#endif // ZCOLORCOMBOBOX_H

