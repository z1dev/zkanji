/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZLINEEDIT_H
#define ZLINEEDIT_H

#include <QLineEdit>
#include <QUndoStack>
#include <QTextLayout>
#include <QValidator>
#include <QMenu>
#include <memory>

// Replaces the Undo/Redo functionality of QLineEdit with a custom one to
// allow proper handling of undo/redo when using a validator.
class ZLineEdit : public QLineEdit
{
    Q_OBJECT
signals:
    // Signals hiding the original lineedit signals. Always connect to
    // ZLineEdit functions and not QLineEdit.
    void textChanged(const QString &text);
    void textEdited(const QString &text);
    void cursorPositionChanged(int oldp, int newp);
    void selectionChanged();

    // Signals that the line edit gained or lost keyboard focus. The value of activated is set
    // to true when the focus is gained and false when it's lost.
    void focusChanged(bool activated);
public:
    ZLineEdit(QWidget *parent = nullptr);
    ~ZLineEdit();

    bool isModified() const;
    void setModified(bool newmod);

    void setCursorMoveStyle(Qt::CursorMoveStyle style);

    bool isRedoAvailable() const;
    bool isUndoAvailable() const;

    void setValidator(const QValidator* v);
    bool hasAcceptableInput() const;

    // Sets the preferred width of this line edit in number of characters. Only works in
    // layouts that use the sizeHint() function to determine the width of widgets. The
    // function must be called after the font changes to accomodate its character sizes.
    void setCharacterSize(double chsiz);

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
public slots:
    void setText(const QString &str);
    void clear();
    void undo();
    void redo();
    
    void cut();
    void paste();
    void deleteSelected();
protected:
    // Checks if there's a validator installed, and if it is, returns whether
    // the text currently entered is valid or not.
    bool validInput(bool allowintermediate = false) const;

    // Override in derived classes to replace or change the context menu shown by the editor.
    virtual QMenu* createContextMenu(QContextMenuEvent *e);

    virtual void contextMenuEvent(QContextMenuEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void inputMethodEvent(QInputMethodEvent* event) override;

    virtual void focusInEvent(QFocusEvent *e) override;
    virtual void focusOutEvent(QFocusEvent *e) override;

    // Emits textEdited and textChanged. Set edited to false if only
    // textChanged should be emited.
    void emitEditChange(bool edited = true);
private:
    struct UndoItem
    {
        // Type of operation to be done when undoing/redoing.
        // - Delete: The text was removed by pressing del or backspace. If
        //           pressed multiple times, the existing Delete is expanded.
        // - Input: A character key was pressed and input into the line edit.
        //          For multiple characters, the existing Input is expanded.
        // - SeparateInput: Text was pasted. Works the same way as Input, but
        //                  it won't be merged with the last Input operation.
        // - Replace: The selection was removed. This can come before Delete
        //            or Input, and in those cases it will be undone/redone
        //            with them together.
        enum UndoTypes { Delete, Input, SeparateInput, Replace };

        UndoTypes type;
        // Textual data of the undo item.
        std::unique_ptr<QChar> data;
        // Start position in a selection.
        int start;
        // Cursor position, usually marks the end of a selection 
        int end;

        UndoItem(const UndoItem &) = delete;
        UndoItem& operator=(const UndoItem &) = delete;
        UndoItem(UndoItem &&src) : type(src.type), start(src.start), end(src.end) { std::swap(data, src.data); }
        UndoItem& operator=(UndoItem &&src) { std::swap(type, src.type); std::swap(start, src.start), std::swap(end, src.end); std::swap(data, src.data); return *this; }
        UndoItem() { ; }
    };

    // Adds a delete operation to the undo stack, when the del or back space
    // keys were pressed. If a character was deleted by pressing del,
    // delend will be larger than delstart, and smaller in case back space was
    // pressed instead. If the previous undo operation was a similar delete,
    // it is expanded.
    void addDelUndo(const QString &delstr, int delstart, int delend);
    // Adds a replace operation to the undo stack. This is always added as the
    // first part of an undo item group, because it signifies the removal of
    // the selected text, and the next item will describe what was added in
    // the selection's place.
    // If not input or delete comes after this in the undo stack, it is a
    // redone as a simple delete.
    void addReplaceUndo(const QString &delstr, int oldselstart, int oldselend);
    // Adds an input operation to the undo stack. Inpstart is the character
    // position where inpstr was added, and inpend is the character
    // position after inpstr, marking the length of the input string. If the
    // previous undo operation was input as well, that one is expanded.
    void addInputUndo(const QString &inpstr, int inpstart, int inpend, bool separate);

    void updateLayout();

    // Does the work for paste or the paste key press. Behavior can be
    // different on X11, so if the original X11 event is sent, it's used,
    // otherwise this is a normal paste operation.
    void _paste(QKeyEvent *e);

    // Sets the text and selection in the line edit to the last known fully
    // acceptable state if it is currently not "Acceptable" by the validator.
    // Returns whether restore was needed.
    // Set emitchange to true if changes should be signalled.
    bool restoreAcceptable(bool emitchange);

    // Puts current text in oldtext and current selection in oldstart and
    // oldend. Does not check text validity. Only call with acceptable
    // input.
    void saveTextState();
    // Saves an empty text and 0 selection in oldtext and oldstart / oldend.
    void saveClearTextState();

    std::vector<UndoItem> undostack;
    int undopos;

    // The line edit is in a modified state.
    bool mod;

    // Text is always saved to be able to match it with new changes.
    QString oldtext;
    // Saved selection start position.
    int oldstart;
    // Saved selection end/cursor position.
    int oldend;

    // Used for navigating to the next or previous character/word for
    // ctr+del, ctrl+backspace etc. handling so it finds the next character
    // the same way the original control does.
    QTextLayout layout;
    // Set when the text or the text layout in the program changes.
    bool layoutdirty;
    // Set when updating the text internally and any text change event must be
    // ignored.
    bool ignorechange;

    // If differs when emitting textchanged signals, a selectionchanged
    // signal is emitted as well.
    int savedselstart;
    // If differs when emitting textchanged signals, a selectionchanged
    // signal is emitted as well. The cursor position is saved in this value
    // as well, so a cursorposition change signal can be emitted too.
    int savedselend;

    // Preferred width of the line edit. -1 by default, which means sizeHint() workds the
    // original way. Changed when calling setCharacterSize().
    int prefwidth;

    // The text wasn't validated since the last text change. If this is false,
    // the text's validity state is stored in savedstate.
    mutable bool statedirty;
    mutable QValidator::State savedstate;

    typedef QLineEdit   base;
private slots:
    void textChangedSlot(const QString &newtext);
    //void selectionChangedSlot();
    void layoutDirectionChanged(Qt::LayoutDirection direction);

    void cursorPositionChangedSlot(int oldp, int newp);
    void selectionChangedSlot();
};

#endif // ZLINEEDIT_H

