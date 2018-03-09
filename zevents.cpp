/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "zevents.h"


//-------------------------------------------------------------


SetDictEvent::SetDictEvent(int dictindex) : base(), dix(dictindex)
{

}

int SetDictEvent::index()
{
    return dix;
}


//-------------------------------------------------------------


GetDictEvent::GetDictEvent() : base(), dix(-1)
{
}

void GetDictEvent::setResult(int dictindex)
{
    dix = dictindex;
}

int GetDictEvent::result()
{
    return dix;
}


//-------------------------------------------------------------


//IsImportantWindowEvent::IsImportantWindowEvent() : base(), r(false)
//{
//
//}
//
//void IsImportantWindowEvent::setImportant()
//{
//    r = true;
//}
//
//bool IsImportantWindowEvent::result()
//{
//    return r;
//}


//-------------------------------------------------------------


TreeRemoveFakeItemEvent::TreeRemoveFakeItemEvent(TreeItem *parent) : base(), parent(parent)
{

}

TreeItem* TreeRemoveFakeItemEvent::itemParent()
{
    return parent;
}


//-------------------------------------------------------------


TreeAddFakeItemEvent::TreeAddFakeItemEvent(TreeItem *parent) : base(), parent(parent)
{

}

TreeItem* TreeAddFakeItemEvent::itemParent()
{
    return parent;
}


//-------------------------------------------------------------
