/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef EXAMPLEWIDGET_H
#define EXAMPLEWIDGET_H

#include <QWidget>
#include <QBasicTimer>

namespace Ui {
    class ExampleWidget;
}

class Dictionary;
struct ExampleSentenceData;
class QXmlStreamWriter;
class QXmlStreamReader;
class QToolButton;

enum class ExampleDisplay : uchar;
class ExampleWidget : public QWidget
{
    Q_OBJECT
signals:
    void wordSelected(ushort block, uchar line, int wordpos, int wordform);
public:
    ExampleWidget(QWidget *parent = nullptr);
    ~ExampleWidget();

    //// Saves widget state to the passed xml stream.
    //void saveXMLSettings(QXmlStreamWriter &writer) const;
    //// Loads widget state from the passed xml stream.
    //void loadXMLSettings(QXmlStreamReader &reader);

    // Notifies the example strip that the examples for the passed dictionary and word index
    // should be shown.
    void setItem(Dictionary *d, int windex);

    // Calling setItem() will have no effect until unlock is called.
    void lock();

    ExampleDisplay displayed() const;
    void setDisplayed(ExampleDisplay disp);

    // Whether changing currently shown word by clicking inside the text area is allowed.
    bool hasInteraction() const;
    // Allow or deny changing currently shown word by clicking inside the text area.
    void setInteraction(bool allow);

    // Allows changes to the displayed sentence after calling lock(). If the dictionary and
    // word indexes are specified, tries to update the display of the current sentence to use
    // that word as a base. If the current sentence is not found for that word, equals to
    // calling this without arguments and then calling setItem().
    void unlock(Dictionary *d = nullptr, int windex = -1, int wordpos = -1, int wordform = -1);

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    bool event(QEvent *e) override;
private slots:
    void onReset();
    void stripChanged();
    void on_jptrButton_clicked(bool checked);

    void on_prevButton_clicked();
    void on_nextButton_clicked();
    void on_indexEdit_textEdited(const QString &text);
    void on_linkButton_clicked(bool checked);
private:
    Ui::ExampleWidget *ui;

    // When set calling setItem or any other function won't change the
    // displayed sentence. Call lock() to set and unlock() to unset.
    bool locked;

    // Used for auto repeat right-clicks.
    QBasicTimer repeattimer;
    // The button being down by the mouse.
    QToolButton *repeatbutton;

    typedef QWidget base;
};

#endif // EXAMPLEWIDGET_H
