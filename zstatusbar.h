#ifndef ZSTATUSBAR_H
#define ZSTATUSBAR_H

#include <QStatusBar>
#include <QLayout>
#include <vector>

// Type of status widgets added to the status bar.
// * TitleValue : A title label and a value label.
// * TitleDouble : A title label and two value labels next to each other.
// * DoubleValue : No title label and two value labels next to each other.
// * SingleValue : A single value label that can extend horizontally.
enum class StatusTypes : int { TitleValue, TitleDouble, DoubleValue, SingleValue };

class ZStatusLayout : public QLayout
{
    Q_OBJECT
public:
    ZStatusLayout(QWidget *parent);
    virtual ~ZStatusLayout();

    virtual void addItem(QLayoutItem *item) override;

    virtual QLayoutItem* itemAt(int index) const override;
    virtual QLayoutItem* takeAt(int index) override;
    virtual int count() const override;
    virtual QSize sizeHint() const override;
    virtual QSize minimumSize() const override;

    virtual void invalidate() override;
    virtual void setGeometry(const QRect &r) override;
private:
    QSize minimumSize();

    // Helper function for recompute(), for computing the vertical and horizontal spacing
    // between items.
    void computeSpacing(QLayoutItem *first, QLayoutItem *next, int &spacew, int hs, const QStyle *style);

    // Moves the widgets to their correct positions on the parent widget.
    void realign(const QRect &r);

    QList<QLayoutItem*> list;

    mutable int cacheheight;

    typedef QLayout base;
};

class ZStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    ZStatusBar(QWidget *parent = nullptr);
    ~ZStatusBar();

    // Number of widgets added to the status bar.
    int size() const;

    // Returns true if the size() is 0 and no widgets were added to the status bar.
    bool empty() const;

    // Deletes all previously added widgets from the status bar.
    void clear();

    // Adds a widget to the status bar with a single value label. The label's size is
    // restricted to the passed number of characters. Pass 0 to vsiz to make it extening
    // (mainly when adding the last label that can have a generic long text.)
    // Returns the index of the widget that can be used in latter calls of setValue().
    int add(QString value, int vsiz);

    // Adds a widget to the status bar with a title and a value label. The labels' sizes are
    // restricted to the passed number of characters. Pass 0 to sizes to make them fixed size
    // which depends on the label texts. Set alignright to true to align the text in the value
    // label to the right.
    // Returns the index of the widget that can be used in latter calls of setValue().
    int add(QString title, int lsiz, QString value, int vsiz, bool alignright = false);

    // Changes the text of a value label in the widget added at index position when there was
    // a single value label.
    void setValue(int index, QString str);
    // Returns the text set for the value of the widget at index position when there was a
    // single value label.
    QString value(int index) const;

    // Adds a widget to the status bar with a title, and two value labels. The labels' sizes
    // are restricted to the passed number of characters. Pass 0 to sizes to make them fixed
    // size which depends on the label texts.
    // The first of the two values is always aligned right while the second is left aligned.
    // The spacing between them is smaller than between the title and the first value.
    // Returns the index of the widget that can be used in latter calls of setValues().
    // When passing an empty string for title, no label will be added in its place.
    int add(QString title, int lsiz, QString value1, int vsiz1, QString value2, int vsiz2);

    // Changes the text of the two value labels on the widget at the index position. Only
    // valid if the widget was added by the two values override of add().
    void setValues(int index, QString val1, QString val2);
protected:
    virtual void showEvent(QShowEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
private:
    void addWidget(QWidget *w);

    // Widgets added to the status bar.
    std::vector<std::pair<StatusTypes, QWidget*>> list;

    typedef QStatusBar  base;
};


#endif // ZSTATUSBAR_H
