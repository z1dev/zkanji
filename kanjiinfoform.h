/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJIINFOFORM_H
#define KANJIINFOFORM_H

#include <QtCore>
#include <QMainWindow>
#include <QComboBox>
#include <QToolButton>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <memory>
#include "fastarray.h"
#include "zwindow.h"

namespace Ui {
    class KanjiInfoForm;
}

class Dictionary;
class QLabel;
class KanjiScrollerModel;
class SimilarKanjiScrollerModel;
class KanjiWordsItemModel;
struct KanjiInfoData;
struct KanjiInfoFormData;

class KanjiInfoForm : public ZWindow
{
    Q_OBJECT
signals:
    void formLock(bool locked);
public:
    KanjiInfoForm(QWidget *parent = nullptr);
    ~KanjiInfoForm();

    void saveXMLSettings(QXmlStreamWriter &writer);
    bool loadXMLSettings(QXmlStreamReader &reader);

    void saveState(KanjiInfoData &data) const;
    void restoreState(const KanjiInfoData &data);

    void saveState(KanjiInfoFormData &data) const;
    bool restoreState(const KanjiInfoFormData &data);

    // Changes the displayed kanji in the form. Use popup() instead if the form is not yet
    // visible to show the kanji.
    void setKanji(Dictionary *d, int kindex);

    // Shows the info form with the passed kanji with information from the d dictionary.
    void popup(Dictionary *d, int kindex);

    virtual QRect resizing(int side, QRect r) override;
    virtual QWidget* captionWidget() const override;

    bool locked() const;

    void copy() const;
    void append() const;
    void copyData() const;
    void addToGroup() const;
    //void appendData() const;
protected:
    virtual bool event(QEvent *e) override;
    virtual bool eventFilter(QObject *o, QEvent *e) override;

    virtual void contextMenuEvent(QContextMenuEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void hideEvent(QHideEvent *e) override;
private:
    void translateUI();

    // Creates labels with kanji reference numbers and puts them in the reference layout.
    //void createRefLabels();
    // Refreshes the displayed reference texts to match the current kanji.
    //void updateRefLabels();

    // Called in the resizeEvent or when showing/hiding elements that can change the size of
    // the kanjiView object. Set post to true to handle the resize after other events.
    void _resized(bool post = false);
    // Sets the size of the kanjiview to have the same width and height.
    void restrictKanjiViewSize();
private slots:
    void on_loopButton_clicked(bool checked);
    void on_playButton_clicked(bool checked);
    void on_pauseButton_clicked(bool checked);
    void on_stopButton_clicked(bool checked);
    void on_rewindButton_clicked();
    void on_backButton_clicked();
    void on_foreButton_clicked();
    //void on_endButton_clicked();

    void on_sodButton_clicked(bool checked);

    void on_gridButton_clicked(bool checked);
    void on_shadowButton_clicked(bool checked);
    void on_numberButton_clicked(bool checked);
    void on_dirButton_clicked(bool checked);
    void on_speedSlider_valueChanged(int value);

    //void on_refButton_clicked(bool checked);
    void on_simButton_clicked(bool checked);
    void on_partsButton_clicked(bool checked);
    void on_partofButton_clicked(bool checked);
    void on_wordsButton_clicked(bool checked);

    void on_lockButton_clicked(bool checked);
    void disableUnlock();

    void strokeChanged(int index, bool ended);

    // Called when toggling the button to show only kanji example words.
    void readingFilterClicked(bool checked);
    // Called when the current index in readingCBox changes.
    void readingBoxChanged(int index);
    // Called when toggling the button for selecging/deselecting words as examples for kanji.
    void exampleSelButtonClicked(bool checked);

    void scrollerClicked(int index);

    void settingsChanged();

    void dictionaryReset();
    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);

    void wordSelChanged();
private:
    // Reads an SVG image file as text, replacing the black color (#000000) with the passed
    // color, and returning it as an icon of size siz.
    QIcon loadColorIcon(QString name, QColor col, QSize siz);

    // Displays the form's context menu at the specified global position.
    void showContextMenu(QPoint pos);

    Ui::KanjiInfoForm *ui;

    // Toggle button for only showing selected example words for kanji.
    QToolButton *readingFilterButton;
    // List of kanji readings to limit the displayed words.
    QComboBox *readingCBox;
    QToolButton *exampleSelButton;

    Dictionary *dict;
    fastarray<QLabel*> refLabels;

    // Stops handling resize events while true. When panels are shown or hidden the kanji
    // display would change size twice. This value is used to prevent that.
    int ignoreresize;

    // Saved height of the words listing. Widgets in splitters never have any size while they
    // are hidden, so this value must be saved separately.
    int dicth;

    std::unique_ptr<SimilarKanjiScrollerModel> simmodel;
    std::unique_ptr<KanjiScrollerModel> partsmodel;
    std::unique_ptr<KanjiScrollerModel> partofmodel;

    std::unique_ptr<KanjiWordsItemModel> wordsmodel;

    typedef ZWindow base;
};

#endif // KANJIINFOFORM_H
