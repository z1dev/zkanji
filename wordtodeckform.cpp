/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPainter>
#include <algorithm>
#include <QtEvents>
#include <QPushButton>
#include "wordtodeckform.h"
#include "ui_wordtodeckform.h"
#include "words.h"
#include "grammar_enums.h"
#include "zkanjimain.h"
#include "zui.h"
#include "worddeck.h"
#include "wordstudylistform.h"
#include "globalui.h"
#include "generalsettings.h"
#include "formstates.h"


//-------------------------------------------------------------


WordsToDeckItemModel::WordsToDeckItemModel(Dictionary *dict, WordDeck* deck, const std::vector<int> &list, QObject *parent) : base(parent), dict(dict), deck(deck)
{
    setColumns({
        { (int)DictColumnTypes::Kanji, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(80), QString() },
        { (int)DictColumnTypes::Kana, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(100), QString() },
        { (int)DictColumnTypes::Definition, Qt::AlignLeft, ColumnAutoSize::NoAuto, false, Settings::scaled(6400), QString() }
    });

    std::vector<int> indexes = list;

    // The passed index list can contain duplicates, which are to be removed while maintaining
    // the original order of the other items.
    std::vector<int> order;
    order.reserve(indexes.size());
    for (int ix = 0; ix != indexes.size(); ++ix)
        order.push_back(ix);
    std::sort(order.begin(), order.end(), [&indexes](int a, int b) {
        if (indexes[a] != indexes[b])
            return indexes[a] < indexes[b];
        return a < b;
    });
    for (int ix = 1, pos = 0; ix != order.size(); ++ix)
    {
        if (indexes[order[ix]] == indexes[order[pos]])
            indexes[order[ix]] = -1;
        else
            pos = ix;
    }
    indexes.resize(std::remove(indexes.begin(), indexes.end(), -1) - indexes.begin());


    checks.reserve(indexes.size());

    // Check every word part to unset/disable checkboxes when the kanji form is not generally
    // used, the written form has no kanji or a part has already been added to the deck.
    for (int ix = 0; ix != indexes.size(); ++ix)
    {
        char val = 0;

        WordEntry *w = dict->wordEntry(indexes[ix]);
        bool haskanji = false;
        bool haskana = false;
        bool hasdef = false;

        WordDeckWord *dw = deck == nullptr ? nullptr : deck->wordFromIndex(indexes[ix]);

        bool kanaonly = (w->defs[0].attrib.notes & (int)WordNotes::KanaOnly) != 0;

        if (dw == nullptr || (dw->types & (int)WordPartBits::Kanji) == 0)
        {
            int ksiz = w->kanji.size();
            for (int iy = 0; !haskanji && iy != ksiz; ++iy)
                if (KANJI(w->kanji[iy].unicode()))
                    haskanji = true;
        }
        haskana = (dw == nullptr || (dw->types & (int)WordPartBits::Kana) == 0);
        hasdef = (dw == nullptr || (dw->types & (int)WordPartBits::Definition) == 0);

        if (!haskanji)
            val |= (1 << 3);
        if ((haskanji && !kanaonly) || (dw != nullptr && dw->types & (int)WordPartBits::Kanji) != 0)
            val |= 1;

        if (!haskana)
            val |= (1 << 4);
        val |= (1 << 1);

        if (!hasdef)
            val |= (1 << 5);
        val |= (1 << 2);

        if (((val >> 3) & 7) == 7)
        {
            // Every disable bit is set, the item shouldn't be listed when adding words to a deck.
            indexes[ix] = -1;
            continue;
        }

        checks.push_back(val);
    }

    indexes.resize(std::remove(indexes.begin(), indexes.end(), -1) - indexes.begin());

    // Must be unordered because orderChanged() etc. functions are not implemented.
    setWordList(dict, indexes);
}

WordsToDeckItemModel::~WordsToDeckItemModel()
{

}

void WordsToDeckItemModel::setColumnTexts()
{
    setColumnText(0, tr("Written"));
    setColumnText(1, tr("Kana"));
    setColumnText(2, tr("Definition"));
}

bool WordsToDeckItemModel::hasBoxChecked() const
{
    for (int ix = 0; ix != checks.size(); ++ix)
    {
        char val = checks[ix];
        // Returns if any of the lower 3 bits is checked, and the checkbox is not disabled.
        if (((val & 7) & (~(val >> 3)) & 7) != 0)
            return true;
    }
    return false;
}

//WordDeck* WordsToDeckItemModel::wordDeck() const
//{
//    return deck;
//}
//
Qt::ItemFlags WordsToDeckItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags r = base::flags(index);
    char val = checks[index.row()];
    if ((val & (1 << (index.column() + 3))) == 0)
        r |= Qt::ItemIsUserCheckable;
    if ((val & (1 << index.column())) == 0 || (val & (1 << (index.column() + 3))) != 0)
        r &= ~Qt::ItemIsEnabled;

    return r;
}

QVariant WordsToDeckItemModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::CheckStateRole && role != (int)CellRoles::Custom)
        return base::data(index, role);

    if (role == (int)CellRoles::Custom)
    {
        // The custom role returns the state of every checkbox in the row of index that are
        // not disabled.
        return (checks[index.row()] & 7) & ~(checks[index.row()] >> 3);
    }

    // role == Qt::CheckStateRole

    if ((checks[index.row()] & (1 << index.column())) != 0)
        return Qt::Checked;
    return Qt::Unchecked;
}

bool WordsToDeckItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole)
        return base::setData(index, value, role);

    char val = checks[index.row()];

    // The function is called with invalid indexes from the window's code to update multiple
    // boxes. The validity of the change must be checked.
    if ((val & (1 << (index.column() + 3))) != 0)
        return false;

    bool checked = value.toInt() == (int)Qt::Checked;
    bool changed = false;
    if ((val & (1 << (index.column() + 3))) == 0)
    {
        if (checked && (val & (1 << index.column())) == 0)
        {
            changed = true;
            val |= (1 << index.column());
        }
        else if (!checked && (val & (1 << index.column())) != 0)
        {
            changed = true;
            val &= ~(1 << index.column());
        }
    }

    if (changed)
    {
        checks[index.row()] = val;
        emit dataChanged(index, index, { Qt::CheckStateRole/*, Qt::DisplayRole*/ });
        emit headerDataChanged(Qt::Horizontal, index.column(), index.column());
    }

    return changed;
}

QVariant WordsToDeckItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::CheckStateRole || orientation != Qt::Horizontal)
        return base::headerData(section, orientation, role);

    int siz = checks.size();

    // Number of checked or disabled checkboxes.
    int cnt = 0;
    // Number of disabled checkboxes.
    int badcnt = 0;

    for (int ix = 0; ix != siz; ++ix)
    {
        char val = checks[ix];
        // Disabled checkboxes are counted as checked, to be able to determine whether the
        // header should show the intermediate, checked or unchecked states.
        if ((val & (1 << (section + 3))) != 0)
            ++cnt, ++badcnt;
        else if ((val & (1 << section)) != 0)
            ++cnt;
    }

    if (cnt == badcnt)
        return Qt::Unchecked;
    if (cnt == siz)
        return Qt::Checked;
    return Qt::PartiallyChecked;
}

bool WordsToDeckItemModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole || orientation != Qt::Horizontal)
        return base::setHeaderData(section, orientation, value, role);

    bool check = (Qt::CheckState)value.toInt() == Qt::Checked;

    bool changed = false;
    for (int ix = 0; ix != checks.size(); ++ix)
    {
        char val = checks[ix];
        // Check or uncheck enabled boxes.
        if (check && (val & (1 << (section + 3))) == 0)
            val |= (1 << section);
        if (!check && (val & (1 << (section + 3))) == 0)
            val &= ~(1 << section);

        if (checks[ix] != val)
        {
            changed = true;
            checks[ix] = val;
        }
    }

    if (changed)
    {
        emit headerDataChanged(Qt::Horizontal, section, section);
        emit dataChanged(index(0, section), index(checks.size(), section), { Qt::CheckStateRole/*, Qt::DisplayRole*/ });
    }

    return changed;
}

void WordsToDeckItemModel::entryChanged(int windex, bool studydef)
{
    auto list = getWordList();
    auto it = std::find(list.begin(), list.end(), windex);
    if (it != list.end())
    {
        WordEntry *w = dictionary()->wordEntry(windex);

        bool kanaonly = (w->defs[0].attrib.notes & (int)WordNotes::KanaOnly) != 0;

        bool haskanji = false;
        int ksiz = w->kanji.size();
        for (int iy = 0; !haskanji && iy != ksiz; ++iy)
            if (KANJI(w->kanji[iy].unicode()))
                haskanji = true;

        bool changed = false;

        char val = checks[it - list.begin()];
        if (!haskanji && (val & (1 << 3)) == 0)
        {
            val |= (1 << 3);
            changed = true;
        }
        //else if (!kanaonly)
        //{
        //    val |= 1;
        //    changed = true;
        //}

        QModelIndex ind = index(it - list.begin(), 0);
        if (changed)
            emit dataChanged(ind, ind, { Qt::DisplayRole });
    }

    base::entryChanged(windex, studydef);
}

void WordsToDeckItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    auto list = getWordList();
    auto it = std::find(list.begin(), list.end(), windex);
    if (it != list.end())
        checks.erase(checks.begin() + (it - list.begin()));

    base::entryRemoved(windex, abcdeindex, aiueoindex);
}


//-------------------------------------------------------------


DeckFormListView::DeckFormListView(QWidget *parent) : base(parent)
{
    setHorizontalHeader(new WordsToDeckHeader(this));
}

DeckFormListView::~DeckFormListView()
{
    ;
}

bool DeckFormListView::cancelActions()
{
    bool r = base::cancelActions();

    if (dynamic_cast<WordsToDeckHeader*>(horizontalHeader()) != nullptr)
        r |= ((WordsToDeckHeader*)horizontalHeader())->cancelActions();

    return r;
}


//-------------------------------------------------------------

WordsToDeckHeader::WordsToDeckHeader(ZListView *parent) : base(Qt::Horizontal, parent), mousecell(-1), hover(false), pressed(false)
{

}

WordsToDeckHeader::~WordsToDeckHeader()
{

}

void WordsToDeckHeader::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    if (!rect.isValid())
        return;

    painter->save();

    painter->setClipRect(rect);

    QStyleOptionHeader opt;
    initStyleOption(&opt);

    QStyle::State state = QStyle::State_None;
    if (isEnabled())
        state |= QStyle::State_Enabled;
    if (window()->isActiveWindow())
        state |= QStyle::State_Active;

    QVariant textAlignment = model()->headerData(logicalIndex, Qt::Horizontal, Qt::TextAlignmentRole);

    opt.rect = rect;
    opt.section = logicalIndex;
    opt.state |= state;

    QVariant foregroundBrush = model()->headerData(logicalIndex, Qt::Horizontal, Qt::ForegroundRole);
    if (foregroundBrush.canConvert<QBrush>())
        opt.palette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(foregroundBrush));

    QPointF oldBO = painter->brushOrigin();
    QVariant backgroundBrush = model()->headerData(logicalIndex, Qt::Horizontal, Qt::BackgroundRole);
    if (backgroundBrush.canConvert<QBrush>()) {
        opt.palette.setBrush(QPalette::Button, qvariant_cast<QBrush>(backgroundBrush));
        opt.palette.setBrush(QPalette::Window, qvariant_cast<QBrush>(backgroundBrush));
        painter->setBrushOrigin(opt.rect.topLeft());
    }

    // If columns can be hidden or moved these values would be different, but it's never the
    // case with the custom header view.
    bool first = logicalIndex == 0;
    bool last = logicalIndex == 2;
    if (first && last)
        opt.position = QStyleOptionHeader::OnlyOneSection;
    else if (first)
        opt.position = QStyleOptionHeader::Beginning;
    else if (last)
        opt.position = QStyleOptionHeader::End;
    else
        opt.position = QStyleOptionHeader::Middle;

    opt.orientation = Qt::Horizontal;

    // No selection in the custom header. Otherwise it'd be drawn differently and in a way
    // that's complicated to get from the base class.
    opt.selectedPosition = QStyleOptionHeader::NotAdjacent;

    style()->drawControl(QStyle::CE_Header, &opt, painter, this);

    painter->setBrushOrigin(oldBO);

    QRect r = drawCheckBox(painter, logicalIndex);

    QString text = model()->headerData(logicalIndex, Qt::Horizontal, Qt::DisplayRole).toString();
    
    QFontMetrics fm(painter->font());
    text = fm.elidedText(text, textElideMode(), r.width());
    int left = r.right() + 4;
    painter->drawText(left, rect.top(), rect.width() - 8, rect.height(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);

    painter->restore();
}

QSize WordsToDeckHeader::sectionSizeFromContents(int logicalIndex) const
{
    QSize s = base::sectionSizeFromContents(logicalIndex);

    s.setWidth(s.width() + 4 + checkBoxRect(logicalIndex).width());

    return s;
}

void WordsToDeckHeader::mouseMoveEvent(QMouseEvent *e)
{
    int i = logicalIndexAt(e->pos());

    if (hover && (i != mousecell || !checkBoxHitRect(mousecell).contains(e->pos())))
    {
        hover = false;
        viewport()->update(checkBoxRect(mousecell));
        if (!pressed)
            mousecell = -1;
    }

    if (hover || i == -1 || (pressed && i != mousecell) || (!pressed && e->buttons().testFlag(Qt::LeftButton)))
    {
        base::mouseMoveEvent(e);
        return;
    }

    // Moving over a checkbox.
    if (checkBoxHitRect(i).contains(e->pos()))
    {
        hover = true;
        mousecell = i;

        viewport()->update(checkBoxRect(i));

    }

    if (!pressed)
        mousecell = i;

    base::mouseMoveEvent(e);
    e->accept();
}

void WordsToDeckHeader::mouseDoubleClickEvent(QMouseEvent *e)
{
    base::mouseDoubleClickEvent(e);
    e->accept();
}

void WordsToDeckHeader::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        int i = logicalIndexAt(e->pos());

        if (hover && (i != mousecell || !checkBoxHitRect(mousecell).contains(e->pos())))
        {
            hover = false;
            viewport()->update(checkBoxRect(mousecell));
            mousecell = -1;
        }

        // Moving over a checkbox.
        if (!hover && i != -1 && checkBoxHitRect(i).contains(e->pos()))
        {
            hover = true;
            mousecell = i;
        }

        if (hover)
        {
            pressed = true;
            viewport()->update(checkBoxRect(mousecell));
            return;
        }

    }

    base::mousePressEvent(e);
    e->accept();
}

void WordsToDeckHeader::mouseReleaseEvent(QMouseEvent *e)
{
    if (pressed && e->button() == Qt::LeftButton)
    {
        int i = logicalIndexAt(e->pos());

        QRect chr = i == mousecell ? checkBoxHitRect(mousecell) : QRect();
        if (hover && (i != mousecell || !chr.contains(e->pos())))
        {
            hover = false;
            viewport()->update(checkBoxRect(mousecell));
            mousecell = -1;
        }

        if (!hover && i == mousecell && chr.contains(e->pos()))
            hover = true;

        if (hover)
        {
            pressed = false;
            viewport()->update(checkBoxRect(mousecell));

            model()->setHeaderData(mousecell, Qt::Horizontal, model()->headerData(mousecell, Qt::Horizontal, Qt::CheckStateRole).toInt() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        }

        pressed = false;
        return;
    }

    base::mouseReleaseEvent(e);
    e->accept();
}

void WordsToDeckHeader::leaveEvent(QEvent *e)
{
    if (hover)
    {
        hover = false;
        viewport()->update(checkBoxRect(mousecell));
        mousecell = -1;
    }

    base::leaveEvent(e);
}

bool WordsToDeckHeader::cancelActions()
{
    bool r = pressed;
    pressed = false;
    return r;
}

QRect WordsToDeckHeader::sectionRect(int index) const
{
    int vpos = sectionViewportPosition(index);
    int siz = sectionSize(index);
    if (vpos >= width() || vpos + siz <= 0)
        return QRect();

    return QRect(vpos, 0, siz, height());
}

CheckBoxMouseState WordsToDeckHeader::checkBoxMouseState(int index) const
{
    if (mousecell != index || (!hover && !pressed))
        return CheckBoxMouseState::None;

    if (hover && !pressed)
        return CheckBoxMouseState::Hover;
    if (pressed && !hover)
        return CheckBoxMouseState::Down;

    return CheckBoxMouseState::DownHover;
}

Qt::CheckState WordsToDeckHeader::checkState(int index) const
{
    return (Qt::CheckState)model()->headerData(index, Qt::Horizontal, Qt::CheckStateRole).toInt();
}

QRect WordsToDeckHeader::checkBoxRect(int index) const
{
    QStyleOptionButton opb;
    initCheckBoxOption(index, &opb);

    return ::checkBoxRect(this, 4, sectionRect(index), &opb);
}

QRect WordsToDeckHeader::checkBoxHitRect(int index) const
{
    QStyleOptionButton opb;
    initCheckBoxOption(index, &opb);

    return ::checkBoxHitRect(this, 4, sectionRect(index), &opb);
}

QRect WordsToDeckHeader::drawCheckBox(QPainter *p, int index) const
{
    QStyleOptionButton opb;
    initCheckBoxOption(index, &opb);

    ::drawCheckBox(p, this, 4, sectionRect(index), &opb);
    return opb.rect;
}

void WordsToDeckHeader::initCheckBoxOption(int index, QStyleOptionButton *opt) const
{
    ::initCheckBoxOption(this, checkBoxMouseState(index), isEnabled(), checkState(index), opt);
}


//-------------------------------------------------------------


WordToDeckForm::WordToDeckForm(QWidget *parent) : base(parent), ui(new Ui::WordToDeckForm), model(nullptr)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->wordsTable->setSelectionType(ListSelectionType::Extended);
    ui->wordsTable->setStudyDefinitionUsed(true);

    //restrictWidgetSize(ui->decksCBox, 16, AdjustedValue::Min);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &WordToDeckForm::okButtonClicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &WordToDeckForm::close);

    ui->wordsTable->assignStatusBar(ui->listStatus);

    FormStates::restoreDialogSize("WordToDeck", this, true);
}

WordToDeckForm::~WordToDeckForm()
{
    delete ui;
}

void WordToDeckForm::exec(WordDeck *studydeck, Dictionary *dictionary, const std::vector<int> &ind)
{
    if (ind.empty() || (studydeck == nullptr && dictionary == nullptr))
    {
        deleteLater();
        return;
    }

    indexes = ind;

    connect(model, &ZAbstractTableModel::dataChanged, this, &WordToDeckForm::checkStateChanged);
    connect(ui->wordsTable, &ZDictionaryListView::rowSelectionChanged, this, &WordToDeckForm::selChanged);

    deck = studydeck;

    dict = deck != nullptr ? deck->dictionary() : dictionary;

    if (!dict->wordDecks()->empty())
    {
        for (int ix = 0, siz = dict->wordDecks()->size(); ix != siz; ++ix)
            ui->decksCBox->addItem(dict->wordDecks()->items(ix)->getName());
        ui->decksCBox->setCurrentIndex(dict->wordDecks()->indexOf(deck));
    }
    else
    {
        // Placeholder text added. Translated in setWidgetTexts().
        ui->decksCBox->addItem(QString());
        ui->decksCBox->setCurrentIndex(0);
    }
    ui->decksCBox->setEnabled(!dict->wordDecks()->empty());

    model = new WordsToDeckItemModel(dict, deck, indexes, this);
    if (model->rowCount() == 0)
    {
        if (deck->dictionary()->wordDecks()->size() <= 1)
        {
            QMessageBox::information(parentWidget(), "zkanji", tr("Nothing new to study. All word parts are already in the deck."), QMessageBox::Ok);

            deleteLater();
            return;
        }
    }
    ui->wordsTable->setModel(model);

    updateOkButton();

    translateTexts();

    //setWindowModality(Qt::ApplicationModal);

    // Warning: if changing from showModal() to show(), make sure word or dictionary changes
    // are handled correctly.
    showModal();
}

bool WordToDeckForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateTexts();
    }

    return base::event(e);
}

void WordToDeckForm::showEvent(QShowEvent *e)
{
    base::showEvent(e);
    if (model->rowCount() == 0)
        QTimer::singleShot(0, [this]() { QMessageBox::information(window(), "zkanji", tr("Nothing new to study. All word parts are already in the deck.\n\nYou can select another deck if you want to add them."), QMessageBox::Ok); });
}

void WordToDeckForm::checkStateChanged(const QModelIndex &first, const QModelIndex &last, const QVector<int> roles)
{
    // Check every row in the same column if we clicked inside a selection.

    //if (checking)
    //    return;

    //WordsToDeckItemModel *m = (WordsToDeckItemModel*)ui->wordsTable->model();
    //if (!roles.contains(Qt::CheckStateRole) || first.row() != last.row() || m->rowCount() <= 1)
    //{
    //    updateOkButton();
    //    return;
    //}

    //std::vector<int> sel;
    //ui->wordsTable->selectedRows(sel);
    //if (sel.size() < 2 || std::find(sel.begin(), sel.end(), first.row()) == sel.end())
    //{
    //    updateOkButton();
    //    return;
    //}

    //checking = true;
    //Qt::CheckState chk = (Qt::CheckState)first.data(Qt::CheckStateRole).toInt();

    //// Send this check state to every selected row.
    //for (int ix = 0; ix != sel.size(); ++ix)
    //    m->setData(m->index(sel[ix], first.column()), chk, Qt::CheckStateRole);

    //checking = false;

    updateOkButton();
}

void WordToDeckForm::selChanged()
{
    std::vector<int> sel;
    ui->wordsTable->selectedRows(sel);
    for (int ix = 0, siz = sel.size(); ix != siz; ++ix)
        sel[ix] = model->indexes(sel[ix]);
    ui->defEditor->setWords(dict, sel);
}

void WordToDeckForm::okButtonClicked(bool)
{
    WordsToDeckItemModel *m = (WordsToDeckItemModel*)ui->wordsTable->model();
    int cnt = m->rowCount();

    std::vector<std::pair<int, int>> words;
    for (int ix = 0; ix != cnt; ++ix)
    {
        // The custom role returns the state of every checkbox in the row of index that are
        // not disabled.
        int val = m->data(m->index(ix, 0), (int)CellRoles::Custom).toInt();
        if (val != 0)
            words.push_back(std::make_pair(m->indexes(ix), val));
    }

    if (deck == nullptr)
    {
        dict->wordDecks()->add(tr("Deck 1"));
        deck = dict->wordDecks()->items(0);
    }
    int r = deck->queueWordItems(words);
    deck->owner()->setLastSelected(deck);

    //WordStudyListForm *f = WordStudyListForm::Instance(deck, DeckStudyPages::Items, true);
    //f->showQueue();

    close();

    QTimer::singleShot(0, [r]() { QMessageBox::information((QWidget*)gUI->mainForm(), "zkanji", tr("%n item(s) added to the study deck", "", r));  });
}

void WordToDeckForm::on_decksCBox_currentIndexChanged(int index)
{
    if (model == nullptr || deck == nullptr)
        return;

    deck = dict->wordDecks()->items(index);
    model->deleteLater();
    model = new WordsToDeckItemModel(dict, deck, indexes, this);
    ui->wordsTable->setModel(model);

    if (model->rowCount() == 0)
        QMessageBox::information(window(), "zkanji", tr("Nothing new to study. All word parts are already in the deck.\n\nYou can select another deck if you want to add them."), QMessageBox::Ok);

    updateOkButton();
}

void WordToDeckForm::updateOkButton()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(model->hasBoxChecked());
}

void WordToDeckForm::translateTexts()
{
    setWindowTitle(QString("zkanji - %1").arg(tr("Add words to deck")));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Add to study deck"));

    if (deck == nullptr)
        ui->decksCBox->setItemText(0, tr("Deck 1"));

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(qApp->translate("ButtonBox", "OK"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}


//-------------------------------------------------------------


void addWordsToDeck(WordDeck *deck, const std::vector<int> &indexes, QWidget *dialogParent)
{
    if (deck == nullptr)
#ifdef _DEBUG
        throw "Error, no deck passed to addWordsToDeck";
#else
        return;
#endif

    if (dialogParent == nullptr)
        dialogParent = gUI->activeMainForm();
    WordToDeckForm *frm = new WordToDeckForm(dialogParent);
    frm->exec(deck, nullptr, indexes);
}

void addWordsToDeck(Dictionary *dictionary, const std::vector<int> &indexes, QWidget *dialogParent)
{
    if (dictionary == nullptr)
#ifdef _DEBUG
        throw "Error, no dictionary passed to addWordsToDeck";
#else
        return;
#endif

    if (dialogParent == nullptr)
        dialogParent = gUI->activeMainForm();
    WordToDeckForm *frm = new WordToDeckForm(dialogParent);
    frm->exec(dictionary->wordDecks()->lastSelected(), dictionary, indexes);
}


//-------------------------------------------------------------
