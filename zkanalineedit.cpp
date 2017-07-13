#include <QValidator>
#include <QDesktopServices>
#include <QStringBuilder>
#include "zkanalineedit.h"
#include "zui.h"
#include "fontsettings.h"
#include "globalui.h"
#include "zkanjimain.h"
#include "kanji.h"
#include "sites.h"

//-------------------------------------------------------------


ZKanaLineEdit::ZKanaLineEdit(QWidget *parent) : base(parent), dict(nullptr)
{
    connect(gUI, &GlobalUI::settingsChanged, this, &ZKanaLineEdit::settingsChanged);
    settingsChanged();
}

ZKanaLineEdit::~ZKanaLineEdit()
{

}

Dictionary* ZKanaLineEdit::dictionary() const
{
    return dict;
}

void ZKanaLineEdit::setDictionary(Dictionary *d)
{
    if (dict == d)
        return;
    dict = d;
    emit dictionaryChanged();
}

void ZKanaLineEdit::settingsChanged()
{
    QFont f = Settings::kanaFont();
    f.setPointSizeF(font().pointSizeF());
    setFont(f);
}

QMenu* ZKanaLineEdit::createContextMenu(QContextMenuEvent *e)
{
    QMenu *menu = base::createContextMenu(e);

    QString str = selectedText();
    if (str.isEmpty())
    {
        QString txt;

        int cpos = cursorPositionAt(e->pos());
        if (cursorPosition() != cpos)
            setCursorPosition(cpos);
        QRect r = cursorRect();
        int l, t, r2, b, l2, t2, r3, b2;
        getTextMargins(&l, &t, &r2, &b);
        getContentsMargins(&l2, &t2, &r3, &b2);
        r.translate(-l - l2, -t - t2);

        if (r.center().x() >= e->pos().x())
            --cpos;

        if (cpos >= 0)
            txt = text();
        if (cpos < 0 || cpos >= txt.size())
            return menu;

        str = txt.at(cpos);
    }

    if (str.isEmpty())
        return menu;

    if (ZKanji::lookup_sites.size() != 0)
    {
        menu->insertSeparator(menu->actions().at(0));
        QMenu *sub = menu->insertMenu(menu->actions().at(0), new QMenu(tr("Online lookup"), menu))->menu();
        for (int ix = 0, siz = ZKanji::lookup_sites.size(); ix != siz; ++ix)
        {
            QAction *a = sub->addAction(ZKanji::lookup_sites[ix].name);
            connect(a, &QAction::triggered, [str, ix]() {
                SiteItem &site = ZKanji::lookup_sites[ix];
                QString url = site.url.left(site.insertpos) % str % site.url.mid(site.insertpos);
                QDesktopServices::openUrl(QUrl(url));
            });
        }
    }

    if (dict != nullptr && str.size() == 1 && KANJI(str.at(0).unicode()))
    {
        menu->insertSeparator(menu->actions().at(0));
        QAction *a = new QAction(tr("Kanji information"), menu);
        int kindex = ZKanji::kanjiIndex(str.at(0));
        connect(a, &QAction::triggered, [this, kindex]() { gUI->showKanjiInfo(dict, kindex); });
        menu->insertAction(menu->actions().at(0), a);
    }

    return menu;
}

void ZKanaLineEdit::contextMenuEvent(QContextMenuEvent *e)
{
    // Prevents the context menu if the current input is half typed kana.
    if (!validInput())
    {
        //e->ignore();
        return;
    }

    base::contextMenuEvent(e);
}

void ZKanaLineEdit::keyPressEvent(QKeyEvent *e)
{
    // Prevents keyboard navigation, paste etc. if the current input is half typed kana.
    if (!validInput())
    {
        int key = e->key();
        // Check whether the input is not acceptable while typing in kana.
        if (key == Qt::Key_Escape)
        {
            fixInput();
            return;
        }

        // Prevent handling any key that would move the cursor or modify the text apart from
        // normal input.
        // TODO: (later) These keys are taken from qwidgetlinecontrol.cpp. If the Qt version
        // changes, it's possible that new keys might be added to this list.
        if (e == QKeySequence::Undo || e == QKeySequence::Redo || e == QKeySequence::SelectAll ||
            e == QKeySequence::Copy || e == QKeySequence::Paste || e == QKeySequence::Cut ||
            e == QKeySequence::DeleteEndOfLine || e == QKeySequence::MoveToNextChar ||
            e == QKeySequence::MoveToStartOfLine || e == QKeySequence::MoveToStartOfBlock ||
            e == QKeySequence::MoveToEndOfLine || e == QKeySequence::MoveToEndOfBlock ||
            e == QKeySequence::SelectStartOfLine || e == QKeySequence::SelectStartOfBlock ||
            e == QKeySequence::SelectEndOfLine || e == QKeySequence::SelectEndOfBlock ||
            e == QKeySequence::SelectNextChar || e == QKeySequence::MoveToPreviousChar ||
            e == QKeySequence::SelectPreviousChar || e == QKeySequence::MoveToNextWord ||
            e == QKeySequence::MoveToPreviousWord || e == QKeySequence::SelectNextWord ||
            e == QKeySequence::SelectPreviousWord || e == QKeySequence::Delete ||
            e == QKeySequence::DeleteEndOfWord || e == QKeySequence::DeleteStartOfWord ||
            e == QKeySequence::DeleteCompleteLine ||
            key == Qt::Key_Up || key == Qt::Key_Down ||
            ((e->modifiers() & Qt::ControlModifier) != 0 && key == Qt::Key_Backspace) ||
            key == Qt::Key_Back || key == Qt::Key_Direction_L || key == Qt::Key_Direction_R)
            return;
    }

    base::keyPressEvent(e);
}

void ZKanaLineEdit::mousePressEvent(QMouseEvent* e)
{
    if (!validInput())
        return;
    base::mousePressEvent(e);
}

void ZKanaLineEdit::mouseMoveEvent(QMouseEvent* e)
{
    if (!validInput())
        return;
    base::mouseMoveEvent(e);
}

void ZKanaLineEdit::mouseReleaseEvent(QMouseEvent* e)
{
    if (!validInput())
        return;
    base::mouseReleaseEvent(e);
}

void ZKanaLineEdit::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (!validInput())
        return;
    base::mouseDoubleClickEvent(e);
}

void ZKanaLineEdit::fixInput()
{
    const JapaneseValidator *v = dynamic_cast<const JapaneseValidator*>(validator());
    if (v == nullptr)
        return;
    int cpos = cursorPosition();
    QString t = text();
    bool changed = false;
    for (int ix = t.size() - 1; ix != -1; --ix)
        if (!v->validChar(t.at(ix).unicode()))
        {
            setSelection(ix, 1);
            insert(QStringLiteral(""));
            if (ix < cpos)
                --cpos;
            changed = true;
        }

    if (changed)
    {
        setSelection(cpos, 0);

        emitEditChange(true);
    }
}


//-------------------------------------------------------------

