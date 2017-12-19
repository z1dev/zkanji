/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include <QSystemTrayIcon>
#include <QFileDialog>
#include <QStylePainter>

#include <cmath>

#include "settingsform.h"
#include "ui_settingsform.h"
#include "zevents.h"
#include "settings.h"
#include "fontsettings.h"
#include "generalsettings.h"
#include "printsettings.h"
#include "dialogsettings.h"
#include "popupsettings.h"
#include "recognizersettings.h"
#include "colorsettings.h"
#include "dictionarysettings.h"
#include "groupsettings.h"
#include "shortcutsettings.h"
#include "kanjisettings.h"
#include "studysettings.h"
#include "datasettings.h"
#include "globalui.h"
#include "kanji.h"
#include "ranges.h"
#include "sites.h"
#include "worddeck.h"
#include "zkanjimain.h"
#include "zui.h"
#include "romajizer.h"


//-------------------------------------------------------------


fontPreviewWidget::fontPreviewWidget(QWidget *parent) : base(parent), sizeheight(-1)
{

}

void fontPreviewWidget::setFonts(const QString &kf, const QString &df, const QString &nf, FontStyle nfs, LineSize siz)
{
    kanafont = kf;
    deffont = df;
    notesfont = nf;
    notestyle = nfs;
    sizes = siz;
    
    update();
}

QSize fontPreviewWidget::minimumSizeHint() const
{
    return sizeHint();
}

extern const double kanjiRowSize;
extern const double defRowSize;
extern const double notesRowSize;

QSize fontPreviewWidget::sizeHint() const
{
    int l, t, r, b;
    getContentsMargins(&l, &t, &r, &b);

    if (sizeheight != -1)
        return QSize(l + r + 1, t + b + sizeheight);

    // When measuring the size hint, the size is always the maximum so the widget doesn't grow
    // or shrink when the line size changes.
    int linesiz = 24 + 1;

    QFont kf = QApplication::font();
    kf.setPixelSize(Settings::scaled(linesiz * kanjiRowSize));
    QFontMetrics kfm(kf);
    QFont mf = QApplication::font();
    mf.setPixelSize(Settings::scaled(linesiz * defRowSize));
    QFontMetrics mfm(mf);

    sizeheight = kfm.height() + mfm.height();
    return QSize(l + r + 1, t + b + sizeheight);
}

void fontPreviewWidget::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    QStylePainter p(this);

    int l, t, rr, b;
    getContentsMargins(&l, &t, &rr, &b);
    QRect r = rect();
    r.adjust(l, t, -rr, -b);
    p.fillRect(r, Settings::textColor(hasFocus(), ColorSettings::Bg));

    r.adjust(4, 0, -4, 0);
    p.setClipRect(r);

    p.setPen(Settings::textColor(hasFocus(), ColorSettings::Text));

    QString str = toKana(QString("hiraganaKATAKANA"), true) + QChar(0x611f) + QChar(0x3058) + QChar(0x5e79) + QChar(0x4e8b) + QChar(0x6f22) + QChar(0x5b57) + QChar(0x76e3) + QChar(0x4e8b) + QChar(0x5b8c) + QChar(0x6cbb);

    // Size the text would take up in a dictionary listing.
    int linesiz = (sizes == FontSettings::Medium ? 19 : sizes == FontSettings::Small ? 17 : 24) - 1;
    // Available space for the kanji line.
    int rowsiz = r.height() * (kanjiRowSize / (kanjiRowSize + defRowSize));

    QFont kf{ kanafont, 9 };
    kf.setPixelSize(Settings::scaled(linesiz * kanjiRowSize));
    QFont mf{ deffont, 9 };
    mf.setPixelSize(Settings::scaled(linesiz * defRowSize));
    QFont nf{ notesfont, 7, notestyle == FontSettings::Bold || notestyle == FontSettings::BoldItalic ? QFont::Bold : -1, notestyle == FontSettings::Italic || notestyle == FontSettings::BoldItalic };
    nf.setPixelSize(Settings::scaled(linesiz * notesRowSize));
    QFontMetrics nfm(nf);

    p.setFont(kf);
    drawTextBaseline(&p, r.left(), r.top() + rowsiz * 0.8, false, r, str);

    r.setTop(r.top() + rowsiz);
    rowsiz = r.height();

    p.setFont(nf);
    str = tr("notes");
    drawTextBaseline(&p, r.left(), r.top() + rowsiz * 0.8, false, r, str);

    r.setLeft(r.left() + nfm.width(str));
    p.setFont(mf);
    str = tr("The lazy dog jumps over the quick brown fox.", "preview text in font settings");
    drawTextBaseline(&p, r.left() + 4, r.top() + rowsiz * 0.8, false, r, str);
}


//-------------------------------------------------------------


KanjiRefListModel::KanjiRefListModel(QObject *parent) : base(parent)
{
    reset();
}

KanjiRefListModel::~KanjiRefListModel()
{

}

void KanjiRefListModel::reset()
{
    memcpy(showref, Settings::kanji.showref, sizeof(bool) * ZKanji::kanjiReferenceCount());
    memcpy(reforder, Settings::kanji.reforder, sizeof(int) * ZKanji::kanjiReferenceCount());
}

void KanjiRefListModel::apply()
{
    memcpy(Settings::kanji.showref, showref, sizeof(bool) * ZKanji::kanjiReferenceCount());
    memcpy(Settings::kanji.reforder, reforder, sizeof(int) * ZKanji::kanjiReferenceCount());
}

void KanjiRefListModel::moveUp(int currpos, const std::vector<int> &indexes)
{
    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);

    int rix = _rangeOfPos(ranges, currpos);
    if (rix != -1)
        currpos = std::max(0, ranges[rix]->first - 1);
    else
        currpos = std::max(0, currpos);
    _moveRanges(ranges, currpos, reforder
#ifdef _DEBUG
        , ZKanji::kanjiReferenceCount()
#endif
        );

    signalRowsMoved(ranges, currpos);
}

void KanjiRefListModel::moveDown(int currpos, const std::vector<int> &indexes)
{
    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);

    int rix = _rangeOfPos(ranges, currpos);
    if (rix != -1)
        currpos = std::min(ZKanji::kanjiReferenceCount(), ranges[rix]->last + 2);
    else
        currpos = std::min(ZKanji::kanjiReferenceCount(), currpos + 1);
    _moveRanges(ranges, currpos, reforder
#ifdef _DEBUG
        , ZKanji::kanjiReferenceCount()
#endif
        );

    signalRowsMoved(ranges, currpos);
}


int KanjiRefListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ZKanji::kanjiReferenceCount();
}

QVariant KanjiRefListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole && role != Qt::CheckStateRole)
        return QVariant();

    if (role == Qt::CheckStateRole)
        return showref[reforder[index.row()]] ? Qt::Checked : Qt::Unchecked;

    return ZKanji::kanjiReferenceTitle(reforder[index.row()], true);
}

bool KanjiRefListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::CheckStateRole)
        return false;
    if (showref[reforder[index.row()]] == (value.toInt() == Qt::Checked))
        return false;
    showref[reforder[index.row()]] = value.toInt() == Qt::Checked;
    return true;
}

Qt::ItemFlags KanjiRefListModel::flags(const QModelIndex &index) const
{
    return base::flags(index) | Qt::ItemIsUserCheckable;
}

Qt::DropActions KanjiRefListModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions KanjiRefListModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (samesource)
        return Qt::MoveAction;
    return 0;
}

bool KanjiRefListModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row == -1 || column == -1 || action != Qt::MoveAction)
        return false;
    return true;
}

bool KanjiRefListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    QByteArray arr = data->data("zkanji/refrows");
    int *rows = (int*)arr.constData();

    smartvector<Range> ranges;
    _rangeFromIndexes(rows, arr.size() / sizeof(int), ranges);

    _moveRanges(ranges, row, reforder
#ifdef _DEBUG
        , ZKanji::kanjiReferenceCount()
#endif
        );

    signalRowsMoved(ranges, row);

    return arr.size() != 0;
}

QMimeData* KanjiRefListModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *data = new QMimeData();

    QByteArray arr;
    arr.resize(sizeof(int) * indexes.size());
    int *d = (int*)arr.data();
    for (int ix = 0, siz = indexes.size(); ix != siz; ++ix)
    {
        *d = indexes.at(ix).row();
        ++d;
    }

    data->setData(QStringLiteral("zkanji/refrows"), arr);

    return data;
}



//-------------------------------------------------------------


SettingsForm::SettingsForm(QWidget *parent) : base(parent), ui(new Ui::SettingsForm), ignoresitechange(false), sitesele(-1), sitesels(-1)
{
    ui->setupUi(this);

    scaleWidget(this);

    if (!QSystemTrayIcon::isSystemTrayAvailable())
        ui->trayBox->setEnabled(false);

    setAttribute(Qt::WA_DontShowOnScreen);
    show();
    for (int ix = ui->pagesStack->count() - 1; ix != -1; --ix)
        ui->pagesStack->setCurrentIndex(ix);
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    setAttribute(Qt::WA_DeleteOnClose);

    fixWrapLabelsHeight(this, -1/*200*/);
    adjustSize();

    ui->pagesTree->expandAll();
    QWidget *showpage = ui->interfacePage;
    if (!Settings::dialog.lastsettingspage.isEmpty())
    {
        QWidget *w = ui->pagesStack->findChild<QWidget*>(Settings::dialog.lastsettingspage, Qt::FindDirectChildrenOnly);
        if (w != nullptr)
            showpage = w;
    }
    ui->pagesTree->setCurrentItem(pageTreeItem(ui->pagesStack->indexOf(showpage)));
    ui->pagesStack->setCurrentWidget(showpage);

    //QStyleOptionViewItem gridopt;
    //gridopt.initFrom(ui->sitesTable);
    //ui->colGridCBox->setDefaultColor(static_cast<QRgb>(qApp->style()->styleHint(QStyle::SH_Table_GridLineColor, &gridopt, ui->sitesTable)));
    ui->colGridCBox->setDefaultColor(Settings::defUiColor(ColorSettings::Grid));

    ui->colBgCBox->setDefaultColor(qApp->palette().color(QPalette::Active, QPalette::Base));
    ui->colTextCBox->setDefaultColor(qApp->palette().color(QPalette::Active, QPalette::Text));
    ui->colSelBgCBox->setDefaultColor(qApp->palette().color(QPalette::Active, QPalette::Highlight));
    ui->colSelTextCBox->setDefaultColor(qApp->palette().color(QPalette::Active, QPalette::HighlightedText));
    ui->coliBgCBox->setDefaultColor(qApp->palette().color(QPalette::Inactive, QPalette::Base));
    ui->coliTextCBox->setDefaultColor(qApp->palette().color(QPalette::Inactive, QPalette::Text));
    ui->coliSelBgCBox->setDefaultColor(qApp->palette().color(QPalette::Inactive, QPalette::Highlight));
    ui->coliSelTextCBox->setDefaultColor(qApp->palette().color(QPalette::Inactive, QPalette::HighlightedText));

    ui->colStudyCorrectCBox->setDefaultColor(Settings::defUiColor(ColorSettings::StudyCorrect));
    ui->colStudyWrongCBox->setDefaultColor(Settings::defUiColor(ColorSettings::StudyWrong));
    ui->colStudyNewCBox->setDefaultColor(Settings::defUiColor(ColorSettings::StudyNew));

    ui->colInfCBox->setDefaultColor(Settings::defUiColor(ColorSettings::Inf));
    ui->colAttribCBox->setDefaultColor(Settings::defUiColor(ColorSettings::Attrib));
    ui->colTypeCBox->setDefaultColor(Settings::defUiColor(ColorSettings::Types));
    ui->colUsageCBox->setDefaultColor(Settings::defUiColor(ColorSettings::Notes));
    ui->colFieldCBox->setDefaultColor(Settings::defUiColor(ColorSettings::Fields));
    ui->colDialectCBox->setDefaultColor(Settings::defUiColor(ColorSettings::Dialects));
    ui->colKanaCBox->setDefaultColor(Settings::defUiColor(ColorSettings::KanaOnly));
    ui->colKanjiExCBox->setDefaultColor(Settings::defUiColor(ColorSettings::KanjiExBg));
    ui->colKanjiTestPosCBox->setDefaultColor(Settings::defUiColor(ColorSettings::KanjiTestPos));

    ui->colN5CBox->setDefaultColor(Settings::defUiColor(ColorSettings::N5));
    ui->colN4CBox->setDefaultColor(Settings::defUiColor(ColorSettings::N4));
    ui->colN3CBox->setDefaultColor(Settings::defUiColor(ColorSettings::N3));
    ui->colN2CBox->setDefaultColor(Settings::defUiColor(ColorSettings::N2));
    ui->colN1CBox->setDefaultColor(Settings::defUiColor(ColorSettings::N1));

    ui->colNoWordsCBox->setDefaultColor(Settings::defUiColor(ColorSettings::KanjiNoWords));
    ui->colUnsortedCBox->setDefaultColor(Settings::defUiColor(ColorSettings::KanjiUnsorted));

    ui->colOkuCBox->setDefaultColor(Settings::defUiColor(ColorSettings::Oku));

    ui->bgKataCBox->setDefaultColor(Settings::defUiColor(ColorSettings::KataBg));
    ui->bgHiraCBox->setDefaultColor(Settings::defUiColor(ColorSettings::HiraBg));
    ui->bgSimilarCBox->setDefaultColor(Settings::defUiColor(ColorSettings::SimilarBg));
    ui->textSimilarCBox->setDefaultColor(Settings::defUiColor(ColorSettings::SimilarText));
    ui->bgPartsCBox->setDefaultColor(Settings::defUiColor(ColorSettings::PartsBg));
    ui->bgPartOfCBox->setDefaultColor(Settings::defUiColor(ColorSettings::PartOfBg));

    ui->colStat1CBox->setDefaultColor(Settings::defUiColor(ColorSettings::Stat1));
    ui->colStat2CBox->setDefaultColor(Settings::defUiColor(ColorSettings::Stat2));
    ui->colStat3CBox->setDefaultColor(Settings::defUiColor(ColorSettings::Stat3));

    QLocale validatorlocale;
    validatorlocale.setNumberOptions(QLocale::OmitGroupSeparator);
    QIntValidator *validator = new QIntValidator(1, 9999, this);
    validator->setLocale(validatorlocale);
    ui->historyLimitEdit->setValidator(validator);

    ui->tooltipTimeoutEdit->setValidator(validator);

    ui->studyLimitEdit->setValidator(validator);
    ui->suspendItemsEdit->setValidator(validator);
    ui->suspendTimeEdit->setValidator(validator);

    ui->backupEdit->setValidator(validator);

    validator = new QIntValidator(0, 9999, this);
    ui->studyIncludeEdit->setValidator(validator);

    validator = new QIntValidator(1, 60, this);
    ui->studyWaitEdit->setValidator(validator);
    ui->autoSaveEdit->setValidator(validator);

    validator = new QIntValidator(1, 100, this);
    ui->backupDaysEdit->setValidator(validator);

    validator = new QIntValidator(2, 100, this);
    ui->historyTimeoutEdit->setValidator(validator);

    ui->kanjiRef1CBox->addItem(tr("None"));
    ui->kanjiRef2CBox->addItem(tr("None"));
    ui->kanjiRef3CBox->addItem(tr("None"));
    ui->kanjiRef4CBox->addItem(tr("None"));
    for (int ix = 1, siz = ZKanji::kanjiReferenceCount(); ix != siz; ++ix)
    {
        ui->kanjiRef1CBox->addItem(ZKanji::kanjiReferenceTitle(ix));
        ui->kanjiRef2CBox->addItem(ZKanji::kanjiReferenceTitle(ix));
        ui->kanjiRef3CBox->addItem(ZKanji::kanjiReferenceTitle(ix));
        ui->kanjiRef4CBox->addItem(ZKanji::kanjiReferenceTitle(ix));
    }

    ui->refTable->setModel(new KanjiRefListModel(this));
    ui->refTable->setSelectionType(ListSelectionType::Extended);
    ui->refTable->setAcceptDrops(true);
    ui->refTable->setDragEnabled(true);
    connect(ui->refTable, &ZListBox::rowSelectionChanged, this, &SettingsForm::refSelChanged);
    connect(ui->refTable, &ZListBox::currentRowChanged, this, &SettingsForm::refSelChanged);

    ui->sitesTable->setModel(new SitesListModel(ui->sitesTable));
    ui->sitesTable->setItemDelegate(new SitesItemDelegate(ui->sitesTable));
    connect(ui->sitesTable, &ZListBox::currentRowChanged, this, &SettingsForm::sitesSelChanged);
    ui->sitesTable->setDragEnabled(true);
    ui->sitesTable->setAcceptDrops(true);
    ui->sitesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    reset();

    ui->kanjiPreview->installEventFilter(this);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &SettingsForm::okClicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &SettingsForm::reset);
    connect(ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsForm::applyClicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, this, &SettingsForm::close);
    connect(ui->kanjiAliasBox, &QCheckBox::toggled, ui->kanjiPreview, (void (QWidget::*)())&QWidget::update);
    connect(ui->kanjiFontCBox, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, ui->kanjiPreview, (void (QWidget::*)())&QWidget::update);

    connect(ui->dictKanaFontCBox, &QComboBox::currentTextChanged, this, &SettingsForm::fontCBoxChanged);
    connect(ui->mainFontCBox, &QComboBox::currentTextChanged, this, &SettingsForm::fontCBoxChanged);
    connect(ui->dictInfoFontCBox, &QComboBox::currentTextChanged, this, &SettingsForm::fontCBoxChanged);
    connect(ui->dictInfoStyleCBox, &QComboBox::currentTextChanged, this, &SettingsForm::fontCBoxChanged);

    connect(ui->dictSizeCBox, &QComboBox::currentTextChanged, this, &SettingsForm::fontSizeCBoxChanged);
    connect(ui->popupSizeCBox, &QComboBox::currentTextChanged, this, &SettingsForm::popupSizeCBoxChanged);

    connect(ui->printDictFontsBox, &QCheckBox::toggled, this, &SettingsForm::printBoxToggled);
    connect(ui->printKanaFontCBox, &QComboBox::currentTextChanged, this, &SettingsForm::printCBoxChanged);
    connect(ui->printDefFontCBox, &QComboBox::currentTextChanged, this, &SettingsForm::printCBoxChanged);
    connect(ui->printInfoFontCBox, &QComboBox::currentTextChanged, this, &SettingsForm::printCBoxChanged);
    connect(ui->printInfoStyleCBox, &QComboBox::currentTextChanged, this, &SettingsForm::printCBoxChanged);
}

SettingsForm::~SettingsForm()
{
    delete ui;
}

void SettingsForm::show(SettingsPage page)
{
    switch (page)
    {
    case SettingsPage::Printing:
        ui->pagesTree->setCurrentItem(pageTreeItem(ui->pagesStack->indexOf(ui->printPage)));
    }
    base::show();
}

void SettingsForm::reset()
{
    ui->dateFormatCBox->setCurrentIndex((int)Settings::general.dateformat);

    ui->savePosBox->setChecked(Settings::general.savewinstates);
    ui->saveToolPosBox->setChecked(Settings::general.savetoolstates);
    ui->startupStateCBox->setCurrentIndex((int)Settings::general.startstate);
    if (QSystemTrayIcon::isSystemTrayAvailable())
        ui->trayBox->setChecked(Settings::general.minimizetotray);
    on_savePosBox_toggled();

    ui->recogSaveBox->setChecked(Settings::recognizer.savesize);
    ui->recogPositionCBox->setCurrentIndex(Settings::recognizer.saveposition ? 1 : 0);

    ui->hideSuspendedBox->setChecked(Settings::group.hidesuspended);

    ui->autoSizeBox->setChecked((int)Settings::dictionary.autosize);
    ui->inflectionCBox->setCurrentIndex((int)Settings::dictionary.inflection);
    ui->browseOrderCBox->setCurrentIndex((int)Settings::dictionary.browseorder);
    ui->wordGroupBox->setChecked(Settings::dictionary.showingroup);
    ui->jlptBox->setChecked(Settings::dictionary.showjlpt);
    ui->jlptPosCBox->setCurrentIndex((int)Settings::dictionary.jlptcolumn);
    on_jlptBox_toggled();

    ui->popupWidenBox->setChecked(Settings::popup.widescreen);
    ui->popupHideCBox->setCurrentIndex(Settings::popup.autohide ? 1 : 0);
    ui->popupActivationCBox->setCurrentIndex((int)Settings::popup.activation);
    ui->popupTransparencySlider->setValue(Settings::popup.transparency);

    ui->historyLimitEdit->setText(QString::number(Settings::dictionary.historylimit));
    ui->historyTimerBox->setChecked(Settings::dictionary.historytimer);
    ui->historyTimeoutEdit->setText(QString::number(Settings::dictionary.historytimeout));

    ui->fromEnableBox->setChecked(Settings::shortcuts.fromenable);
    ui->fromModCBox->setCurrentIndex((int)Settings::shortcuts.frommodifier);
    ui->fromShiftBox->setChecked(Settings::shortcuts.fromshift);
    ui->fromKeyCBox->setCurrentText(Settings::shortcuts.fromkey);
    ui->toEnableBox->setChecked(Settings::shortcuts.toenable);
    ui->toModCBox->setCurrentIndex((int)Settings::shortcuts.tomodifier);
    ui->toShiftBox->setChecked(Settings::shortcuts.toshift);
    ui->toKeyCBox->setCurrentText(Settings::shortcuts.tokey);
    ui->kanjiEnableBox->setChecked(Settings::shortcuts.kanjienable);
    ui->kanjiModCBox->setCurrentIndex((int)Settings::shortcuts.kanjimodifier);
    ui->kanjiShiftBox->setChecked(Settings::shortcuts.kanjishift);
    ui->kanjiKeyCBox->setCurrentText(Settings::shortcuts.kanjikey);
    ui->kanjiHideCBox->setCurrentIndex(Settings::popup.kanjiautohide ? 1 : 0);

    ui->saveKanjiBox->setChecked(Settings::kanji.savefilters);
    ui->kanjiResetBox->setChecked(Settings::kanji.resetpopupfilters);

    ui->kanjiRef1CBox->setCurrentIndex(Settings::kanji.mainref1);
    ui->kanjiRef2CBox->setCurrentIndex(Settings::kanji.mainref2);
    ui->kanjiRef3CBox->setCurrentIndex(Settings::kanji.mainref3);
    ui->kanjiRef4CBox->setCurrentIndex(Settings::kanji.mainref4);

    ui->okuColorBox->setChecked(Settings::colors.coloroku);
    ui->colOkuCBox->setCurrentColor(Settings::colors.okucolor);
    on_okuColorBox_toggled();

    ui->bgKataCBox->setCurrentColor(Settings::colors.katabg);
    ui->bgHiraCBox->setCurrentColor(Settings::colors.hirabg);
    ui->bgSimilarCBox->setCurrentColor(Settings::colors.similarbg);
    ui->textSimilarCBox->setCurrentColor(Settings::colors.similartext);
    ui->bgPartsCBox->setCurrentColor(Settings::colors.partsbg);
    ui->bgPartOfCBox->setCurrentColor(Settings::colors.partofbg);

    ui->colStat1CBox->setCurrentColor(Settings::colors.stat1);
    ui->colStat2CBox->setCurrentColor(Settings::colors.stat2);
    ui->colStat3CBox->setCurrentColor(Settings::colors.stat3);


    ((KanjiRefListModel*)ui->refTable->model())->reset();

    ui->kanjiPartsBox->setChecked(Settings::kanji.listparts);
    ui->tooltipBox->setChecked(Settings::kanji.tooltip);
    ui->tooltipHideBox->setChecked(Settings::kanji.hidetooltip);
    ui->tooltipTimeoutEdit->setText(QString::number(Settings::kanji.tooltipdelay));

    ui->autoSaveBox->setChecked(Settings::data.autosave);
    ui->autoSaveEdit->setText(QString::number(Settings::data.interval));
    ui->backupBox->setChecked(Settings::data.backup);
    ui->backupEdit->setText(QString::number(Settings::data.backupcnt));
    ui->backupDaysEdit->setText(QString::number(Settings::data.backupskip));
    ui->backupFolderEdit->setText(QDir::toNativeSeparators(Settings::data.location));

    ((SitesListModel*)ui->sitesTable->model())->reset();
    ui->sitesTable->setCurrentRow(0);

    ui->scaleSlider->setValue((Settings::general.savedscale - 100) / 25);

    ui->dictKanaFontCBox->setCurrentIndex(ui->dictKanaFontCBox->findText(Settings::fonts.kana));
    ui->mainFontCBox->setCurrentIndex(ui->mainFontCBox->findText(Settings::fonts.main));
    ui->dictInfoFontCBox->setCurrentIndex(ui->dictInfoFontCBox->findText(Settings::fonts.info));
    ui->dictInfoStyleCBox->setCurrentIndex((int)Settings::fonts.infostyle);
    //ui->extraFontCBox->setCurrentIndex(ui->extraFontCBox->findText(Settings::fonts.extra));
    //ui->extraStyleCBox->setCurrentIndex((int)Settings::fonts.extrastyle);
    ui->dictSizeCBox->setCurrentIndex((int)Settings::fonts.mainsize);
    ui->popupSizeCBox->setCurrentIndex((int)Settings::fonts.popsize);

    ui->fontPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), (fontPreviewWidget::LineSize)ui->dictSizeCBox->currentIndex());
    ui->popupPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), (fontPreviewWidget::LineSize)ui->popupSizeCBox->currentIndex());

    ui->kanjiFontCBox->setCurrentIndex(ui->kanjiFontCBox->findText(Settings::fonts.kanji));
    ui->kanjiAliasBox->setChecked(Settings::fonts.nokanjialias);
    ui->kanjiSizeCBox->setCurrentText(QString::number(Settings::fonts.kanjifontsize));
    on_kanjiSizeCBox_currentIndexChanged(ui->kanjiSizeCBox->currentIndex());

    ui->colGridCBox->setCurrentColor(Settings::colors.grid);
    ui->colBgCBox->setCurrentColor(Settings::colors.bg);
    ui->colTextCBox->setCurrentColor(Settings::colors.text);
    ui->colSelBgCBox->setCurrentColor(Settings::colors.selbg);
    ui->colSelTextCBox->setCurrentColor(Settings::colors.seltext);
    ui->coliBgCBox->setCurrentColor(Settings::colors.bgi);
    ui->coliTextCBox->setCurrentColor(Settings::colors.texti);
    ui->coliSelBgCBox->setCurrentColor(Settings::colors.selbgi);
    ui->coliSelTextCBox->setCurrentColor(Settings::colors.seltexti);

    ui->colStudyCorrectCBox->setCurrentColor(Settings::colors.studycorrect);
    ui->colStudyWrongCBox->setCurrentColor(Settings::colors.studywrong);
    ui->colStudyNewCBox->setCurrentColor(Settings::colors.studynew);

    ui->colInfCBox->setCurrentColor(Settings::colors.inf);
    ui->colAttribCBox->setCurrentColor(Settings::colors.attrib);
    ui->colTypeCBox->setCurrentColor(Settings::colors.types);
    ui->colUsageCBox->setCurrentColor(Settings::colors.notes);
    ui->colFieldCBox->setCurrentColor(Settings::colors.fields);
    ui->colDialectCBox->setCurrentColor(Settings::colors.dialect);
    ui->colKanaCBox->setCurrentColor(Settings::colors.kanaonly);
    ui->colKanjiExCBox->setCurrentColor(Settings::colors.kanjiexbg);
    ui->colKanjiTestPosCBox->setCurrentColor(Settings::colors.kanjitestpos);

    ui->colN5CBox->setCurrentColor(Settings::colors.n5);
    ui->colN4CBox->setCurrentColor(Settings::colors.n4);
    ui->colN3CBox->setCurrentColor(Settings::colors.n3);
    ui->colN2CBox->setCurrentColor(Settings::colors.n2);
    ui->colN1CBox->setCurrentColor(Settings::colors.n1);

    ui->colNoWordsCBox->setCurrentColor(Settings::colors.kanjinowords);
    ui->colUnsortedCBox->setCurrentColor(Settings::colors.kanjiunsorted);

    ui->studyIncludeCBox->setCurrentIndex((int)Settings::study.includedays);
    ui->studyIncludeEdit->setText(QString::number(Settings::study.includecount));
    on_studyIncludeEdit_textChanged(ui->studyIncludeEdit->text());
    ui->studyUniqueBox->setChecked(Settings::study.onlyunique);
    ui->studyLimitBox->setChecked(Settings::study.limitnew);
    ui->studyLimitEdit->setText(QString::number(Settings::study.limitcount));
    ui->studyStartEdit->setText(QString::number(Settings::study.starthour));
    ui->kanjiKanaRadio->setChecked(Settings::study.kanjihint == WordParts::Kana);
    ui->kanjiDefRadio->setChecked(Settings::study.kanjihint == WordParts::Definition);
    ui->kanaKanjiRadio->setChecked(Settings::study.kanahint == WordParts::Kanji);
    ui->kanaDefRadio->setChecked(Settings::study.kanahint == WordParts::Definition);
    ui->defKanjiRadio->setChecked(Settings::study.defhint == WordParts::Kanji);
    ui->defKanaRadio->setChecked(Settings::study.defhint == WordParts::Kana);
    ui->studyReviewBox->setChecked(Settings::study.review);
    ui->studyEstimateBox->setChecked(Settings::study.showestimate);
    ui->studyHideKanaBox->setChecked(Settings::study.hidekanjikana);
    ui->typeKanjiBox->setChecked(Settings::study.writekanji);
    ui->typeKanaBox->setChecked(Settings::study.writekana);
    ui->typeDefBox->setChecked(Settings::study.writedef);
    ui->studyMatchKanaBox->setChecked(Settings::study.matchkana);
    ui->studyWaitEdit->setText(QString::number(Settings::study.delaywrong));
    ui->studySuspendBox->setChecked(Settings::study.idlesuspend);
    ui->studySuspendCBox->setCurrentIndex((int)Settings::study.idletime);
    ui->suspendItemsBox->setChecked(Settings::study.itemsuspend);
    ui->suspendItemsEdit->setText(QString::number(Settings::study.itemsuspendnum));
    ui->suspendTimeBox->setChecked(Settings::study.timesuspend);
    ui->suspendTimeEdit->setText(QString::number(Settings::study.timesuspendnum));
    ui->studyReadingTypeCBox->setCurrentIndex((int)Settings::study.readings);
    ui->studyReadingWordCBox->setCurrentIndex((int)Settings::study.readingsfrom);

    ui->printSeparateBox->setChecked(Settings::print.doublepage);
    ui->printFlowBox->setChecked(!Settings::print.separated);

    ui->printHeightCBox->setCurrentIndex((int)Settings::print.linesize);

    ui->printColumnsCBox->setCurrentIndex(std::min(3, std::max(1, Settings::print.columns)) - 1);
    ui->printKanjiBox->setChecked(Settings::print.usekanji);
    ui->printTypeBox->setChecked(Settings::print.showtype);
    ui->printUserDefBox->setChecked(Settings::print.userdefs);
    ui->printOrderCBox->setCurrentIndex(Settings::print.reversed ? 1 : 0);
    ui->printFuriganaCBox->setCurrentIndex((int)Settings::print.readings);
    ui->printBackgroundBox->setChecked(Settings::print.background);
    ui->printSeparatorBox->setChecked(Settings::print.separator);
    ui->printPageBox->setChecked(Settings::print.pagenum);

    ui->printDictFontsBox->setChecked(Settings::print.dictfonts);
    ui->printKanaFontCBox->setCurrentIndex(ui->printKanaFontCBox->findText(Settings::fonts.printkana));
    ui->printDefFontCBox->setCurrentIndex(ui->printDefFontCBox->findText(Settings::fonts.printdefinition));
    ui->printInfoFontCBox->setCurrentIndex(ui->printInfoFontCBox->findText(Settings::fonts.printinfo));
    ui->printInfoStyleCBox->setCurrentIndex((int)Settings::fonts.printinfostyle);

    if (ui->printDictFontsBox->isChecked())
        ui->printPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), fontPreviewWidget::Large);
    else
        ui->printPreview->setFonts(ui->printKanaFontCBox->currentText(), ui->printDefFontCBox->currentText(), ui->printInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->printInfoStyleCBox->currentIndex(), fontPreviewWidget::Large);
}

void SettingsForm::on_pagesTree_currentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *prev)
{
    int index = 0;

    QStack<QTreeWidgetItem*> stack;

    while (item->parent() != nullptr)
    {
        int childindex = item->parent()->indexOfChild(item);
        for (int ix = 0; ix != childindex; ++ix)
            stack.push_back(item->parent()->child(ix));
        index += childindex + 1;
        item = item->parent();
    }
    int topix = ui->pagesTree->indexOfTopLevelItem(item);
    index += topix;

    for (int ix = 0; ix != topix; ++ix)
        stack.push(ui->pagesTree->topLevelItem(ix));

    while (!stack.isEmpty())
    {
        QTreeWidgetItem *top = stack.pop();
        index += top->childCount();
        for (int ix = 0, siz = top->childCount(); ix != siz; ++ix)
            stack.push(top->child(ix));
    }

    ui->pagesStack->setCurrentIndex(index);
    Settings::dialog.lastsettingspage = ui->pagesStack->currentWidget()->objectName();
}

void SettingsForm::okClicked()
{
    applyClicked();
    close();
}

void SettingsForm::applyClicked()
{
    if (ui->dateFormatCBox->currentIndex() == -1)
        ui->dateFormatCBox->setCurrentIndex(0);
    Settings::general.dateformat = (GeneralSettings::DateFormat)ui->dateFormatCBox->currentIndex();
    Settings::general.savewinstates = ui->savePosBox->isChecked();
    Settings::general.savetoolstates = ui->saveToolPosBox->isChecked();
    Settings::general.startstate = (GeneralSettings::StartState)ui->startupStateCBox->currentIndex();
    if (QSystemTrayIcon::isSystemTrayAvailable())
        Settings::general.minimizetotray = ui->trayBox->isChecked();

    Settings::recognizer.savesize = ui->recogSaveBox->isChecked();
    Settings::recognizer.saveposition = ui->recogPositionCBox->currentIndex() == 1;

    Settings::group.hidesuspended = ui->hideSuspendedBox->isChecked();

    Settings::dictionary.autosize = ui->autoSizeBox->isChecked();
    Settings::dictionary.inflection = (DictionarySettings::InflectionShow)ui->inflectionCBox->currentIndex();
    Settings::dictionary.browseorder = (BrowseOrder)ui->browseOrderCBox->currentIndex();
    Settings::dictionary.showingroup = ui->wordGroupBox->isChecked();
    Settings::dictionary.showjlpt = ui->jlptBox->isChecked();
    Settings::dictionary.jlptcolumn = (DictionarySettings::JlptColumn)ui->jlptPosCBox->currentIndex();

    Settings::popup.widescreen = ui->popupWidenBox->isChecked();
    Settings::popup.autohide = ui->popupHideCBox->currentIndex() == 1;
    Settings::popup.activation = (PopupSettings::Activation)ui->popupActivationCBox->currentIndex();
    Settings::popup.transparency = ui->popupTransparencySlider->value();

    bool ok;
    int val = ui->historyLimitEdit->text().toInt(&ok);
    if (ok)
        Settings::dictionary.historylimit = val;
    Settings::dictionary.historytimer = ui->historyTimerBox->isChecked();
    val = ui->historyTimeoutEdit->text().toInt(&ok);
    if (ok)
        Settings::dictionary.historytimeout = val;

    Settings::shortcuts.fromenable = ui->fromEnableBox->isChecked();
    Settings::shortcuts.frommodifier = (ShortcutSettings::Modifier)ui->fromModCBox->currentIndex();
    Settings::shortcuts.fromshift = ui->fromShiftBox->isChecked();
    Settings::shortcuts.fromkey = ui->fromKeyCBox->currentText().at(0);

    Settings::shortcuts.toenable = ui->toEnableBox->isChecked();
    Settings::shortcuts.tomodifier = (ShortcutSettings::Modifier)ui->toModCBox->currentIndex();
    Settings::shortcuts.toshift = ui->toShiftBox->isChecked();
    Settings::shortcuts.tokey = ui->toKeyCBox->currentText().at(0);

    Settings::shortcuts.kanjienable = ui->kanjiEnableBox->isChecked();
    Settings::shortcuts.kanjimodifier = (ShortcutSettings::Modifier)ui->kanjiModCBox->currentIndex();
    Settings::shortcuts.kanjishift = ui->kanjiShiftBox->isChecked();
    Settings::shortcuts.kanjikey = ui->kanjiKeyCBox->currentText().at(0);
    Settings::popup.kanjiautohide = ui->kanjiHideCBox->currentIndex() == 1;

    Settings::kanji.savefilters = ui->saveKanjiBox->isChecked();
    Settings::kanji.resetpopupfilters = ui->kanjiResetBox->isChecked();

    if (ui->kanjiRef1CBox->currentIndex() != -1)
        Settings::kanji.mainref1 = ui->kanjiRef1CBox->currentIndex();
    if (ui->kanjiRef2CBox->currentIndex() != -1)
        Settings::kanji.mainref2 = ui->kanjiRef2CBox->currentIndex();
    if (ui->kanjiRef3CBox->currentIndex() != -1)
        Settings::kanji.mainref3 = ui->kanjiRef3CBox->currentIndex();
    if (ui->kanjiRef4CBox->currentIndex() != -1)
        Settings::kanji.mainref4 = ui->kanjiRef4CBox->currentIndex();
    Settings::colors.coloroku = ui->okuColorBox->isChecked();
    Settings::colors.okucolor = ui->colOkuCBox->currentColor();

    Settings::colors.katabg = ui->bgKataCBox->currentColor();
    Settings::colors.hirabg = ui->bgHiraCBox->currentColor();
    Settings::colors.similarbg = ui->bgSimilarCBox->currentColor();
    Settings::colors.similartext = ui->textSimilarCBox->currentColor();
    Settings::colors.partsbg = ui->bgPartsCBox->currentColor();
    Settings::colors.partofbg = ui->bgPartOfCBox->currentColor();

    Settings::colors.stat1 = ui->colStat1CBox->currentColor();
    Settings::colors.stat2 = ui->colStat2CBox->currentColor();
    Settings::colors.stat3 = ui->colStat3CBox->currentColor();

    ((KanjiRefListModel*)ui->refTable->model())->apply();


    Settings::kanji.listparts = ui->kanjiPartsBox->isChecked();
    Settings::kanji.tooltip = ui->tooltipBox->isChecked();
    Settings::kanji.hidetooltip = ui->tooltipHideBox->isChecked();
    val = ui->tooltipTimeoutEdit->text().toInt(&ok);
    if (ok)
        Settings::kanji.tooltipdelay = val;

    Settings::data.autosave = ui->autoSaveBox->isChecked();
    val = ui->autoSaveEdit->text().toInt(&ok);
    if (ok && val >= 1 && val <= 60)
        Settings::data.interval = val;

    Settings::data.backup = ui->backupBox->isChecked();
    val = ui->backupEdit->text().toInt(&ok);
    if (ok && val >= 1 && val <= 9999)
        Settings::data.backupcnt = val;
    val = ui->backupDaysEdit->text().toInt(&ok);
    if (ok && val >= 1 && val <= 100)
        Settings::data.backupskip = val;
    Settings::data.location = QDir::fromNativeSeparators(ui->backupFolderEdit->text());
    if (!Settings::data.location.isEmpty() && *std::prev(Settings::data.location.end()) == '/')
        Settings::data.location.resize(Settings::data.location.size() - 1);
    if (Settings::data.location == ZKanji::userFolder() + "/data")
        Settings::data.location.clear();

    ((SitesListModel*)ui->sitesTable->model())->apply();

    val = ui->scaleSlider->value();
    Settings::general.savedscale = val * 25 + 100;

    if (ui->dictKanaFontCBox->currentIndex() == -1)
        ui->dictKanaFontCBox->setCurrentIndex(ui->dictKanaFontCBox->findText(Settings::fonts.kana));
    Settings::fonts.kana = ui->dictKanaFontCBox->currentText();
    if (ui->mainFontCBox->currentIndex() == -1)
        ui->mainFontCBox->setCurrentIndex(ui->mainFontCBox->findText(Settings::fonts.main));
    Settings::fonts.main = ui->mainFontCBox->currentText();
    if (ui->dictInfoFontCBox->currentIndex() == -1)
        ui->dictInfoFontCBox->setCurrentIndex(ui->dictInfoFontCBox->findText(Settings::fonts.info));
    Settings::fonts.info = ui->dictInfoFontCBox->currentText();
    if (ui->dictInfoStyleCBox->currentIndex() == -1)
        ui->dictInfoStyleCBox->setCurrentIndex((int)Settings::fonts.infostyle);
    Settings::fonts.infostyle = (FontSettings::FontStyle)ui->dictInfoStyleCBox->currentIndex();
    //if (ui->extraFontCBox->currentIndex() == -1)
    //    ui->extraFontCBox->setCurrentIndex(ui->extraFontCBox->findText(Settings::fonts.extra));
    //Settings::fonts.extra = ui->extraFontCBox->currentText();
    //if (ui->extraStyleCBox->currentIndex() == -1)
    //    ui->extraStyleCBox->setCurrentIndex((int)Settings::fonts.extrastyle);
    //Settings::fonts.extrastyle = (FontSettings::FontStyle)ui->extraStyleCBox->currentIndex();

    if (ui->dictSizeCBox->currentIndex() == -1)
        ui->dictSizeCBox->setCurrentIndex((int)Settings::fonts.mainsize);
    Settings::fonts.mainsize = (FontSettings::LineSize)ui->dictSizeCBox->currentIndex();
    if (ui->popupSizeCBox->currentIndex() == -1)
        ui->popupSizeCBox->setCurrentIndex((int)Settings::fonts.popsize);
    Settings::fonts.popsize = (FontSettings::LineSize)ui->popupSizeCBox->currentIndex();

    if (ui->kanjiFontCBox->currentIndex() == -1)
        ui->kanjiFontCBox->setCurrentIndex(ui->kanjiFontCBox->findText(Settings::fonts.kanji));
    Settings::fonts.kanji = ui->kanjiFontCBox->currentText();
    Settings::fonts.nokanjialias = ui->kanjiAliasBox->isChecked();
    if (ui->kanjiSizeCBox->currentIndex() == -1)
        ui->kanjiSizeCBox->setCurrentText(QString::number(Settings::fonts.kanjifontsize));
    Settings::fonts.kanjifontsize = ui->kanjiSizeCBox->currentText().toInt();


    Settings::colors.grid = ui->colGridCBox->currentColor();
    Settings::colors.bg = ui->colBgCBox->currentColor();
    Settings::colors.text = ui->colTextCBox->currentColor();
    Settings::colors.selbg = ui->colSelBgCBox->currentColor();
    Settings::colors.seltext = ui->colSelTextCBox->currentColor();
    Settings::colors.bgi = ui->coliBgCBox->currentColor();
    Settings::colors.texti = ui->coliTextCBox->currentColor();
    Settings::colors.selbgi = ui->coliSelBgCBox->currentColor();
    Settings::colors.seltexti = ui->coliSelTextCBox->currentColor();

    Settings::colors.studycorrect = ui->colStudyCorrectCBox->currentColor();
    Settings::colors.studywrong = ui->colStudyWrongCBox->currentColor();
    Settings::colors.studynew = ui->colStudyNewCBox->currentColor();

    Settings::colors.inf = ui->colInfCBox->currentColor();
    Settings::colors.attrib = ui->colAttribCBox->currentColor();
    Settings::colors.types = ui->colTypeCBox->currentColor();
    Settings::colors.notes = ui->colUsageCBox->currentColor();
    Settings::colors.fields = ui->colFieldCBox->currentColor();
    Settings::colors.dialect = ui->colDialectCBox->currentColor();
    Settings::colors.kanaonly = ui->colKanaCBox->currentColor();
    Settings::colors.kanjiexbg = ui->colKanjiExCBox->currentColor();
    Settings::colors.kanjitestpos = ui->colKanjiTestPosCBox->currentColor();

    Settings::colors.n5 = ui->colN5CBox->currentColor();
    Settings::colors.n4 = ui->colN4CBox->currentColor();
    Settings::colors.n3 = ui->colN3CBox->currentColor();
    Settings::colors.n2 = ui->colN2CBox->currentColor();
    Settings::colors.n1 = ui->colN1CBox->currentColor();

    Settings::colors.kanjinowords = ui->colNoWordsCBox->currentColor();
    Settings::colors.kanjiunsorted = ui->colUnsortedCBox->currentColor();

    Settings::study.includedays = (StudySettings::IncludeDays)ui->studyIncludeCBox->currentIndex();
    val = ui->studyIncludeEdit->text().toInt(&ok);
    if (ok && val >= 0 && val <= 9999)
        Settings::study.includecount = val;
    Settings::study.onlyunique = ui->studyUniqueBox->isChecked();
    Settings::study.limitnew = ui->studyLimitBox->isChecked();
    val = ui->studyLimitEdit->text().toInt(&ok);
    if (ok && val >= 1 && val <= 9999)
        Settings::study.limitcount = val;
    val = ui->studyStartEdit->text().toInt(&ok);
    if (ok && val >= 1 && val <= 12)
        Settings::study.starthour = val;
    Settings::study.kanjihint = ui->kanjiKanaRadio->isChecked() ? WordParts::Kana : WordParts::Definition;
    Settings::study.kanahint = ui->kanaKanjiRadio->isChecked() ? WordParts::Kanji : WordParts::Definition;
    Settings::study.defhint = ui->defKanjiRadio->isChecked() ? WordParts::Kanji : WordParts::Kana;
    Settings::study.review = ui->studyReviewBox->isChecked();
    Settings::study.showestimate = ui->studyEstimateBox->isChecked();
    Settings::study.hidekanjikana = ui->studyHideKanaBox->isChecked();
    Settings::study.writekanji = ui->typeKanjiBox->isChecked();
    Settings::study.writekana = ui->typeKanaBox->isChecked();
    Settings::study.writedef = ui->typeDefBox->isChecked();
    Settings::study.matchkana = ui->studyMatchKanaBox->isChecked();

    val = ui->studyWaitEdit->text().toInt(&ok);
    if (ok && val >= 1 && val <= 60)
        Settings::study.delaywrong = val;
    Settings::study.idlesuspend = ui->studySuspendBox->isChecked();
    Settings::study.idletime = (StudySettings::SuspendWait)ui->studySuspendCBox->currentIndex();
    Settings::study.itemsuspend = ui->suspendItemsBox->isChecked();
    val = ui->suspendItemsEdit->text().toInt(&ok);
    if (ok && val >= 1 && val <= 9999)
        Settings::study.itemsuspendnum = val;
    Settings::study.timesuspend = ui->suspendTimeBox->isChecked();
    val = ui->suspendTimeEdit->text().toInt(&ok);
    if (ok && val >= 1 && val <= 9999)
        Settings::study.timesuspendnum = val;
    Settings::study.readings = (StudySettings::ReadingType)ui->studyReadingTypeCBox->currentIndex();
    Settings::study.readingsfrom = (StudySettings::ReadingFrom)ui->studyReadingWordCBox->currentIndex();

    Settings::print.doublepage = ui->printSeparateBox->isChecked();
    Settings::print.separated = !ui->printFlowBox->isChecked();

    if (ui->printHeightCBox->currentIndex() == -1)
        ui->printHeightCBox->setCurrentIndex(2);
    Settings::print.linesize = (PrintSettings::LineSizes)ui->printHeightCBox->currentIndex();

    if (ui->printColumnsCBox->currentIndex() == -1)
        ui->printColumnsCBox->setCurrentIndex(0);
    Settings::print.columns = ui->printColumnsCBox->currentIndex() + 1;
    Settings::print.usekanji = ui->printKanjiBox->isChecked();
    Settings::print.showtype = ui->printTypeBox->isChecked();
    Settings::print.userdefs = ui->printUserDefBox->isChecked();
    if (ui->printOrderCBox->currentIndex() == -1)
        ui->printOrderCBox->setCurrentIndex(0);
    Settings::print.reversed = ui->printOrderCBox->currentIndex() != 0;
    if (ui->printFuriganaCBox->currentIndex() == -1)
        ui->printFuriganaCBox->setCurrentIndex(0);
    Settings::print.readings = (PrintSettings::Readings)ui->printFuriganaCBox->currentIndex();
    Settings::print.background = ui->printBackgroundBox->isChecked();
    Settings::print.separator = ui->printSeparatorBox->isChecked();
    Settings::print.pagenum = ui->printPageBox->isChecked();

    Settings::print.dictfonts = ui->printDictFontsBox->isChecked();
    if (ui->printKanaFontCBox->currentIndex() == -1)
        ui->printKanaFontCBox->setCurrentIndex(ui->printKanaFontCBox->findText(Settings::fonts.printkana));
    Settings::fonts.printkana = ui->printKanaFontCBox->currentText();
    if (ui->printDefFontCBox->currentIndex() == -1)
        ui->printDefFontCBox->setCurrentIndex(ui->printDefFontCBox->findText(Settings::fonts.printdefinition));
    Settings::fonts.printdefinition = ui->printDefFontCBox->currentText();
    if (ui->printInfoFontCBox->currentIndex() == -1)
        ui->printInfoFontCBox->setCurrentIndex(ui->printInfoFontCBox->findText(Settings::fonts.printinfo));
    Settings::fonts.printinfo = ui->printInfoFontCBox->currentText();
    if (ui->printInfoStyleCBox->currentIndex() == -1)
        ui->printInfoStyleCBox->setCurrentIndex((int)Settings::fonts.printinfostyle);
    Settings::fonts.printinfostyle = (FontSettings::FontStyle)ui->printInfoStyleCBox->currentIndex();

    gUI->applySettings();

    Settings::saveIniFile();
}

void SettingsForm::on_backupFolderButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select folder for user backups."), ui->backupFolderEdit->text());
    if (path.isEmpty())
        return;
    if (path.at(path.size() - 1) == '/')
        path.resize(path.size() - 1);
    ui->backupFolderEdit->setText(path);
}

void SettingsForm::on_savePosBox_toggled()
{
    ui->saveToolPosBox->setEnabled(ui->savePosBox->isChecked());
    ui->startupStateCBox->setEnabled(ui->savePosBox->isChecked());
    ui->startupStateLabel->setEnabled(ui->savePosBox->isChecked());
}

void SettingsForm::on_jlptBox_toggled()
{
    ui->jlptPosCBox->setEnabled(ui->jlptBox->isChecked());
    ui->jlptPosLabel->setEnabled(ui->jlptBox->isChecked());
}

void SettingsForm::on_okuColorBox_toggled()
{
    ui->okuColorLabel->setEnabled(ui->okuColorBox->isChecked());
    ui->colOkuCBox->setEnabled(ui->okuColorBox->isChecked());
}

void SettingsForm::refSelChanged()
{
    std::vector<int> rows;
    ui->refTable->selectedRows(rows);

    // The whole selection is a single block.
    bool single = true;
    for (int ix = 1, sel = rows.size(); single && ix != sel; ++ix)
    {
        if (rows[ix] != rows[ix - 1] + 1)
            single = false;
    }

    ui->refUpButton->setEnabled(!rows.empty() && (!single || rows[0] != 0 || ui->refTable->currentRow() > rows.back() + 1));
    ui->refDownButton->setEnabled(!rows.empty() && (!single || rows.back() != ZKanji::kanjiReferenceCount() || ui->refTable->currentRow() < rows.front() - 1));
}

void SettingsForm::on_refUpButton_clicked()
{
    std::vector<int> sel;
    ui->refTable->selectedRows(sel);

    ((KanjiRefListModel*)ui->refTable->model())->moveUp(ui->refTable->currentRow(), sel);
    ui->refTable->changeCurrentRow(ui->refTable->selectedRow(0));
    ui->refTable->scrollToRow(ui->refTable->currentRow());
}

void SettingsForm::on_refDownButton_clicked()
{
    std::vector<int> sel;
    ui->refTable->selectedRows(sel);

    ((KanjiRefListModel*)ui->refTable->model())->moveDown(ui->refTable->currentRow(), sel);
    ui->refTable->changeCurrentRow(ui->refTable->selectedRow(ui->refTable->selectionSize() - 1));
    ui->refTable->scrollToRow(ui->refTable->currentRow());
}

void SettingsForm::on_studyIncludeEdit_textChanged(const QString &text)
{
    bool ok;
    int val = text.toInt(&ok);
    if (ok)
        ui->studyIncludeCBox->setEnabled(val != 0);
}

void SettingsForm::on_siteTypeCBox_currentIndexChanged(int index)
{
    SitesListModel *m = (SitesListModel*)ui->sitesTable->model();

    ui->sitesTable->setCurrentRow(0);
}

void SettingsForm::sitesSelChanged(int curr, int prev)
{
    SitesListModel *m = (SitesListModel*)ui->sitesTable->model();
    if (curr < 0)
        curr = 0;

    ui->siteLockButton->setEnabled(curr < m->rowCount() - 1);
    ui->siteDelButton->setEnabled(curr < m->rowCount() - 1);

    if (ignoresitechange)
        return;

    if (curr == m->rowCount() - 1)
    {
        ignoresitechange = true;
        ui->siteNameEdit->setText(QString());
        ui->siteUrlEdit->setText(QString("http://"));
        ui->siteLockButton->setChecked(false);
        ignoresitechange = false;

        getSiteUrlSel(sitesele, sitesels);

        return;
    }

    const SiteItem &item = m->items(curr);
    ignoresitechange = true;
    ui->siteNameEdit->setText(item.name);
    ui->siteUrlEdit->setText(item.url);
    ui->siteUrlEdit->setCursorPosition(item.insertpos);
    ui->siteLockButton->setChecked(item.poslocked);
    ignoresitechange = false;

    getSiteUrlSel(sitesele, sitesels);
}

void SettingsForm::on_siteNameEdit_textEdited(const QString &text)
{
    if (ignoresitechange)
        return;

    ignoresitechange = true;
    SitesListModel *m = (SitesListModel*)ui->sitesTable->model();
    int row = ui->sitesTable->currentRow();

    m->setItemName(row, text);
    ui->sitesTable->setCurrentRow(row);
    ignoresitechange = false;
}

void SettingsForm::on_siteUrlEdit_textEdited(const QString &text)
{
    if (ignoresitechange)
        return;
    SitesListModel *m = (SitesListModel*)ui->sitesTable->model();
    int row = ui->sitesTable->currentRow();

    // Changing the url must also update the data insert position, but only for existing site
    // items with a locked position.
    if (row != m->rowCount() - 1 && m->itemInsertPosLocked(row))
    {
        const SiteItem &item = m->items(row);

        // Special case when deleting with a key combination that removes more characters even
        // without a selection.
        if (sitesele == sitesels && text.size() < item.url.size())
        {
            // Text from the right side has been deleted. Other branch: text from the left.
            if (item.url.left(sitesele) == text.left(sitesele))
                sitesele += (item.url.size() - text.size());
            else if (item.url.mid(sitesele) == text.mid(sitesele - (item.url.size() - text.size())))
                sitesele -= (item.url.size() - text.size());
        }

        if (item.url.left(std::min(sitesele, sitesels)) != text.left(std::min(sitesele, sitesels)) ||
            item.url.mid(std::max(sitesele, sitesels)) != text.mid(std::max(sitesele, sitesels) + (text.size() - item.url.size())))
        {
            // The url text field changed even outside the past selection. There's no way to
            // know how to modify the data input position. The position should be unlocked.
            m->lockItemInsertPos(row, false);
            ignoresitechange = true;
            ui->siteLockButton->setChecked(false);
            ignoresitechange = false;
        }
        else if (item.insertpos > std::min(sitesele, sitesels))
        {
            m->lockItemInsertPos(row, false);
            if (item.insertpos >= std::max(sitesele, sitesels))
                m->setItemInsertPos(row, item.insertpos + (text.size() - item.url.size()));
            else
                m->setItemInsertPos(row, std::min(sitesele, sitesels) + std::abs(sitesele - sitesels) + (text.size() - item.url.size()));
            m->lockItemInsertPos(row, true);
        }
    }

    ignoresitechange = true;
    m->setItemUrl(row, text);
    ui->sitesTable->setCurrentRow(row);

    getSiteUrlSel(sitesele, sitesels);
    ignoresitechange = false;
}

void SettingsForm::on_siteUrlEdit_cursorPositionChanged(int oldp, int newp)
{
    if (ignoresitechange)
        return;

    SitesListModel *m = (SitesListModel*)ui->sitesTable->model();
    int row = ui->sitesTable->currentRow();

    m->setItemInsertPos(row, newp);
    ui->sitesTable->setCurrentRow(row);

    getSiteUrlSel(sitesele, sitesels);
}

void SettingsForm::on_siteUrlEdit_selectionChanged()
{
    getSiteUrlSel(sitesele, sitesels);
}

void SettingsForm::on_siteLockButton_toggled(bool checked)
{
    ui->siteLockLabel->setText(!checked ? tr("The data to look up will be inserted at the last cursor position.") : tr("The insert position won't change when the cursor is moved."));

    if (ignoresitechange)
        return;

    ignoresitechange = true;
    SitesListModel *m = (SitesListModel*)ui->sitesTable->model();
    int row = ui->sitesTable->currentRow();

    if (row < m->rowCount() - 1)
        m->lockItemInsertPos(row, checked);

    ui->siteUrlEdit->setFocus();

    if (!checked)
        m->setItemInsertPos(row, ui->siteUrlEdit->cursorPosition());

    ignoresitechange = false;
}

void SettingsForm::on_siteDelButton_clicked()
{
    SitesListModel *m = (SitesListModel*)ui->sitesTable->model();
    int row = ui->sitesTable->currentRow();

    m->deleteSite(row);

    ui->sitesTable->setCurrentRow(std::max(0, std::min(row, m->rowCount() - 2)));
}

void SettingsForm::on_kanjiSizeCBox_currentIndexChanged(int index)
{
    int fontsize;
    if (ui->kanjiSizeCBox->currentIndex() == -1)
        fontsize = Settings::fonts.kanjifontsize;
    else
        fontsize = ui->kanjiSizeCBox->currentText().toInt();

    fontsize = Settings::scaled(fontsize);
    int cellsize = std::ceil(fontsize / 0.7);
    int mleft, mtop, mright, mbottom;
    ui->kanjiPreview->getContentsMargins(&mleft, &mtop, &mright, &mbottom);
    ui->kanjiPreview->setFixedSize(cellsize + mleft + mright, cellsize + mtop + mbottom);
    ui->kanjiPreview->update();
}

void SettingsForm::fontCBoxChanged()
{
    ui->fontPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), (fontPreviewWidget::LineSize)ui->dictSizeCBox->currentIndex());
    ui->popupPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), (fontPreviewWidget::LineSize)ui->popupSizeCBox->currentIndex());
    if (ui->printDictFontsBox->isChecked())
        ui->printPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), fontPreviewWidget::Large);
}

void SettingsForm::fontSizeCBoxChanged()
{
    ui->fontPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), (fontPreviewWidget::LineSize)ui->dictSizeCBox->currentIndex());
}

void SettingsForm::popupSizeCBoxChanged()
{
    ui->popupPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), (fontPreviewWidget::LineSize)ui->popupSizeCBox->currentIndex());
}

void SettingsForm::printBoxToggled()
{
    if (ui->printDictFontsBox->isChecked())
        ui->printPreview->setFonts(ui->dictKanaFontCBox->currentText(), ui->mainFontCBox->currentText(), ui->dictInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->dictInfoStyleCBox->currentIndex(), fontPreviewWidget::Large);
    else
        ui->printPreview->setFonts(ui->printKanaFontCBox->currentText(), ui->printDefFontCBox->currentText(), ui->printInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->printInfoStyleCBox->currentIndex(), fontPreviewWidget::Large);
}

void SettingsForm::printCBoxChanged()
{
    if (ui->printDictFontsBox->isChecked())
        return;

    ui->printPreview->setFonts(ui->printKanaFontCBox->currentText(), ui->printDefFontCBox->currentText(), ui->printInfoFontCBox->currentText(), (fontPreviewWidget::FontStyle)ui->printInfoStyleCBox->currentIndex(), fontPreviewWidget::Large);
}

bool SettingsForm::eventFilter(QObject *o, QEvent *e)
{
    if (o == ui->kanjiPreview && e->type() == QEvent::Paint)
    {
        QPaintEvent *pe = (QPaintEvent*)e;
        QRect r = ui->kanjiPreview->rect();

        int mleft, mtop, mright, mbottom;
        ui->kanjiPreview->getContentsMargins(&mleft, &mtop, &mright, &mbottom);
        r.adjust(mleft, mtop, -mright, -mbottom);
        QStylePainter p(ui->kanjiPreview);

        p.fillRect(r, Settings::textColor(ui->kanjiPreview->hasFocus(), ColorSettings::TextColorTypes::Bg));

        QFont kfont = { ui->kanjiFontCBox->currentText(), Settings::scaled(ui->kanjiSizeCBox->currentText().toInt()) };
        if (ui->kanjiAliasBox->isChecked())
        {
            QFont::StyleStrategy ss = kfont.styleStrategy();
            ss = QFont::StyleStrategy(ss | QFont::NoSubpixelAntialias);
            kfont.setStyleStrategy(ss);
        }

        p.setFont(kfont);

        drawTextBaseline(&p, r.left(), r.top() + r.height() * 0.86, true, QRect(r.left(), r.top(), r.width() - 1, r.height() - 1), QChar(0x6f22)) ;

        return false;
    }
    return base::eventFilter(o, e);
}

void SettingsForm::getSiteUrlSel(int &ss, int &se)
{
    se = ui->siteUrlEdit->selectionStart();
    ss = ui->siteUrlEdit->cursorPosition();
    if (se == -1)
        se = ss;
}

QTreeWidgetItem* SettingsForm::pageTreeItem(int pageindex) const
{
    QTreeWidgetItem *pos = nullptr;// ui->pagesTree->topLevelItem(0);
    int posindex = 0;
    int poscnt = ui->pagesTree->topLevelItemCount();

    while (pageindex > 0 || posindex == poscnt)
    {
        if (posindex == poscnt)
        {
            posindex = pos->parent() == nullptr ? ui->pagesTree->indexOfTopLevelItem(pos)  + 1: pos->parent()->indexOfChild(pos) + 1;
            pos = pos->parent();
            poscnt = pos == nullptr ? ui->pagesTree->topLevelItemCount() : pos->childCount();
            continue;
        }

        pos = pos == nullptr ? ui->pagesTree->topLevelItem(posindex) : pos->child(posindex);
        poscnt = pos->childCount();
        posindex = 0;
        --pageindex;
    }

    return pos == nullptr ? ui->pagesTree->topLevelItem(posindex) : pos->child(posindex);
}


//-------------------------------------------------------------

