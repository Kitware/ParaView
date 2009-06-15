/*****************************************************************************
*
* Copyright (c) 2000 - 2007, The Regents of the University of California
* Produced at the Lawrence Livermore National Laboratory
* All rights reserved.
*
* This file is part of VisIt. For details, see http://www.llnl.gov/visit/. The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or materials provided with the distribution.
*  - Neither the name of the UC/LLNL nor  the names of its contributors may be
*    used to  endorse or  promote products derived from  this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
* CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
* ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

#include <cstdlib> // for qsort
#include <qpainter.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qtimer.h>
#include <QEvent>
#include <QKeyEvent>
#include <QPalette>
#include <QPolygon>
#include <QMatrix>
#include <QStyleOption>
#include "QvisSpectrumBar.h"

// Some constants for paging modes.
#define NO_PAGING      -1
#define INCREMENT       0
#define DECREMENT       1
#define PAGE_INCREMENT  2
#define PAGE_DECREMENT  3
#define PAGE_HOME       4
#define PAGE_END        5

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// Declarations of internally used classes and structs.
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct
{
    int   rank;
    float position;
    float color[3];
} ControlPoint;

static int ControlPointCompare(const void *c1, const void *c2);

// ****************************************************************************
// Class: ControlPointList
//
// Purpose:
//   This is a "private" class that contains the control point list used in
//   the QvisSpectrumBar class.
//
// Notes:
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:00:33 PDT 2001
//
// Modifications:
//
// ****************************************************************************

class ControlPointList
{
public:
    ControlPointList();
    ~ControlPointList();
    void  Add(const ControlPoint *cpt);
    bool  CanBeEdited() const;
    int   ChangeSelectedIndex(float pos, float width, int equal);
    void  Clear();
    float ColorValue(int index) const;
    void  DeleteHighestRank();
    void  GiveHighestRank(int index);
    int   NumColorValues() const;
    int   NumControlPoints() const;
    float Position(int index) const;
    int   Rank(int rank) const;
    void  SetColor(int index, float r, float g, float b);
    void  SetColorValues(const float *cv, int n);
    void  SetEditMode(bool val);
    void  SetPosition(int index, float pos);
    void  Sort();

    const ControlPoint &operator [](int index) const;
private:
    static const int CPLIST_INCREMENT;
    static const ControlPoint defaultControlPoint1;
    static const ControlPoint defaultControlPoint2;

    bool         editable;
    int          nels;
    int          total_nels;
    int          nvals;
    ControlPoint *list;
    float        *colorvals;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// QvisSpectrumBar implementation
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: QvisSpectrumBar::QvisSpectrumBar
//
// Purpose:
//   Constructor for the QvisSpectrumBar class.
//
// Arguments:
//   parent : The widget's parent.
//   name   : The name of the widget instance.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:01:21 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 28 10:15:47 PDT 2001
//   Initialized a new bool that allows updates to be suppressed.
//
//   Brad Whitlock, Wed Mar 13 10:09:38 PDT 2002
//   Made it use a single pixmap.
//
// ****************************************************************************

QvisSpectrumBar::QvisSpectrumBar(QWidget *parent, const char* /*name*/) :
    QWidget(parent)
{
    pixmap = 0;

    orientation = HorizontalTop;
    b_smoothing = true;
    b_equalSpacing = false;
    b_sliding = false;
    b_continuousUpdate = false;
    b_suppressUpdates = false;
    margin = 4;
    paging_mode = NO_PAGING;
    shiftApplied = false;

    controlPoints = new ControlPointList;

    // Create a timer used for paging/incrementing/decrementing.
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(handlePaging()));

    // Set the focus policy to StrongFocus. This means that the widget will
    // accept focus by tabbing and clicking.
    setFocusPolicy(Qt::StrongFocus);

    // Set the widget's minimum width and height.
    setMinimumWidth(50);
    setMinimumHeight(60);
}

// ****************************************************************************
// Method: QvisSpectrumBar::~QvisSpectrumBar
//
// Purpose:
//   The destructor for the QvisSpectrumBar class.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:07:23 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 10:09:38 PDT 2002
//   Made it use a single pixmap.
//
// ****************************************************************************

QvisSpectrumBar::~QvisSpectrumBar()
{
    deletePixmap();

    delete controlPoints;
}

// ****************************************************************************
// Method: QvisSpectrumBar::sizeHint
//
// Purpose:
//   Returns the widget's preferred size.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:07:43 PDT 2001
//
// Modifications:
//
// ****************************************************************************

QSize
QvisSpectrumBar::sizeHint() const
{
    QSize retval(250, 100);

    if(orientation == VerticalLeft || orientation == VerticalRight)
        retval = QSize(100, 250);

    return retval;
}

// ****************************************************************************
// Method: QvisSpectrumBar::sizePolicy
//
// Purpose:
//   Returns the widget's size policy. This is how the widget prefers to grow.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:25:27 PDT 2001
//
// Modifications:
//
// ****************************************************************************

QSizePolicy
QvisSpectrumBar::sizePolicy() const
{
    QSizePolicy retval(QSizePolicy::Preferred, QSizePolicy::Fixed);

    if(orientation == VerticalLeft || orientation == VerticalRight)
        retval = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    return retval;
}

// ****************************************************************************
// Method: QvisSpectrumBar::equalSpacing
//
// Purpose:
//   Returns a flag indicating whether or not the widget's control points are
//   in equal spacing mode.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:26:04 PDT 2001
//
// Modifications:
//
// ****************************************************************************

bool
QvisSpectrumBar::equalSpacing() const
{
    return b_equalSpacing;
}

// ****************************************************************************
// Method: QvisSpectrumBar::smoothing
//
// Purpose:
//   Returns a flag indicating whether or not the colors are interpolated
//   between control points.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:26:53 PDT 2001
//
// Modifications:
//
// ****************************************************************************

bool
QvisSpectrumBar::smoothing() const
{
    return b_smoothing;
}

// ****************************************************************************
// Method: QvisSpectrumBar::continuousUpdate
//
// Purpose:
//   Returns a flag indicating whether or not update signals are emitted during
//   mouse dragging of control points.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:27:47 PDT 2001
//
// Modifications:
//
// ****************************************************************************

bool
QvisSpectrumBar::continuousUpdate() const
{
    return b_continuousUpdate;
}

// ****************************************************************************
// Method: QvisSpectrumBar::controlPointColor
//
// Purpose:
//   Returns the color of a control point.
//
// Arguments:
//   index : The control point index starting at 0.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:28:35 PDT 2001
//
// Modifications:
//
// ****************************************************************************

QColor
QvisSpectrumBar::controlPointColor(int index) const
{
    if(index >= 0 && index < controlPoints->NumControlPoints())
        return QColor((int)(controlPoints->operator[](index).color[0] * 255.),
                     (int)(controlPoints->operator[](index).color[1] * 255.),
                     (int)(controlPoints->operator[](index).color[2] * 255.));
    else
        return QColor(0,0,0);
}

// ****************************************************************************
// Method: QvisSpectrumBar::controlPointPosition
//
// Purpose:
//   Returns the position of a control point. This is a value between 0. and 1.
//
// Arguments:
//   index : The zero-based index of the control point.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:29:19 PDT 2001
//
// Modifications:
//
// ****************************************************************************

float
QvisSpectrumBar::controlPointPosition(int index) const
{
    float retval = 0.;

    if(index >= 0 && index < controlPoints->NumControlPoints())
        retval = controlPoints->Position(index);

    return retval;
}

// ****************************************************************************
// Method: QvisSpectrumBar::numControlPoints
//
// Purpose:
//   Returns the number of control points.
//
// Returns:    The number of control points.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:36:33 PDT 2001
//
// Modifications:
//
// ****************************************************************************

int
QvisSpectrumBar::numControlPoints() const
{
    return controlPoints->CanBeEdited() ? controlPoints->NumControlPoints():0;
}

// ****************************************************************************
// Method: QvisSpectrumBar::activeControlPoint
//
// Purpose:
//   Returns the index of the active control point.
//
// Returns:    The index of the active control point.
//
// Programmer: Brad Whitlock
// Creation:   Sat Feb 3 21:53:27 PST 2001
//
// Modifications:
//
// ****************************************************************************

int
QvisSpectrumBar::activeControlPoint() const
{
    return controlPoints->Rank(controlPoints->NumControlPoints() - 1);
}

// ****************************************************************************
// Method: QvisSpectrumBar::setEqualSpacing
//
// Purpose:
//   Sets the widget's equal spacing mode.
//
// Arguments:
//   val : The new equal spacing mode.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:52:58 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::setEqualSpacing(bool val)
{
    if(val != b_equalSpacing)
    {
        b_equalSpacing = val;
        updateEntireWidget();
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::setSmoothing
//
// Purpose:
//   Sets the widget's color smoothing mode.
//
// Arguments:
//   val : The new smoothing mode.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:53:37 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 28 10:14:52 PDT 2001
//   Added code to allow updates to be suppressed.
//
// ****************************************************************************

void
QvisSpectrumBar::setSmoothing(bool val)
{
    if(val != b_smoothing)
    {
        b_smoothing = val;

        if(isVisible() && !b_suppressUpdates)
        {
            drawSpectrum();
            update(spectrumArea.x(), spectrumArea.y(), spectrumArea.width(),
                   spectrumArea.height());
        }
        else
            deletePixmap();
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::setContinuousUpdate
//
// Purpose:
//   Sets the flag that is used to determine whether or not to emit signals
//   while moving a control point by dragging the mouse.
//
// Arguments:
//   val : If set to true, it updates as the mouse is dragged.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:54:17 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::setContinuousUpdate(bool val)
{
    b_continuousUpdate = val;
}

// ****************************************************************************
// Method: QvisSpectrumBar::suppressUpdates
//
// Purpose:
//   Returns the widget's suppressUpdates flag.
//
// Returns:    The widget's suppressUpdates flag.
//
// Programmer: Brad Whitlock
// Creation:   Wed Mar 28 10:20:39 PDT 2001
//
// Modifications:
//
// ****************************************************************************

bool
QvisSpectrumBar::suppressUpdates() const
{
    return b_suppressUpdates;
}

// ****************************************************************************
// Method: QvisSpectrumBar::setSuppressUpdates
//
// Purpose:
//   Sets the flag that is used to determine whether or not updates should be
//   suppressed.
//
// Arguments:
//   val : If set to true, updates are suppressed.
//
// Programmer: Brad Whitlock
// Creation:   Wed Mar 28 10:18:15 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::setSuppressUpdates(bool val)
{
    b_suppressUpdates = val;
}

// ****************************************************************************
// Method: QvisSpectrumBar::addControlPoint
//
// Purpose:
//   Adds a new control point to the widget.
//
// Arguments:
//   color    : The color of the new control point.
//   position : The position of the new control point.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:55:23 PDT 2001
//
// Notes:      Emits controlPointAdded, activeControlPointChanged signals.
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::addControlPoint(const QColor &color, float position)
{
    int          index;
    ControlPoint temp;

    // Init some values.
    controlPoints->SetEditMode(true);

    // Sort the points in case they are out of order.
    controlPoints->Sort();
    index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);

    // Store the colors in the control point.
    temp.color[0] = (float)color.red() / 255;
    temp.color[1] = (float)color.green() / 255;
    temp.color[2] = (float)color.blue() / 255;

    // Figure out a position for the new point.
    if(position < 0. || position > 1.)
    {
        // A position of -1 means that we should let the widget figure out
        // where to place the new control point.
        if(position == -1.)
        {
            // Figure the position that should be used.
            if(index == controlPoints->NumControlPoints() - 1)
            {
                // Compute the distance to the next point.
                float dx = controlPoints->Position(index) - controlPoints->Position(index - 1);

                // If the distance is small enough, realign the points and recalculate the
                // distance to the next point.
                if(dx <= 0.)
                {
                    alignControlPoints();
                    dx = controlPoints->Position(index) - controlPoints->Position(index - 1);
                }

                // Add new point to the left.
                temp.position = controlPoints->Position(index - 1) + dx * 0.5f;
            }
            else
            {
                // Compute the distance to the next point.
                float dx = controlPoints->Position(index + 1) - controlPoints->Position(index);

                // If the distance is small enough, realign the points and recalculate the
                // distance to the next point.
                if(dx <= 0.)
                {
                    alignControlPoints();
                    dx = controlPoints->Position(index) - controlPoints->Position(index - 1);
                }

                // Add new point to the right.
                temp.position = controlPoints->Position(index) + dx * 0.5f;
            }
        }
        else
            temp.position = 0.;
    }
    else
        temp.position = position;

    // Add the point.
    controlPoints->Add(&temp);

    // Redraw the widget.
    updateEntireWidget();

    // Get the index of the new point and emit signals.
    index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);
    emit controlPointAdded(index, color, temp.position);
    emit activeControlPointChanged(index);
}

// ****************************************************************************
// Method: QvisSpectrumBar::alignControlPoints
//
// Purpose:
//   Sets the position of all of the control points so they have equal spacing.
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb 5 14:12:47 PST 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::alignControlPoints()
{
    // Sort the points in case they are out of order.
    controlPoints->Sort();

    // Set the position in all of the control points.
    int i;
    float pos = 0.;
    float delta = 1. /(float)(controlPoints->NumControlPoints() - 1);
    float *oldPos = new float[controlPoints->NumControlPoints()];
    for(i = 0; i < controlPoints->NumControlPoints(); ++i)
    {
        // Save the old position
        oldPos[i] = controlPoints->Position(i);

        // Set the new position
        controlPoints->SetPosition(i, pos);
        pos += delta;
    }

    // Redraw the entire widget
    updateEntireWidget();

    // Emit a signal for each of the control points that changed.
    for(i = 0; i < controlPoints->NumControlPoints(); ++i)
    {
        if(oldPos[i] != controlPoints->Position(i))
            emit controlPointMoved(i, controlPoints->Position(i));
    }

    // Delete the old position array.
    delete [] oldPos;
}

// ****************************************************************************
// Method: QvisSpectrumBar::removeControlPoint
//
// Purpose:
//   Removes the control point with the highest rank.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:56:16 PDT 2001
//
// Notes:      Emits controlPointRemoved, activeControlPointChanged signals.
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::removeControlPoint()
{
    // Only do the operation if there are more than 2 control points.
    if(controlPoints->NumControlPoints() > 2)
    {
        controlPoints->SetEditMode(true);

        // Delete the highest ranking control point from the control point list.
        int index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);
        ControlPoint removedPoint = controlPoints->operator[](index);
        controlPoints->DeleteHighestRank();

        // Redraw the widget.
        updateEntireWidget();

        // Emit information about the control point that was removed.
        QColor temp((int)(removedPoint.color[0] * 255.),
                    (int)(removedPoint.color[1] * 255.),
                    (int)(removedPoint.color[2] * 255.));
        emit controlPointRemoved(index, temp, removedPoint.position);

        // Emit the index of the new active control point.
        index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);
        emit activeControlPointChanged(index);
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::setRawColors
//
// Purpose:
//   Sets a palette of colors into the widget. This puts the widget into a
//   mode in which the control points disappear so the color palette can
//   be displayed.
//
// Arguments:
//   colors  : An array of colors stored rgbrgbrgb...
//   ncolors : The number of rgb triples.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:57:15 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::setRawColors(unsigned char *colors, int ncolors)
{
    float *fcolors = new float[ncolors * 3];

    // Turn the unsigned chars into floats.
    for(int i = 0; i < ncolors * 3; ++i)
        fcolors[i] = (float)colors[i] / 255;

    // Note that the fcolors array is owned by the controlPoints object after
    // the call to SetColorValues.
    controlPoints->SetColorValues(fcolors, ncolors);
    controlPoints->SetEditMode(false);

    // Redraw the widget.
    updateEntireWidget();
}

// ****************************************************************************
// Method: QvisSpectrumBar::setEditMode
//
// Purpose:
//   Sets the spectrumbar's edit mode. When edit mode is set to false, the
//   control points are not drawn and the widget's spectrum cannot be modified.
//
// Arguments:
//   val : The new edit mode.
//
// Programmer: Brad Whitlock
// Creation:   Tue Mar 27 15:01:02 PST 2001
//
// Modifications:
//   Brad Whitlock, Mon Jul 14 14:44:11 PST 2003
//   I added some value checking.
//
// ****************************************************************************

void
QvisSpectrumBar::setEditMode(bool val)
{
    if(!val)
    {
        float *fcolors = new float[256 * 3];
        unsigned char *raw = getRawColors(256);

        if(raw)
        {
            // Turn the unsigned chars into floats.
            for(int i = 0; i < 256 * 3; ++i)
                fcolors[i] = (float)raw[i] / 255;

            // Note that the fcolors array is owned by the controlPoints object after
            // the call to SetColorValues.
            controlPoints->SetColorValues(fcolors, 256);
            delete [] raw;
        }
    }

    controlPoints->SetEditMode(val);
    updateEntireWidget();
}

// ****************************************************************************
// Method: QvisSpectrumBar::updateEntireWidget
//
// Purpose:
//   Redraws the entire widget or deletes the internal pixmaps so the entire
//   widget is drawn the next time it is shown.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:59:05 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 28 10:13:51 PDT 2001
//   Added code to allow updates to be suppressed.
//
//   Brad Whitlock, Wed Mar 13 10:09:38 PDT 2002
//   Made it use a single pixmap.
//
// ****************************************************************************

void
QvisSpectrumBar::updateEntireWidget()
{
    if(isVisible() && !b_suppressUpdates)
    {
        drawControls();
        drawSpectrum();
        update();
    }
    else
        deletePixmap();
}

// ****************************************************************************
// Method: QvisSpectrumBar::deletePixmap
//
// Purpose:
//   Deletes the internal pixmap.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 10:59:49 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::deletePixmap()
{
    delete pixmap;
    pixmap = 0;
}

// ****************************************************************************
// Method: QvisSpectrumBar::setControlPointColor
//
// Purpose:
//   Sets the color of a control point.
//
// Arguments:
//   index : The index of the control point to change.
//   color : The new color of the control point.
//
// Note:       Emits controlPointColorChanged signal.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:00:36 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::setControlPointColor(int index, const QColor &color)
{
    if(index >= 0 && index < controlPoints->NumControlPoints())
    {
         float r, g, b;
         r = (float)color.red() / 255.;
         g = (float)color.green() / 255.;
         b = (float)color.blue() / 255.;

         // Set the color and update the widget.
         controlPoints->SetEditMode(true);
         controlPoints->SetColor(index, r, g, b);
         updateEntireWidget();

         // Emit a signal indicating that a control point changed colors.
         emit controlPointColorChanged(index, color);
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::setControlPointPosition
//
// Purpose:
//   Sets the position of a control point.
//
// Arguments:
//   index    : The index of the control point to change.
//   position : The control point's new position.
//
// Note:       Emits controlPointMoved signal.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:08:12 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::setControlPointPosition(int index, float position)
{
    if(index >= 0 && index < controlPoints->NumControlPoints())
    {
        if(position < 0.) position = 0.;
        if(position > 1.) position = 1.;

        // Set the position and update the widget.
        controlPoints->SetEditMode(true);
        moveControlPointRedraw(index, position, true);

        // Emit a signal indicating that a control point moved.
        emit controlPointMoved(index, position);
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::setOrientation
//
// Purpose:
//   Sets the widget's orientation.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:09:31 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::setOrientation(QvisSpectrumBar::ControlOrientation)
{
    // Not supported yet.
}

// ****************************************************************************
// Method: QvisSpectrumBar::drawControls
//
// Purpose:
//   Draws all of the control points in descending rank.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:11:19 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 09:12:31 PDT 2002
//   Removed style coding. Made it use a single pixmap. Removed code to sort
//   the control points to fix a bug that made paging expose incorrectly.
//
//   Brad Whitlock, Thu Aug 21 15:41:42 PST 2003
//   I changed how the brush is created so the widget looks better on MacOS X.
//
// ****************************************************************************

void
QvisSpectrumBar::drawControls()
{
    // If the pixmap is not allocated, allocate it.
    bool totalFill = false;
    if(pixmap == 0)
    {
        pixmap = new QPixmap(width(), height());
        totalFill = true;
    }

#ifdef Q_WS_MACX
    QBrush brush(palette().brush(QPalette::Background));
#else
    QBrush brush(palette().brush(QPalette::Button));
#endif

    // Create a painter and fill in the entire area with the background color.
    QPainter paint(pixmap);
    if(totalFill)
        paint.fillRect(0, 0, width(), height(), brush);
    else
        paint.fillRect(controlsArea.x(), controlsArea.y(),
                       controlsArea.width(), controlsArea.height(),
                       brush);

    // If we're not in editable mode, then we don't need to draw any
    // control points.
    if(!controlPoints->CanBeEdited())
        return;

    // Draw all the control points in the list in the order of lowest to
    // highest rank. This ensures that control points can properly overlap.
    int sel_index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);
    for(int i = 0; i < controlPoints->NumControlPoints(); ++i)
    {
        // Get the control point with rank i.
        int index = controlPoints->Rank(i);

        // Get the bounding rectangle for the index'th control point.
        QPoint cpLocation(controlPointLocation(index));

        // Create a QColor to use to draw the color in the control point.
        QColor controlPointColor((int)(controlPoints->operator[](index).color[0] * 255.),
                                 (int)(controlPoints->operator[](index).color[1] * 255.),
                                 (int)(controlPoints->operator[](index).color[2] * 255.));

        // Hard code the select color to yellow for now.
        QColor selectColor(255, 255, 0);

        // Draw the control point.
        drawControlPoint(&paint,
                         palette().light(),
                         palette().dark(),
#ifdef Q_WS_MACX
                         palette().brush(QPalette::Background),
#else
                         palette().button(),
#endif
                         selectColor,
                         controlPointColor,
                         cpLocation.x(), cpLocation.y(),
                         slider.width(), slider.height(),
                         2,
                         orientation, index == sel_index);
    }

    // Make pixmap the background pixmap.
//    setBackgroundPixmap(*pixmap);
     QPalette palette;
     palette.setBrush(this->backgroundRole(), QBrush(*pixmap));
     this->setPalette(palette);

}

// ****************************************************************************
// Method: QvisSpectrumBar::controlPointLocation
//
// Purpose:
//   Returns the position of the upper left corner of a control point.
//
// Arguments:
//   index : The index of the control point.
//
// Returns:    The position of the upper left corner of a control point.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:12:35 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 10:09:38 PDT 2002
//   Made it use a single pixmap. Fixed margin.
//
// ****************************************************************************

QPoint
QvisSpectrumBar::controlPointLocation(int index) const
{
    int   offset, x, y;
    float pos;

    // Figure out the position where the control point should be drawn.
    // This ignores position information in equal spacing mode.
    if(equalSpacing())
    {
        // Figure out the offset
        pos = (float)index /(float)(controlPoints->NumControlPoints() - 1);
        if(orientation == HorizontalTop || orientation == HorizontalBottom)
            offset = spectrumArea.width() / controlPoints->NumControlPoints();
        else
            offset = spectrumArea.height() / controlPoints->NumControlPoints();
    }
    else
    {
        pos = controlPoints->operator[](index).position;
        offset = 0;
    }

    // Figure the x,y location of where to draw the control point.
    if(orientation == HorizontalTop || orientation == HorizontalBottom)
    {
        x = (int)(pos *(spectrumArea.width() - offset)) + offset/2 + margin;
        y = controlsArea.y();
    }
    else
    {
        x = controlsArea.x();
        y = (int)(pos *(spectrumArea.height() - offset)) + offset/2;
    }

    return QPoint(x, y);
}

// ****************************************************************************
// Method: QvisSpectrumBar::drawControlPoint
//
// Purpose:
//   Draws a control point.
//
// Arguments:
//   paint  : The painter to use to draw the control point.
//   top    : The top highlight color used for the border.
//   bottom : The bottom shadow color used for the border.
//   fore   : The button color used for the control point.
//   sel    : The selection color.
//   cpt    : The control point color.
//   x      : The leftmost x value.
//   y      : The topmost y value.
//   w      : The width of the control point.
//   h      : The height of the control point.
//   shadhow_thick : The shadow thickness.
//   orient : The control point orientation.
//   selected : Whether or not the control point is selected.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:15:16 PDT 2001
//
// Modifications:
//   Brad Whitlock, Tue Mar 12 13:38:59 PST 2002
//   Removed all dependencies on style objects.
//
// ****************************************************************************

void
QvisSpectrumBar::drawControlPoint(QPainter *paint, const QBrush &top,
    const QBrush &bottom, const QBrush &fore, const QColor &sel,
    const QColor &cpt, int x, int y, int w, int h, int shadow_thick,
    ControlOrientation orient, bool selected)
{
    int wd2 = w >> 1;
    int s2 = (shadow_thick << 1) + 1;
    int h1 = (int)((float)wd2 * 1.7320);
    int h2 = (int)((float)shadow_thick * 1.7320) + 1;
    int h3 = (int)((float)(w -(shadow_thick << 1)) * 1.7320 * 0.5) + 1;
    int w2 = (int)((float)(7 * h3 / 30) * 1.7320);

    int X[17], Y[17];
    QPolygon poly(5);

#define COPY_POINT(DI,SI) poly.setPoint((DI), X[SI], Y[SI])

    // Fill the vertices.
    X[0] = x + wd2;              Y[0] = y + h;
    X[1] = x + w;                Y[1] = y + h - h1;
    X[2] = x + w;                Y[2] = y;
    X[3] = x;                    Y[3] = y;
    X[4] = x;                    Y[4] = y + h - h1;
    X[5] = x + wd2;              Y[5] = y + h - h2;
    X[6] = x + w - shadow_thick; Y[6] = y + h - h2 - h3;
    X[7] = x + w - shadow_thick; Y[7] = y + shadow_thick;
    X[8] = x + shadow_thick;     Y[8] = y + shadow_thick;
    X[9] = x + shadow_thick;     Y[9] = y + h - h2 - h3;
    X[10] = x + w - s2;          Y[10] = y + h - h2 - h3;
    X[11] = x + w - s2;          Y[11] = y + s2;
    X[12] = x + s2;              Y[12] = y + s2;
    X[13] = x + s2;              Y[13] = y + h - h2 - h3;
    X[14] = x + wd2;             Y[14] = y + h - h2 - int(h3 * 0.15);
    X[15] = x + wd2 + w2;        Y[15] = y + h - h2 - int(0.85 * h3);
    X[16] = x + wd2 - w2;        Y[16] = y + h - h2 - int(0.85 * h3);

    paint->setPen(Qt::NoPen);            // do not draw outline

    // Create a polygon.
    COPY_POINT(0, 0);
    COPY_POINT(1, 1);
    COPY_POINT(2, 6);
    COPY_POINT(3, 5);
    paint->setBrush(bottom);

    paint->drawPolygon(poly.constData(), 4, Qt::OddEvenFill);

    // Create a polygon.
    COPY_POINT(0, 1);
    COPY_POINT(1, 2);
    COPY_POINT(2, 7);
    COPY_POINT(3, 6);
    paint->drawPolygon(poly.constData(), 4, Qt::OddEvenFill);

    // Create a polygon.
    COPY_POINT(0, 2);
    COPY_POINT(1, 3);
    COPY_POINT(2, 8);
    COPY_POINT(3, 7);
    paint->setBrush(top);
    paint->drawPolygon(poly.constData(), 4, Qt::OddEvenFill);

    // Create a polygon.
    COPY_POINT(0, 3);
    COPY_POINT(1, 4);
    COPY_POINT(2, 9);
    COPY_POINT(3, 8);
    paint->drawPolygon(poly.constData(), 4, Qt::OddEvenFill);

    // Create a polygon.
    COPY_POINT(0, 4);
    COPY_POINT(1, 0);
    COPY_POINT(2, 5);
    COPY_POINT(3, 9);
    paint->drawPolygon(poly.constData(), 4, Qt::OddEvenFill);

    // Create a polygon.
    COPY_POINT(0, 5);
    COPY_POINT(1, 6);
    COPY_POINT(2, 7);
    COPY_POINT(3, 8);
    COPY_POINT(4, 9);
    paint->setBrush(fore);
    paint->drawPolygon(poly.constData(), 5, Qt::OddEvenFill);

    // If the width is > 2 times the shadow thickness then we have
    // room to draw the interior color rectangle.
    if(w >(shadow_thick << 1))
    {
        // Create a polygon.
        COPY_POINT(0, 10);
        COPY_POINT(1, 11);
        COPY_POINT(2, 12);
        COPY_POINT(3, 13);
        paint->setBrush(cpt);
        paint->drawPolygon(poly.constData(), 4, Qt::OddEvenFill);

        int boxX, boxY, boxWidth, boxHeight;

        switch(orient)
        {
        case HorizontalTop:
            boxX = X[12];
            boxY = Y[12];
            boxWidth = X[11] - X[12];
            boxHeight = Y[10] - Y[11];
            break;
        default:
            boxX = boxY = boxWidth = boxHeight = 0; // just for now.
        }

        // Draw the sunken bevel around the spectrum.
        drawBox(paint, QRect(boxX, boxY, boxWidth, boxHeight),
          palette().dark().color(), palette().light().color());

        // Create the select polygon.
        if(selected)
        {
            QPalette g(palette());
            g.setColor(QPalette::Button, sel);
            drawArrow(paint, true, X[16], Y[16], w2 << 1,(int)(h3 * 0.65), g);
        }
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::drawBox
//
// Purpose:
//   Draws a highlighted box that looks like the edges of a button.
//
// Arguments:
//   paint : The painter used to draw.
//   r     : The bounding rectangle of the frame
//   light : The light color
//   dark  : The dark color
//   lw    : The width of the box.
//
// Programmer: Brad Whitlock
// Creation:   Tue Mar 12 18:45:56 PST 2002
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::drawBox(QPainter *paint, const QRect &r,
    const QColor &light, const QColor &dark, int lw)
{
    int i;
    int X  = r.x();
    int X2 = r.x() + r.width() - 1;
    int Y  = r.y();
    int Y2 = r.y() + r.height() - 1;

    // Draw the highlight
    paint->setPen(QPen(light));
    for(i = 0; i < lw; ++i)
    {
        paint->drawLine(QPoint(X + i, Y + i), QPoint(X + i, Y2 - i));
        paint->drawLine(QPoint(X + i, Y + i), QPoint(X2 - i, Y + i));
    }

    // Draw the shadow
    paint->setPen(QPen(dark));
    for(i = 0; i < lw; ++i)
    {
        paint->drawLine(QPoint(X + i + 1, Y2 - i), QPoint(X2, Y2 - i));
        paint->drawLine(QPoint(X2 - i, Y + i + 1), QPoint(X2 - i, Y2));
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::drawArrow
//
// Purpose:
//   Draws an arrow.
//
// Note:       This code was copied from Qt 2.3.0 and trimmed down to be
//             less general.
//
// Programmer: Brad Whitlock
// Creation:   Wed Mar 13 09:37:51 PDT 2002
//
// Modifications:
//   Brad Whitlock, Thu Aug 21 15:50:13 PST 2003
//   I changed how the brush is created so the arrows look better on MacOS X.
//
// ****************************************************************************

void
QvisSpectrumBar::drawArrow(QPainter *p, bool down, int x, int y, int w, int h,
    const QPalette &g)
{
    QPolygon bFill;            // fill polygon
    QPolygon bTop;             // top shadow.
    QPolygon bBot;             // bottom shadow.
    QPolygon bLeft;            // left shadow.
    QMatrix  matrix;           // xform matrix
    bool vertical = orientation == HorizontalTop ||
                    orientation == HorizontalBottom;
    bool horizontal = !vertical;
    int     dim = w < h ? w : h;
    int     colspec = 0x0000;     // color specification array

    if(dim < 2)                   // too small arrow
        return;

    // adjust size and center(to fix rotation below)
    if(w >  dim)
    {
        x += (w-dim)/2;
        w = dim;
    }
    if(h > dim)
    {
        y += (h-dim)/2;
        h = dim;
    }

    if(dim > 3)
    {
        if(dim > 6)
            bFill.resize(dim & 1 ? 3 : 4);
        bTop.resize((dim/2)*2);
        bBot.resize(dim & 1 ? dim + 1 : dim);
        bLeft.resize(dim > 4 ? 4 : 2);
        bLeft.putPoints(0, 2, 0,0, 0,dim-1);

        if(dim > 4)
            bLeft.putPoints(2, 2, 1,2, 1,dim-3);
        bTop.putPoints(0, 4, 1,0, 1,1, 2,1, 3,1);
        bBot.putPoints(0, 4, 1,dim-1, 1,dim-2, 2,dim-2, 3,dim-2);

        for(int i=0; i<dim/2-2 ; i++)
        {
            bTop.putPoints(i*2+4, 2, 2+i*2,2+i, 5+i*2, 2+i);
            bBot.putPoints(i*2+4, 2, 2+i*2,dim-3-i, 5+i*2,dim-3-i);
        }

        if(dim & 1)                // odd number size: extra line
            bBot.putPoints(dim-1, 2, dim-3,dim/2, dim-1,dim/2);
        if(dim > 6)
        {            // dim>6: must fill interior
            bFill.putPoints(0, 2, 1,dim-3, 1,2);
            if(dim & 1)            // if size is an odd number
                bFill.setPoint(2, dim - 3, dim / 2);
            else
                bFill.putPoints(2, 2, dim-4,dim/2-1, dim-4,dim/2);
        }
    }
    else
    {
        if(dim == 3)
        {            // 3x3 arrow pattern
            bLeft.setPoints(4, 0,0, 0,2, 1,1, 1,1);
            bTop .setPoints(2, 1,0, 1,0);
            bBot .setPoints(2, 1,2, 2,1);
        }
        else
        {                    // 2x2 arrow pattern
            bLeft.setPoints(2, 0,0, 0,1);
            bTop .setPoints(2, 1,0, 1,0);
            bBot .setPoints(2, 1,1, 1,1);
        }
    }

    if(orientation == HorizontalBottom || orientation == VerticalRight)
    {
        matrix.translate(x, y);
        if(vertical)
        {
            matrix.translate(0, h - 1);
            matrix.rotate(-90);
        }
        else
        {
            matrix.translate(w - 1, h - 1);
            matrix.rotate(180);
        }

        if(down)
            colspec = horizontal ? 0x2334 : 0x2343;
        else
            colspec = horizontal ? 0x1443 : 0x1434;
    }
    else
    {
        matrix.translate(x, y);
        if(vertical)
        {
            matrix.translate(w-1, 0);
            matrix.rotate(90);
        }
        if(down)
            colspec = horizontal ? 0x2443 : 0x2434;
        else
            colspec = horizontal ? 0x1334 : 0x1343;
    }

    QColor *cols[5];
    cols[0] = 0;
    cols[1] = (QColor *)&g.button();
    cols[2] = (QColor *)&g.mid();
    cols[3] = (QColor *)&g.light();
    cols[4] = (QColor *)&g.dark();

#define CMID    *cols[(colspec>>12) & 0xf ]
#define CLEFT   *cols[(colspec>>8) & 0xf ]
#define CTOP    *cols[(colspec>>4) & 0xf ]
#define CBOT    *cols[ colspec & 0xf ]

    QPen     savePen   = p->pen();     // save current pen
    QBrush   saveBrush = p->brush();   // save current brush
    QMatrix wxm = p->worldMatrix();
    QPen     pen(Qt::NoPen);
#ifdef Q_WS_MACX
    QBrush   brush(g.brush(QPalette::Background));
#else
    QBrush   brush(g.brush(QPalette::Button));
#endif

    p->setPen(pen);
    p->setBrush(brush);
    p->setWorldMatrix(matrix, TRUE);   // set transformation matrix
    p->drawPolygon(bFill);             // fill arrow
    p->setBrush(Qt::NoBrush);              // don't fill

    p->setPen(CLEFT);
    p->drawLines(bLeft.constData(),bLeft.size());
    p->setPen(CTOP);
    p->drawLines(bTop.constData(),bTop.size());
    p->setPen(CBOT);
    p->drawLines(bBot.constData(),bBot.size());

    p->setWorldMatrix(wxm);
    p->setBrush(saveBrush);            // restore brush
    p->setPen(savePen);                // restore pen

#undef CMID
#undef CLEFT
#undef CTOP
#undef CBOT
}

// ****************************************************************************
// Method: QvisSpectrumBar::drawSpectrum
//
// Purpose:
//   Draws the color spectrum into an internal pixmap.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:19:06 PDT 2001
//
// Modifications:
//   Brad Whitlock, Tue Mar 12 13:59:45 PST 2002
//   Removed all dependencies on style objects and made it use a single
//   pixmap.
//
//   Brad Whitlock, Thu Aug 21 15:42:50 PST 2003
//   I changed how the brush is selected so the widget looks better on MacOS X.
//
// ****************************************************************************

void
QvisSpectrumBar::drawSpectrum()
{
    // If the pixmap is not allocated, allocate it.
    bool totalFill = false;
    if(pixmap == 0)
    {
        pixmap = new QPixmap(width(), height());
        totalFill = true;
    }

#ifdef Q_WS_MACX
    QBrush brush(palette().brush(QPalette::Background));
#else
    QBrush brush(palette().brush(QPalette::Button));
#endif

    // Create a painter and fill in the entire area with the background color.
    QPainter paint(pixmap);
    if(totalFill)
        paint.fillRect(0, 0, width(), height(), brush);

    // Get the area that we can draw the spectrum in.
    QRect area(spectrumArea.x() + 2, spectrumArea.y() + 2,
               spectrumArea.width() - 4, spectrumArea.height() - 4);

    // Figure out the range based on the orientation of the widget.
    int range;
    if(orientation == HorizontalTop || orientation == HorizontalBottom)
        range = area.width();
    else
        range = area.height();

    // Get an array containing the interpolated color spectrum in a form we can use.
    unsigned char *interpolatedColors = getRawColors(range);

    // Draw the spectrum.
    if(interpolatedColors)
    {
        unsigned char *cptr = interpolatedColors;

        // Draw the spectrum based on the orientation of the widget.
        if(orientation == HorizontalTop || orientation == HorizontalBottom)
        {
            // Draw vertical lines.
            for(int i = 0; i < range; ++i, cptr += 3)
            {
                paint.setPen(QPen(QColor((int)cptr[0],(int)cptr[1],(int)cptr[2])));
                paint.drawLine(i + area.x(), area.y(),
                               i + area.x(), area.y() + area.height() + 1);
            }
        }
        else
        {
            // Draw horizontal lines.
            for(int i = range - 1; i >= 0; --i, cptr += 3)
            {
                paint.setPen(QPen(QColor((int)cptr[0],(int)cptr[1],(int)cptr[2])));
                paint.drawLine(area.x(), i + area.y(),
                               area.x() + area.width() + 1, i + area.y());
            }
        }

        // Draw the sunken bevel around the spectrum.
        drawBox(&paint, spectrumArea, palette().dark().color(), palette().light().color());

        // Delete the color array.
        delete [] interpolatedColors;
    }

    // Make pixmap the background pixmap.
//    setBackgroundPixmap(*pixmap);
     QPalette palette;
     palette.setBrush(this->backgroundRole(), QBrush(*pixmap));
     this->setPalette(palette);
}

// ****************************************************************************
// Method: QvisSpectrumBar::getRawColors
//
// Purpose:
//   Figures out the raw colors that will be used to draw the spectrum.
//
// Arguments:
//     w     : The widget for which we're getting raw colors.
//     range : The number of available color slots.
//
// Returns:
//     This function returns a block of memory range*3 bytes long.
//     Colors are arranged in the memory as rgbrgbrgb... where each
//     color channel is one byte.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:19:33 PDT 2001
//
// Modifications:
//   Brad Whitlock, Thu Oct 24 11:06:22 PDT 2002
//   I removed the restriction that the endpoints lie at 0 and 1.
//
//   Brad Whitlock, Mon Jul 14 14:43:04 PST 2003
//   I added a little range checking code.
//
// ****************************************************************************

unsigned char *
QvisSpectrumBar::getRawColors(int range)
{
    unsigned char *row = NULL;
    int i, ci, npoints, c = 0;
    ControlPoint *oldpts = NULL, *newpts = NULL;
    ControlPoint *c1 = NULL, *c2 = NULL;

    // Return early if the range is bad.
    if(range < 1)
        return 0;

    // Allocate memory for the array to be returned.
    int arrayLength = range * 3;
    row = new unsigned char[arrayLength];

    /*******************************************
     * Phase I -- If the widget is non-editable
     *            then it has no control points.
     *            We must do colors differently.
     ******************************************/
    if(!this->controlPoints->CanBeEdited())
    {
        for(i = 0; i < range; ++i)
        {
            /* The desired index into the colors we have. */
            int src_index = (int)(((float)i /(float)(range - 1)) *
               (this->controlPoints->NumColorValues() - 1)) * 3;

            row[i * 3]     = (unsigned char)(controlPoints->ColorValue(src_index) * 255.);
            row[i * 3 + 1] = (unsigned char)(controlPoints->ColorValue(src_index+1) * 255.);
            row[i * 3 + 2] = (unsigned char)(controlPoints->ColorValue(src_index+2) * 255.);
        }

        return row;
    }

    /*******************************************
     * Phase II -- Determine the number of
     *             control points needed and
     *             allocate storage.
     ******************************************/
    npoints = this->controlPoints->NumControlPoints();
    if(equalSpacing() || !smoothing())
        oldpts = new ControlPoint[npoints + 1];
    else
        oldpts = new ControlPoint[npoints];

    // Sort the color control points.
    for(i = 0; i < this->controlPoints->NumControlPoints(); ++i)
        oldpts[i] = this->controlPoints->operator[](i);
    qsort((void *)oldpts, npoints, sizeof(ControlPoint), ControlPointCompare);

    /*******************************************
     * Phase III -- Process the control points.
     ******************************************/
    if(equalSpacing() || !smoothing())
    {
        ++npoints;
        newpts = new ControlPoint[npoints];

        if(equalSpacing())
        {
            // Do the equal spacing case.
            for(i = 0; i < npoints; ++i)
            {
                ci = (i <(npoints - 2)) ? i :(npoints - 2);
                newpts[i].position = (float)i /(float)(npoints - 1);

                if(!smoothing())
                {
                    memcpy((char *)newpts[i].color,
                          (char *)oldpts[ci].color, 3 * sizeof(float));
                }
                else
                {
                    if(i < 1 || i >= npoints - 1)
                    {
                        memcpy((char *)newpts[i].color,
                              (char *)oldpts[ci].color,
                               3 * sizeof(float));
                    }
                    else
                    {
                        newpts[i].color[0] = (oldpts[i].color[0] +
                            oldpts[i-1].color[0])*0.5;
                        newpts[i].color[1] = (oldpts[i].color[1] +
                            oldpts[i-1].color[1])*0.5;
                        newpts[i].color[2] = (oldpts[i].color[2] +
                            oldpts[i-1].color[2])*0.5;
                    }
                }
            } // end for
        } // end if equal spacing
        else
        {
            // Do non-equal, non-smooth case.
            memcpy((char *)newpts,(char *)oldpts, sizeof(ControlPoint));
            for(i = 1; i < npoints - 1; i++)
            {
                newpts[i].position = oldpts[i-1].position +
                   ((oldpts[i].position - oldpts[i-1].position) * 0.5);
                memcpy((char *)newpts[i].color,
                      (char *)oldpts[i].color,
                       3 * sizeof(float));
            }
            memcpy((char *)&newpts[npoints-1],(char *)&oldpts[npoints-2],
                   sizeof(ControlPoint));
        }
        c1 = newpts;
    }
    else
        c1 = oldpts;

    /********************************************
     * Phase IV -- Figure the colors for a row.
     ********************************************/
    c2 = c1;
    int consecutiveZeroLengthRanges = 0;
    for(ci = 0; ci < npoints - 1; ci++)
    {
        float delta_r, delta_g, delta_b;
        float r_sum, g_sum, b_sum;
        int   color_start_i, color_end_i, color_range;

        // Initialize some variables.
        c2++;
        color_start_i = int(c1->position * range);
        color_end_i = int(c2->position * range);
        color_range = color_end_i - color_start_i;

        if(color_range > 1)
        {
            consecutiveZeroLengthRanges = 0;
            if(ci == 0 && color_start_i != 0)
            {
                for(i = 0; i < color_start_i; i++)
                {
                    if(c < arrayLength)
                    {
                        row[c++] = (unsigned char)(c1->color[0] * 255);
                        row[c++] = (unsigned char)(c1->color[1] * 255);
                        row[c++] = (unsigned char)(c1->color[2] * 255);
                    }
                }
            }

            // Figure out some deltas.
            if(smoothing())
            {
                delta_r = (float)(c2->color[0]-c1->color[0])/(float)(color_range-1);
                delta_g = (float)(c2->color[1]-c1->color[1])/(float)(color_range-1);
                delta_b = (float)(c2->color[2]-c1->color[2])/(float)(color_range-1);
            }
            else
                delta_r = delta_g = delta_b = 0.;

            // Initialize sums.
            r_sum = c1->color[0]; g_sum = c1->color[1]; b_sum = c1->color[2];

            // Interpolate color1 to color2.
            for(i = color_start_i; i < color_end_i; i++)
            {
                // Store the colors as 24 bit rgb.
                if(c < arrayLength)
                {
                    row[c++] = (unsigned char)(r_sum * 255);
                    row[c++] = (unsigned char)(g_sum * 255);
                    row[c++] = (unsigned char)(b_sum * 255);
                }

                // Add the color deltas.
                r_sum += delta_r; g_sum += delta_g; b_sum += delta_b;
            }

            if(ci == npoints - 2 && color_end_i != range)
            {
                for(i = color_end_i; i < range; i++)
                {
                    if(c < arrayLength)
                    {
                        row[c++] = (unsigned char)(c2->color[0] * 255);
                        row[c++] = (unsigned char)(c2->color[1] * 255);
                        row[c++] = (unsigned char)(c2->color[2] * 255);
                    }
                }
            }
        }
        else if(c < arrayLength)
        {
            row[c++] = (unsigned char)(c1->color[0] * 255);
            row[c++] = (unsigned char)(c1->color[1] * 255);
            row[c++] = (unsigned char)(c1->color[2] * 255);

            // If this is the second zero length range in a row, back up.
            if(++consecutiveZeroLengthRanges > 1)
                c -= 3;
        }

        c1++;
    }

    // Free unneeded memory.
    delete [] oldpts;
    delete [] newpts;

    // Return some color information.
    return row;
}

// ****************************************************************************
// Method: QvisSpectrumBar::paintEvent
//
// Purpose:
//   Handles paint events for the widget.
//
// Arguments:
//   e : The paint event.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:10:01 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 10:22:05 PDT 2002
//   Made it use a single pixmap and added a focus rectangle.
//
// ****************************************************************************

void
QvisSpectrumBar::paintEvent(QPaintEvent* /*e*/)
{
    // bool clipByRegion = true;
/*
    // If the spectrum pixmap does not exist, generate it.
    if(pixmap == 0)
    {
        drawControls();
        drawSpectrum();
        clipByRegion = false;
    }

    // Create a painter to draw on this widget. If the pixmaps that we're
    // blitting did not have to be created, pay attention to the clipping
    // region stored in the paint event.
    QPainter paint(this);
    if(clipByRegion && !e->region().isEmpty())
        paint.setClipRegion(e->region());

    // Blit the pixmaps to the screen.
    paint.drawPixmap(QPoint(0,0), *pixmap);

    // If this widget has the focus then draw the focus rectangle.
    if(hasFocus())
    {

        QStyleOptionFocusRect option;
        option.init(this);
        option.backgroundColor = palette().color(QPalette::Background);
        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &paint, this);
    }
*/
}

// ****************************************************************************
// Method: QvisSpectrumBar::resizeEvent
//
// Purpose:
//   Handles resize events for the widget.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:10:38 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 10:09:38 PDT 2002
//   Made it use a single pixmap.
//
// ****************************************************************************

void
QvisSpectrumBar::resizeEvent(QResizeEvent *)
{
    // Recalculate the size of the spectrum area and the controls area.
    if(orientation == HorizontalTop)
    {
        int controlHeight = int(height() * 0.4) - margin;
        if(controlHeight > 60)
            controlHeight = 60;

        controlsArea.setHeight(controlHeight);
        slider.setWidth((int)(controlsArea.height() / 1.6));
        slider.setHeight(controlsArea.height());

        controlsArea.setX(margin);
        controlsArea.setY(margin);
        controlsArea.setWidth(width() -(margin << 1));

        // These are not important since the slider never moves.
        slider.setX(0);
        slider.setY(controlsArea.y());

        // Figure the area for the spectrum area.
        spectrumArea.setX(margin +(slider.width() >> 1));
        spectrumArea.setY(margin + controlsArea.height());
        spectrumArea.setWidth(width() -(spectrumArea.x() << 1));
        spectrumArea.setHeight(height() - spectrumArea.y() - margin);
    }
    else
        qDebug("This orientation is not supported yet!");

    // Update the whole widget.
    deletePixmap();
    update();
}

// ****************************************************************************
// Method: QvisSpectrumBar::paletteChange
//
// Purpose:
//   This method forces the widget to redraw when the palette is changed.
//
// Programmer: Brad Whitlock
// Creation:   Fri Sep 7 12:48:19 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 10:09:38 PDT 2002
//   Made it use a single pixmap.
//
// ****************************************************************************

void
QvisSpectrumBar::paletteChange(const QPalette &)
{
    // Update the whole widget.
    deletePixmap();
    update();
}

// ****************************************************************************
// Method: QvisSpectrumBar::keyPressEvent
//
// Purpose:
//   Handles keypress events for the widgets.
//
// Arguments:
//   e : The key press event.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:21:56 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 12:05:25 PDT 2002
//   Made key support better.
//
// ****************************************************************************

void
QvisSpectrumBar::keyPressEvent(QKeyEvent *e)
{
    // Figure the currently selected point.
    int current_sel = controlPoints->Rank(controlPoints->NumControlPoints() - 1);
    int new_sel;

    if(equalSpacing())
    {
        switch(e->key())
        {
        case Qt::Key_Left:
            if(current_sel > 0)
                new_sel = current_sel - 1;
            else
                new_sel = controlPoints->NumControlPoints() - 1;
            controlPoints->GiveHighestRank(new_sel);
            updateControlPoints();
            break;
        case Qt::Key_Right:
            if(current_sel < controlPoints->NumControlPoints() - 1)
                new_sel = current_sel + 1;
            else
                new_sel = 0;
            controlPoints->GiveHighestRank(new_sel);
            updateControlPoints();
            break;
        case Qt::Key_Up:
        case Qt::Key_Return:
            colorSelected(current_sel);
        }
    }
    else
    {
        switch(e->key())
        {
        case Qt::Key_Shift:
            shiftApplied = true;
            break;
        case Qt::Key_Left:
            if(shiftApplied)
                moveControlPoint(PAGE_DECREMENT);
            else
                moveControlPoint(DECREMENT);
            break;
        case Qt::Key_Up:
        case Qt::Key_Return:
            colorSelected(current_sel);
            break;
        case Qt::Key_Right:
            if(shiftApplied)
                moveControlPoint(PAGE_INCREMENT);
            else
                moveControlPoint(INCREMENT);
            break;
        case Qt::Key_PageUp:
            moveControlPoint(PAGE_INCREMENT);
            break;
        case Qt::Key_PageDown:
            moveControlPoint(PAGE_DECREMENT);
            break;
        case Qt::Key_Home:
            moveControlPoint(PAGE_HOME);
            break;
        case Qt::Key_End:
            moveControlPoint(PAGE_END);
            break;
        case Qt::Key_Backspace:
        case Qt::Key_Space:
            // Find the index of the control point with the lowest rank.
            new_sel = controlPoints->Rank(0);
            // Give the control point the highest rank and redraw the controls.
            controlPoints->GiveHighestRank(new_sel);
            updateControlPoints();
            break;
        }
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::colorSelected
//
// Purpose:
//   This routine is called when we want to select a color. It emits signals
//   that could be used for a popup menu.
//
// Arguments:
//   index : The index of the control point that we're selecting.
//
// Programmer: Brad Whitlock
// Creation:   Wed Mar 13 17:55:29 PST 2002
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::colorSelected(int index)
{
    // Get the location of the current control point.
    QPoint p(controlPointLocation(index));
    // Figure out the center of the control point.
    QPoint pc(p.x() + slider.width() / 2, p.y() + slider.height() / 2);

    emit selectColor(index);
    emit selectColor(index, mapToGlobal(pc));
}

// ****************************************************************************
// Method: QvisSpectrumBar::keyReleaseEvent
//
// Purpose:
//   Handles key release events for the widget. This is mainly to determine
//   when the shift key has been released.
//
// Arguments:
//   e : The key event.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:22:28 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::keyReleaseEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Shift)
        shiftApplied = false;
}

// ****************************************************************************
// Method: QvisSpectrumBar::mousePressEvent
//
// Purpose:
//   Handles mouse press events for the widget.
//
// Arguments:
//   e : The mouse press event.
//
// Notes:      Emits activeControlPointChanged signal.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:23:13 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 11:01:03 PDT 2002
//   Modified the clip region.
//
// ****************************************************************************

void
QvisSpectrumBar::mousePressEvent(QMouseEvent * /*e*/)
{
/*
    // Check to see if a slider was clicked. If so, we need to
    // set the active slider and update the controls.
    int   new_sel, current_sel;
    float fpos, fwidth;

    // Figure the currently selected point.
    current_sel = controlPoints->Rank(controlPoints->NumControlPoints() - 1);

    // Figure the position of the event and the width of the
    // slider as numbers in the range [0.,1.].
    if(orientation == HorizontalTop || orientation == HorizontalBottom)
    {
        fpos = ((float)(e->x() - controlsArea.x() - slider.width()/2) /
               (float)(controlsArea.width() - slider.width()));
        fwidth = (float)slider.width() /
                (float)(controlsArea.width() - slider.width());
    }
    else
    {
        fpos = ((float)(e->y() - controlsArea.y()) /
               (float)controlsArea.height());
        fwidth = (float)slider.height() /
                (float)controlsArea.height();
    }

    // See if we selected a new control point.
    new_sel = controlPoints->ChangeSelectedIndex(fpos, fwidth, equalSpacing());

    // If we did select a point, set its location into the fields
    // that are used to track the slider's location.
    if(new_sel != -1 && new_sel != current_sel)
    {
        // Sort the control points and get the new index.
        controlPoints->Sort();
        new_sel = controlPoints->Rank(controlPoints->NumControlPoints() - 1);

        // Redraw the controls.
        drawControls();

        // Figure out the regions covered by the old and new control points
        // and only repaint those regions.
        QPoint newLocation(controlPointLocation(new_sel));
        QPoint oldLocation(controlPointLocation(current_sel));

        QRegion r(newLocation.x(), newLocation.y(),
                  slider.width(), slider.height());
        QRegion r2(oldLocation.x(), oldLocation.y(),
                   slider.width(), slider.height());

        // Update the region covered by the new and old control points.
        repaint(r + r2);

        // Emit a signal containing the index of the new control point.
        emit activeControlPointChanged(new_sel);
    }
    else if(new_sel == -1 && controlPoints->CanBeEdited() && !b_equalSpacing)
    {
        // The trough: check to see what side of the current control point we're on.
        new_sel = controlPoints->Rank(controlPoints->NumControlPoints() - 1);

        // If we're on the left of the current control point, decrement.
        bool page_decrement = (fpos < controlPoints->operator[](new_sel).position);

        if(page_decrement)
        {
            moveControlPoint(PAGE_DECREMENT);
            paging_mode = PAGE_DECREMENT;
            timer->start(150);
        }
        else
        {
            moveControlPoint(PAGE_INCREMENT);
            paging_mode = PAGE_INCREMENT;
            timer->start(150);
        }
    }

    // If a control point was clicked with the right mouse button, emit
    // a signal that will tell external customers that the control point
    // wants a new color.
    if(new_sel != -1 && e->button() == Qt::RightButton)
        colorSelected(new_sel);
*/
}

// ****************************************************************************
// Method: QvisSpectrumBar::mouseMoveEvent
//
// Purpose:
//   Handles mouse move events for the widget.
//
// Arguments:
//   e : The mouse move event.
//
// Notes:      Emits controlPointMoved signal.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:24:02 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 11:44:06 PDT 2002
//   Added code to return if we have equal spacing.
//
//   Brad Whitlock, Thu Mar 14 08:26:53 PDT 2002
//   Added code to make sure the control points are sorted if we are in
//   continuous update mode.
//
// ****************************************************************************

void
QvisSpectrumBar::mouseMoveEvent(QMouseEvent *e)
{
    // If the widget is doing paging events, ignore mouse motion.
    if(paging_mode != NO_PAGING || !controlPoints->CanBeEdited() || b_equalSpacing)
        return;

    // Indicate that the mouse is sliding a control point.
    b_sliding = true;

    int   current_sel;
    float fpos;

    // Figure the currently selected point.
    current_sel = controlPoints->Rank(controlPoints->NumControlPoints() - 1);

    // Figure the position of the event as a number in the range [0.,1.].
    if(orientation == HorizontalTop || orientation == HorizontalBottom)
    {
        fpos = ((float)(e->x() - controlsArea.x() - slider.width()/2) /
               (float)(controlsArea.width() - slider.width()));
    }
    else
    {
        fpos = ((float)(e->y() - controlsArea.y()) /
               (float)controlsArea.height());
    }

    // Clamp the position to [0, 1.]
    if(fpos < 0.) fpos = 0.;
    if(fpos > 1.) fpos = 1.;

    // If the position is different than the current position, udpate the
    // control area and emit a signal.
    if(fpos != controlPoints->Position(current_sel))
    {
        // Move the control point to its new location and redraw.
        moveControlPointRedraw(current_sel, fpos, continuousUpdate());

        // If we're emitting signals with each mouse movement, emit the
        // position of the current control point.
        if(continuousUpdate())
        {
            controlPoints->Sort();
            current_sel = controlPoints->Rank(controlPoints->NumControlPoints() - 1);
            emit controlPointMoved(current_sel, fpos);
        }
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::mouseReleaseEvent
//
// Purpose:
//   Handles mouse release events for the widget.
//
// Note:       Emits controlPointMoved signal.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:24:28 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::mouseReleaseEvent(QMouseEvent *)
{
    // If we're doing some paging, stop the timer.
    if(paging_mode != NO_PAGING)
    {
        timer->stop();
        paging_mode = NO_PAGING;
    }

    if(b_sliding)
    {
        b_sliding = false;

        if(!continuousUpdate())
        {
            drawSpectrum();
            QRegion r(spectrumArea.x(), spectrumArea.y(), spectrumArea.width(),
                      spectrumArea.height());
            repaint(r);

            // Emit a signal containing the index and position of the control
            // point with the highest rank.
            controlPoints->Sort();
            int index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);
            emit controlPointMoved(index, controlPoints->Position(index));
        }
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::updateControlPoints
//
// Purpose:
//   Changes the active control point.
//
// Note:       Emits activeControlPointChanged signal.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:26:40 PDT 2001
//
// Modifications:
//   Brad Whitlock, Thu Mar 14 08:30:06 PDT 2002
//   Added code to sort the control points.
//
// ****************************************************************************

void
QvisSpectrumBar::updateControlPoints()
{
    controlPoints->Sort();
    int index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);

    if(isVisible())
    {
        drawControls();
        update(controlsArea.x(), controlsArea.y(), controlsArea.width(),
               controlsArea.height());
    }
    else
        deletePixmap();

    // Emit a signal containing the index of the new control point.
    emit activeControlPointChanged(index);
}

// ****************************************************************************
// Method: QvisSpectrumBar::moveControlPointRedraw
//
// Purpose:
//   Moves a control point to a new position and redraws the controls.
//
// Arguments:
//   index          : The index of the control point to move.
//   pos            : The new control point position.
//   redrawSpectrum : Whether or not to redraw the spectrum.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:27:15 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 28 10:13:00 PDT 2001
//   Added code to suppress updates.
//
//   Brad Whitlock, Wed Mar 13 10:09:38 PDT 2002
//   Made it use a single pixmap.
//
// ****************************************************************************

void
QvisSpectrumBar::moveControlPointRedraw(int index, float pos, bool redrawSpectrum)
{
    // Get the current location of the control point.
    QPoint oldLocation(controlPointLocation(index));

    // Set the new position and draw the controls.
    controlPoints->SetPosition(index, pos);

    // If we're suppressing updates, delete the pixmaps and return.
    if(b_suppressUpdates)
    {
        deletePixmap();
        return;
    }

    if(isVisible())
    {
        drawControls();

        // Construct a region that covers the old and new locations of the control point.
        QPoint newLocation(controlPointLocation(index));
        QRegion r(newLocation.x(), newLocation.y(),
                  slider.width(), slider.height());
        QRegion r2(oldLocation.x(), oldLocation.y(),
                   slider.width(), slider.height());
        QRegion final(r + r2);

        // Update the spectrum
        if(redrawSpectrum)
        {
            drawSpectrum();
            QRegion r3(spectrumArea.x(), spectrumArea.y(), spectrumArea.width(),
                       spectrumArea.height());
            final = final + r3;
        }

        // Update the region covered by the new and old control points and
        // maybe the spectrum area. By only repainting those relatively small
        // rectangles, flicker due to repainting is much reduced.
        repaint(final);
    }
    else
        deletePixmap();
}

// ****************************************************************************
// Method: QvisSpectrumBar::moveControlPoint
//
// Purpose:
//   Moves a control point in the specified manner.
//
// Arguments:
//   changeType : How to move the control point.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:28:43 PDT 2001
//
// Modifications:
//   Brad Whitlock, Wed Mar 13 11:49:55 PDT 2002
//   Added code to return if we cannot modify the control point.
//
// ****************************************************************************

void
QvisSpectrumBar::moveControlPoint(int changeType)
{
    int   index;
    float increment, page_increment;

    // Return if we cannot modify the control point.
    if(!controlPoints->CanBeEdited() || b_equalSpacing)
        return;

    // Get the index of the highest ranking control point.
    index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);

    // Figure out the increments and page increments based on the
    // widget's orientation.
    if(orientation == HorizontalTop || orientation == HorizontalBottom)
    {
        increment = 1. /(float)controlsArea.width();
        page_increment = (float)slider.width() * increment;
    }
    else
    {
        increment = 1. /(float)controlsArea.height();
        page_increment = (float)slider.height() * increment;
    }

    // Change the value by the appropriate amount
    float old_position = controlPoints->Position(index);
    float new_position = controlPoints->Position(index);
    switch(changeType)
    {
    case INCREMENT:
        new_position += increment;
        break;
    case DECREMENT:
        new_position -= increment;
        break;
    case PAGE_INCREMENT:
        new_position += page_increment;
        break;
    case PAGE_DECREMENT:
        new_position -= page_increment;
        break;
    case PAGE_HOME:
        new_position = 0.;
        break;
    case PAGE_END:
        new_position = 1.;
        break;
    }

    // Make sure the new position is in [0., 1.]
    if(new_position < 0.) new_position = 0.;
    if(new_position > 1.) new_position = 1.;

    // If the new position differs from the old position, store the new
    // position and update the widget.
    if(new_position != old_position)
    {
        // Move the control point and redraw the spectrum and the controls.
        moveControlPointRedraw(index, new_position, true);

        // Emit a signal containing the index and position of the control
        // point with the highest rank.
        controlPoints->Sort();
        index = controlPoints->Rank(controlPoints->NumControlPoints() - 1);
        emit controlPointMoved(index, new_position);
    }
}

// ****************************************************************************
// Method: QvisSpectrumBar::handlePaging
//
// Purpose:
//   This is a Qt slot function that is called by the widget's timer in order
//   to move the control point by paging.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:29:46 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisSpectrumBar::handlePaging()
{
    moveControlPoint(paging_mode);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// Definitions of internally used classes and structs.
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

const int ControlPointList::CPLIST_INCREMENT = 5;
const ControlPoint ControlPointList::defaultControlPoint1 = {1, 0., {1., 0., 0.}};
const ControlPoint ControlPointList::defaultControlPoint2 = {0, 1., {0., 0., 1.}};

// ****************************************************************************
// Method: ControlPointList::ControlPointList
//
// Purpose:
//   Constructor for the ControlPointList class.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:30:38 PDT 2001
//
// Modifications:
//
// ****************************************************************************

ControlPointList::ControlPointList()
{
    editable = true;
    nvals = 0;
    colorvals = NULL;

    nels = 2;
    total_nels = CPLIST_INCREMENT;
    list = new ControlPoint[CPLIST_INCREMENT];

    // Init the control points.
    list[0] = defaultControlPoint1;
    list[1] = defaultControlPoint2;
}

// ****************************************************************************
// Method: ControlPointList::~ControlPointList
//
// Purpose:
//   Destructor for the ControlPointList class.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:31:01 PDT 2001
//
// Modifications:
//
// ****************************************************************************

ControlPointList::~ControlPointList()
{
    Clear();
}

// ****************************************************************************
// Method: ControlPointList::Clear
//
// Purpose:
//   Frees memory used by the class.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:31:20 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::Clear()
{
    nels = 0;
    total_nels = 0;
    delete [] list;
    nvals = 0;
    delete [] colorvals;
}

// ****************************************************************************
// Method: ControlPointList::ColorValue
//
// Purpose:
//   Returns the color value for the specified index.
//
// Arguments:
//   index : The index of the colorval to return.
//
// Returns:    The color value for the specified index.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:31:40 PDT 2001
//
// Modifications:
//
// ****************************************************************************

float
ControlPointList::ColorValue(int index) const
{
    if(nvals == 0 || colorvals == NULL || index < 0 || index >= nvals*3)
        return 0.;
    else
        return colorvals[index];
}

// ****************************************************************************
// Method: ControlPointList::operator []
//
// Purpose:
//   Returns a reference to the specified control point.
//
// Arguments:
//   index : The index of the specified control point.
//
// Returns:    A reference to the specified control point.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:32:24 PDT 2001
//
// Modifications:
//
// ****************************************************************************

const ControlPoint &
ControlPointList::operator [](int index) const
{
    if(nels == 0 || list == NULL || index < 0 || index >= nels)
        return defaultControlPoint1;
    else
        return list[index];
}

// ****************************************************************************
// Method: ControlPointList::Position
//
// Purpose:
//   Returns the position of the specified control point.
//
// Arguments:
//   index : The index of the control point.
//
// Returns:    The position of the specified control point.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:33:12 PDT 2001
//
// Modifications:
//
// ****************************************************************************

float
ControlPointList::Position(int index) const
{
    if(nels == 0 || list == NULL || index < 0 || index >= nels)
        return 0.;
    else
        return list[index].position;
}

// ****************************************************************************
// Method: ControlPointList::SetPosition
//
// Purpose:
//   Sets the position of a control point.
//
// Arguments:
//   index : The index of the control point to change.
//   pos   : The control point's new position.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:34:02 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::SetPosition(int index, float pos)
{
    if(nels != 0 && list != NULL && index >= 0 && index < nels)
        list[index].position = pos;
}

// ****************************************************************************
// Method: ControlPointList::NumControlPoints
//
// Purpose:
//   Returns the number of control points.
//
// Returns:    The number of control points.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:34:49 PDT 2001
//
// Modifications:
//
// ****************************************************************************

int
ControlPointList::NumControlPoints() const
{
    return nels;
}

// ****************************************************************************
// Method: ControlPointList::NumColorValues
//
// Purpose:
//   Returns the number of color values.
//
// Returns:    The number of color values.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:35:25 PDT 2001
//
// Modifications:
//
// ****************************************************************************

int
ControlPointList::NumColorValues() const
{
    return nvals;
}

// ****************************************************************************
// Method: ControlPointList::CanBeEdited
//
// Purpose:
//   Returns whether or not the control point list can be edited.
//
// Returns:    Whether or not the control point list can be edited.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:36:10 PDT 2001
//
// Modifications:
//
// ****************************************************************************

bool
ControlPointList::CanBeEdited() const
{
    return editable;
}

// ****************************************************************************
// Method: ControlPointList::SetEditMode
//
// Purpose:
//   Sets the list's edit mode.
//
// Arguments:
//   val : The new edit mode.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:36:44 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::SetEditMode(bool val)
{
    editable = val;
}

// ****************************************************************************
// Method: ControlPointList::SetColor
//
// Purpose:
//   Sets the color for the specified control point.
//
// Arguments:
//   index : The control point index.
//   r     : The red value [0., 1.]
//   g     : The green value [0., 1.]
//   b     : The blue value [0., 1.]
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:37:12 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::SetColor(int index, float r, float g, float b)
{
    if(nels != 0 && list != NULL && index >= 0 && index < nels)
    {
        list[index].color[0] = r;
        list[index].color[1] = g;
        list[index].color[2] = b;
    }
}

// ****************************************************************************
// Method: ControlPointList::SetColorValues
//
// Purpose:
//   Sets the color values for the list.
//
// Arguments:
//    colors  : The new colors array.
//    ncolors : The number of colors.
//
// Note:       The colors array is owned by this object after this call.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:38:11 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::SetColorValues(const float *colors, int ncolors)
{
    // If there are already raw colors in the list, free them.
    delete [] colorvals;

    // Make this object own the colors array that is passed in.
    nvals = ncolors;
    colorvals = (float *)colors;
}

// ****************************************************************************
// Method: ControlPointList::Add
//
// Purpose: Adds a new control point to the cp_list. The new control point
//          gets the highest rank and becomes the selected point.
//
// Arguments:
//    cpt : The control point being added.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:39:19 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::Add(const ControlPoint *cpt)
{
    ControlPoint *newList = NULL;

    // See if we need to resize.
    if(nels + 1 > total_nels)
    {
        total_nels += CPLIST_INCREMENT;
        newList = new ControlPoint[total_nels];

        // Copy the elements into the new array.
        memcpy((char *)newList,(char *)list, nels * sizeof(ControlPoint));

        // Update the lists.
        delete [] list;
        list = newList;
    }

    // Put the new control point in the list. Ignore any rank it might
    // have. We're going to manage them here.
    memcpy((char *)&list[nels],(char *)cpt, sizeof(ControlPoint));
    list[nels].rank = nels;
    ++nels;

    // Sort the points.
    Sort();
}

// ****************************************************************************
// Method: ControlPointList::Rank
//
// Purpose:
//   Finds the index of the control point with a given rank.
//
// Arguments:
//   rank : The rank we're looking for.
//
// Returns:    The index of the control point with a given rank.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:40:44 PDT 2001
//
// Modifications:
//
// ****************************************************************************

int
ControlPointList::Rank(int rank) const
{
    int i, retval = 0;

    for(i = 0; i < nels; ++i)
        if(rank == list[i].rank)
        {
            retval = i;
            break;
        }

    return retval;
}

// ****************************************************************************
// Method: ControlPointList::DeleteHighestRank
//
// Purpose:
//   Deletes the control point with the highest rank.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:41:29 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::DeleteHighestRank()
{
    int rank = 0;

    if(nels <= 2)
        return;

    // Get the rank of the point we're deleting.
    rank = Rank(nels - 1);

    // Move all of the control points and ranks that were to the
    // right of the point that we want to delete.
    if(rank != nels - 1)
    {
        memcpy((char *)&list[rank],(char *)&list[rank + 1],
              (nels - 1 - rank) * sizeof(ControlPoint));
    }

    // Reduce the length of the list.
    --nels;
}

// ****************************************************************************
// Method: ControlPointList::ChangeSelectedIndex
//
// Purpose:
//   Finds the control point that is being selected.
//
// Arguments:
//    pos   : A float value in the range [0,1] that represents the center of
//            a control point.
//    width : The width of a control point in terms of a number(0,1).
//    equal : Whether or not the control points are equally spaced.
//
// Returns:    The control point that is being selected or -1 is not control
//             point was at the specified position.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:42:16 PDT 2001
//
// Modifications:
//
// ****************************************************************************

int
ControlPointList::ChangeSelectedIndex(float pos, float width, int equal)
{
    int   i, index, retval = -1;
    float position, offset, wd2 = width * 0.6;

    if(equal)
        offset = 1. / nels;
    else
        offset = 0.;

    // Descend through the ranks.
    for(i = nels - 1; i >= 0; --i)
    {
        // Find the index with rank i.
        index = Rank(i);

        if(equal)
            position = (index *(1. - offset)) +(offset * 0.5);
        else
            position = list[index].position;

        /* See if the position is in the control point. */
        if((pos - wd2 <= position) &&(position <= pos + wd2))
        {
            retval = index;
            break;
        }
    }

    // If we found a control point, adjust the ranks so it
    // has the highest rank.
    if(retval >= 0)
    {
        GiveHighestRank(retval);
    }

    return retval;
}

// ****************************************************************************
// Method: ControlPointList::GiveHighestRank
//
// Purpose:
//   Gives the highest rank to the specified control point.
//
// Arguments:
//   index : The index of control point we want to have the highest rank.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:44:43 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::GiveHighestRank(int index)
{
    for(int i = 0; i < nels; ++i)
    {
        if(list[i].rank > list[index].rank)
            --(list[i].rank);
    }

    // Give the specified point the highest rank.
    list[index].rank = nels - 1;
}

// ****************************************************************************
// Method: ControlPointList::Sort
//
// Purpose:
//   Sorts the list.
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:45:34 PDT 2001
//
// Modifications:
//
// ****************************************************************************

void
ControlPointList::Sort()
{
    qsort((void *)list, nels, sizeof(ControlPoint), ControlPointCompare);
}

// ****************************************************************************
// Function: ControlPointCompare
//
// Purpose:
//   Compare two control_pt variables. This is used as a callback for qsort.
//
// Arguments:
//   c1 : A pointer to the first ControlPoint struct.
//   c2 : A pointer to the second ControlPoint struct.
//
//
// Programmer: Brad Whitlock
// Creation:   Wed Jan 3 11:45:59 PDT 2001
//
// Modifications:
//
// ****************************************************************************

static int
ControlPointCompare(const void *c1, const void *c2)
{
    ControlPoint *cp1 = (ControlPoint *)c1;
    ControlPoint *cp2 = (ControlPoint *)c2;

    if(cp1->position < cp2->position)
        return -1;
    else if(cp1->position == cp2->position)
    {
        if(cp1->rank < cp2->rank)
            return -1;
        else if(cp1->rank == cp2->rank)
            return 0;
        else
            return 1;
    }
    else
        return 1;
}
