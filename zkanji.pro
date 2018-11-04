#-------------------------------------------------
#
# Project created by QtCreator 2017-07-21T18:31:40
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets svg printsupport
greaterThan(QT_MAJOR_VERSION, 4): unix:QT += x11extras

TARGET = zkanji
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

debug:DEFINES += _DEBUG

unix:CONFIG += link_pkgconfig
unix:PKGCONFIG += xcb x11

SOURCES += \
    Qxt/qxtglobal.cpp \
    Qxt/qxtglobalshortcut.cpp \
    Qxt/qxtglobalshortcut_x11.cpp \
    bits.cpp \
    collectwordsform.cpp \
    definitionwidget.cpp \
    dialogwindow.cpp \
    dictionaryeditorform.cpp \
    dictionaryexportform.cpp \
    dictionaryimportform.cpp \
    dictionarystatsform.cpp \
    dictionarytextform.cpp \
    dictionarywidget.cpp \
    examplewidget.cpp \
    filterlistform.cpp \
    formstates.cpp \
    furigana.cpp \
    globalui.cpp \
    grammar.cpp \
    groupexportform.cpp \
    groupimportform.cpp \
    grouppickerform.cpp \
    groups.cpp \
    groupslegacy.cpp \
    groupstudy.cpp \
    groupstudylegacy.cpp \
    groupwidget.cpp \
    import.cpp \
    importreplaceform.cpp \
    jlptreplaceform.cpp \
    kanji.cpp \
    kanjidefform.cpp \
    kanjigroupwidget.cpp \
    kanjiinfoform.cpp \
    kanjilegacy.cpp \
    kanjireadingpracticeform.cpp \
    kanjisearchwidget.cpp \
    kanjistrokes.cpp \
    kanjitogroupform.cpp \
    kanjitooltipwidget.cpp \
    popupdict.cpp \
    popupkanjisearch.cpp \
    printpreviewform.cpp \
    qcharstring.cpp \
    radform.cpp \
    ranges.cpp \
    recognizerform.cpp \
    romajizer.cpp \
    searchtree.cpp \
    searchtreelegacy.cpp \
    selectdictionarydialog.cpp \
    sentences.cpp \
    settings.cpp \
    settingsform.cpp \
    sites.cpp \
    studydecks.cpp \
    studydeckslegacy.cpp \
    treebuilder.cpp \
    wordattribwidget.cpp \
    worddeck.cpp \
    worddeckform.cpp \
    worddecklegacy.cpp \
    wordeditorform.cpp \
    wordgroupwidget.cpp \
    words.cpp \
    wordslegacy.cpp \
    wordstudyform.cpp \
    wordstudylistform.cpp \
    wordtestresultsform.cpp \
    wordtestsettingsform.cpp \
    wordtodeckform.cpp \
    wordtodictionaryform.cpp \
    wordtogroupform.cpp \
    zabstracttablemodel.cpp \
    zabstracttreemodel.cpp \
    zcolorcombobox.cpp \
    zcombobox.cpp \
    zdictionariesmodel.cpp \
    zdictionarylistview.cpp \
    zdictionarymodel.cpp \
    zevents.cpp \
    zexamplestrip.cpp \
    zflowlayout.cpp \
    zgrouptreemodel.cpp \
    zitemscroller.cpp \
    zkanalineedit.cpp \
    zkanjidiagram.cpp \
    zkanjiform.cpp \
    zkanjigridmodel.cpp \
    zkanjigridview.cpp \
    zkanjimain.cpp \
    zkanjiwidget.cpp \
    zlineedit.cpp \
    zlistbox.cpp \
    zlistboxmodel.cpp \
    zlistview.cpp \
    zlistviewitemdelegate.cpp \
    zproxytablemodel.cpp \
    zradicalgrid.cpp \
    zscrollarea.cpp \
    zstackedwidget.cpp \
    zstrings.cpp \
    zstudylistmodel.cpp \
    ztooltip.cpp \
    ztreeview.cpp \
    zui.cpp \
    zwindow.cpp \
    main.cpp \
    kanapracticesettingsform.cpp \
    kanareadingpracticeform.cpp \
    kanawritingpracticeform.cpp \
    stayontop_x11.cpp \
    zstatview.cpp \
    zdictionarycombobox.cpp \
    zstatusbar.cpp \
    languages.cpp \
    languageform.cpp

HEADERS += \
    Qxt/qxtglobal.h \
    Qxt/qxtglobalshortcut.h \
    Qxt/qxtglobalshortcut_p.h \
    bits.h \
    collectwordsform.h \
    colorsettings.h \
    datasettings.h \
    definitionwidget.h \
    dialogs.h \
    dialogsettings.h \
    dialogwindow.h \
    dictionaryeditorform.h \
    dictionaryexportform.h \
    dictionaryimportform.h \
    dictionarysettings.h \
    dictionarystatsform.h \
    dictionarytextform.h \
    dictionarywidget.h \
    examplewidget.h \
    fastarray.h \
    filterlistform.h \
    fontsettings.h \
    formstates.h \
    furigana.h \
    generalsettings.h \
    globalui.h \
    grammar.h \
    grammar_enums.h \
    groupexportform.h \
    groupimportform.h \
    grouppickerform.h \
    groups.h \
    groupsettings.h \
    groupstudy.h \
    groupwidget.h \
    import.h \
    importreplaceform.h \
    jlptreplaceform.h \
    kanji.h \
    kanjidefform.h \
    kanjigroupwidget.h \
    kanjiinfoform.h \
    kanjireadingpracticeform.h \
    kanjisearchwidget.h \
    kanjisettings.h \
    kanjistrokes.h \
    kanjitogroupform.h \
    kanjitooltipwidget.h \
    popupdict.h \
    popupkanjisearch.h \
    popupsettings.h \
    printpreviewform.h \
    printsettings.h \
    qcharstring.h \
    radform.h \
    ranges.h \
    recognizerform.h \
    recognizersettings.h \
    resource.h \
    romajizer.h \
    searchtree.h \
    selectdictionarydialog.h \
    sentences.h \
    settings.h \
    settingsform.h \
    shortcutsettings.h \
    sites.h \
    smartvector.h \
    studydecks.h \
    studysettings.h \
    treebuilder.h \
    wordattribwidget.h \
    worddeck.h \
    worddeckform.h \
    wordeditorform.h \
    wordgroupwidget.h \
    words.h \
    wordstudyform.h \
    wordstudylistform.h \
    wordtestresultsform.h \
    wordtestsettingsform.h \
    wordtodeckform.h \
    wordtodictionaryform.h \
    wordtogroupform.h \
    zabstracttablemodel.h \
    zabstracttreemodel.h \
    zcolorcombobox.h \
    zcombobox.h \
    zdictionariesmodel.h \
    zdictionarylistview.h \
    zdictionarymodel.h \
    zevents.h \
    zexamplestrip.h \
    zflowlayout.h \
    zgrouptreemodel.h \
    zitemscroller.h \
    zkanalineedit.h \
    zkanjidiagram.h \
    zkanjiform.h \
    zkanjigridmodel.h \
    zkanjigridview.h \
    zkanjimain.h \
    zkanjiwidget.h \
    zlineedit.h \
    zlistbox.h \
    zlistboxmodel.h \
    zlistview.h \
    zlistviewitemdelegate.h \
    zproxytablemodel.h \
    zradicalgrid.h \
    zscrollarea.h \
    zstackedwidget.h \
    zstrings.h \
    zstudylistmodel.h \
    ztooltip.h \
    ztreeview.h \
    zui.h \
    zwindow.h \
    kanapracticesettingsform.h \
    kanareadingpracticeform.h \
    kanawritingpracticeform.h \
    stayontop_x11.h \
    zabstractstatmodel.h \
    zstatview.h \
    zdictionarycombobox.h \
    zstatusbar.h \
    languages.h \
    languagesettings.h \
    languageform.h \
	checked_cast.h

FORMS += \
    collectwordsform.ui \
    definitionwidget.ui \
    dictionaryeditorform.ui \
    dictionaryexportform.ui \
    dictionaryimportform.ui \
    dictionarystatsform.ui \
    dictionarytextform.ui \
    dictionarywidget.ui \
    examplewidget.ui \
    filterlistform.ui \
    groupexportform.ui \
    groupimportform.ui \
    grouppickerform.ui \
    groupwidget.ui \
    import.ui \
    importreplaceform.ui \
    jlptreplaceform.ui \
    kanjidefform.ui \
    kanjigroupwidget.ui \
    kanjiinfoform.ui \
    kanjireadingpracticeform.ui \
    kanjisearchwidget.ui \
    kanjitogroupform.ui \
    popupdict.ui \
    popupkanjisearch.ui \
    printpreviewform.ui \
    radform.ui \
    recognizer.ui \
    selectdictionarydialog.ui \
    settingsform.ui \
    wordattribwidget.ui \
    worddeckform.ui \
    wordeditorform.ui \
    wordgroupwidget.ui \
    wordstudyform.ui \
    wordstudylistform.ui \
    wordtestresultsform.ui \
    wordtestsettingsform.ui \
    wordtodeckform.ui \
    wordtodictionaryform.ui \
    wordtogroupform.ui \
    zkanjiform.ui \
    zkanjiwidget.ui \
    kanareadingpracticeform.ui \
    kanawritingpracticeform.ui \
    kanapracticesettingsform.ui \
    languageform.ui

DISTFILES +=

RESOURCES += \
    Resources/resources.qrc

TRANSLATIONS = \
	lang/zkanji.ts \
	lang/zkanji_hu.ts
