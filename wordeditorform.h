/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDEDITORFORM_H
#define WORDEDITORFORM_H

#include <QtCore>
#include <QMainWindow>
#include <QItemSelectionModel>
#include <memory>
#include <set>
#include "zdictionarylistview.h"
#include "dialogwindow.h"

namespace Ui {
    class WordEditorForm;
}

//class WordEditorForm;

// When the current item is about to change, notifies the parent window, which can decide
// whether it's valid to change it. Only useful in this window, as it can only handle a single
// selection which is the same as the current item.
//class WordEditorSelectionModel : public QItemSelectionModel
//{
//    Q_OBJECT
//public:
//    WordEditorSelectionModel(WordEditorForm *owner, QAbstractItemModel *model, QObject *parent = nullptr);
//    virtual ~WordEditorSelectionModel();
//public slots:
//    virtual void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override;
//    virtual void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override;
//    virtual void setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override;
//private:
//    WordEditorForm *owner;
//
//    typedef QItemSelectionModel base;
//};


class QPainter;
struct WordEntry;
class DictionaryListEditDelegate : public DictionaryListDelegate
{
    Q_OBJECT
private:
    typedef DictionaryListDelegate  base;
public:
    DictionaryListEditDelegate(ZDictionaryListView *parent = nullptr);

    virtual void paintDefinition(QPainter *painter, QColor textcolor, QRect r, int y, WordEntry *e, std::vector<InfTypes> *inf, int defix, bool selected) const override;
};

class ZDictionaryEditListView : public ZDictionaryListView
{
    Q_OBJECT
public:
    ZDictionaryEditListView(QWidget *parent = nullptr);
protected:
    //virtual ZListViewItemDelegate* createItemDelegate() override;
private:
    typedef ZDictionaryListView base;
};

class Dictionary;
class WordEditorForm;

// Class with the only purpose to get around Qt's signal handling limitations. Its job is to
// remove the editor form from the word editor map when the entry was deleted from the source
// dictionary.
struct WordEditorFormFactory : public QObject
{
    Q_OBJECT
public:
    WordEditorFormFactory(const WordEditorFormFactory&) = delete;
    WordEditorFormFactory(WordEditorFormFactory &&) = delete;
    WordEditorFormFactory& operator=(const WordEditorFormFactory&) = delete;
    WordEditorFormFactory& operator=(WordEditorFormFactory&&) = delete;

    static WordEditorFormFactory& instance();

    void createForm(Dictionary *d, int windex, int defindex, QWidget *parent = nullptr);
    void createForm(Dictionary *srcd, int srcwindex, const std::vector<int> &srcdindexes, Dictionary *dest, int dwindex, QWidget *parent = nullptr);
private slots:
    void dictEntryRemoved(int windex);
private:
    // If an editor is open with the dictionary and word index, it's activated. Returns true
    // if such an editor was found.
    bool activateEditor(Dictionary *d, int windex);

    WordEditorFormFactory();
    void addEditor(Dictionary *dict, int windex, WordEditorForm *form);
    void editorClosed(Dictionary *dict, int windex);

    static WordEditorFormFactory *inst;

    // Mapping of [word index, word editor] pairs for the given dictionary.
    std::map<Dictionary *, std::map<int, WordEditorForm*>> editors;

    friend class WordEditorForm;
};


class QPushButton;
class DictionaryEntryItemModel;
class WordEditorForm : public DialogWindow
{
    Q_OBJECT
public:
    WordEditorForm() = delete;
    WordEditorForm(const WordEditorForm&) = delete;
    WordEditorForm(WordEditorForm&&) = delete;

    virtual ~WordEditorForm();

    // Shows the window for editing the word at windex in d dictionary. Set defindex to the
    // index of a definition in the word that should be initially selected.
    // Pass -1 for windex to add a new word to the d dictionary, instead of editing an
    // existing one.
    void exec(Dictionary *d, int windex, int defindex);
    // Shows the window for adding a new word or a new meaning to an existing word in the dest
    // dictionary, based on the srcwindex word's definitions in srcd dictionary. The
    // definitions listed in srcdindexes are copied to the end of the destination word, and
    // can be edited.
    // Pass -1 for dwindex to add a new word to the dest dictionary, instead of editing an
    // existing one.
    void exec(Dictionary *srcd, int srcwindex, const std::vector<int> &srcdindexes, Dictionary *dest, int dwindex);

public slots:
    // Closes the editor without asking to save changes, leaving the word unchanged.
    void uncheckedClose();
protected:
    virtual bool event(QEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;
private slots:
    // Checks the text of the kanji, kana, frequency and definition edit boxes, and enables or
    // disables the create and update buttons.
    void checkInput();

    void dictEntryChanged(int windex, bool studydef);
    //void dictEntryDefinitionAdded(int windex);
    //void dictEntryDefinitionChanged(int windex, int dindex);
    //void dictEntryDefinitionRemoved(int windex, int dindex);

    // Edited the kanji/kana, word attrib or frequency boxes.
    void wordChanged();
    // Edited the word's currently selected definition or definition attributes.
    void attribDefChanged();

    void defCurrentChanged(int row, int prev);
    // Deletes a definition from the edited word. This does not update the dictionary as the
    // word being edited is just a copy.
    void delDefClicked();
    void cloneDefClicked();

    void resetClicked();
    void applyClicked();
    void okClicked();

    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
    void dictionaryRenamed(const QString &oldname, int index, int orderindex);
    void dictionaryFlagChanged(int index, int order);
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);
private:
    WordEditorForm(QWidget *parent = nullptr);

    // Sets the word index to -1 and erases all data. Closing the form after a reset() will
    // happen silently.
    void reset();

    // Returns the value of the currently checked word info fields in wordAttribList.
    uint checkedInfoBits();
    // Returns the definition in the definition text edit box, trimming each line and
    // replacing the newline characters with GLOSS_SEP_CHAR.
    QString enteredDefinition();
    // Returns whether there are unsaved changes in the edited word's attributes. Before
    // adding the first definition of a new word, the result is always false.
    //bool wordUnsaved();
    // Returns whether there are unsaved changes in the currently selected definition. Before
    // adding the first definition of a new word, the result is always false.
    //bool defUnsaved();

    // Returns true if the edited entry differs from the saved original.
    bool canReset() const;

    void updateDictionaryLabels();

    // Translate text of buttons
    void translateTexts();

    Ui::WordEditorForm *ui;

    DictionaryEntryItemModel *model;

    QPushButton *resetBtn;
    QPushButton *okBtn;
    QPushButton *cancelBtn;
    QPushButton *applyBtn;

    // Destination dictionary for word creation or dictionary holding the edited word.
    Dictionary *dict;
    // Word index when editing a word. -1 for new words.
    int index;

    //// Something has been changed by the user on the edited word. This value can be true even
    //// without user input, if the word being edited has additional definitions added at the
    //// start.
    //bool modified;
    //// Used to restore the original modified when the reset button is clicked. This value is
    //// only set to true before changes were applied once and the editor was opened with
    //// meanings added to the original word.
    //bool origmodified;

    // When set, ignore slots that handle attribute, definition or word property change.
    bool ignoreedits;

    // Word entry being edited. This is a copy created to avoid corrupting the dictionary.
    // Once the user accepts the changes the attributes of the entry are copied to the final
    // word.
    std::unique_ptr<WordEntry> entry;
    // Copy of word entry created when first showing the dialog. The entry is restored to this
    // state if the reset button is clicked. The original is updated when the current changes
    // are applied.
    std::unique_ptr<WordEntry> original;

    friend struct WordEditorFormFactory;

    typedef DialogWindow base;
};

#endif // WORDEDITORFORM_H

