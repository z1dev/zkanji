#ifndef KANJIREADINGPRACTICEFORM_H
#define KANJIREADINGPRACTICEFORM_H

#include <QMainWindow>

namespace Ui {
    class KanjiReadingPracticeForm;
}

class WordDeck;
class DictionaryWordListItemModel;
class KanjiReadingPracticeForm : public QMainWindow
{
    Q_OBJECT
public:
    KanjiReadingPracticeForm(WordDeck *deck, QWidget *parent = nullptr);
    ~KanjiReadingPracticeForm();

    void exec();
private slots:
    void answerChanged();
    void readingAccepted();
    void nextClicked();
private:
    // Updates the labes, buttons etc. to show the next reading.
    void initNextRound();

    Ui::KanjiReadingPracticeForm *ui;
    WordDeck *deck;

    int tries;

    DictionaryWordListItemModel *model;

    typedef QMainWindow base;
};


#endif // KANJIREADINGPRACTICEFORM_H

