/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMenu>
#include <QInputEvent>
#include <QApplication>
#include <QClipboard>
#include "zlineedit.h"
#include "qcharstring.h"

#include "checked_cast.h"

//-------------------------------------------------------------


ZLineEdit::ZLineEdit(QWidget *parent) : base(parent), undopos(0), oldstart(0), oldend(0), layoutdirty(false),
        ignorechange(false), savedselstart(0), savedselend(0), prefwidth(-1), statedirty(false),
        savedstate(QValidator::Acceptable)
{
    connect(this, &QLineEdit::textChanged, this, &ZLineEdit::textChangedSlot);
    //connect(this, &QLineEdit::selectionChanged, this, &ZLineEdit::selectionChangedSlot);
    connect(qApp, &QGuiApplication::layoutDirectionChanged, this, &ZLineEdit::layoutDirectionChanged);

    connect(this, &QLineEdit::selectionChanged, this, &ZLineEdit::selectionChangedSlot);
    connect(this, &QLineEdit::cursorPositionChanged, this, &ZLineEdit::cursorPositionChangedSlot);
}

ZLineEdit::~ZLineEdit()
{

}

bool ZLineEdit::isModified() const
{
    return mod;
}

void ZLineEdit::setModified(bool newmod)
{
    mod = newmod;
    if (!mod)
        undostack.clear();
}

void ZLineEdit::setCursorMoveStyle(Qt::CursorMoveStyle style)
{
    base::setCursorMoveStyle(style);
    layout.setCursorMoveStyle(style);
}

bool ZLineEdit::isRedoAvailable() const
{
    return !isReadOnly() && undopos != tosigned(undostack.size());
}

bool ZLineEdit::isUndoAvailable() const
{
    return !isReadOnly() && undopos != 0;
}

void ZLineEdit::setValidator(const QValidator* v)
{
    if (validator() == v)
        return;
    statedirty = v != nullptr;
    base::setValidator(v);
    if (v == nullptr)
        savedstate = QValidator::Acceptable;
}

bool ZLineEdit::hasAcceptableInput() const
{
    return validInput();
}

void ZLineEdit::setCharacterSize(double chsiz)
{
    QFontMetrics fm(font());
    int w = fm.averageCharWidth();
    prefwidth = w * (chsiz + 1);
}

QSize ZLineEdit::sizeHint() const
{
    if (prefwidth == -1)
        return base::sizeHint();
    QSize siz = base::sizeHint();
    siz.setWidth(prefwidth);
    return siz;
}

QSize ZLineEdit::minimumSizeHint() const
{
    if (prefwidth == -1)
        return base::minimumSizeHint();
    return sizeHint();
}

void ZLineEdit::setText(const QString &str)
{
    const QValidator *v = validator();
    QString t = str;

    QString savedtext = text();

    int cpos;
    bool acceptable = true;

    if (v)
    {
        cpos = t.size();
        QValidator::State vs = v->validate(t, cpos);
        if ((!hasFocus() || vs != QValidator::Intermediate) && vs != QValidator::Acceptable)
            v->fixup(t);
        cpos = t.size();

        QValidator::State state = v->validate(t, cpos);
        if (state == QValidator::Invalid)
            return;
        acceptable = state == QValidator::Acceptable;

        statedirty = false;
        savedstate = state;
    }
    else
        cpos = t.size();

    ignorechange = true;
    base::setText(t);
    setCursorPosition(cpos);
    ignorechange = false;

    undostack.clear();
    undopos = 0;
    mod = false;

    if (acceptable)
        saveTextState();
    else
        saveClearTextState();

    if (savedtext != text())
        emitEditChange(false);
}

void ZLineEdit::clear()
{
    bool hadtext = !text().isEmpty();

    ignorechange = true;
    base::clear();
    ignorechange = false;

    undostack.clear();
    undopos = 0;
    mod = false;

    saveClearTextState();

    if (hadtext)
    {
        statedirty = true;
        emitEditChange(false);
    }
}

void ZLineEdit::undo()
{
    // Restore last valid state and don't undo.
    if (restoreAcceptable(true))
        return;

    if (undopos == 0)
        return;

    ignorechange = true;
    QString t = text();

    UndoItem &item = undostack[undopos - 1];
    switch (item.type)
    {
    case UndoItem::Replace:
    case UndoItem::Delete:
        base::setText(t.left(std::min(item.start, item.end)) + QString::fromRawData(item.data.get(), abs(item.start - item.end)) + t.right(t.size() - std::min(item.start, item.end)));
        if (item.type == UndoItem::Delete)
            setCursorPosition(item.start);
        else
            setSelection(item.start, item.end - item.start);
        break;
    case UndoItem::Input:
    case UndoItem::SeparateInput:
        base::setText(t.left(item.start) + t.right(t.size() - item.end));
        setCursorPosition(item.start);
        break;
    }

    --undopos;
    ignorechange = false;

    if (item.type != UndoItem::Replace && undopos != 0 && undostack[undopos - 1].type == UndoItem::Replace)
        undo();
    else
    {
        statedirty = false;
        savedstate = QValidator::Acceptable;

        saveTextState();
        emitEditChange();
    }
}

void ZLineEdit::redo()
{
    restoreAcceptable(undopos == tosigned(undostack.size()));

    if (undopos == tosigned(undostack.size()))
        return;

    ignorechange = true;

    QString t = text();

    UndoItem &item = undostack[undopos];
    switch (item.type)
    {
    case UndoItem::Input:
    case UndoItem::SeparateInput:
        base::setText(t.left(item.start) + QString::fromRawData(item.data.get(), item.end - item.start) + t.right(t.size() - item.start));
        setCursorPosition(item.end);
        break;
    case UndoItem::Delete:
    case UndoItem::Replace:
        base::setText(t.left(std::min(item.start, item.end)) + t.right(t.size() - std::max(item.start, item.end)));
        setCursorPosition(std::min(item.start, item.end));
        break;
    }

    ignorechange = false;

    ++undopos;
    if (undopos != tosigned(undostack.size()))
    {
        UndoItem &i2 = undostack[undopos];
        if (item.type == UndoItem::Replace && (i2.type == UndoItem::Input || i2.type == UndoItem::SeparateInput || i2.type == UndoItem::Delete))
        {
            redo();
            return;
        }
        else if (i2.type == UndoItem::Replace)
            setSelection(i2.start, i2.end - i2.start);
    }

    statedirty = false;
    savedstate = QValidator::Acceptable;

    saveTextState();
    emitEditChange();
}

void ZLineEdit::cut()
{
    if (restoreAcceptable(true))
        return;

    bool changed = false;

    ignorechange = true;
    if (hasSelectedText())
    {
        int cpos = cursorPosition();
        int spos = selectionStart();
        QString str = selectedText();

        addReplaceUndo(str, spos == cpos ? (spos + str.size()) : spos, cpos);

        base::cut();

        changed = true;
    }
    ignorechange = false;

    if (changed)
    {
        if (validInput(false))
            saveTextState();
        emitEditChange();
    }
}

void ZLineEdit::paste()
{
    _paste(nullptr);
}

void ZLineEdit::deleteSelected()
{
    // Restore last valid state and don't delete.
    if (restoreAcceptable(true))
        return;

    bool changed = false;

    ignorechange = true;
    if (hasSelectedText())
    {
        int cpos = cursorPosition();
        int spos = selectionStart();
        QString str = selectedText();

        addReplaceUndo(str, spos == cpos ? (spos + str.size()) : spos, cpos);

        base::del();

        changed = true;
    }
    ignorechange = false;

    if (changed)
    {
        if (validInput(false))
            saveTextState();
        emitEditChange();
    }
}

bool ZLineEdit::validInput(bool allowintermediate) const
{
    if (!statedirty)
        return savedstate == QValidator::Acceptable || (allowintermediate && savedstate == QValidator::Intermediate);

    statedirty = false;

    const QValidator *v = validator();
    if (v == nullptr)
    {
        savedstate = QValidator::Acceptable;
        return true;
    }

    QString t = text();
    int cpos = cursorPosition();
    QString tcopy = t;
    int cposcopy = cpos;

    QValidator::State state = v->validate(tcopy, cposcopy);


    if (allowintermediate && state == QValidator::Intermediate)
    {
        savedstate = QValidator::Intermediate;
        return true;
    }

    if (state != QValidator::Acceptable || t != tcopy || cpos != cposcopy)
    {
        if (state == QValidator::Acceptable)
            savedstate = QValidator::Intermediate;
        else
            savedstate = state;
        return false;
    }

    savedstate = QValidator::Acceptable;
    return true;
}

QMenu* ZLineEdit::createContextMenu(QContextMenuEvent * /*e*/)
{
    // Create the default context menu for the line edit and replace its undo/redo action
    // slots with our own.
    QMenu *menu = createStandardContextMenu();
    if (menu == nullptr)
        return nullptr;

    QList<QAction*> actions = menu->actions();
    for (QAction *a : actions)
    {
        QString atxt = a->text();
        int p = atxt.indexOf(QLatin1Char('\t'));
        if (p != -1)
            atxt = atxt.left(p);
        if (atxt == QLineEdit::tr("&Undo"))
        {
            disconnect(a, 0, this, 0);
            connect(a, &QAction::triggered, this, &ZLineEdit::undo);
            a->setEnabled(!isReadOnly() && undopos != 0);
            continue;
        }
        if (atxt == QLineEdit::tr("&Redo"))
        {
            disconnect(a, 0, this, 0);
            connect(a, &QAction::triggered, this, &ZLineEdit::redo);
            a->setEnabled(!isReadOnly() && undopos != tosigned(undostack.size()));
            continue;
        }
        if (atxt == QLineEdit::tr("Cu&t"))
        {
            disconnect(a, 0, this, 0);
            connect(a, &QAction::triggered, this, &ZLineEdit::cut);
            a->setEnabled(!isReadOnly() && hasSelectedText());
            continue;
        }
        if (atxt == QLineEdit::tr("&Paste"))
        {
            disconnect(a, 0, this, 0);
            connect(a, &QAction::triggered, this, &ZLineEdit::paste);
            a->setEnabled(!isReadOnly() && !QApplication::clipboard()->text().isEmpty());
            continue;
        }
        if (atxt == QLineEdit::tr("Delete"))
        {
            disconnect(a, 0, 0, 0);
            connect(a, &QAction::triggered, this, &ZLineEdit::deleteSelected);
            a->setEnabled(!isReadOnly() && hasSelectedText());
            continue;
        }
    }

    return menu;
}

void ZLineEdit::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createContextMenu(e);

    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(e->globalPos());
    e->accept();
}

void ZLineEdit::keyPressEvent(QKeyEvent *e)
{
    if (isReadOnly())
    {
        base::keyPressEvent(e);
        return;
    }

    // Set to true when the key press is handled here and shouldn't be sent to the base class.
    bool handled = false;

    // Set to true if we changed the text here and the text edited and changed signals should
    // be sent.
    bool changed = false;

    // Try to catch any key combination that will modify the text in the line edit so it can
    // respond to that in the new undo/redo mechanism.
    if (e == QKeySequence::Undo)
    {
        undo();
        handled = true;
    }
    else if (e == QKeySequence::Redo)
    {
        redo();
        handled = true;
    }
    else if (e == QKeySequence::Paste)
    {
        _paste(e);
        handled = true;
    }
    else if (e == QKeySequence::Cut)
    {
        cut();
        handled = true;
    }
    else if (e == QKeySequence::DeleteEndOfLine) /* del anything after cursor */
    {
        if (!restoreAcceptable(true))
        {
            ignorechange = true;
            QString t = text();
            int cpos = cursorPosition();
            if (cpos != t.size())
            {
                addReplaceUndo(t.right(t.size() - cpos), cpos, t.size());
                setSelection(cpos, t.size() - cpos);
                copy();
                del();
                changed = true;
            }
            ignorechange = false;

            if (changed)
            {
                if (validInput(false))
                    saveTextState();
                emitEditChange();
            }
        }

        handled = true;
    }
    else if (e == QKeySequence::DeleteEndOfWord) /* ctrl+del */
    {
        if (!restoreAcceptable(true))
        {
            ignorechange = true;
            updateLayout();

            QString t = text();
            int cpos = cursorPosition();

            int npos = layout.nextCursorPosition(cpos, QTextLayout::SkipWords);
            if (npos != cpos)
            {
                addReplaceUndo(t.mid(cpos, npos - cpos), npos, cpos);
                base::setText(t.left(cpos) + t.right(t.size() - npos));
                setCursorPosition(cpos);
                changed = true;
            }
            ignorechange = false;

            if (changed)
            {
                if (validInput(false))
                    saveTextState();
                emitEditChange();
            }
        }

        handled = true;
    }
    else if (e == QKeySequence::DeleteStartOfWord || (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Backspace))
    {
        if (!restoreAcceptable(true))
        {
            ignorechange = true;
            updateLayout();

            QString t = text();
            int cpos = cursorPosition();

            int npos = layout.previousCursorPosition(cpos, QTextLayout::SkipWords);
            if (npos != cpos)
            {
                addReplaceUndo(t.mid(npos, cpos - npos), npos, cpos);
                base::setText(t.left(npos) + t.right(t.size() - cpos));
                setCursorPosition(npos);
                changed = true;
            }
            ignorechange = false;

            if (changed)
            {
                if (validInput(false))
                    saveTextState();
                emitEditChange();
            }
        }

        handled = true;
    }
    else if (e == QKeySequence::DeleteCompleteLine)
    {
        if (!restoreAcceptable(true))
        {
            ignorechange = true;

            QString t = text();

            if (!t.isEmpty())
            {
                addReplaceUndo(t, 0, t.size());
                selectAll();
                copy();
                del();
                changed = true;
            }
            ignorechange = false;

            if (changed)
            {
                statedirty = false;
                savedstate = QValidator::Acceptable;

                saveClearTextState();
                emitEditChange();
            }
        }

        handled = true;
    }

    if (e->key() == Qt::Key_Direction_L || e->key() == Qt::Key_Direction_R)
        layoutdirty = true;

    if (!handled)
        base::keyPressEvent(e);
    else
        e->accept();
}

void ZLineEdit::inputMethodEvent(QInputMethodEvent* event)
{
    // The original line edit emits cursorPositionChanged event before the textChanged event
    // in inputMethod events, resulting in invalid positions when the old text is not updated
    // yet. This confuses the undo/redo handling and validating in ZLineEdit. If there's a
    // change in the text, the control will ignore the signals and will re-check text and
    // selection only after the input method event has been handled.

    bool hascommit = !event->commitString().isEmpty();

    QString t = text();
    int oldss = selectionStart();
    int oldse = cursorPosition();
    if (oldss == oldse)
        oldss += selectedText().size();
    else if (oldss == -1)
        oldss = oldse;

    if (hascommit)
        ignorechange = true;
    base::inputMethodEvent(event);
    if (hascommit)
    {
        ignorechange = false;

        QString newtext = text();

        int ss = selectionStart();
        int se = cursorPosition();
        if (ss == se)
            ss += selectedText().size();
        else if (ss == -1)
            ss = se;

        if (newtext != t)
        {
            emit base::textChanged(newtext);
            emit base::textEdited(newtext);
        }

        if (ss != oldss || se != oldse)
        {
            emit base::selectionChanged();
            emit base::cursorPositionChanged(oldse, se);
        }
    }
}

void ZLineEdit::focusInEvent(QFocusEvent *e)
{
    base::focusInEvent(e);
    emit focusChanged(true);
}

void ZLineEdit::focusOutEvent(QFocusEvent *e)
{
    QString str = text();
    base::focusOutEvent(e);
    if (str != text())
        emitEditChange(true);

    emit focusChanged(false);
}

void ZLineEdit::emitEditChange(bool edited)
{
    // The selection might have changed while we were not looking.
    int ss = selectionStart();
    int se = ss;
    int cpos = cursorPosition();
    if (ss == -1)
        se = ss = cpos;
    else if (hasSelectedText())
        se = ss + selectedText().size();
    if (se != ss && cpos == ss)
        std::swap(se, ss);

    int oldss = savedselstart;
    int oldse = savedselend;
    savedselstart = ss;
    savedselend = se;

    QString t = text();
    if (edited)
    {
        setModified(true);
        emit textEdited(t);
    }
    else
        setModified(false);
    emit textChanged(t);

    if (ss != oldss || se != oldse)
        emit selectionChanged();

    if (oldse != cpos)
        emit cursorPositionChanged(oldse, cpos);
}

void ZLineEdit::addDelUndo(const QString &delstr, int delstart, int delend)
{
    undostack.erase(undostack.begin() + undopos, undostack.end());

    // Del key pressed: delstart < delend.
    // Back space pressed: delstart > delend.
    // Delstart is the initial cursor position in both cases.

    if (delstr.size() == 1 && !undostack.empty() && undostack.back().type == UndoItem::Delete &&
       ((undostack.back().start < undostack.back().end) == (delstart < delend)))
    {
        // Expand previous item if it matches.
        UndoItem &item = undostack.back();

        if (item.start < item.end && delstart == item.start)
        {
            // Previous delete was response to the del key press too. Expand.
            QChar *tmp = item.data.release();
            item.data.reset(new QChar[item.end - item.start + 1]);
            memcpy(item.data.get(), tmp, sizeof(QChar) * (item.end - item.start));
            delete[] tmp;
            item.data.get()[item.end - item.start] = delstr.at(0);
            ++item.end;
            return;
        }
        else if (item.start > item.end && delstart == item.end)
        {
            // Previous one was a back space key press too. Expand.
            QChar *tmp = item.data.release();
            item.data.reset(new QChar[item.start - item.end + 1]);
            memcpy(item.data.get() + 1, tmp, sizeof(QChar) * (item.start - item.end));
            delete[] tmp;
            item.data.get()[0] = delstr.at(0);
            --item.end;
            return;
        }
    }

    // New undo group has to be added. The previous one was either not a del,
    // or it had different attributes (cursor position, del or back space
    // used.)

    UndoItem item;
    item.start = delstart;
    item.end = delend;
    item.type = UndoItem::Delete;

    item.data.reset(new QChar[abs(delstart - delend)]);
    memcpy(item.data.get(), delstr.constData(), sizeof(QChar) * abs(delstart - delend));

    undostack.push_back(std::move(item));
    undopos = tosigned(undostack.size());
}

void ZLineEdit::addReplaceUndo(const QString &delstr, int oldselstart, int oldselend)
{
    undostack.erase(undostack.begin() + undopos, undostack.end());

    UndoItem item;
    item.start = oldselstart;
    item.end = oldselend;
    item.type = UndoItem::Replace;

    item.data.reset(new QChar[abs(oldselstart - oldselend)]);
    memcpy(item.data.get(), delstr.constData(), sizeof(QChar) * abs(oldselstart - oldselend));

    undostack.push_back(std::move(item));
    undopos = tosigned(undostack.size());
}

void ZLineEdit::addInputUndo(const QString &inpstr, int inpstart, int inpend, bool separate)
{
    undostack.erase(undostack.begin() + undopos, undostack.end());

    // inpstart is the cursor position before the input, inpend is after. They
    // also mark the length of the input.

    if (!separate && !undostack.empty() && undostack.back().type == UndoItem::Input && undostack.back().end == inpstart)
    {
        // Previous undo item was input too. Expand it with the new input.
        UndoItem &item = undostack.back();

        QChar *tmp = item.data.release();
        item.data.reset(new QChar[item.end - item.start + (inpend - inpstart)]);
        memcpy(item.data.get(), tmp, sizeof(QChar) * (item.end - item.start));
        memcpy(item.data.get() + (item.end - item.start), inpstr.constData(), sizeof(QChar) * (inpend - inpstart));
        item.end = inpend;
        return;
    }

    UndoItem item;
    item.start = inpstart;
    item.end = inpend;
    item.type = separate ? UndoItem::SeparateInput : UndoItem::Input;

    item.data.reset(new QChar[abs(inpstart - inpend)]);
    memcpy(item.data.get(), inpstr.constData(), sizeof(QChar) * abs(inpstart - inpend));

    undostack.push_back(std::move(item));
    undopos = tosigned(undostack.size());
}

void ZLineEdit::updateLayout()
{
    if (!layoutdirty)
        return;

    layoutdirty = false;

    layout.setText(text());
    QTextOption option = layout.textOption();
    option.setTextDirection(layoutDirection());
    option.setFlags(QTextOption::IncludeTrailingSpaces);
    layout.setTextOption(option);
}

void ZLineEdit::_paste(QKeyEvent *e)
{
    // Restore last valid state and don't paste.
    if (restoreAcceptable(true))
        return;

    bool changed = false;

    ignorechange = true;
    int cpos = cursorPosition();
    int spos = selectionStart();
    QString st = selectedText();
    QString t = text();
    int len = t.size();
    int slen = st.size();

    if (e == nullptr)
        base::paste();
    else
    {
        // On X11 the paste operation can paste some selected text. It cannot
        // be checked here as this check is done in the private part of Qt so
        // we have to let it do its thing in the original key event.
        base::keyPressEvent(e);
    }

    QString t2 = text();
    if (t != t2 || cpos != cursorPosition() || spos != selectionStart())
    {
        if (slen)
            addReplaceUndo(st, spos == cpos ? (spos + st.size()) : spos, cpos);

        if (spos == -1)
            spos = cpos;

        addInputUndo(t2.mid(std::min(cpos, spos), t2.size() - len + slen), std::min(cpos, spos), std::min(cpos, spos) + t2.size() - len + slen, true);
        changed = true;
    }

    ignorechange = false;

    if (changed)
    {
        if (validInput(false))
            saveTextState();
        emitEditChange();
    }
}

bool ZLineEdit::restoreAcceptable(bool /*emitchange*/)
{
    bool valid;

    QString t;
    int cpos = 0;

    if (statedirty)
    {
        cpos = cursorPosition();
        t = text();
        QString tcopy = t;
        int cposcopy = cpos;
        const QValidator *v = validator();
        valid = !v || (v->validate(tcopy, cposcopy) == QValidator::Acceptable && tcopy == t && cposcopy == cpos);
    }
    else
    {
        valid = savedstate == QValidator::Acceptable;
        if (!valid)
        {
            t = text();
            cpos = cursorPosition();
        }
    }

    if (!valid)
    {
        int spos = selectionStart();
        if (spos == cpos)
            spos = cpos + selectedText().size();
        else if (spos == -1)
            spos = cpos;

        ignorechange = true;
        base::setText(oldtext);
        base::setSelection(oldstart, oldend);
        ignorechange = false;

        statedirty = false;
        savedstate = QValidator::Acceptable;

        if (t != oldtext)
            emitEditChange();
        else if (spos != oldstart || cpos != oldend)
            selectionChangedSlot();
    }
    else
    {
        statedirty = false;
        savedstate = QValidator::Acceptable;
    }

    return !valid;
}

void ZLineEdit::textChangedSlot(const QString &newtext)
{
    // Every time the text changes, the new text is compared to the old one to
    // put on the undo stack. Some changes can be undone/redone together. This
    // counts as a single undo group. Multiple groups are always undone/redone
    // separately.
    // The text can change for several reasons:
    //  - Single character key or del was pressed: can change a larger string
    //    depending of the past selection. Deleting or inserting from the last
    //    saved position expands the last undo item if it matches.
    //  - Pasted/cut text into the line edit. Also changes the selection, but
    //    it can't be undone together with other changes.
    //  - Some other key combination changed the text. It also counts as a
    //    separate undo item.
    //  - The text changed because of a context-menu action. These changes are
    //    always put in a separate undo item.
    //  - The text changed because of the validator. Only valid input is used
    //    for undo/redo.
    // A new undo group is started for the following reasons:
    //  - Keyboard position/selection changed after a text change.
    //  - Text changed for a special reason.
    //  - Text delete after input, or input after delete count as a separate
    //    undo item but they can be undone/redone together usually.


    layoutdirty = true;
    statedirty = true;

    if (ignorechange || oldtext == newtext)
    {
        if (!ignorechange)
            emitEditChange();
        return;
    }

    QString tcopy = newtext;
    int cpos = cursorPosition();
    int cposcopy = cpos;
    const QValidator *v = validator();
    QValidator::State state = !v ? QValidator::Acceptable : v->validate(tcopy, cposcopy);
    bool valid = !v || (state == QValidator::Acceptable && tcopy == newtext && cposcopy == cpos);

    statedirty = false;
    if (!valid)
    {
        if (state == QValidator::Intermediate || state == QValidator::Acceptable)
        {
            savedstate = QValidator::Intermediate;
            emitEditChange();
        }
        else
            savedstate = QValidator::Invalid;
        return;
    }

    statedirty = false;
    savedstate = QValidator::Acceptable;

    // Handles the case when no special action was taken. The old selection
    // has either been replaced by one or more characters, or deleted.

    if (newtext.size() == oldtext.size() - std::max(abs(oldstart - oldend), 1))
    {
        // Deleted selection or prev/next character.
        int sellen = std::max(abs(oldstart - oldend), 1);

        // Set to true if the next character was deleted with the del key, and
        // not the previous one with backspace.
        // If text was selected previously, both del and backspace does the
        // same thing. Del is only used if this is not true.
        bool del = oldstart == oldend && newtext.size() == oldtext.size() - 1 &&
            qcharncmp(oldtext.constData(), newtext.constData(), oldstart) == 0 &&
            qcharcmp(oldtext.constData() + oldstart + 1, newtext.constData() + oldstart) == 0;
        // One character was deleted with backspace instead.
        bool bckspc = !del && oldstart == oldend && newtext.size() == oldtext.size() - 1 &&
            qcharncmp(oldtext.constData(), newtext.constData(), oldstart - 1) == 0 &&
            qcharcmp(oldtext.constData() + oldstart, newtext.constData() + oldstart - 1) == 0;
        // The selection was deleted with either del or backspace.
        bool seldel = !del && !bckspc && oldstart != oldend && newtext.size() == oldtext.size() - sellen &&
            qcharncmp(oldtext.constData(), newtext.constData(), std::min(oldstart, oldend)) == 0 &&
            qcharcmp(oldtext.constData() + std::max(oldstart, oldend), newtext.constData() + std::min(oldstart, oldend)) == 0;


        // The change is deletion of the selection or previous character.
        if (del || bckspc || seldel)
        {
            if (seldel)
                addReplaceUndo(oldtext.mid(std::min(oldstart, oldend), sellen), oldstart, oldend);
            else if (bckspc)
                addDelUndo(oldtext.mid(oldstart - 1, 1), oldstart, oldstart - 1);
            else // del
                addDelUndo(oldtext.mid(oldstart, 1), oldstart, oldstart + 1);

            saveTextState();
            emitEditChange();

            return;
        }
    }
    else if (newtext.size() > oldtext.size() - abs(oldstart - oldend))
    {
        // New character was typed replacing the selection.

        int sizediff = newtext.size() - oldtext.size() + abs(oldstart - oldend);

        bool good = (newtext.size() == oldtext.size() - abs(oldstart - oldend) + 1) &&
            qcharncmp(oldtext.constData(), newtext.constData(), std::min(oldstart, oldend)) == 0 &&
            qcharcmp(oldtext.constData() + std::max(oldstart, oldend),
            newtext.constData() + std::min(oldstart, oldend) + sizediff) == 0;

        if (good)
        {
            if (oldstart != oldend)
                addReplaceUndo(oldtext.mid(std::min(oldstart, oldend), abs(oldstart - oldend)), oldstart, oldend);
            addInputUndo(newtext.mid(std::min(oldstart, oldend), sizediff), std::min(oldstart, oldend), std::min(oldstart, oldend) + sizediff, false);

            saveTextState();
            emitEditChange();

            return;
        }
    }

    // Not a single key press caused text change. Maybe validator or something
    // else changed it. Figure out if the change can be described by some text
    // replacing the old selection. (Old text and new text matches apart from
    // old selection.)

    if (oldtext.size() - abs(oldstart - oldend) <= newtext.size())
    {
        bool good = true;
        // Text in front of old selection matches?
        for (int ix = 0; good && ix != std::min(oldstart, oldend); ++ix)
            if (oldtext.at(ix) != newtext.at(ix))
                good = false;
        // Text after old selection matches?
        for (int ix = 0; good && ix != oldtext.size() - std::max(oldstart, oldend); ++ix)
            if (oldtext.at(oldtext.size() - 1 - ix) != newtext.at(newtext.size() - 1 - ix))
                good = false;

        if (good)
        {
            if (oldstart != oldend)
                addReplaceUndo(oldtext.mid(std::min(oldstart, oldend), abs(oldstart - oldend)), oldstart, oldend);
            if (oldtext.size() - abs(oldstart - oldend) != newtext.size())
            {
                int sizediff = newtext.size() - oldtext.size() + abs(oldstart - oldend);
                addInputUndo(newtext.mid(std::min(oldstart, oldend), sizediff), std::min(oldstart, oldend), std::min(oldstart, oldend) + sizediff, false);
            }

            saveTextState();
            emitEditChange();

            return;
        }

    }

    // Text change can't be explained by anything the line edit knows about. 
    // Mark the whole text as changed and put the old and new text on the undo
    // stack.

    if (!oldtext.isEmpty())
        addReplaceUndo(oldtext, 0, oldtext.size());
    if (!newtext.isEmpty())
        addInputUndo(newtext, 0, newtext.size(), true);

    saveTextState();
    emitEditChange();
}

void ZLineEdit::cursorPositionChangedSlot(int /*oldp*/, int newp)
{
    if (ignorechange)
        return;

    if (savedselend != newp)
    {
        QString tmpt = text();
        if (validInput())
        {
            oldstart = selectionStart();
            oldend = newp;
            if (oldstart == oldend)
                oldstart += selectedText().size();
            else if (oldstart == -1)
                oldstart = oldend;
        }
        emit cursorPositionChanged(savedselend, newp);
    }
    savedselend = newp;
}

void ZLineEdit::selectionChangedSlot()
{
    if (ignorechange)
        return;

    int ss = selectionStart();
    int se = cursorPosition();
    if (se == ss)
        ss += selectedText().size();
    else if (ss == -1)
        ss = se;

    if (validInput())
    {
        oldstart = ss;
        oldend = se;
    }

    if (ss != savedselstart || se != savedselend)
        emit selectionChanged();
    savedselstart = ss;
    if (savedselend != se)
        emit cursorPositionChanged(savedselend, se);
    savedselend = se;
}

void ZLineEdit::layoutDirectionChanged(Qt::LayoutDirection /*direction*/)
{
    layoutdirty = true;
}

void ZLineEdit::saveTextState()
{
    oldtext = text();
    oldstart = selectionStart();
    oldend = cursorPosition();
    if (oldstart == oldend)
        oldstart = oldend + selectedText().size();
    if (oldstart == -1)
        oldstart = oldend;
}

void ZLineEdit::saveClearTextState()
{
    oldtext = QString();
    oldstart = 0;
    oldend = 0;
}


//-------------------------------------------------------------
