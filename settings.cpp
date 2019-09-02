/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSettings>
#include <QFile>
#include <QFontDatabase>
#include <QDir>
#include <QByteArray>
#include <QStringBuilder>

#include <QScreen>

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
#include "languagesettings.h"
#include "zkanjimain.h"
#include "globalui.h"
#include "zkanjiform.h"
#include "zradicalgrid.h"
#include "zui.h"
#include "formstates.h"
#include "kanji.h"
#include "sites.h"
#include "groups.h"
#include "words.h"

#include "checked_cast.h"

namespace Settings
{
    // Default colors for the light user interface.
    const QColor studycorrectldef = QColor(30, 160, 70);
    const QColor studywrongldef = QColor(245, 0, 0);
    const QColor studynewldef = QColor(80, 140, 200);

    const QColor infldef = QColor(255, 255, 234);
    const QColor attribldef = Qt::black;
    const QColor typesldef = QColor(64, 128, 48);
    const QColor notesldef = QColor(144, 64, 32);
    const QColor fieldsldef = QColor(48, 64, 128);
    const QColor dialectldef = QColor(128, 48, 64);
    const QColor kanaonlyldef = QColor(144, 176, 160);
    const QColor sentencewordldef = QColor(255, 0, 0);
    const QColor kanjiexbgldef = QColor(210, 255, 210);
    const QColor kanjitestposldef = QColor(255, 0, 0);

    const QColor okucolorldef = QColor(48, 162, 255);

    const QColor n5ldef = QColor(255, 34, 34);
    const QColor n4ldef = QColor(50, 68, 220);
    const QColor n3ldef = QColor(7, 160, 153);
    const QColor n2ldef = QColor(145, 77, 200);
    const QColor n1ldef = QColor(Qt::black);

    const QColor nowordsldef = QColor(192, 192, 192);
    const QColor unsortedldef = QColor(247, 247, 240);
    const QColor notranslationldef = QColor(255, 0, 0);

    const QColor katabgldef = QColor(157, 216, 243);
    const QColor hirabgldef = QColor(157, 243, 149);

    const QColor similartextldef = QColor(0, 145, 245);
    //const QColor similarbgldef = QColor(241, 247, 247);
    //const QColor partsbgldef = QColor(231, 247, 231);
    //const QColor partofbgldef = QColor(247, 239, 231);
    const QColor strokedotldef = QColor(255, 0, 0);

    const QColor stat1ldef = QColor(0, 162, 212);
    const QColor stat2ldef = QColor(120, 212, 40);
    const QColor stat3ldef = QColor(232, 160, 0);

    // Default colors for the dark user interface.
    const QColor studycorrectddef = QColor(30, 160, 70);
    const QColor studywrongddef = QColor(245, 0, 0);
    const QColor studynewddef = QColor(80, 140, 200);

    const QColor infddef = QColor(56, 56, 44);
    const QColor attribddef = QColor(175, 206, 175);
    const QColor typesddef = QColor(76, 218, 50);
    const QColor notesddef = QColor(235, 80, 40);
    const QColor fieldsddef = QColor(42, 160, 230);
    const QColor dialectddef = QColor(188, 30, 112);
    const QColor sentencewordddef = QColor(255, 0, 0);
    const QColor kanaonlyddef = QColor(144, 176, 160);
    const QColor kanjiexbgddef = QColor(70, 125, 90);
    const QColor kanjitestposddef = QColor(255, 0, 0);

    const QColor okucolorddef = QColor(48, 162, 255);

    const QColor n5ddef = QColor(255, 34, 34);
    const QColor n4ddef = QColor(75, 145, 255);
    const QColor n3ddef = QColor(27, 190, 183);
    const QColor n2ddef = QColor(175, 107, 230);
    const QColor n1ddef = QColor(248, 248, 255);

    const QColor nowordsddef = QColor(192, 192, 192);
    const QColor unsortedddef = QColor(66, 68, 81);
    const QColor notranslationddef = QColor(255, 0, 0);

    const QColor katabgddef = QColor(0, 100, 130);
    const QColor hirabgddef = QColor(50, 92, 12);

    const QColor similartextddef = QColor(40, 136, 205);
    //const QColor similarbgddef = QColor(241, 247, 247);
    //const QColor partsbgddef = QColor(231, 247, 231);
    //const QColor partofbgddef = QColor(247, 239, 231);
    const QColor strokedotddef = QColor(165, 80, 72);

    const QColor stat1ddef = QColor(0, 162, 212);
    const QColor stat2ddef = QColor(120, 212, 40);
    const QColor stat3ddef = QColor(232, 160, 0);

    QString language;

    FontSettings fonts;
    GeneralSettings general;
    GroupSettings group;
    PrintSettings print;
    DialogSettings dialog;
    PopupSettings popup;
    RecognizerSettings recognizer;
    ColorSettings colors;
    DictionarySettings dictionary;
    ShortcutSettings shortcuts;
    KanjiSettings kanji;
    StudySettings study;
    DataSettings data;

    qreal _scaleratio = 1;

    void saveIniFile()
    {
        QSettings ini(ZKanji::userFolder() + "/zkanji.ini", QSettings::IniFormat);

        // General settings

        ini.setValue("language", Settings::language);

        ini.setValue("dateformat", general.dateformat == GeneralSettings::YearMonthDay ? "YearMonthDay" : general.dateformat == GeneralSettings::MonthDayYear ? "MonthDayYear" : "DayMonthYear");
        ini.setValue("savewinpos", general.savewinpos);
        //ini.setValue("savetoolstates", general.savetoolstates);
        ini.setValue("startstate", general.startstate == GeneralSettings::SaveState ? "savestate" : general.startstate == GeneralSettings::AlwaysMinimize ? "minimize" : general.startstate == GeneralSettings::AlwaysMaximize ? "maximize" : "forget");
        ini.setValue("minimizebehavior", general.minimizebehavior == GeneralSettings::TrayOnClose ? "closetotray" : general.minimizebehavior == GeneralSettings::TrayOnMinimize ? "minimizetotray" : "default" );
        ini.setValue("scaling", clamp(general.savedscale, 100, 400));

        // Font settings

        ini.setValue("fonts/kanji", fonts.kanji);
        ini.setValue("fonts/kanjifontsize", fonts.kanjifontsize);

        ini.setValue("fonts/nokanjialias", fonts.nokanjialias);
        ini.setValue("fonts/kana", fonts.kana);
        ini.setValue("fonts/main", fonts.main);
        ini.setValue("fonts/notes", fonts.info);
        ini.setValue("fonts/notesstyle", fonts.infostyle == FontStyle::Bold ? "Bold" : fonts.infostyle == FontStyle::Italic ? "Italic" : fonts.infostyle == FontStyle::BoldItalic ? "BoldItalic" : "Normal");
        //ini.setValue("fonts/extra", fonts.extra);
        //ini.setValue("fonts/extrastyle", fonts.extrastyle == FontStyle::Bold ? "Bold" : fonts.extrastyle == FontStyle::Italic ? "Italic" : fonts.extrastyle == FontStyle::BoldItalic ? "BoldItalic" : "Normal");

        ini.setValue("fonts/printkana", fonts.printkana);
        ini.setValue("fonts/printdefinition", fonts.printdefinition);
        ini.setValue("fonts/printinfo", fonts.printinfo);
        ini.setValue("fonts/printinfostyle", fonts.printinfostyle == FontStyle::Bold ? "Bold" : fonts.printinfostyle == FontStyle::Italic ? "Italic" : fonts.printinfostyle == FontStyle::BoldItalic ? "BoldItalic" : "Normal");

        ini.setValue("fonts/mainsize", (int)fonts.mainsize);
        ini.setValue("fonts/popsize", (int)fonts.popsize);

        // Group settings

        ini.setValue("groups/hidesuspended", group.hidesuspended);

        // Print settings

        ini.setValue("print/usedictionaryfonts", print.dictfonts);
        ini.setValue("print/doublepage", print.doublepage);
        ini.setValue("print/separatecolumns", print.separated);
        ini.setValue("print/linesize", (int)print.linesize);
        ini.setValue("print/columns", print.columns);
        ini.setValue("print/usekanji", print.usekanji);
        ini.setValue("print/showtype", print.showtype);
        ini.setValue("print/userdefs", print.userdefs);
        ini.setValue("print/reversed", print.reversed);
        ini.setValue("print/readings", (int)print.readings);
        ini.setValue("print/background", print.background);
        ini.setValue("print/separatorline", print.separator);
        ini.setValue("print/pagenum", print.pagenum);
        ini.setValue("print/printername", print.printername);
        ini.setValue("print/portraitmode", print.portrait);
        ini.setValue("print/leftmargin", print.leftmargin);
        ini.setValue("print/topmargin", print.topmargin);
        ini.setValue("print/rightmargin", print.rightmargin);
        ini.setValue("print/bottommargin", print.bottommargin);
        ini.setValue("print/sizeid", print.pagesizeid);

        // Dialog settings

        ini.setValue("dialogs/settingspage", dialog.lastsettingspage);

        // Popup settings

        ini.setValue("popup/autohide", popup.autohide);
        ini.setValue("popup/widescreen", popup.widescreen);
        ini.setValue("popup/statusbar", popup.statusbar);
        //ini.setValue("popup/floating", popup.floating);
        //ini.setValue("popup/floatrect", popup.dictfloatrect);
        //ini.setValue("popup/size", popup.normalsize);
        ini.setValue("popup/autosearch", (int)popup.activation);
        ini.setValue("popup/transparency", popup.transparency);
        ini.setValue("popup/dictionary", popup.dict);
        //ini.setValue("popup/kanjirect", popup.kanjifloatrect);
        ini.setValue("popup/kanjiautohide", popup.kanjiautohide);
        ini.setValue("popup/kanjidictionary", popup.kanjidict);

        // Recognizer settings

        ini.setValue("recognizer/savesize", recognizer.savesize);
        ini.setValue("recognizer/savepos", recognizer.saveposition);
        //ini.setValue("recognizer/rect", recognizer.rect);

        // Color settings

        ini.setValue("colors/useinactive", colors.useinactive);
        ini.setValue("colors/bg", colors.bg);
        ini.setValue("colors/text", colors.text);
        ini.setValue("colors/selbg", colors.selbg);
        ini.setValue("colors/seltext", colors.seltext);
        ini.setValue("colors/bgi", colors.bgi);
        ini.setValue("colors/texti", colors.texti);
        ini.setValue("colors/selbgi", colors.selbgi);
        ini.setValue("colors/seltexti", colors.seltexti);
        ini.setValue("colors/scrollbg", colors.scrollbg);
        ini.setValue("colors/scrollh", colors.scrollh);
        ini.setValue("colors/scrollbgi", colors.scrollbgi);
        ini.setValue("colors/scrollhi", colors.scrollhi);
        ini.setValue("colors/grid", colors.grid);
        ini.setValue("colors/studycorrect", colors.studycorrect);
        ini.setValue("colors/studywrong", colors.studywrong);
        ini.setValue("colors/studynew", colors.studynew);
        ini.setValue("colors/inf", colors.inf);
        ini.setValue("colors/attrib", colors.attrib);
        ini.setValue("colors/types", colors.types);
        ini.setValue("colors/notes", colors.notes);
        ini.setValue("colors/fields", colors.fields);
        ini.setValue("colors/dialect", colors.dialect);
        ini.setValue("colors/kanaonly", colors.kanaonly);
        ini.setValue("colors/sentenceword", colors.sentenceword);
        ini.setValue("colors/kanjiexamplebg", colors.kanjiexbg);
        ini.setValue("colors/kanjitestpos", colors.kanjitestpos);
        ini.setValue("colors/n5", colors.n5);
        ini.setValue("colors/n4", colors.n4);
        ini.setValue("colors/n3", colors.n3);
        ini.setValue("colors/n2", colors.n2);
        ini.setValue("colors/n1", colors.n1);
        ini.setValue("colors/kanjinowords", colors.kanjinowords);
        ini.setValue("colors/kanjiunsorted", colors.kanjiunsorted);
        ini.setValue("colors/kanjinotranslation", colors.kanjinotranslation);
        ini.setValue("colors/coloroku", colors.coloroku);
        ini.setValue("colors/oku", colors.okucolor);
        ini.setValue("colors/katabg", colors.katabg);
        ini.setValue("colors/hirabg", colors.hirabg);
        ini.setValue("colors/similartext", colors.similartext);
        //ini.setValue("colors/similarbg", colors.similarbg);
        //ini.setValue("colors/partsbg", colors.partsbg);
        //ini.setValue("colors/partofbg", colors.partofbg);
        ini.setValue("colors/strokedot", colors.strokedot);
        ini.setValue("colors/stat1", colors.stat1);
        ini.setValue("colors/stat2", colors.stat2);
        ini.setValue("colors/stat3", colors.stat3);

        // Dictionary

        ini.setValue("dict/autosize", dictionary.autosize);
        ini.setValue("dict/inflection", (int)dictionary.inflection);
        ini.setValue("dict/resultorder", dictionary.resultorder == ResultOrder::Relevance ? "relevance" : dictionary.resultorder == ResultOrder::Frequency ? "frequency" : dictionary.resultorder == ResultOrder::JLPTfrom1 ? "JLPTfrom1" : "JLPTfrom5");
        ini.setValue("dict/browseorder", dictionary.browseorder == BrowseOrder::ABCDE ? "abcde" : "aiueo");
        ini.setValue("dict/showgroup", dictionary.showingroup);
        ini.setValue("dict/showjlpt", dictionary.showjlpt);
        ini.setValue("dict/jlptcol", (int)dictionary.jlptcolumn);
        ini.setValue("dict/historylimit", dictionary.historylimit);
        ini.setValue("dict/historytimer", dictionary.historytimer);
        ini.setValue("dict/historytimeout", dictionary.historytimeout);

        // Shortcut settings

        ini.setValue("shortcuts/from", shortcuts.fromenable);
        ini.setValue("shortcuts/frommod", shortcuts.frommodifier == ShortcutSettings::Alt ? "Alt" : shortcuts.frommodifier == ShortcutSettings::Control ? "Control" : "AltControl");
        ini.setValue("shortcuts/fromshift", shortcuts.fromshift);
        ini.setValue("shortcuts/fromkey", QString(shortcuts.fromkey));

        ini.setValue("shortcuts/to", shortcuts.toenable);
        ini.setValue("shortcuts/tomod", shortcuts.tomodifier == ShortcutSettings::Alt ? "Alt" : shortcuts.tomodifier == ShortcutSettings::Control ? "Control" : "AltControl");
        ini.setValue("shortcuts/toshift", shortcuts.toshift);
        ini.setValue("shortcuts/tokey", QString(shortcuts.tokey));

        ini.setValue("shortcuts/kanji", shortcuts.kanjienable);
        ini.setValue("shortcuts/kanjimod", shortcuts.kanjimodifier == ShortcutSettings::Alt ? "Alt" : shortcuts.kanjimodifier == ShortcutSettings::Control ? "Control" : "AltControl");
        ini.setValue("shortcuts/kanjishift", shortcuts.kanjishift);
        ini.setValue("shortcuts/kanjikey", QString(shortcuts.kanjikey));

        // Kanji settings

        ini.setValue("kanji/savefilters", kanji.savefilters);
        ini.setValue("kanji/popupreset", kanji.resetpopupfilters);
        ini.setValue("kanji/ref1", kanji.mainref1);
        ini.setValue("kanji/ref2", kanji.mainref2);
        ini.setValue("kanji/ref3", kanji.mainref3);
        ini.setValue("kanji/ref4", kanji.mainref4);
        ini.setValue("kanji/parts", kanji.listparts);
        ini.setValue("kanji/showpos", kanji.showpos == KanjiSettings::NearCursor ? "nearcursor" : kanji.showpos == KanjiSettings::RestoreLast ? "last" : "default");
        ini.setValue("kanji/tooltip", kanji.tooltip);
        ini.setValue("kanji/hidetooltip", kanji.hidetooltip);
        ini.setValue("kanji/tooltipdelay", kanji.tooltipdelay);

        for (int ix = 0, siz = tosigned(kanji.showref.size()); ix != siz; ++ix)
        {
            ini.setValue("kanji/showref" + QString::number(ix), kanji.showref[ix]);
            ini.setValue("kanji/reforder" + QString::number(ix), kanji.reforder[ix]);
        }

        // Study settings

        ini.setValue("study/starthour", study.starthour);
        ini.setValue("study/includefreq", (int)study.includedays);
        ini.setValue("study/includecnt", study.includecount);
        ini.setValue("study/onlyunique", study.onlyunique);
        ini.setValue("study/limitnew", study.limitnew);
        ini.setValue("study/limitcount", study.limitcount);

        ini.setValue("study/kanjihint", (int)study.kanjihint);
        ini.setValue("study/kanahint", (int)study.kanahint);
        ini.setValue("study/defhint", (int)study.defhint);

        ini.setValue("study/showestimate", study.showestimate);
        ini.setValue("study/hidekanjikana", study.hidekanjikana);
        ini.setValue("study/writekanji", study.writekanji);
        ini.setValue("study/writekana", study.writekana);
        ini.setValue("study/writedef", study.writedef);
        ini.setValue("study/delaywrong", study.delaywrong);
        ini.setValue("study/review", study.review);
        ini.setValue("study/idlesuspend", study.idlesuspend);
        ini.setValue("study/idletime", (int)study.idletime);
        ini.setValue("study/itemsuspend", study.itemsuspend);
        ini.setValue("study/itemsuspendnum", study.itemsuspendnum);
        ini.setValue("study/timesuspend", study.timesuspend);
        ini.setValue("study/timesuspendnum", study.timesuspendnum);

        ini.setValue("study/readings", (int)study.readings);
        ini.setValue("study/readingsfrom", (int)study.readingsfrom);

        // Data settings

        ini.setValue("data/autosave", data.autosave);
        ini.setValue("data/interval", data.interval);
        ini.setValue("data/backup", data.backup);
        ini.setValue("data/backupcount", data.backupcnt);
        ini.setValue("data/backupskip", data.backupskip);
        ini.setValue("data/location", data.location);

        // Dictionary order

        QStringList strs;
        for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
            strs << ZKanji::dictionary(ZKanji::dictionaryPosition(ix))->name();

        ini.setValue("dictionaryorder", strs.join(QChar('/')));
    }

    void saveStatesFile()
    {
        QString fname = ZKanji::userFolder() + "/states.xml";

        QFile f(fname);
        if (!f.open(QIODevice::WriteOnly))
        {
            QMessageBox::warning(gUI->activeMainForm(), "zkanji", qApp->translate("gUI", "Couldn't create or overwrite the state settings file. Please make sure you have permission to modify the file %1").arg(QDir::toNativeSeparators(fname)), QMessageBox::Ok);
            return;
        }

        QXmlStreamWriter writer(&f);
        writer.setAutoFormatting(true);

        writer.writeStartDocument();

        writer.writeStartElement("ZKanjiSettings");

        if (!radicalFiltersModel().empty() || !wordFiltersEmpty())
        {
            writer.writeStartElement("Filters");

            if (!radicalFiltersModel().empty())
            {
                writer.writeStartElement("RadicalFilters");
                saveXMLRadicalFilters(writer);
                writer.writeEndElement(); /* RadicalFilters */
            }

            if (!wordFiltersEmpty())
            {
                writer.writeStartElement("WordFilters");
                saveXMLWordFilters(writer);
                writer.writeEndElement(); /* WordFilters */
            }

            writer.writeEndElement(); /* Filters */
        }

        if (!ZKanji::lookup_sites.empty())
        {
            writer.writeStartElement("LookupSites");
            ZKanji::saveXMLLookupSites(writer);
            writer.writeEndElement(); /* LookupSites */
        }

        writer.writeStartElement("WindowStates");

        for (auto &state : FormStates::splitters)
        {
            if (!FormStates::emptyState(state.second))
            {
                writer.writeStartElement(state.first);
                FormStates::saveXMLSettings(state.second, writer);
                writer.writeEndElement();
            }
        }

        for (auto &state : FormStates::sizes)
        {
            if (state.second.isValid())
            {
                writer.writeStartElement(state.first);
                FormStates::saveXMLDialogSize(state.second, writer);
                writer.writeEndElement();
            }
        }

        for (auto &state : FormStates::maxsizes)
        {
            if (state.second.siz.isValid())
            {
                writer.writeStartElement(state.first);
                FormStates::saveXMLDialogMaximizedAndSize(state.second, writer);
                writer.writeEndElement();
            }
        }

        if (!FormStates::emptyState(FormStates::collectform))
        {
            writer.writeStartElement("CollectWords");
            FormStates::saveXMLSettings(FormStates::collectform, writer);
            writer.writeEndElement();
        }

        if (!FormStates::emptyState(FormStates::kanjiinfo))
        {
            // Differs from saving currently shown kanji information windows' states. This
            // setting will apply to newly opened kanji information windows.
            writer.writeStartElement("KanjiInformation");
            FormStates::saveXMLSettings(FormStates::kanjiinfo, writer);
            writer.writeEndElement();
        }

        if (!FormStates::emptyState(FormStates::wordstudylist))
        {
            writer.writeStartElement("WordStudyListForm");
            FormStates::saveXMLSettings(FormStates::wordstudylist, writer);
            writer.writeEndElement();
        }

        if (!FormStates::emptyState(FormStates::popupkanji))
        {
            writer.writeStartElement("PopupKanji");
            FormStates::saveXMLSettings(FormStates::popupkanji, writer);
            writer.writeEndElement();
        }

        if (!FormStates::emptyState(FormStates::popupdict))
        {
            writer.writeStartElement("PopupDictionary");
            FormStates::saveXMLSettings(FormStates::popupdict, writer);
            writer.writeEndElement();
        }

        writer.writeStartElement("KanaPractice");
        FormStates::saveXMLSettings(FormStates::kanapractice, writer);
        writer.writeEndElement();

        if (!FormStates::emptyState(FormStates::recognizer))
        {
            writer.writeStartElement("RecognizerForm");
            FormStates::saveXMLSettings(FormStates::recognizer, writer);
            writer.writeEndElement();
        }

        writer.writeEndElement(); /* WindowStates */

        writer.writeStartElement("Windows");

        writer.writeStartElement("MainWindow");
        gUI->mainForm()->saveXMLSettings(writer);
        writer.writeEndElement(); /* MainWindow */

        for (int ix = 1, siz = tosigned(gUI->formCount()); ix != siz; ++ix)
        {
            writer.writeStartElement("Window");
            gUI->forms(ix)->saveXMLSettings(writer);
            writer.writeEndElement(); /* Window */
        }

        //if (Settings::general.savetoolstates)
        //{
        //    // Differs from saving last used values in a kanji information window. This saves
        //    // the state of currently visible kanji information windows.
        //    writer.writeStartElement("KanjiInfoWindows");
        //    gUI->saveXMLKanjiInfo(writer);
        //    writer.writeEndElement(); /* KanjiInfoWindows */
        //}

        writer.writeEndElement(); /* Windows */

        writer.writeStartElement("Misc");

        writer.writeStartElement("LastDecks");
        gUI->saveXMLLastDecks(writer);
        writer.writeEndElement(); /* LastDecks */

        writer.writeStartElement("LastGroups");
        gUI->saveXMLLastGroups(writer);
        writer.writeEndElement(); /* LastGroups */

        writer.writeStartElement("LastSelections");
        gUI->saveXMLLastSettings(writer);
        writer.writeEndElement(); /* LastGroups */

        writer.writeEndElement(); /* Misc */


        writer.writeEndElement(); /* ZKanjiSettings */

        writer.writeEndDocument();

        if (writer.hasError())
        {
            f.close();

            int r = showAndReturn(gUI->activeMainForm(), "zkanji", qApp->translate("gUI", "There was an error writing the state settings file."), qApp->translate("gUI", "Please make sure there's enough disk space to write to %1 and \"%2\".").arg(QDir::toNativeSeparators(ZKanji::userFolder())).arg(qApp->translate("gUI", "Try again")),
            {
                { qApp->translate("gUI", "Try again"), QMessageBox::AcceptRole },
                { qApp->translate("gUI", "Cancel"), QMessageBox::RejectRole }
            });
            if (r == 0)
                saveSettingsToFile();
            return;
        }
    }

    void saveSettingsToFile()
    {
        saveIniFile();
        saveStatesFile();
    }

    static bool hasSimSun = false;
    static QString defaultJapaneseFont(const QStringList &allJpFonts)
    {
        QString defjp;

        // Fonts to look for to set as default:
        // Windows: Meiryo, MS UI Gothic, MS Gothic
        // Mac: Hiragino Kaku Gothic Pro, Osaka, Arial Unicode
        // Linux: Sazanami Gothic, VL Gothic, IPAGothic, Droid Sans Fallback, Gnu Unifont
        if (!allJpFonts.isEmpty())
        {
#ifdef Q_OS_MAC
            if (allJpFonts.indexOf("Hiragino Kaku Gothic Pro") != -1)
                defjp = "Hiragino Kaku Gothic Pro";
            else if (allJpFonts.indexOf("Osaka") != -1)
                defjp = "Osaka";
            else
            {
                // Try to find a font which starts with Osaka and use the first one found.
                for (QString &s : allJpFonts)
                {
                    if (s.startsWith("Osaka"))
                    {
                        defjp = s;
                        break;
                    }
                }
            }
#elif defined Q_OS_WIN
            if (defjp.isEmpty())
            {
                // Try to find Meiryo, MS UI Gothic, MS Gothic, Arial Unicode in this order.
                int mix = -1;
                bool mx = false;
                int uix = -1;
                bool ux = false;
                int gix = -1;
                bool gx = false;

                int ix = 0;
                for (const QString &s : allJpFonts)
                {
                    if (!mx && s.startsWith("Meiryo"))
                    {
                        if (s == "Meiryo")
                            mx = true;
                        mix = ix;
                        if (mx)
                            break;
                    }
                    if (!ux && s.startsWith("MS UI Gothic"))
                    {
                        if (s == "MS UI Gothic")
                            ux = true;
                        uix = ix;
                    }
                    else if (!gx && s.startsWith("MS Gothic"))
                    {
                        if (s == "MS Gothic")
                            gx = true;
                        gix = ix;
                    }
                    //else if (!ax && s == "Arial Unicode")
                    //{
                    //    if (s == "Arial Unicode" || s == "Arial Unicode MS")
                    //        ax = true;
                    //    aix = ix;
                    //}
                    if (mx && ux && gx)
                        break;
                    ++ix;
                }

                if (mix != -1)
                    defjp = allJpFonts.at(mix);
                else if (uix != -1)
                    defjp = allJpFonts.at(uix);
                else if (gx)
                    defjp = allJpFonts.at(gix);
            }
#elif defined Q_OS_LINUX
            if (defjp.isEmpty())
            {
                // Linux: Sazanami Gothic, VL Gothic, IPAGothic, Droid Sans Fallback, Gnu Unifont
                int ixs[]{ -1, -1, -1, -1, -1 };
                bool fx[]{ false, false, false, false, false };
                QString strnames[] = { "Sazanami Gothic", "VL Gothic", "IPAGothic", "Droid Sans Fallback", "Gnu Unifont" };

                int ix = 0;
                for (const QString &s : allJpFonts)
                {
                    int iy = 0;
                    for (const QString &s2 : strnames)
                    {
                        if (!fx[iy] && s.startsWith(s2.split(' ').at(0)))
                        {
                            ixs[iy] = ix;
                            if (s == s2)
                            {
                                fx[iy] = true;
                                if (iy == 0)
                                    break;
                            }
                        }
                        ++iy;
                    }

                    if (fx[0])
                        break;
                    ++ix;
                }

                for (ix = 0; ix != 5; ++ix)
                {
                    if (ixs[ix] != -1)
                    {
                        defjp = allJpFonts.at(ixs[ix]);
                        break;
                    }
                }
            }
#endif
            if (defjp.isEmpty())
            {
                for (const QString &s : allJpFonts)
                {
                    if (s.startsWith("Arial Unicode"))
                    {
                        defjp = s;
                        break;
                    }
                }
            }

            if (defjp.isEmpty())
            {
                // Find the first font that starts with "Arial" or "Adobe" or contains "Gothic".
                for (const QString &s : allJpFonts)
                {
                    if (s.startsWith("Arial") || s.startsWith("Adobe") || s.contains("Gothic"))
                    {
                        defjp = s;
                        break;
                    }
                }
            }

            // Use a random font.
            if (defjp.isEmpty())
                defjp = allJpFonts.at(rnd(0, allJpFonts.size() - 1));
        }


        if (defjp.isEmpty())
            defjp = qApp->font().family();

        return defjp;
    }
    
    void loadScaleSettingFromFile()
    {
        QSettings ini(ZKanji::userFolder() + "/zkanji.ini", QSettings::IniFormat);
        QString tmp;
        bool ok;
        int val;

        /*
        ini.setValue("scaling", general.savedscale);
        */
        tmp = ini.value("scaling", "100").toString();
        val = tmp.toInt(&ok);
        if (!ok)
        {
            Settings::general.scale = 100;
            Settings::general.savedscale = 100;
        }
        else
        {
            // Make sure value is valid. Can be a number between 100 and 400, divisible by 25.
            val = clamp(val, 100, 400);
            val = (val / 25) * 25;
            Settings::general.scale = val;
            Settings::general.savedscale = val;

            QFont f = qApp->font();
            f.setPointSizeF(f.pointSizeF() * Settings::general.scale / 100.0);

            gUI->applyStyleSheet();
            qApp->setFont(f);
        }

        _scaleratio = (qreal)QApplication::primaryScreen()->logicalDotsPerInchY() / QApplication::primaryScreen()->physicalDotsPerInchY();

    }

    void loadIniFile()
    {
        QSettings ini(ZKanji::userFolder() + "/zkanji.ini", QSettings::IniFormat);

        QString str;
        bool ok;
        int val;

        // General settings

        str = ini.value("dateformat", "daymonthyear").toString().toLower();
        general.dateformat = str == "yearmonthday" ? GeneralSettings::YearMonthDay : str == "monthdayyear" ? GeneralSettings::MonthDayYear : GeneralSettings::DayMonthYear;

        //val = ini.value("settingspage", 0).toInt(&ok);
        //if (val >= 0 && val <= 999)
        //    general.settingspage = val;

        general.savewinpos = ini.value("savewinpos", true).toBool();
        //general.savetoolstates = ini.value("savetoolstates", true).toBool();

        str = ini.value("startstate", "savestate").toString().toLower();
        general.startstate = str == "forget" ? GeneralSettings::ForgetState : str == "minimize" ? GeneralSettings::AlwaysMinimize : str == "maximize" ? GeneralSettings::AlwaysMaximize : GeneralSettings::SaveState;

        str = ini.value("startplacement", "mainonactive").toString().toLower();

        str = ini.value("minimizebehavior", "default").toString();
        if (str == "closetotray")
            general.minimizebehavior = GeneralSettings::TrayOnClose;
        else if (str == "minimizetotray")
            general.minimizebehavior = GeneralSettings::TrayOnMinimize;
        else
            general.minimizebehavior = GeneralSettings::DefaultMinimize;

        // Font settings

        fonts.kanji = ini.value("fonts/kanji", QString()).toString();
        val = ini.value("fonts/kanjifontsize", QString()).toInt(&ok);
        if (ok)
            fonts.kanjifontsize = val;
        fonts.nokanjialias = ini.value("fonts/nokanjialias", true).toBool();
        fonts.kana = ini.value("fonts/kana", QString()).toString();
        fonts.main = ini.value("fonts/main", qApp->font().family()).toString();
        fonts.info = ini.value("fonts/notes", qApp->font().family()).toString();
        str = ini.value("fonts/notesstyle", "normal").toString().toLower();
        fonts.infostyle = (str == "bold") ? FontStyle::Bold : (str == "italic") ? FontStyle::Italic : (str == "bolditalic") ? FontStyle::BoldItalic : FontStyle::Normal;
        //fonts.extra = ini.value("fonts/extra", qApp->font().family()).toString();
        //str = ini.value("fonts/extrastyle", "Bold").toString();
        //fonts.extrastyle = (str == "Bold") ? FontStyle::Bold : (str == "Italic") ? FontStyle::Italic : (str == "BoldItalic") ? FontStyle::BoldItalic : FontStyle::Normal;

        fonts.printkana = ini.value("fonts/printkana", QString()).toString();
        fonts.printdefinition = ini.value("fonts/printdefinition", qApp->font().family()).toString();
        fonts.printinfo = ini.value("fonts/printinfo", qApp->font().family()).toString();
        str = ini.value("fonts/printinfostyle", "italic").toString().toLower();
        fonts.printinfostyle = (str == "bold") ? FontStyle::Bold : (str == "normal") ? FontStyle::Normal : (str == "bolditalic") ? FontStyle::BoldItalic : FontStyle::Italic;

        val = ini.value("fonts/mainsize", 1).toInt(&ok);
        if (ok && val >= 0 && val <= 3)
            fonts.mainsize = (LineSize)val;
        val = ini.value("fonts/popsize", 1).toInt(&ok);
        if (ok && val >= 0 && val <= 3)
            fonts.popsize = (LineSize)val;

        // Group settings

        group.hidesuspended = ini.value("groups/hidesuspended", true).toBool();

        // Print settings

        print.dictfonts = ini.value("print/usedictionaryfonts", true).toBool();
        print.doublepage = ini.value("print/doublepage", false).toBool();
        print.separated = ini.value("print/separatecolumns", false).toBool();

        val = ini.value("print/linesize", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 6)
            print.linesize = (PrintSettings::LineSizes)val;
        val = ini.value("print/columns", 2).toInt(&ok);
        if (ok && val >= 1 && val <= 3)
            print.columns = val;

        print.usekanji = ini.value("print/usekanji", true).toBool();
        print.showtype = ini.value("print/showtype", true).toBool();
        print.userdefs = ini.value("print/userdefs", true).toBool();
        print.reversed = ini.value("print/reversed", false).toBool();

        val = ini.value("print/readings", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            print.readings = (PrintSettings::Readings)val;

        print.background = ini.value("print/background", false).toBool();
        print.separator = ini.value("print/separatorline", false).toBool();
        print.pagenum = ini.value("print/pagenum", false).toBool();

        print.printername = ini.value("print/printername").toString();
        print.portrait = ini.value("print/portraitmode", true).toBool();
        val = ini.value("print/leftmargin", 16).toInt(&ok);
        if (ok && val >= 0)
            print.leftmargin = val;
        val = ini.value("print/topmargin", 16).toInt(&ok);
        if (ok && val >= 0)
            print.topmargin = val;
        val = ini.value("print/rightmargin", 16).toInt(&ok);
        if (ok && val >= 0)
            print.rightmargin = val;
        val = ini.value("print/bottommargin", 16).toInt(&ok);
        if (ok && val >= 0)
            print.bottommargin = val;
        val = ini.value("print/sizeid", -1).toInt(&ok);
        if (ok && val >= -1)
            print.pagesizeid = val;

        // Dialog settings

        dialog.lastsettingspage = ini.value("dialogs/settingspage", QString()).toString();

        // Popup settings

        popup.autohide = ini.value("popup/autohide", true).toBool();
        popup.widescreen = ini.value("popup/widescreen", false).toBool();
        popup.statusbar = ini.value("popup/statusbar", false).toBool();
        //popup.floating = ini.value("popup/floating", false).toBool();
        //popup.dictfloatrect = ini.value("popup/floatrect", QRect()).toRect();
        //popup.normalsize = ini.value("popup/size", QSize()).toSize();
        val = ini.value("popup/autosearch", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            popup.activation = (PopupSettings::Activation)val;
        val = ini.value("popup/transparency", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 10)
            popup.transparency = val;

        popup.dict = ini.value("popup/dictionary", QString()).toString();

        //popup.kanjifloatrect = ini.value("popup/kanjirect", QRect()).toRect();
        popup.kanjiautohide = ini.value("popup/kanjiautohide", false).toBool();

        popup.kanjidict = ini.value("popup/kanjidictionary", QString()).toString();

        // Recognizer settings

        recognizer.savesize = ini.value("recognizer/savesize", true).toBool();
        recognizer.saveposition = ini.value("recognizer/savepos", false).toBool();
        //recognizer.rect = ini.value("recognizer/rect", recognizer.rect).toRect();

        // Color settings

        colors.useinactive = ini.value("colors/useinactive", true).value<bool>();
        colors.bg = ini.value("colors/bg").value<QColor>();
        colors.text = ini.value("colors/text").value<QColor>();
        colors.selbg = ini.value("colors/selbg").value<QColor>();
        colors.seltext = ini.value("colors/seltext").value<QColor>();
        colors.bgi = ini.value("colors/bgi").value<QColor>();
        colors.texti = ini.value("colors/texti").value<QColor>();
        colors.selbgi = ini.value("colors/selbgi").value<QColor>();
        colors.seltexti = ini.value("colors/seltexti").value<QColor>();

        colors.scrollbg = ini.value("colors/scrollbg").value<QColor>();
        colors.scrollh = ini.value("colors/scrollh").value<QColor>();
        colors.scrollbgi = ini.value("colors/scrollbgi").value<QColor>();
        colors.scrollhi = ini.value("colors/scrollhi").value<QColor>();

        colors.grid = ini.value("colors/grid").value<QColor>();
        colors.studycorrect = ini.value("colors/studycorrect").value<QColor>();
        colors.studywrong = ini.value("colors/studywrong").value<QColor>();
        colors.studynew = ini.value("colors/studynew").value<QColor>();
        colors.inf = ini.value("colors/inf").value<QColor>();
        colors.attrib = ini.value("colors/attrib").value<QColor>();
        colors.types = ini.value("colors/types").value<QColor>();
        colors.notes = ini.value("colors/notes").value<QColor>();
        colors.fields = ini.value("colors/fields").value<QColor>();
        colors.dialect = ini.value("colors/dialect").value<QColor>();
        colors.kanaonly = ini.value("colors/kanaonly").value<QColor>();
        colors.sentenceword = ini.value("colors/sentenceword").value<QColor>();
        colors.kanjiexbg = ini.value("colors/kanjiexamplebg").value<QColor>();
        colors.kanjitestpos = ini.value("colors/kanjitestpos").value<QColor>();

        colors.n5 = ini.value("colors/n5").value<QColor>();
        colors.n4 = ini.value("colors/n4").value<QColor>();
        colors.n3 = ini.value("colors/n3").value<QColor>();
        colors.n2 = ini.value("colors/n2").value<QColor>();
        colors.n1 = ini.value("colors/n1").value<QColor>();
        colors.kanjinowords = ini.value("colors/kanjinowords").value<QColor>();
        colors.kanjiunsorted = ini.value("colors/kanjiunsorted").value<QColor>();
        colors.kanjinotranslation = ini.value("colors/kanjinotranslation").value<QColor>();
        colors.coloroku = ini.value("colors/coloroku", true).toBool();
        colors.okucolor = ini.value("colors/oku").value<QColor>();

        colors.katabg = ini.value("colors/katabg").value<QColor>();
        colors.hirabg = ini.value("colors/hirabg").value<QColor>();
        colors.similartext = ini.value("colors/similartext").value<QColor>();
        //colors.similarbg = ini.value("colors/similarbg").value<QColor>();
        //colors.partsbg = ini.value("colors/partsbg").value<QColor>();
        //colors.partofbg = ini.value("colors/partofbg").value<QColor>();
        colors.strokedot = ini.value("colors/strokedot").value<QColor>();
        colors.stat1 = ini.value("colors/stat1").value<QColor>();
        colors.stat2 = ini.value("colors/stat2").value<QColor>();
        colors.stat3 = ini.value("colors/stat3").value<QColor>();

        // Dictionary

        dictionary.autosize = ini.value("dict/autosize", false).toBool();
        val = ini.value("dict/inflection", 1).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            dictionary.inflection = (DictionarySettings::InflectionShow)val;

        str = ini.value("dict/resultorder", "relevance").toString().toLower();
        dictionary.resultorder = str == "frequency" ? ResultOrder::Frequency : str == "jlptfrom1" ? ResultOrder::JLPTfrom1 : str == "jlptfrom5" ? ResultOrder::JLPTfrom5 : ResultOrder::Relevance;

        str = ini.value("dict/browseorder", "abcde").toString().toLower();
        dictionary.browseorder = str == "aiueo" ? BrowseOrder::AIUEO : BrowseOrder::ABCDE;

        dictionary.showingroup = ini.value("dict/showgroup", false).toBool();
        dictionary.showjlpt = ini.value("dict/showjlpt", true).toBool();
        val = ini.value("dict/jlptcol", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            dictionary.jlptcolumn = (DictionarySettings::JlptColumn)val;
        val = ini.value("dict/historylimit", 1000).toInt(&ok);
        if (ok && val >= 0 && val < 10000)
            dictionary.historylimit = val;
        dictionary.historytimer = ini.value("dict/historytimer", true).toBool();
        val = ini.value("dict/historytimeout", 4).toInt(&ok);
        if (ok && val > 1 && val <= 100)
            dictionary.historytimeout = val;

        // Shortcut settings

        shortcuts.fromenable = ini.value("shortcuts/from", false).toBool();
        str = ini.value("shortcuts/frommod", "altcontrol").toString().toLower();
        shortcuts.frommodifier = str == "alt" ? ShortcutSettings::Alt : str == "control" ? ShortcutSettings::Control : ShortcutSettings::AltControl;
        shortcuts.fromshift = ini.value("shortcuts/fromshift", false).toBool();
        str = ini.value("shortcuts/fromkey", "J").toString().toUpper();
        if (str.size() == 1)
            shortcuts.fromkey = str.at(0);
        if (shortcuts.fromkey.unicode() == 0 || shortcuts.fromkey < 'A' || shortcuts.fromkey > 'Z')
        {
            shortcuts.fromenable = false;
            shortcuts.fromkey = 'J';
        }

        shortcuts.toenable = ini.value("shortcuts/to", false).toBool();
        str = ini.value("shortcuts/tomod", "altcontrol").toString().toLower();
        shortcuts.tomodifier = str == "alt" ? ShortcutSettings::Alt : str == "control" ? ShortcutSettings::Control : ShortcutSettings::AltControl;
        shortcuts.toshift = ini.value("shortcuts/toshift", false).toBool();
        str = ini.value("shortcuts/tokey", "E").toString().toUpper();
        if (str.size() == 1)
            shortcuts.tokey = str.at(0);
        if (shortcuts.tokey < 'A' || shortcuts.tokey > 'Z' || (shortcuts.fromenable && shortcuts.tokey == shortcuts.fromkey))
        {
            shortcuts.toenable = false;
            shortcuts.tokey = 'E';
        }

        shortcuts.kanjienable = ini.value("shortcuts/kanji", false).toBool();
        str = ini.value("shortcuts/kanjimod", "altcontrol").toString().toLower();
        shortcuts.kanjimodifier = str == "alt" ? ShortcutSettings::Alt : str == "control" ? ShortcutSettings::Control : ShortcutSettings::AltControl;
        shortcuts.kanjishift = ini.value("shortcuts/kanjishift", false).toBool();
        str = ini.value("shortcuts/kanjikey", "K").toString().toUpper();
        if (str.size() == 1)
            shortcuts.kanjikey = str.at(0);
        if (shortcuts.kanjikey < 'A' || shortcuts.kanjikey > 'Z' || (shortcuts.fromenable && shortcuts.kanjikey == shortcuts.fromkey) || (shortcuts.toenable && shortcuts.kanjikey == shortcuts.tokey))
        {
            shortcuts.kanjienable = false;
            shortcuts.kanjikey = 'K';
        }

        // Kanji settings

        const int kanjirefcnt = ZKanji::kanjiReferenceCount();

        kanji.savefilters = ini.value("kanji/savefilters", true).toBool();
        kanji.resetpopupfilters = ini.value("kanji/popupreset", false).toBool();
        val = ini.value("kanji/ref1", 3).toInt(&ok);
        if (ok && val >= 0 && val <= kanjirefcnt)
            kanji.mainref1 = val;
        val = ini.value("kanji/ref2", 4).toInt(&ok);
        if (ok && val >= 0 && val <= kanjirefcnt)
            kanji.mainref2 = val;
        val = ini.value("kanji/ref3", 0).toInt(&ok);
        if (ok && val >= 0 && val <= kanjirefcnt)
            kanji.mainref3 = val;
        val = ini.value("kanji/ref4", 0).toInt(&ok);
        if (ok && val >= 0 && val <= kanjirefcnt)
            kanji.mainref4 = val;
        kanji.listparts = ini.value("kanji/parts", false).toBool();


        str = ini.value("kanji/showpos", "nearcursor").toString().toLower();
        if (str == "default")
            kanji.showpos = KanjiSettings::SystemDefault;
        else if (str == "last")
            kanji.showpos = KanjiSettings::RestoreLast;
        else
            kanji.showpos = KanjiSettings::NearCursor;

        kanji.tooltip = ini.value("kanji/tooltip", true).toBool();
        kanji.hidetooltip = ini.value("kanji/hidetooltip", true).toBool();
        val = ini.value("kanji/tooltipdelay", 5).toInt(&ok);
        if (ok && val >= 1 && val <= 9999)
            kanji.tooltipdelay = val;

        kanji.showref.resize(kanjirefcnt);
        kanji.reforder.resize(kanjirefcnt);

        QSet<int> foundrefs;
        bool brokenorder = false;
        for (int ix = 0; ix != kanjirefcnt && !brokenorder; ++ix)
        {
            kanji.showref[ix] = ini.value(QString("kanji/showref%1").arg(ix), true).toBool();

            val = ini.value(QString("kanji/reforder%1").arg(ix), kanjirefcnt).toInt(&ok);
            if (ok && val >= 0 && val < kanjirefcnt && !foundrefs.contains(val))
            {
                foundrefs.insert(val);
                kanji.reforder[ix] = val;
            }
            else
                brokenorder = true;
        }

        if (brokenorder)
        {
            std::vector<char> tmp(kanjirefcnt, (const char)1);
            kanji.showref.swap(tmp);
            //kanji.showref.swap(std::vector<char>(kanjirefcnt, (const char)1));
            std::iota(kanji.reforder.begin(), kanji.reforder.end(), 0);
        }

        // Study settings

        val = ini.value("study/starthour", 4).toInt(&ok);
        if (ok && val >= 1 && val <= 12)
            study.starthour = val;
        val = ini.value("study/includefreq", (int)StudySettings::One).toInt(&ok);
        if (ok && val >= 0 && val <= 4)
            study.includedays = (StudySettings::IncludeDays)val;
        val = ini.value("study/includecnt", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 9999)
            study.includecount = val;
        study.onlyunique = ini.value("study/onlyunique", true).toBool();
        study.limitnew = ini.value("study/limitnew", true).toBool();
        val = ini.value("study/limitcount", 60).toInt(&ok);
        if (ok && val >= 0 && val <= 9999)
            study.limitcount = val;

        val = ini.value("study/kanjihint", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 3 && val != 0)
            study.kanjihint = (WordParts)val;
        val = ini.value("study/kanahint", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 3 && val != 1)
            study.kanahint = (WordParts)val;
        val = ini.value("study/defhint", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 3 && val != 2)
            study.defhint = (WordParts)val;

        study.showestimate = ini.value("study/showestimate", true).toBool();
        study.hidekanjikana = ini.value("study/hidekanjikana", false).toBool();
        study.writekanji = ini.value("study/writekanji", false).toBool();
        study.writekana = ini.value("study/writekana", false).toBool();
        study.writedef = ini.value("study/writedef", false).toBool();
        val = ini.value("study/delaywrong", 10).toInt(&ok);
        if (ok && val >= 1 && val <= 60)
            study.delaywrong = val;
        study.review = ini.value("study/review", true).toBool();
        study.idlesuspend = ini.value("study/idlesuspend", false).toBool();
        val = ini.value("study/idletime", 3).toInt(&ok);
        if (ok && val >= 0 && val <= 4)
            study.idletime = (StudySettings::SuspendWait)val;
        study.itemsuspend = ini.value("study/itemsuspend", false).toBool();
        val = ini.value("study/itemsuspendnum", 60).toInt(&ok);
        if (ok && val >= 1 && val <= 9999)
            study.itemsuspendnum = val;
        study.timesuspend = ini.value("study/timesuspend", false).toBool();
        val = ini.value("study/timesuspendnum", 20).toInt(&ok);
        if (ok && val >= 1 && val <= 9999)
            study.timesuspendnum = val;

        val = ini.value("study/readings", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 3)
            study.readings = (StudySettings::ReadingType)val;
        val = ini.value("study/readingsfrom", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 3)
            study.readingsfrom = (StudySettings::ReadingFrom)val;

        // Data settings

        data.autosave = ini.value("data/autosave", false).toBool();
        val = ini.value("data/interval", 2).toInt(&ok);
        if (ok && val >= 1 && val <= 60)
            data.interval = val;
        data.backup = ini.value("data/backup", false).toBool();
        val = ini.value("data/backupcount", 4).toInt(&ok);
        if (ok && val >= 1 && val <= 9999)
            data.backupcnt = val;
        val = ini.value("data/backupskip", 3).toInt(&ok);
        if (ok && val >= 1 && val <= 100)
            data.backupskip = val;
        data.location = ini.value("data/location", QString()).toString();
    }

    void loadDictionaryOrder()
    {
        QSettings ini(ZKanji::userFolder() + "/zkanji.ini", QSettings::IniFormat);

        // Dictionary order

        QSet<quint8> found;
        std::list<quint8> order;
        QStringList strs = ini.value("dictionaryorder", QString()).toString().split('/');
        for (int ix = strs.size() - 1; ix != -1; --ix)
        {
            int dix = ZKanji::dictionaryIndex(strs[ix]);
            if (dix == -1)
            {
                strs.removeAt(ix);
                continue;
            }
            order.push_front(dix);
            found.insert(dix);
        }
        for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
        {
            if (found.contains(ix))
                continue;
            order.push_back(ix);
        }

        ZKanji::changeDictionaryOrder(order);
    }

    void loadSettingsFromFile()
    {
        loadIniFile();

        QFontDatabase db;
        QStringList allJpFonts = db.families(QFontDatabase::Japanese);
        hasSimSun = db.families(QFontDatabase::Any).indexOf("SimSun") != -1;

        if (!fonts.kanji.isEmpty() && allJpFonts.indexOf(fonts.kanji) == -1)
            fonts.kanji.clear();
        if (!fonts.kana.isEmpty() && allJpFonts.indexOf(fonts.kana) == -1)
            fonts.kana.clear();
        if (!fonts.printkana.isEmpty() && allJpFonts.indexOf(fonts.printkana) == -1)
            fonts.printkana.clear();

        // A default Japanese font that can be used when nothing is set.
        QString defjp;

        if (fonts.kanji.isEmpty() || fonts.kana.isEmpty() || fonts.printkana.isEmpty())
            defjp = defaultJapaneseFont(allJpFonts);
        if (fonts.kanji.isEmpty())
            fonts.kanji = defjp;
        if (fonts.kana.isEmpty())
            fonts.kana = defjp;
        if (fonts.printkana.isEmpty())
            fonts.printkana = defjp;

    }

    void loadStatesFromFile()
    {
        loadDictionaryOrder();
        
        QString fname = ZKanji::userFolder() + "/states.xml";
        QFile f(fname);
        if (!f.open(QIODevice::ReadOnly))
        {
            if (!f.exists())
                return;
            QMessageBox::warning(nullptr, "zkanji", qApp->translate("gUI", "Couldn't open the state settings file for reading. Please make sure you have permission to access the file %1").arg(QDir::toNativeSeparators(fname)), QMessageBox::Ok);
            return;
        }

        QXmlStreamReader reader(&f);
        reader.setNamespaceProcessing(false);

        reader.readNextStartElement();
        if (reader.name() != "ZKanjiSettings")
            reader.raiseError("MissingZKanjiSettingsElement");


        // When expanding later: Every loadXML****() function MUST leave the current block,
        // and any level before that should expect this to happen, either by calling the
        // stream reader's skipCurrentElement(), or by a while(reader.readNextStartElement())
        // loop.
        // The reader's readNextStartElement() function reads until the next starting OR next
        // ending element, and returns true if it currently is at a starting element. When it
        // returns false, the attributes will be valid for the ending, and the next read will
        // start the next block outside the one that just ended.

        while (!reader.atEnd())
        {
            if (!reader.readNextStartElement() || reader.atEnd())
                break;

            if (reader.name() == "Filters")
            {
                while (reader.readNextStartElement())
                {
                    if (reader.name() == "RadicalFilters")
                        loadXMLRadicalFilters(reader);
                    else if (reader.name() == "WordFilters")
                        loadXMLWordFilters(reader);
                }
            }
            else if (reader.name() == "LookupSites")
            {
                ZKanji::loadXMLLookupSites(reader);
            }
            else if (reader.name() == "WindowStates")
            {
                /*if (!Settings::general.savewinstates)
                    reader.skipCurrentElement();
                else*/ while (reader.readNextStartElement())
                {
                    // To avoid spamming the state file, splitter states and sizes are only
                    // loaded for currently existing forms.
                    if (reader.name() == "WordEditor" || reader.name() == "WordToGroup" || reader.name() == "WordToDictionary" || reader.name() == "WordFilterList" || reader.name() == "KanjiToGroup")
                        FormStates::loadXMLDialogSplitterState(reader);
                    else if (reader.name() == "WordToDeck" || reader.name() == "DictionaryEditor" || reader.name() == "DictionaryExport" || reader.name() == "DictionaryImport" ||
                            reader.name() == "DictionaryStats" || reader.name() == "DictionaryText" || reader.name() == "GroupExport" || reader.name() == "GroupImport" ||
                            reader.name() == "GroupPicker" || reader.name() == "KanjiDefinition")
                        FormStates::loadXMLDialogSize(reader);
                    else if (reader.name() == "PrintPreview")
                        FormStates::loadXMLDialogMaximizedAndSize(reader);
                    else if (reader.name() == "CollectWords")
                        FormStates::loadXMLSettings(FormStates::collectform, reader);
                    else if (reader.name() == "KanjiInformation")
                        FormStates::loadXMLSettings(FormStates::kanjiinfo, reader);
                    else if (reader.name() == "WordStudyListForm")
                        FormStates::loadXMLSettings(FormStates::wordstudylist, reader);
                    else if (reader.name() == "PopupKanji")
                        FormStates::loadXMLSettings(FormStates::popupkanji, reader);
                    else if (reader.name() == "PopupDictionary")
                        FormStates::loadXMLSettings(FormStates::popupdict, reader);
                    else if (reader.name() == "KanaPractice")
                        FormStates::loadXMLSettings(FormStates::kanapractice, reader);
                    else if (reader.name() == "RecognizerForm")
                        FormStates::loadXMLSettings(FormStates::recognizer, reader);
                    else
                        reader.skipCurrentElement();
                }
            }
            else if (reader.name() == "Windows")
            {
                /*if (!Settings::general.savewinstates)
                    reader.skipCurrentElement();
                else*/ while (reader.readNextStartElement())
                {
                    bool ismain = reader.name() == "MainWindow";
                    if (ismain && gUI->mainForm() != nullptr)
                    {
                        // Multiple MainWindow tags found in the xml.
                        reader.skipCurrentElement();
                        continue;
                    }
                    if (ismain || reader.name() == "Window")
                        gUI->loadXMLWindow(ismain, reader);
                    //else if (Settings::general.savetoolstates && reader.name() == "KanjiInfoWindows")
                    //    gUI->loadXMLKanjiInfo(reader);
                    else
                        reader.skipCurrentElement();
                }
            }
            else if (reader.name() == "Misc")
            {
                while (reader.readNextStartElement())
                {
                    if (reader.name() == "LastDecks")
                    {
                        //if (!Settings::general.savewinstates)
                        //    reader.skipCurrentElement();
                        //else
                        gUI->loadXMLLastDecks(reader);
                    }
                    else if (reader.name() == "LastGroups")
                    {
                        //if (!Settings::general.savewinstates)
                        //    reader.skipCurrentElement();
                        //else
                        gUI->loadXMLLastGroups(reader);
                    }
                    else if (reader.name() == "LastSelections")
                        gUI->loadXMLLastSettings(reader);
                    else
                        reader.skipCurrentElement();
                }
            }
            else
                reader.skipCurrentElement();
        }

        if (reader.hasError())
        {
            QString errorstr;
            switch (reader.error())
            {
            case QXmlStreamReader::UnexpectedElementError:
                errorstr = "UnexpectedElementError";
                break;
            case QXmlStreamReader::NotWellFormedError:
                errorstr = "NotWellFormedError";
                break;
            case QXmlStreamReader::PrematureEndOfDocumentError:
                errorstr = "PrematureEndOfDocumentError";
                break;
            case QXmlStreamReader::CustomError:
                errorstr = reader.errorString();
                break;
            default:
                break;
            }

            if (QMessageBox::warning(nullptr, "zkanji", qApp->translate("gUI", "There was an error in states.xml at line %1, position %2, of the type %3. The program can load normally with some settings lost.").arg(reader.lineNumber()).arg(reader.columnNumber()).arg(errorstr), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Cancel)
                exit(0);
        }
    }


    // TODO: update these to reflect settings, and to work on other OS-s. Or use SOD.
    QString radicalFontName()
    {
#ifdef Q_OS_WIN
        if (hasSimSun)
            return QStringLiteral("SimSun");
#endif
        return Settings::fonts.kanji;
    }

    QFont kanjiFont(bool /*scaled*/)
    {
        QFont f = { fonts.kanji, Settings::scaled(fonts.kanjifontsize / _scaleratio) };
        if (Settings::fonts.nokanjialias)
        {
            QFont::StyleStrategy ss = f.styleStrategy();
            ss = QFont::StyleStrategy(ss | QFont::NoSubpixelAntialias);
            f.setStyleStrategy(ss);
        }
        return f;
    }

    QFont radicalFont(bool scaled)
    {
        return QFont{ radicalFontName(), scaled ? Settings::scaled(fonts.kanjifontsize) : fonts.kanjifontsize};
    }

    QFont kanaFont(bool scaled)
    {
        return QFont{ Settings::fonts.kana, scaled ? Settings::scaled(9): 9 };
    }

    QFont mainFont(bool scaled)
    {
        return QFont{ Settings::fonts.main, scaled ? Settings::scaled(9) : 9 };
    }

    QFont notesFont(bool scaled)
    {
        return QFont{ Settings::fonts.info, scaled ? Settings::scaled(7) : 7, Settings::fonts.infostyle == FontStyle::Bold || Settings::fonts.infostyle == FontStyle::BoldItalic ? QFont::Bold : -1, Settings::fonts.infostyle == FontStyle::Italic || Settings::fonts.infostyle == FontStyle::BoldItalic };
    }

    QFont extraFont(bool scaled)
    {
        return QFont{ Settings::fonts.main, scaled ? Settings::scaled(9) : 9, QFont::Bold };
    }

    QFont printKanaFont()
    {
        return QFont{ !Settings::print.dictfonts ? Settings::fonts.printkana : Settings::fonts.kana, 120 };
    }

    QFont printDefFont()
    {
        return QFont{ !Settings::print.dictfonts ? Settings::fonts.printdefinition : Settings::fonts.main, 120 };
    }

    QFont printInfoFont()
    {
        return QFont{ !Settings::print.dictfonts ? Settings::fonts.printinfo : Settings::fonts.info, 120, Settings::print.dictfonts ? false : Settings::fonts.printinfostyle == FontStyle::Bold || Settings::fonts.printinfostyle == FontStyle::BoldItalic ? QFont::Bold : -1, Settings::print.dictfonts ? true : Settings::fonts.printinfostyle == FontStyle::Italic || Settings::fonts.printinfostyle == FontStyle::BoldItalic };
    }

    QColor textColor(const QPalette &pal, bool active, ColorSettings::SystemColorTypes type)
    {
        if (!colors.useinactive)
            active = true;

        switch (type)
        {
        case ColorSettings::Bg:
            if ((active && colors.bg.isValid()) || (!active && colors.bgi.isValid()))
                return active ? colors.bg : colors.bgi;
            return pal.color(active ? QPalette::Active : QPalette::Inactive, QPalette::Base);
        case ColorSettings::Text:
            if ((active && colors.text.isValid()) || (!active && colors.texti.isValid()))
                return active ? colors.text : colors.texti;
            return pal.color(active ? QPalette::Active : QPalette::Inactive, QPalette::Text);
        case ColorSettings::SelBg:
            if ((active && colors.selbg.isValid()) || (!active && colors.selbgi.isValid()))
                return active ? colors.selbg : colors.selbgi;
            return pal.color(active ? QPalette::Active : QPalette::Inactive, QPalette::Highlight);
        case ColorSettings::SelText:
            if ((active && colors.seltext.isValid()) || (!active && colors.seltexti.isValid()))
                return active ? colors.seltext : colors.seltexti;
            return pal.color(active ? QPalette::Active : QPalette::Inactive, QPalette::HighlightedText);

        // TODO: instead of using default background and text colors for the scrollbars, check
        // the system defaults. This is currently not available in Qt.
        case ColorSettings::ScrollBg:
            if ((active && colors.scrollbg.isValid()) || (!active && colors.scrollbgi.isValid()))
                return active ? colors.scrollbg : colors.scrollbgi;
            return pal.color(active ? QPalette::Active : QPalette::Inactive, QPalette::Base);
        case ColorSettings::ScrollHandle:
            if ((active && colors.scrollh.isValid()) || (!active && colors.scrollhi.isValid()))
                return active ? colors.scrollh : colors.scrollhi;
            return pal.color(active ? QPalette::Active : QPalette::Inactive, QPalette::Text);

        default:
            return QColor();
        }
    }

    QColor textColor(bool active, ColorSettings::SystemColorTypes type)
    {
        return textColor(qApp->palette(), active, type);
    }

    QColor textColor(QWidget *w, ColorSettings::SystemColorTypes type)
    {
        return textColor(w->hasFocus(), type);
    }

    QColor textColor(ColorSettings::SystemColorTypes type)
    {
        return textColor(qApp->palette(), true, type);
    }

    QColor uiColor(ColorSettings::UIColorTypes type)
    {
        switch (type)
        {
        case ColorSettings::Grid:
            if (colors.grid.isValid())
                return colors.grid;
            else
            {
                QStyleOptionViewItem opts;
                opts.widget = nullptr;
                opts.state = QStyle::State_Enabled | QStyle::State_Active | QStyle::State_HasFocus;
                opts.direction = Qt::LayoutDirectionAuto;
                opts.palette = qApp->palette();
                opts.fontMetrics = qApp->fontMetrics();
                opts.styleObject = nullptr;
                opts.showDecorationSelected = true;
                int gridHint = qApp->style()->styleHint(QStyle::SH_Table_GridLineColor, &opts, nullptr);
                return static_cast<QRgb>(gridHint);
            }
        case ColorSettings::StudyCorrect:
            if (colors.studycorrect.isValid())
                return colors.studycorrect;
            break;
        case ColorSettings::StudyWrong:
            if (colors.studywrong.isValid())
                return colors.studywrong;
            break;
        case ColorSettings::StudyNew:
            if (colors.studynew.isValid())
                return colors.studynew;
            break;
        case ColorSettings::Inf:
            if (colors.inf.isValid())
                return colors.inf;
            break;
        case ColorSettings::Attrib:
            if (colors.attrib.isValid())
                return colors.attrib;
            break;
        case ColorSettings::Types:
            if (colors.types.isValid())
                return colors.types;
            break;
        case ColorSettings::Notes:
            if (colors.notes.isValid())
                return colors.notes;
            break;
        case ColorSettings::Fields:
            if (colors.fields.isValid())
                return colors.fields;
            break;
        case ColorSettings::Dialects:
            if (colors.dialect.isValid())
                return colors.dialect;
            break;
        case ColorSettings::KanaOnly:
            if (colors.kanaonly.isValid())
                return colors.kanaonly;
            break;
        case ColorSettings::SentenceWord:
            if (colors.sentenceword.isValid())
                return colors.sentenceword;
            break;
        case ColorSettings::KanjiExBg:
            if (colors.kanjiexbg.isValid())
                return colors.kanjiexbg;
            break;
        case ColorSettings::KanjiTestPos:
            if (colors.kanjitestpos.isValid())
                return colors.kanjitestpos;
            break;
        case ColorSettings::Oku:
            if (colors.okucolor.isValid())
                return colors.okucolor;
            break;
        case ColorSettings::N5:
            if (colors.n5.isValid())
                return colors.n5;
            break;
        case ColorSettings::N4:
            if (colors.n4.isValid())
                return colors.n4;
            break;
        case ColorSettings::N3:
            if (colors.n3.isValid())
                return colors.n3;
            break;
        case ColorSettings::N2:
            if (colors.n2.isValid())
                return colors.n2;
            break;
        case ColorSettings::N1:
            if (colors.n1.isValid())
                return colors.n1;
            break;
        case ColorSettings::KanjiNoWords:
            if (colors.kanjinowords.isValid())
                return colors.kanjinowords;
            break;
        case ColorSettings::KanjiUnsorted:
            if (colors.kanjiunsorted.isValid())
                return colors.kanjiunsorted;
            break;
        case ColorSettings::KanjiNoTranslation:
            if (colors.kanjinotranslation.isValid())
                return colors.kanjinotranslation;
            break;
        case ColorSettings::KataBg:
            if (colors.katabg.isValid())
                return colors.katabg;
            break;
        case ColorSettings::HiraBg:
            if (colors.hirabg.isValid())
                return colors.hirabg;
            break;
        case ColorSettings::SimilarText:
            if (colors.similartext.isValid())
                return colors.similartext;
            break;
        //case ColorSettings::SimilarBg:
        //    if (colors.similarbg.isValid())
        //        return colors.similarbg;
        //    break;
        //case ColorSettings::PartsBg:
        //    if (colors.partsbg.isValid())
        //        return colors.partsbg;
        //    break;
        //case ColorSettings::PartOfBg:
        //    if (colors.partofbg.isValid())
        //        return colors.partofbg;
        //    break;
        case ColorSettings::StrokeDot:
            if (colors.strokedot.isValid())
                return colors.strokedot;
            break;
        case ColorSettings::Stat1:
            if (colors.stat1.isValid())
                return colors.stat1;
            break;
        case ColorSettings::Stat2:
            if (colors.stat2.isValid())
                return colors.stat2;
            break;
        case ColorSettings::Stat3:
            if (colors.stat3.isValid())
                return colors.stat3;
            break;
        }

        return defUiColor(type);
    }

    QColor defUiColor(ColorSettings::UIColorTypes type)
    {
        switch (type)
        {
        case ColorSettings::Grid:
        {
            QStyleOptionViewItem opts;
            opts.widget = nullptr;
            opts.state = QStyle::State_Enabled | QStyle::State_Active | QStyle::State_HasFocus;
            opts.direction = Qt::LayoutDirectionAuto;
            opts.palette = qApp->palette();
            opts.fontMetrics = qApp->fontMetrics();
            opts.styleObject = nullptr;
            opts.showDecorationSelected = true;
            int gridHint = qApp->style()->styleHint(QStyle::SH_Table_GridLineColor, &opts, nullptr);
            return static_cast<QRgb>(gridHint);
        }
        case ColorSettings::StudyCorrect:
            return colors.lighttheme ? studycorrectldef : studycorrectddef;
        case ColorSettings::StudyWrong:
            return colors.lighttheme ? studywrongldef : studywrongddef;
        case ColorSettings::StudyNew:
            return colors.lighttheme ? studynewldef : studynewddef;
        case ColorSettings::Inf:
            return colors.lighttheme ? infldef : infddef;
        case ColorSettings::Attrib:
            return colors.lighttheme ? attribldef : attribddef;
        case ColorSettings::Types:
            return colors.lighttheme ? typesldef : typesddef;
        case ColorSettings::Notes:
            return colors.lighttheme ? notesldef : notesddef;
        case ColorSettings::Fields:
            return colors.lighttheme ? fieldsldef : fieldsddef;
        case ColorSettings::Dialects:
            return colors.lighttheme ? dialectldef : dialectddef;
        case ColorSettings::KanaOnly:
            return colors.lighttheme ? kanaonlyldef : kanaonlyddef;
        case ColorSettings::SentenceWord:
            return colors.lighttheme ? sentencewordldef : sentencewordddef;
        case ColorSettings::KanjiExBg:
            return colors.lighttheme ? kanjiexbgldef : kanjiexbgddef;
        case ColorSettings::KanjiTestPos:
            return colors.lighttheme ? kanjitestposldef : kanjitestposddef;
        case ColorSettings::Oku:
            return colors.lighttheme ? okucolorldef : okucolorddef;
        case ColorSettings::N5:
            return colors.lighttheme ? n5ldef : n5ddef;
        case ColorSettings::N4:
            return colors.lighttheme ? n4ldef : n4ddef;
        case ColorSettings::N3:
            return colors.lighttheme ? n3ldef : n3ddef;
        case ColorSettings::N2:
            return colors.lighttheme ? n2ldef : n2ddef;
        case ColorSettings::N1:
            return colors.lighttheme ? n1ldef : n1ddef;
        case ColorSettings::KanjiNoWords:
            return colors.lighttheme ? nowordsldef : nowordsddef;
        case ColorSettings::KanjiUnsorted:
            return colors.lighttheme ? unsortedldef : unsortedddef;
        case ColorSettings::KanjiNoTranslation:
            return colors.lighttheme ? notranslationldef : notranslationddef;
        case ColorSettings::KataBg:
            return colors.lighttheme ? katabgldef : katabgddef;
        case ColorSettings::HiraBg:
            return colors.lighttheme ? hirabgldef : hirabgddef;
        case ColorSettings::SimilarText:
            return colors.lighttheme ? similartextldef : similartextddef;
        //case ColorSettings::SimilarBg:
        //    return qApp->style()->standardPalette().color(QPalette::Active, QPalette::Base); // colors.lighttheme ? similarbgldef : similarbgddef;
        //case ColorSettings::PartsBg:
        //    return qApp->style()->standardPalette().color(QPalette::Active, QPalette::Base); // colors.lighttheme ? partsbgldef : partsbgddef;
        //case ColorSettings::PartOfBg:
        //    return qApp->style()->standardPalette().color(QPalette::Active, QPalette::Base); // colors.lighttheme ? partofbgldef : partofbgddef;
        case ColorSettings::StrokeDot:
            return colors.lighttheme ? strokedotldef : strokedotddef;
        case ColorSettings::Stat1:
            return colors.lighttheme ? stat1ldef : stat1ddef;
        case ColorSettings::Stat2:
            return colors.lighttheme ? stat2ldef : stat2ddef;
        case ColorSettings::Stat3:
            return colors.lighttheme ? stat3ldef : stat3ddef;
        default:
            return QColor(0, 0, 0);
        }
    }

    void updatePalette(QWidget *w)
    {
        QPalette newpal = w->palette();
        newpal.setColor(QPalette::Active, QPalette::Base, textColor(true, ColorSettings::Bg));
        newpal.setColor(QPalette::Active, QPalette::Text, textColor(true, ColorSettings::Text));
        newpal.setColor(QPalette::Active, QPalette::Highlight, textColor(true, ColorSettings::SelBg));
        newpal.setColor(QPalette::Active, QPalette::HighlightedText, textColor(true, ColorSettings::SelText));
        newpal.setColor(QPalette::Inactive, QPalette::Base, textColor(false, ColorSettings::Bg));
        newpal.setColor(QPalette::Inactive, QPalette::Text, textColor(false, ColorSettings::Text));
        newpal.setColor(QPalette::Inactive, QPalette::Highlight, textColor(false, ColorSettings::SelBg));
        newpal.setColor(QPalette::Inactive, QPalette::HighlightedText, textColor(false, ColorSettings::SelText));
        newpal.setBrush(QPalette::Active, QPalette::Base, textColor(true, ColorSettings::Bg));
        newpal.setBrush(QPalette::Active, QPalette::Text, textColor(true, ColorSettings::Text));
        newpal.setBrush(QPalette::Active, QPalette::Highlight, textColor(true, ColorSettings::SelBg));
        newpal.setBrush(QPalette::Active, QPalette::HighlightedText, textColor(true, ColorSettings::SelText));
        newpal.setBrush(QPalette::Inactive, QPalette::Base, textColor(false, ColorSettings::Bg));
        newpal.setBrush(QPalette::Inactive, QPalette::Text, textColor(false, ColorSettings::Text));
        newpal.setBrush(QPalette::Inactive, QPalette::Highlight, textColor(false, ColorSettings::SelBg));
        newpal.setBrush(QPalette::Inactive, QPalette::HighlightedText, textColor(false, ColorSettings::SelText));
        w->setPalette(newpal);

        QList<QWidget*> ws = w->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *ww : ws)
            updatePalette(ww);
        //QAbstractScrollArea *sw = dynamic_cast<QAbstractScrollArea*>(w);
        //if (sw != nullptr && sw->viewport() != nullptr)
        //    updatePalette(sw->viewport());
    }

    double scaling()
    {
        return general.scale / 100.0 * _scaleratio;
    }

    int scaled(int siz)
    {
        return siz * general.scale / 100.0 * _scaleratio;
    }

    int scaled(double siz)
    {
        return siz * general.scale / 100.0 * _scaleratio;
    }

    QSize scaled(QSize siz)
    {
        return QSize(scaled(siz.width()), scaled(siz.height()));
    }

    QSizeF scaled(QSizeF siz)
    {
        return QSizeF(scaled(siz.width()), scaled(siz.height()));
    }

}

