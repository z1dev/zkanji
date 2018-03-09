/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZEVENTS_H
#define ZEVENTS_H

#include <QtEvents>


// Header for custom events that automatically assign a type for themselves.
// Usage:
// Define either with ZEVENT(eventname), or derive from EventTBase<>. See examples below.
// The new events are best added in this file, so this header is not needed in other headers,
// only in source files.
// Sending an event created this way: qApp->postEvent/sendEvent([reciever], new [EventTypeName])
// Handling the event: override QObject::event() in the class that needs to handle the custom
// events. To check the type of the received event use:
//      if (event->type() == [EventTypeName]::Type()) { ... }


// Template base for event type registering. Only used by EventTBase, and
// shouldn't be relied on it in any other class.
template<typename EventT>
class EventTypeReg
{
public:
    static QEvent::Type Type()
    {
        if (regtype == -1)
            regtype = QEvent::registerEventType();
        return (QEvent::Type)regtype;
    }
private:
    static int regtype;
};
template<typename EventT>
int EventTypeReg<EventT>::regtype = -1;

// Base class of all custom made events that register the event type at first
// call.
template<typename EventT>
class EventTBase : public QEvent
{
public:
    EventTBase() : base(EventTypeReg<EventT>::Type()) {}
    static QEvent::Type Type()
    {
        return EventTypeReg<EventT>::Type();
    }
private:
    typedef QEvent  base;
};

// Every custom event must be derived from EventTBase<self type>
// For simple events that don't require arguments this define generates code
// for the class
#define ZEVENT(eventname)     class eventname : public EventTBase< eventname > { };

// The following types are events. Call their built in Type() function
// when checking in the event handling of widgets.

ZEVENT(InitWindowEvent)
ZEVENT(StartEvent)
ZEVENT(EndEvent)
ZEVENT(EditorClosedEvent)
ZEVENT(ConstructedEvent)
ZEVENT(SaveSizeEvent)

// Notifies a main window to change its active dictionary to another.
class SetDictEvent : public EventTBase<SetDictEvent>
{
public:
    SetDictEvent(int dictindex);
    int index();
private:
    int dix;

    typedef EventTBase<SetDictEvent>  base;
};

// Asks a main window about its current active dictionary. The window should
// call setResult in its event handler to specify the dictionary index. The
// event must be sent with sendEvent as postEvent is executed at a random time
// and the value can't be retrieved later.
class GetDictEvent : public EventTBase<GetDictEvent>
{
public:
    GetDictEvent();

    // Used when handling the event to return the current dictionary index.
    void setResult(int dictindex);
    // The value set by the handling window or -1 if the event hasn't been
    // handled.
    int result();
private:
    int dix;

    typedef EventTBase<GetDictEvent>  base;
};

// Asks a window whether it's important enough, to hide all tool windows that would otherwise
// be topmost, and possibly hide the view from the important window. When the event is not
// handled or setImportant() is not called, the result will be false.
//class IsImportantWindowEvent : public EventTBase<IsImportantWindowEvent>
//{
//public:
//    IsImportantWindowEvent();
//
//    void setImportant();
//    bool result();
//private:
//    bool r;
//    typedef EventTBase<IsImportantWindowEvent>  base;
//};

// Notifies a tree model that it should remove its fake tree item which was
// giving instructions to users but had no other role.
class TreeItem;
class TreeRemoveFakeItemEvent : public EventTBase<TreeRemoveFakeItemEvent>
{
public:
    TreeRemoveFakeItemEvent(TreeItem *parent);

    TreeItem* itemParent();
private:
    TreeItem *parent;

    typedef EventTBase<TreeRemoveFakeItemEvent>    base;
};

class TreeAddFakeItemEvent : public EventTBase<TreeAddFakeItemEvent>
{
public:
    TreeAddFakeItemEvent(TreeItem *parent);

    TreeItem* itemParent();
private:
    TreeItem *parent;

    typedef EventTBase<TreeAddFakeItemEvent>    base;
};

#endif // ZEVENTS_H
