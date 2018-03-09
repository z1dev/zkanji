/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZCOMBOBOX_H
#define ZCOMBOBOX_H

#include <QComboBox>

// Combo box class based on QComboBox that replaces the lineedit with a ZLineEdit. Because
// of the nature of Qt, items must be added to the combo box' list programmatically.
// If the box is not set to be editable, it behaves like a normal combo box.
class ZComboBox : public QComboBox
{
    Q_OBJECT
public:
    ZComboBox(QWidget *parent = nullptr);
    virtual ~ZComboBox();

    void setEditable(bool editable);
    void setValidator(const QValidator *val);
    QString text() const;
    void setText(const QString &str);

    // Sets the preferred width of this line edit in number of characters. Only works in
    // layouts that use the sizeHint() function to determine the width of widgets. The
    // function must be called after the font changes to accomodate its character sizes.
    void setCharacterSize(int chsiz);

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
protected:
    virtual void installEditor();
    // Makes sure nothing gets selected initially.
    //virtual void showEvent(QShowEvent *e) override;
private slots:
    void indexChanged(int index);
private:
    // Throws exception if the editor is invalid.
    void checkThrow() const;

    bool checkchange;
    //bool firstshow;

    // Width of the size hint returned by sizeHint() in pixels. Only used when it's a positive
    // number.
    int shwidth;

    typedef QComboBox   base;
    using base::setEditable;
    using base::currentText;
    using base::setCurrentText;
};

class Dictionary;
class ZKanaComboBox : public ZComboBox
{
    Q_OBJECT
public:
    ZKanaComboBox(QWidget *parent = nullptr);
    virtual ~ZKanaComboBox();

    Dictionary* dictionary() const;
    void setDictionary(Dictionary *dict);
protected:
    virtual void installEditor() override;
private:
    typedef ZComboBox   base;
};

#endif // ZCOMBOBOX_H

