/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZKANALINEEDIT_H
#define ZKANALINEEDIT_H

#include "zlineedit.h"

class Dictionary;
class ZKanaLineEdit : public ZLineEdit
{
    Q_OBJECT
signals:
    void dictionaryChanged();
public:
    ZKanaLineEdit(QWidget *parent);
    virtual ~ZKanaLineEdit();

    Dictionary* dictionary() const;
    void setDictionary(Dictionary *d);
public slots:
    void settingsChanged();
protected:
    virtual QMenu* createContextMenu(QContextMenuEvent *e) override;

    virtual void contextMenuEvent(QContextMenuEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;

    virtual void mousePressEvent(QMouseEvent* e) override;
    virtual void mouseMoveEvent(QMouseEvent* e) override;
    virtual void mouseReleaseEvent(QMouseEvent* e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
private:
    // If a validator is installed, updates the current text to be valid.
    void fixInput();

    // The dictionary to be used in the kanji information window displayed from the context
    // menu.
    Dictionary *dict;

    typedef ZLineEdit   base;
};


#endif // ZKANALINEEDIT_H
