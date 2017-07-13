#ifndef ZSTACKEDWIDGET_H
#define ZSTACKEDWIDGET_H

#include <QStackedWidget>

// Modification of QStackedWidget to allow the minimum/maximum and hint sizes to match the
// currently visible widget only.
class ZStackedWidget : public QStackedWidget
{
    Q_OBJECT
signals:
    void currentChanged(int index);
public:
    ZStackedWidget(QWidget *parent = nullptr);
    ~ZStackedWidget();

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int width) const override;
private:
    using QStackedWidget::currentChanged;

    void handleCurrentChanged(int index);

    typedef QStackedWidget  base;
};

#endif // ZSTACKEDWIDGET_H
