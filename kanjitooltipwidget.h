#ifndef KANJITOOLTIPWIDGET_H
#define KANJITOOLTIPWIDGET_H

#include <QWidget>

namespace Ui {
    class KanjiToolTipWidget;
}

class KanjiEntry;
class Dictionary;
class KanjiToolTipWidget : public QWidget
{
    Q_OBJECT
public:
    KanjiToolTipWidget(QWidget *parent = nullptr);
    virtual ~KanjiToolTipWidget();

    void setKanji(Dictionary *d, int kindex);

    virtual QSize sizeHint() const override;
    virtual void paintEvent(QPaintEvent *e) override;
private:
    void recalculateSize();

    Ui::KanjiToolTipWidget *ui;

    const int hpad;
    const int vpad;
    const int penw;

    Dictionary *dict;
    int index;

    QSize hint;

    typedef QWidget base;
};

#endif // KANJITOOLTIPWIDGET_H
