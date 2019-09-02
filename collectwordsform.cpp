/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <set>
#include "collectwordsform.h"
#include "ui_collectwordsform.h"
#include "globalui.h"
#include "words.h"
#include "kanji.h"
#include "zui.h"
#include "grouppickerform.h"
#include "groups.h"
#include "zkanjigridmodel.h"
#include "ranges.h"
#include "zflowlayout.h"
#include "zkanjimain.h"
#include "zdictionarymodel.h"
#include "furigana.h"
#include "formstates.h"
#include "zdictionarylistview.h"
#include "generalsettings.h"

#include "checked_cast.h"


//-------------------------------------------------------------


CheckableKanjiReadingsListModel::CheckableKanjiReadingsListModel(QObject *parent) : base(parent), kindex(-1)
{

}

CheckableKanjiReadingsListModel::~CheckableKanjiReadingsListModel()
{

}

void CheckableKanjiReadingsListModel::setKanji(int kanjiindex, ushort checkstate)
{
    if (kindex == kanjiindex)
        return;
    beginResetModel();
    kindex = kanjiindex;
    checks = checkstate;
    endResetModel();
}

int CheckableKanjiReadingsListModel::kanji()
{
    return kindex;
}

int CheckableKanjiReadingsListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || kindex == -1)
        return 0;

    return ZKanji::kanjiReadingCount(ZKanji::kanjis[kindex], true);
}

QVariant CheckableKanjiReadingsListModel::data(const QModelIndex &index, int role) const
{
    if (kindex == -1 || !index.isValid())
        return QVariant();

    int row = index.row();
    if (role == Qt::CheckStateRole)
        return (checks & (1 << row)) == 0 ? Qt::Unchecked : Qt::Checked;

    if (role == Qt::DisplayRole)
    {
        KanjiEntry *k = ZKanji::kanjis[kindex];
        return ZKanji::kanjiCompactReading(k, row);
    }

    return QVariant();
}

bool CheckableKanjiReadingsListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (kindex == -1 || !index.isValid() || role != Qt::CheckStateRole)
        return false;

    int row = index.row();
    bool changed = false;

    ushort allreadings = 0xffff >> (16 - ZKanji::kanjiReadingCount(ZKanji::kanjis[kindex], true));
    if (value.toBool())
    {
        changed = (checks & (1 << row)) == 0;
        checks |= (1 << row);

        // Setting the check state to full if every kanji reading is checked.
        if ((checks & allreadings) == allreadings)
            checks = 0xffff;
    }
    else
    {
        changed = (checks & (1 << row)) != 0;
        checks &= ~(1 << row);

        // Setting the check state to null if no kanji reading is checked.
        if ((checks & allreadings) == 0)
            checks = 0;
    }

    if (changed)
        emit readingsChecked(checks);

    return true;
}

Qt::ItemFlags CheckableKanjiReadingsListModel::flags(const QModelIndex &index) const
{
    return base::flags(index) | Qt::ItemIsUserCheckable;
}

//-------------------------------------------------------------


enum class CollectedColumnTypes
{
    Checked = (int)DictColumnTypes::Last
};

CollectedWordListModel::CollectedWordListModel(QObject *parent) : base(parent), checkedcnt(0)
{
    setColumns({
        { (int)CollectedColumnTypes::Checked, Qt::AlignHCenter, ColumnAutoSize::NoAuto, false, checkBoxSize().width() + 8, QString() },
        { (int)DictColumnTypes::Frequency, Qt::AlignHCenter, ColumnAutoSize::NoAuto, false, -1, QString() },
        { (int)DictColumnTypes::Kanji, Qt::AlignLeft, ColumnAutoSize::Auto, true, Settings::scaled(50), QString() },
        { (int)DictColumnTypes::Kana, Qt::AlignLeft, ColumnAutoSize::Auto, true, Settings::scaled(50), QString() },
        { (int)DictColumnTypes::Definition, Qt::AlignLeft, ColumnAutoSize::NoAuto, false, Settings::scaled(6400), QString() }
    });
}

CollectedWordListModel::~CollectedWordListModel()
{

}

void CollectedWordListModel::setWordList(Dictionary *d, std::vector<int> &&wordlist)
{
    checks.resize(wordlist.size());
    memset(checks.data(), true, checks.size());
    checkedcnt = tosigned(checks.size());
    base::setWordList(d, std::move(wordlist), true);
}

bool CollectedWordListModel::hasCheckedWord() const
{
    return checkedcnt != 0;
}

void CollectedWordListModel::getCheckedWords(std::vector<int> &result) const
{
    result.resize(checkedcnt);
    int pos = 0;
    for (int ix = 0, siz = tosigned(checks.size()); ix != siz && pos != checkedcnt; ++ix)
        if (checks[ix] != 0)
            result[pos++] = indexes(ix);
}

QVariant CollectedWordListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::CheckStateRole || headerData(index.column(), Qt::Horizontal, (int)DictColumnRoles::Type).toInt() != (int)CollectedColumnTypes::Checked)
        return base::data(index, role);

    int row = index.row();
    return checks[row] != 0 ? Qt::Checked : Qt::Unchecked;
}

bool CollectedWordListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::CheckStateRole || headerData(index.column(), Qt::Horizontal, (int)DictColumnRoles::Type).toInt() != (int)CollectedColumnTypes::Checked)
        return base::setData(index, value, role);

    int row = index.row();
    bool changed = checks[row] != (value.toInt() != Qt::Unchecked ? 1 : 0);
    if (changed)
    {
        checks[row] = (value.toInt() != Qt::Unchecked ? 1 : 0);
        checkedcnt += (value.toInt() != Qt::Unchecked ? 1 : -1);
        emit checkStateChanged();
        emit statusChanged();
    }

    return changed;
}

Qt::ItemFlags CollectedWordListModel::flags(const QModelIndex &index) const
{
    if (headerData(index.column(), Qt::Horizontal, (int)DictColumnRoles::Type).toInt() != (int)CollectedColumnTypes::Checked)
        return base::flags(index);
    return base::flags(index) | Qt::ItemIsUserCheckable;
}

void CollectedWordListModel::removeAt(int index)
{
    if (checks[index] == 1)
        --checkedcnt;
    checks.erase(checks.begin() + index);

    base::removeAt(index);
}

int CollectedWordListModel::statusCount() const
{
    return base::statusCount() + 1;
}

StatusTypes CollectedWordListModel::statusType(int statusindex) const
{
    if (statusindex > 0)
        return base::statusType(statusindex - 1);
    return StatusTypes::TitleValue;

}

QString CollectedWordListModel::statusText(int statusindex, int labelindex, int rowpos) const
{
    if (statusindex > 0)
        return base::statusText(statusindex - 1, labelindex, rowpos);
    if (labelindex < 0)
        return "Checked:";

    return QString::number(checkedcnt);
}

int CollectedWordListModel::statusSize(int statusindex, int labelindex) const
{
    if (statusindex > 0)
        return base::statusSize(statusindex - 1, labelindex);

    if (labelindex < 0)
        return 0;

    return 8;
}

bool CollectedWordListModel::statusAlignRight(int statusindex) const
{
    if (statusindex > 0)
        return base::statusAlignRight(statusindex - 1);

    return false;
}

void CollectedWordListModel::setColumnTexts()
{
    setColumnText(2, tr("Written"));
    setColumnText(3, tr("Kana"));
    setColumnText(4, tr("Definition"));
}

void CollectedWordListModel::orderChanged(const std::vector<int> &ordering)
{
    std::vector<char> tmp;
    tmp.swap(checks);
    checks.resize(tmp.size());
    for (int ix = 0, siz = tosigned(tmp.size()); ix != siz; ++ix)
        checks[ix] = tmp[ordering[ix]];
}

bool CollectedWordListModel::addNewEntry(int /*windex*/, int &/*position*/)
{
    return false;
}

void CollectedWordListModel::entryAddedPosition(int /*pos*/)
{
    // addNewEntry() returns false so we never get here.
    ;
}

void CollectedWordListModel::entryRemoved(int windex, int /*abcdeindex*/, int /*aiueoindex*/)
{
    int wpos = -1;
    for (int ix = 0, siz = rowCount(); wpos == -1 && ix != siz; ++ix)
        if (indexes(ix) == windex)
            wpos = ix;

    if (wpos != -1)
    {
        bool checked = checks[wpos];
        checks.erase(checks.begin() + wpos);
        if (checked)
        {
            --checkedcnt;
            emit checkStateChanged();
        }
    }
}


//-------------------------------------------------------------


CollectWordsForm::CollectWordsForm(QWidget *parent) : base(parent), ui(new Ui::CollectWordsForm), dict(nullptr), kmodel(nullptr), wmodel(nullptr), readingsmodel(nullptr)
{
    ui->setupUi(this);
    ui->dictWidget->showStatusBar();
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->readingsView->setSelectionType(ListSelectionType::Extended);
    ui->readingsView->setCheckBoxColumn(0);

    ui->readingsView->setFontSizeHint(26, 12);

    ui->kanjiNumEdit->setCharacterSize(6);
    ui->kanaLenEdit->setCharacterSize(6);
    ui->freqEdit->setCharacterSize(6);

    QLocale validatorlocale;
    validatorlocale.setNumberOptions(QLocale::OmitGroupSeparator);

    ui->kanjiNumEdit->setValidator(new IntRangeValidator(1, 20, false, this));
    QIntValidator *validator = new QIntValidator(1, 99, this);
    validator->setLocale(validatorlocale);
    ui->kanaLenEdit->setValidator(validator);
    validator = new QIntValidator(0, 9999, this);
    validator->setLocale(validatorlocale);
    ui->freqEdit->setValidator(validator);

    ui->dictWidget->setListMode(DictionaryWidget::Filtered);
    ui->dictWidget->setExamplesVisible(false);
    ui->dictWidget->setMultilineVisible(false);
    ui->dictWidget->setSelectionType(ListSelectionType::Extended);
    ui->dictWidget->setCheckBoxColumn(0);
    ui->dictWidget->view()->setGroupDisplay(true);

    //ui->groupLabel->setStyleSheet("QLabel { color: gray }");

    connect(ui->kanjiGrid, &ZKanjiGridView::selectionChanged, this, &CollectWordsForm::kanjiSelectionChanged);

    connect(ui->closeButton, &QPushButton::clicked, this, &CollectWordsForm::close);
    //addButton = ui->buttonBox->addButton(tr("Add selected to group"), QDialogButtonBox::AcceptRole);
    //addButton->setEnabled(false);
    //connect(addButton, &QPushButton::clicked, this, &CollectWordsForm::addClicked);

    connect(ui->posCBox, &QComboBox::currentTextChanged, this, &CollectWordsForm::positionBoxChanged);

    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &CollectWordsForm::dictionaryToBeRemoved);
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &CollectWordsForm::dictionaryReplaced);

    ui->kanjiGrid->assignStatusBar(ui->kanjiStatus);
}

CollectWordsForm::~CollectWordsForm()
{
    delete ui;
}

void CollectWordsForm::exec(Dictionary *d, const std::vector<ushort> &kanjis)
{
    // Checking for duplicates and only saving the first kanji in accepted.
    std::set<ushort> found;
    std::vector<ushort> accepted;
    accepted.reserve(kanjis.size());
    for (int ix = 0, siz = tosigned(kanjis.size()); ix != siz; ++ix)
    {
        ushort val = kanjis[ix];
        if (found.count(val) != 0)
            continue;

        found.insert(val);
        accepted.push_back(val);
    }

    readings.resize(accepted.size(), 0xffff);
    placement.resize(readings.size());

    readingsmodel = new CheckableKanjiReadingsListModel(this);
    ui->readingsView->setModel(readingsmodel);
    connect(readingsmodel, &CheckableKanjiReadingsListModel::readingsChecked, this, &CollectWordsForm::readingsChecked);

    dict = nullptr;
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(d)));
    if (dict != d)
        connect(dict, &Dictionary::dictionaryReset, this, &CollectWordsForm::dictionaryReset);
        dict = d;


    kmodel = new KanjiListModel(this);
    kmodel->setList(accepted);
    kmodel->setAcceptDrop(true);
    ui->kanjiGrid->setModel(kmodel);
    connect(kmodel, &KanjiGridModel::itemsInserted, this, &CollectWordsForm::kanjiInserted);

    //connect(&dict->wordGroups(), &WordGroups::groupDeleted, this, &CollectWordsForm::groupDeleted);
    ui->kanjiGrid->setDictionary(dict);
    ui->dictWidget->setDictionary(dict);
    wmodel = new CollectedWordListModel(this);
    wmodel->setWordList(dict, std::vector<int>());
    ui->dictWidget->setModel(wmodel);

    // Restoring saved window state.
    if (!FormStates::emptyState(FormStates::collectform))
    {
        CollectFormData &dat = FormStates::collectform;

        ui->kanjiNumEdit->setText(dat.kanjinum);
        ui->kanaLenEdit->setText(QString::number(dat.kanalen));
        ui->freqEdit->setText(QString::number(dat.minfreq));
        ui->strictBox->setChecked(dat.strict);
        ui->limitBox->setChecked(dat.limit);
        //QRect geo = geometry();
        //int wdif = dat.siz.width() - geo.width();
        //int hdif = dat.siz.height() - geo.height();
        //move(geo.left() - wdif / 2, geo.top() / hdif / 2);
        resize(dat.siz);

        ui->dictWidget->restoreState(dat.dict);
    }

    show();


    // Test to see the number of readings in a kanji. Not used in final code.
    //int cnt = 0;
    //QString k;
    //for (int ix = 0, siz = ZKanji::kanjis.size(); ix != siz; ++ix)
    //{
    //    int val = ZKanji::kanjiReadingCount(ZKanji::kanjis[ix], true);
    //    if (val > cnt)
    //    {
    //        cnt = val;
    //        k = ZKanji::kanjis[ix]->ch;
    //    }
    //    else if (val == cnt)
    //        k = k + ZKanji::kanjis[ix]->ch;
    //}
    //QMessageBox::information(this, "blahblah", QString("%1, %2").arg(cnt).arg(k), QMessageBox::Ok);
}

void CollectWordsForm::closeEvent(QCloseEvent *e)
{
    CollectFormData &dat = FormStates::collectform;
    dat.kanjinum = ui->kanjiNumEdit->text();
    dat.kanalen = ui->kanaLenEdit->text().toInt();
    dat.minfreq = ui->freqEdit->text().toInt();
    dat.strict = ui->strictBox->isChecked();
    dat.limit = ui->limitBox->isChecked();
    dat.siz = rect().size();
    ui->dictWidget->saveState(dat.dict);

    base::closeEvent(e);
}

//void CollectWordsForm::on_groupButton_clicked()
//{
//    dest = dynamic_cast<WordGroup*>(GroupPickerForm::select(GroupWidget::Modes::Words, tr("Select word group where the words from the generated list will be placed."), dict, this));
//    if (dest == nullptr)
//        return;
//
//    //ui->groupLabel->setStyleSheet(QString());
//    //ui->groupLabel->setText(dest->name());
//    //QFont f = ui->groupLabel->font();
//    //f.setItalic(false);
//    //ui->groupLabel->setFont(f);
//}
//
//void CollectWordsForm::groupDeleted(GroupCategoryBase *parent, int index, void *oldaddress)
//{
//    if (oldaddress != dest)
//        return;
//
//    dest = nullptr;
//    ui->groupLabel->setText(QString());
//    //ui->groupLabel->setStyleSheet("QLabel { color: gray }");
//    //ui->groupLabel->setText(tr("Select Group"));
//    //QFont f = ui->groupLabel->font();
//    //f.setItalic(true);
//    //ui->groupLabel->setFont(f);
//}

void CollectWordsForm::kanjiInserted(const smartvector<Interval> &intervals)
{
    for (int ix = tosigned(intervals.size()) - 1; ix != -1; --ix)
    {
        readings.insert(readings.begin() + intervals[ix]->index, intervals[ix]->count, 0xffff);
        placement.insert(placement.begin() + intervals[ix]->index, intervals[ix]->count, Anywhere);
    }
}

void CollectWordsForm::readingsChecked(ushort checkstate)
{
    if (ui->kanjiGrid->selCount() != 1)
        return;
    int cell = ui->kanjiGrid->selectedCell(0);
    readings[cell] = checkstate;
}

void CollectWordsForm::positionBoxChanged()
{
    if (ui->posCBox->currentIndex() == -1)
        return;
    KanjiPlacement pos = (KanjiPlacement)ui->posCBox->currentIndex();
    for (int ix = 0, siz = ui->kanjiGrid->selCount(); ix != siz; ++ix)
        placement[ui->kanjiGrid->selectedCell(ix)] = pos;
}

void CollectWordsForm::kanjiSelectionChanged()
{
    if (ui->kanjiGrid->selCount() != 1)
    {
        readingsmodel->setKanji(-1);

        if (ui->kanjiGrid->selCount() != 0)
        {
            int cell = ui->kanjiGrid->selectedCell(0);
            KanjiPlacement pc = placement[cell];
            bool match = true;
            for (int ix = 1, siz = ui->kanjiGrid->selCount(); match && ix != siz; ++ix)
                match = pc == placement[ui->kanjiGrid->selectedCell(ix)];

            if (!match)
                ui->posCBox->setCurrentIndex(-1);
            else
                ui->posCBox->setCurrentIndex((int)pc);
        }
        else
            ui->posCBox->setCurrentIndex(-1);

        return;
    }

    int cell = ui->kanjiGrid->selectedCell(0);
    readingsmodel->setKanji(kmodel->getList()[cell], readings[cell]);
    ui->posCBox->setCurrentIndex((int)placement[cell]);
}

void CollectWordsForm::on_generateButton_clicked()
{
    const std::vector<ushort> kanji = kmodel->getList();

    std::vector<int> words;
    dict->getKanjiWords(kanji, words);

    int minfreq = ui->freqEdit->text().toInt();
    int maxklen = ui->kanaLenEdit->text().toInt();

    int minkanji;
    int maxkanji;
    if (!findStrIntMinMax(ui->kanjiNumEdit->text(), 0, 99, minkanji, maxkanji))
        minkanji = 0;
    if (maxkanji == -1 || maxkanji == 100)
        maxkanji = 99;

    bool limit = ui->limitBox->isChecked();
    bool strict = ui->strictBox->isChecked();
    //int minjlpt = ui->jlptMinCBox->currentIndex() == 0 ? 6 : 6 - ui->jlptMinCBox->currentIndex();
    //int maxjlpt = ui->jlptMaxCBox->currentIndex() == 0 ? -1 : 6 - ui->jlptMaxCBox->currentIndex();

    // [kanji index, <checked readings, placement in word>]
    std::map<ushort, std::pair<ushort, KanjiPlacement>> listkanji;
    bool checkkanji = false;
    bool needfuri = false;

    for (int ix = 0, siz = tosigned(kanji.size()); ix != siz; ++ix)
    {

        ushort kindex = kanji[ix];
        ushort reading = readings[ix];
        // Kanji with no reading checked should be skipped.
        if (reading == 0)
            continue;
        KanjiPlacement place = placement[ix];
        listkanji[ZKanji::kanjis[kindex]->ch.unicode()] = std::make_pair(reading, place);
        if (place != KanjiPlacement::Anywhere || reading != 0xffff)
            checkkanji = true;
        if (reading != 0xffff)
            needfuri = true;
    }

    for (int ix = 0, siz = tosigned(words.size()); ix != siz; ++ix)
    {
        int windex = words[ix];
        const WordEntry *e = dict->wordEntry(windex);
        //WordCommons *cm;
        if (tosigned(e->freq) < minfreq || (maxklen > 0 && tosigned(e->kana.size()) > maxklen) /*||
            ((minjlpt != 6 || maxjlpt != -1) && ((cm = ZKanji::commons.findWord(e->kanji.data(), e->kana.data(), e->romaji.data())) == nullptr || cm->jlptn < maxjlpt || cm->jlptn > minjlpt))*/)
            words[ix] = -1;
        else if (limit || maxkanji != -1 || checkkanji)
        {
            int kanjicnt = 0;
            bool kanjifound = !checkkanji;

            std::vector<FuriganaData> furi;
                
            for (int iy = 0, siz2 = e->kanji.size(); iy != siz2; ++iy)
            {
                if (KANJI(e->kanji[iy].unicode()))
                {
                    ++kanjicnt;

                    std::map<ushort, std::pair<ushort, KanjiPlacement>>::iterator it;
                    if ((maxkanji != -1 && (/*kanjicnt < minkanji ||*/ kanjicnt > maxkanji)) || (limit && ((it = listkanji.find(e->kanji[iy].unicode())) == listkanji.end() || it->second.first == 0)))
                    {
                        words[ix] = -1;
                        break;
                    }
                    else if (!limit)
                        it = listkanji.find(e->kanji[iy].unicode());

                    if (!checkkanji || it == listkanji.end() || it->second.first == 0)
                        continue;

                    bool goodplace = (it->second.second == KanjiPlacement::Anywhere ||
                        (iy == 0 && (it->second.second == KanjiPlacement::Front || it->second.second == KanjiPlacement::FrontEnd || it->second.second == KanjiPlacement::FrontMiddle)) ||
                        (iy == siz2 - 1 && (it->second.second == KanjiPlacement::End || it->second.second == KanjiPlacement::FrontEnd || it->second.second == KanjiPlacement::MiddleEnd)) ||
                        (iy != 0 && iy != siz2 - 1 && (it->second.second == KanjiPlacement::FrontMiddle || it->second.second == KanjiPlacement::MiddleEnd || it->second.second == KanjiPlacement::Middle)));
                    if (!goodplace && strict)
                    {
                        words[ix] = -1;
                        break;
                    }

                    bool goodfuri = !needfuri;
                    if (needfuri)
                    {
                        if (furi.empty())
                            findFurigana(e->kanji, e->kana, furi);

                        int r = findKanjiReading(e->kanji, e->kana, iy, nullptr, &furi);
                        if ((it->second.first & (1 << r)) != 0)
                            goodfuri = true;
                    }
                    if (!goodfuri && strict)
                    {
                        words[ix] = -1;
                        break;
                    }

                    if (goodfuri && goodplace)
                        kanjifound = true;
                }
            }
            if (!kanjifound || kanjicnt < minkanji)
                words[ix] = -1;
        }
    }

    words.resize(std::remove(words.begin(), words.end(), -1) - words.begin());
    if (wmodel != nullptr)
        wmodel->deleteLater();

    wmodel = new CollectedWordListModel(this);
    wmodel->setWordList(dict, std::move(words));
    ui->dictWidget->setModel(wmodel);
    connect(wmodel, &CollectedWordListModel::checkStateChanged, this, &CollectWordsForm::wordChecked);

    wordChecked();
}

//void CollectWordsForm::on_jlptMinCBox_currentTextChanged()
//{
//    if (ui->jlptMaxCBox->currentIndex() != 0 && ui->jlptMaxCBox->currentIndex() < ui->jlptMinCBox->currentIndex())
//        ui->jlptMaxCBox->setCurrentIndex(ui->jlptMinCBox->currentIndex());
//}
//
//void CollectWordsForm::on_jlptMaxCBox_currentTextChanged()
//{
//    if (ui->jlptMinCBox->currentIndex() != 0 && ui->jlptMaxCBox->currentIndex() < ui->jlptMinCBox->currentIndex())
//        ui->jlptMinCBox->setCurrentIndex(ui->jlptMaxCBox->currentIndex());
//}

void CollectWordsForm::on_addButton_clicked()
{
    WordGroup *grpdest = dynamic_cast<WordGroup*>(GroupPickerForm::select(GroupWidget::Modes::Words, tr("Select a word group for the checked words."), dict, false, false, this));
    if (grpdest == nullptr)
        return;

    std::vector<int> windexes;
    wmodel->getCheckedWords(windexes);
    grpdest->add(windexes);
}

void CollectWordsForm::wordChecked()
{
    ui->addButton->setEnabled(wmodel->hasCheckedWord());
}

void CollectWordsForm::on_dictCBox_currentIndexChanged(int index)
{
    Dictionary *d = ZKanji::dictionary(ZKanji::dictionaryPosition(index));
    if (dict == d)
        return;

    if (dict != nullptr)
        disconnect(dict, &Dictionary::dictionaryReset, this, &CollectWordsForm::dictionaryReset);
    dict = d;
    if (dict != nullptr)
        connect(dict, &Dictionary::dictionaryReset, this, &CollectWordsForm::dictionaryReset);

    if (wmodel != nullptr)
        wmodel->deleteLater();

    ui->kanjiGrid->setDictionary(dict);
    ui->dictWidget->setDictionary(dict);

    wmodel = new CollectedWordListModel(this);
    wmodel->setWordList(dict, std::vector<int>());
    ui->dictWidget->setModel(wmodel);

    ui->addButton->setEnabled(false);
}

void CollectWordsForm::dictionaryReset()
{
    if (wmodel != nullptr)
        wmodel->deleteLater();

    wmodel = new CollectedWordListModel(this);
    wmodel->setWordList(dict, std::vector<int>());
    ui->dictWidget->setModel(wmodel);

    ui->addButton->setEnabled(false);
}

void CollectWordsForm::dictionaryToBeRemoved(int /*index*/, int orderindex, Dictionary *d)
{
    if (dict == d)
        ui->dictCBox->setCurrentIndex(orderindex == 0 ? 1 : 0);
}

void CollectWordsForm::dictionaryReplaced(Dictionary *old, Dictionary * /*newdict*/, int /*index*/)
{
    if (dict == old)
        close();
}


//-------------------------------------------------------------


void collectKanjiWords(Dictionary *dict, const std::vector<ushort> &kanjis)
{
    CollectWordsForm *f = new CollectWordsForm((QWidget*)gUI->activeMainForm());
    f->exec(dict, kanjis);
}


//-------------------------------------------------------------
