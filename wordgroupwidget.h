/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDGROUPFORM_H
#define WORDGROUPFORM_H

#include <QMainWindow>

namespace Ui {
    class WordGroupWidget;
}

class QToolButton;
class GroupBase;
class WordGroup;
class QXmlStreamWriter;
class QXmlStreamReader;
class WordGroupWidget : public QWidget
{
    Q_OBJECT

public:
    WordGroupWidget(QWidget *parent = 0);
    ~WordGroupWidget();

    void saveXMLSettings(QXmlStreamWriter &writer) const;
    void loadXMLSettings(QXmlStreamReader &reader);

    //void makeModeSpace(const QSize &size);

    void setDictionary(int index);
private slots:
    //void on_groupWidget_currentItemChanged(GroupBase *current);
    void on_groupWidget_selectionChanged();
    void wordSelectionChanged();
    void on_showButton_clicked();
    void on_continueButton_clicked();
    void on_abortButton_clicked();
    void on_hideButton_clicked();
    void studyButtonClicked();
    void printButtonClicked();
    void delButtonClicked();
    void upButtonClicked();
    void downButtonClicked();
    void appWindowsVisibilityChanged(bool shown);
private:
    void updateStudyDisplay();

    Ui::WordGroupWidget *ui;

    QToolButton *studyButton;
    QToolButton *printButton;
    QToolButton *delButton;
    QToolButton *upButton;
    QToolButton *downButton;

    WordGroup *group;

    QIcon testpaperico;
    QIcon testsettingsico;

    typedef QWidget base;
};



#endif // WORDGROUPFORM_H
