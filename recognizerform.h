/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef RECOGNIZERFORM_H
#define RECOGNIZERFORM_H

#include <QMainWindow>
#include <qframe.h>
#include <qelapsedtimer.h>
#include "zwindow.h"
#include "kanjistrokes.h"
#include "zitemscroller.h"


namespace Ui {
    class RecognizerForm;
}

class QStylePainter;
struct RecognizerFormData;
class RecognizerArea : public QFrame
{
    Q_OBJECT

        signals :
    // Signals that the list of recognized characters was updated, and passes the list of
    // element indexes of the recognized characters in l.
    void changed(const std::vector<int> &l);
public:
    RecognizerArea(QWidget *parent = nullptr);
    ~RecognizerArea();

    bool empty() const;
    bool hasNext() const;

    bool showGrid();
    void setShowGrid(bool showgrid);

    enum CharacterMatch { Kanji = 0x01, Kana = 0x02, Other = 0x04 };
    Q_DECLARE_FLAGS(CharacterMatches, CharacterMatch);

    // Whether to list kana and other characters in the results not just kanji.
    CharacterMatches characterMatch();
    // Set whether to list kana and other characters in the results not just kanji.
    void setCharacterMatch(CharacterMatches charmatch);

    // Like an undo, removes the last stroke from the area and updates the results.
    void prev();
    // Like a redo, adds back a removed stroke and updates the results.
    void next();

    // Clears the draw area, resetting any drawing and removing recognized strokes.
    void clear();
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
private:
    // Called when the user is drawing a stroke in the recognizer area
    // for every new mouse position.
    void draw(QPointF pt);
    // Called when the user released the mouse when drawing a stroke.
    void stopDrawing();

    // Recomputes the candidate list and emits a change signal.
    void updateCandidates();

    // Paints a stroke with the color in the draw area using the painter.
    // Pass -1 to paint the current stroke with varied line widths. Segments
    // of the stroke outside the clip rectangle are not drawn.
    void paintStroke(int index, QStylePainter &p, QColor color, const QRect &clip);

    // Number of segments int the stroke at index. Pass -1 to get the value
    // for the currently drawn stroke.
    int segmentCount(int index);
    // Returns the length of seg segment of index stroke, when drawing it
    // in a unit sized area. Pass -1 for index to get the value for the
    // currently drawn stroke. If the segment is -1, the value of the last
    // segment is returned.
    double segmentLength(int index, int seg);
    // Returns the line of seg segment of index stroke, when drawing it
    // in a unit sized area. Pass -1 for index to get the value for the
    // currently drawn stroke. If the segment is -1, the value of the last
    // segment is returned.
    QLineF segmentLine(int index, int seg);
    // Returns the width of seg segment of index stroke, when drawing it
    // in a unit sized area. Pass -1 for index to get the value for the
    // currently drawn stroke. If seg is -1, the value of the last segment
    // is returned.
    double segmentWidth(int index, int seg);
    // Returns the bounding rectangle of seg segment of index stroke, when
    // drawing it in a unit sized area. Pass -1 for index to get the value
    // for the currently drawn stroke. If the segment is -1, the value of the
    // last segment is returned.
    QRectF segmentRect(int index, int seg);

    // Computes the area size and font size. If no recalculation is needed
    // immediately returns.
    void recalculate();

    // Returns the string of instructions when nothing is drawn yet.
    QString instructions();

    // Takes the point in area coordinates and transforms it to unit
    // coordinates that are between 0 and 1. The area point must be
    // translated to an area at 0,0. 
    QPointF areaToUnit(QPointF pt);
    // Takes the point in unit coordinates that are between 0 and 1 and
    // transforms it to area coordinates, if the area is at 0,0.
    QPointF unitToArea(QPointF pt);

    // Show the visual guide grid or not.
    bool grid;
    // Type of characters to list among the candidates.
    CharacterMatches match;

    // The area in the widget where the user can paint, and the grid is
    // displayed. Only the background is painted outside of it.
    // The area always takes up a square in the middle of the widget of
    // the largest size that fits.
    QRect area;

    // Set to true when the area size changed.
    bool recalc;
    // Size of font used for showing instructions in an empty recognizer.
    int ptsize;

    // The user is currently drawing a stroke.
    bool drawing;

    // Index of next stroke to be drawn. This can be less than the real size of strokes, if
    // prev() was called.
    int strokepos;

    // Points currently being drawn.
    Stroke newstroke;

    StrokeList strokes;

    typedef QFrame base;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RecognizerArea::CharacterMatches)

// Handler for installed button and edit box signals, as the RecognizerForm
// must be destroyed every time it's hidden. (Handle with care. Should be
// private in the RecognizerForm scope but the Qt MOC doesn't allow that.)
class RecognizerObject : public QObject
{
    Q_OBJECT
protected:
    virtual bool event(QEvent *e) override;
    virtual bool eventFilter(QObject *obj, QEvent *e) override;
    private slots:
    void buttonClicked();
    void buttonDestroyed();
private:
    typedef QObject base;

    friend class RecognizerForm;
};

// The recognizer window is positioned according to a value of this enum, unless it's
// explicitly specified in the settings to save the last position and use that.
// Below: Position the window below the button on popup, unless there's no
//        available space left. In that case position above.
// Above: Position the window above the button on popup, unless there's no
//        available space left. In that case position below.
// StartBelow: The first time the window is shown, position it like Below was
//        used. This value will be changed to SavedPos after that.
// StartAbove: The first time the window is shown, position it like Above was
//        used. This value will be changed to SavedPos after that.
// SavedPos: Use the last saved rectangle of the recognizer.
enum class RecognizerPosition { Below, Above, StartBelow, StartAbove, SavedPos };


class QToolButton;
class ZKanaLineEdit;
class ZKanaComboBox;
class CandidateKanjiScrollerModel;
class RecognizerForm : public ZWindow
{
    Q_OBJECT
public:
    // Installs the recognizer window for a passed button and edit box. Once installed,
    // pressing the button, or the space key in the edit box will pop-up the recognizer
    // window. Pass the window position in settings. If settings is nullptr, the global
    // settings will be used. When the user moves the window, the rect in the settings will be
    // updated accordingly.
    static void install(QToolButton *btn, ZKanaLineEdit *edit, RecognizerPosition pos = RecognizerPosition::Above);
    static void uninstall(QToolButton *btn);

    // Clears the current drawing from the recognizer area, if the RecognizerForm instance
    // exists.
    static void clear();

    static void popup(QToolButton *btn);
public slots:
    void on_gridButton_toggled(bool checked);
    void on_generalButton_toggled(bool checked);
    void on_clearButton_clicked();
    void on_prevButton_clicked();
    void on_nextButton_clicked();
    void candidatesChanged(const std::vector<int> &l);
    void candidateClicked(int index);
    void editorDictionaryChanged();
    void appFocusChanged(QWidget *lost, QWidget *received);
    void appStateChanged(Qt::ApplicationState state);
protected:
    virtual QRect resizing(int side, QRect r) override;

    virtual QWidget* captionWidget() const override;

    virtual bool event(QEvent *e) override;
    virtual void hideEvent(QHideEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void keyReleaseEvent(QKeyEvent *e) override;
    virtual void moveEvent(QMoveEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
private:
    RecognizerForm(QWidget *parent = nullptr);
    ~RecognizerForm();

    static void douninstall(QToolButton *btn, bool destroyed);

    void savePosition();

    Ui::RecognizerForm *ui;

    // Only object created of this class. Only not null while the form is visible.
    static RecognizerForm *instance;

    // Handler for installed button and edit box signals. Valid after the first install call.
    static RecognizerObject *p;

    // Button, edit box and settings installed for use with the recognizer
    // window.
    static std::map<QToolButton*, std::pair<ZKanaLineEdit*, RecognizerPosition>> controls;
    // Mapping edit box to buttons installed in controls. Symmetric with the controls map.
    static std::map<ZKanaLineEdit*, QToolButton*> editforbuttons;

    static CandidateKanjiScrollerModel *candidates;

    // The button that was pressed to show the form.
    static QToolButton *button;
    // The line edit installed with the pressed button that showed the form.
    static ZKanaLineEdit *edit;

    // The editor which has a dictionary changed slot currently connected to the recognizer.
    static ZKanaLineEdit *connected;

    typedef ZWindow base;

    friend class RecognizerObject;
};

// Sets up the button and text edit box to use the global recognizer window.
// When the button is pressed the window will be shown and will respond to
// user input, listing possible character candidates. If the user selects one
// candidate it'll be inserted in the text edit box at the current position.
// Pressing the button again will hide the editor.
void installRecognizer(QToolButton *btn, ZKanaLineEdit *edit, RecognizerPosition pos = RecognizerPosition::Above);
void uninstallRecognizer(QToolButton *btn, ZKanaLineEdit *edit);

void installRecognizer(QToolButton *btn, ZKanaComboBox *edit, RecognizerPosition pos = RecognizerPosition::Above);
void uninstallRecognizer(QToolButton *btn, ZKanaComboBox *edit);




#endif // RECOGNIZERFORM_H
