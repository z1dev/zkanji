/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZKANJIWIDGET_H
#define ZKANJIWIDGET_H

#include <QtCore>
#include <QWidget>
#include <memory>

#include "zbasictimer.h"

class QImage;
enum class StrokeDirection;

class ZKanjiDiagram : public QWidget
{
    Q_OBJECT
signals:
    // Signaled when a new stroke begins to paint. Index is the 0 based index
    // of the next stroke to be painted. Ended is set to true when the last
    // stroke finished painting and playback stopped.
    void strokeChanged(int index, bool ended);
public:
    ZKanjiDiagram(QWidget *parent = nullptr);
    virtual ~ZKanjiDiagram();

    void setKanjiIndex(int newindex);
    // Index of currently displayed kanji or element. When displaying elements, this value is
    // negative. When no kanji nor element is set, INT_MIN is returned.
    int kanjiIndex() const;

    bool strokes() const;
    void setStrokes(bool setshow);

    bool radical() const;
    void setRadical(bool setshow);

    bool diagram() const;
    void setDiagram(bool setshow);

    bool grid() const;
    void setGrid(bool setshow);

    bool shadow() const;
    void setShadow(bool setshow);

    bool numbers() const;
    void setNumbers(bool setshow);

    bool direction() const;
    void setDirection(bool setshow);

    // The speed of the stroke drawing animation between 1 and 4.
    int speed() const;
    // Sets the speed of the stroke drawing animation between 1 and 4.
    void setSpeed(int val);

    bool playing() const;
    bool paused() const;

    // Whether playback starts over when the last stroke is drawn.
    bool looping() const;
    // Set whether playback starts over when the last stroke is drawn.
    void setLooping(bool val);

    void play();
    void pause();
    void stop();
    void rewind();
    void back();
    void next();
protected:
    virtual bool event(QEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void paintEvent(QPaintEvent *e) override;
private slots:
    void settingsChanged();
    void appStateChanged();
private:
    // Paints the missing strokes to image, creating or clearing it if
    // neccesary.
    void updateImage();

    // Calculates the draw rectangle of the stroke number indicator, based on the stroke's
    // initial direction and starting point, and adds it to numrects.
    void addNumberRect(int strokeix, StrokeDirection dir, QPoint pt);

    // Draws the filled rectangle of the stroke number indicator or the number itself. Set
    // numberdrawing to true to draw the number only and false to draw the rectangle only.
    void drawNumberRect(QPainter &p, int strokeix, bool numberdrawing);

    // Index of the displayed kanji.
    int kindex;

    // Whether showing the stroke order diagram instead of drawing the kanji with a font.
    bool showdiagram;

    bool showstrokes;
    bool showradical;
    bool showgrid;
    bool showshadow;
    bool shownumbers;
    bool showdir;
    int drawspeed;

    // Number of strokes that should be fully drawn.
    int strokepos;

    // The current image of the stroke order diagram. Contains the fully drawn strokes.
    std::unique_ptr<QImage> image;

    // Image of the finished parts of the stroke being drawn right now.
    std::unique_ptr<QImage> stroke;

    // Number of steps needed to animate the current/next stroke.
    int partcnt;

    // Next part to be drawn in the current stroke. This part is not drawn yet. When it's 0
    // nothing should be drawn, when 1 the part with index 0 is drawn etc. so pass
    // partpos - 1 to the drawing functions.
    int partpos;

    // Number of timer ticks to wait before starting the next stroke. This is set to a
    // sensible number between 1-4 when a stroke has been fully drawn and decremented
    // until it reaches 0, and a new stroke can be started.
    int delaycnt;

    // Result of strokePartCount() which must be passed to drawStrokePart().
    std::vector<int> parts;

    // [stroke starting point, number display rectangle] Coordinates where the stroke number
    // rectangles should be drawn. 
    std::vector<std::pair<QPoint, QRect>> numrects;

    ZBasicTimer timer;

    // Number of fully drawn strokes on image. If it's less than strokepos, only the missing
    // strokes are drawn on image before displaying it. If it's larger than strokepos, the
    // image is first cleared and every stroke is repainted.
    int drawnpos;

    // True when the stroke order diagram animation should repeat playing after it finished.
    bool loop;

    typedef QWidget base;
};


#endif // ZKANJIWIDGET_H
