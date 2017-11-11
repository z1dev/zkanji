#ifndef ZABSTRACTSTATMODEL_H
#define ZABSTRACTSTATMODEL_H

#include <QObject>

class ZStatView;
class ZAbstractStatModel : public QObject
{
    Q_OBJECT
public:
    ZAbstractStatModel(QObject *parent = nullptr) : base(parent) { ; }
    virtual ~ZAbstractStatModel() {}

    // Number of data columns in the model.
    virtual int count() const = 0;
    // Pixel width of a bar at column col. This should be enough to draw the text under this
    // bar column.
    virtual int barWidth(ZStatView *view, int col) const = 0;

    // The highest value on the vertical axis.
    virtual int maxValue() const = 0;

    // The lable to be drawn on the axis of the given orientation. If an empty string is
    // returned, the space for the label won't be reserved.
    virtual QString axisLabel(Qt::Orientation ori) const = 0;

    // The label draw below a specific bar.
    virtual QString barLabel(int col) const = 0;

    // Number of values to be shown at a single bar position.
    virtual int valueCount() const = 0;
    // A value at the passed column, at the given value position. The value should refer to
    // the size of the individual bar parts.
    virtual int value(int col, int valpos) const = 0;
private:
    typedef QObject base;
};

#endif // ZABSTRACTSTATMODEL_H
