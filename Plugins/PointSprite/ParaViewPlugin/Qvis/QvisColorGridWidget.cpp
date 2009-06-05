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

#include <qcursor.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <QKeyEvent>
#include "QvisColorGridWidget.h"

// ****************************************************************************
// Method: QvisColorGridWidget::QvisColorGridWidget
//
// Purpose:
//   Constructor for the QvisColorGridWidget class.
//
// Arguments:
//   parent : The parent widget to this object.
//   name   : The name of this object.
//   f      : The window flags. These control how window decorations are done.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:51:26 PST 2000
//
// Modifications:
//   Brad Whitlock, Thu Nov 21 17:13:29 PST 2002
//   Made boxSize and boxPadding values that can be set.
//
// ****************************************************************************

QvisColorGridWidget::QvisColorGridWidget(QWidget *parent, const char* /*name*/)
: QWidget(parent)
{
    numRows = 1;
    numColumns = 1;

    drawFrame = false;

    currentActiveColor = -1;
    currentSelectedColor = -1;

    numPaletteColors = 0;
    paletteColors = 0;

    drawPixmap = 0;

    boxSizeValue = 16;
    boxPaddingValue = 8;
    setMinimumSize(minimumSize());

    // Set the default size policy.
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
        QSizePolicy::MinimumExpanding));
}

// ****************************************************************************
// Method: QvisColorGridWidget::~QvisColorGridWidget
//
// Purpose:
//   Destructor for the QvisColorGridWidget class.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:52:32 PST 2000
//
// Modifications:
//
// ****************************************************************************

QvisColorGridWidget::~QvisColorGridWidget()
{
    if(paletteColors)
       delete [] paletteColors;

    if(drawPixmap)
       delete drawPixmap;
}

// ****************************************************************************
// Method: QvisColorGridWidget::sizeHint
//
// Purpose:
//   Returns the widget's favored size.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:52:56 PST 2000
//
// Modifications:
//
// ****************************************************************************

QSize
QvisColorGridWidget::sizeHint() const
{
    return QSize(boxSizeValue * numColumns + boxPaddingValue * (numColumns + 1),
                 boxSizeValue * numRows + boxPaddingValue * (numRows + 1));
}

// ****************************************************************************
// Method: QvisColorGridWidget::minimumSize
//
// Purpose:
//   Returns the minimum size for the widget.
//
// Programmer: Brad Whitlock
// Creation:   Thu Nov 21 18:22:34 PST 2002
//
// Modifications:
//
// ****************************************************************************

QSize
QvisColorGridWidget::minimumSize() const
{
    return QSize(boxSizeValue * numColumns + boxPaddingValue * (numColumns + 1),
                 boxSizeValue * numRows + boxPaddingValue * (numRows + 1));
}

//
// Properties.
//

void
QvisColorGridWidget::setBoxSize(int val)
{
    boxSizeValue = val;
    setMinimumSize(minimumSize());
}

void
QvisColorGridWidget::setBoxPadding(int val)
{
    boxPaddingValue = val;
    setMinimumSize(minimumSize());
}

int
QvisColorGridWidget::boxSize() const
{
    return boxSizeValue;
}

int
QvisColorGridWidget::boxPadding() const
{
    return boxPaddingValue;
}

// ****************************************************************************
// Method: QvisColorGridWidget::setPaletteColors
//
// Purpose:
//   Sets the widget's palette colors. These are the colors from which the
//   user can select.
//
// Arguments:
//   c    : The array of QColors to copy. Must be rows*columns elements long.
//   rows : The number of rows to draw.
//   cols : The number of columns to draw.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:53:25 PST 2000
//
// Modifications:
//   Brad Whitlock, Fri Nov 22 15:27:27 PST 2002
//   I fixed a bug that prevented the widget from updating correctly.
//
//   Brad Whitlock, Wed Feb 26 12:54:04 PDT 2003
//   I made it take a number of colors instead of row and column. Then I added
//   a suggested columns default argument to set the number of columns that
//   we'd like to use.
//
// ****************************************************************************

void
QvisColorGridWidget::setPaletteColors(const QColor *c, int nColors,
    int suggestedColumns)
{
    if(c != 0 && nColors > 0)
    {
        if(paletteColors)
            delete [] paletteColors;

        // Copy the color array.
        numPaletteColors = nColors;
        paletteColors = new QColor[numPaletteColors];
        for(int i = 0; i < numPaletteColors; ++i)
            paletteColors[i] = c[i];

        // Figure out the number of rows and columns.
        numColumns = suggestedColumns;
        if(numColumns < 1)
            numColumns = 6;
        numRows = nColors / numColumns;
        if(numRows < 1)
            numRows = 1;
        if(numRows * numColumns < nColors)
            ++numRows;

        // Adjust the active and selected colors if necessary.
        currentActiveColor = -1;
        if(currentSelectedColor >= numPaletteColors)
            currentSelectedColor = -1;

        // Make the widget repaint if it is visible.
        if(isVisible())
        {
            delete drawPixmap;
            drawPixmap = 0;
            update();
        }
        else if(drawPixmap)
        {
            delete drawPixmap;
            drawPixmap = 0;
        }
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::setPaletteColor
//
// Purpose:
//   Sets the color of an individual color box and updates the widget.
//
// Arguments:
//   color : The new color.
//   index : The index of the color we're changing.
//
// Programmer: Brad Whitlock
// Creation:   Tue Dec 5 12:43:58 PDT 2000
//
// Modifications:
//   Brad Whitlock, Fri Apr 26 11:36:42 PDT 2002
//   I fixed an error that cropped up on Windows.
//
//   Brad Whitlock, Wed Feb 26 12:53:07 PDT 2003
//   I made it take a single index argument instead of row and column.
//
// ****************************************************************************

void
QvisColorGridWidget::setPaletteColor(const QColor &color, int index)
{
    if(index >= 0 && index < numPaletteColors)
    {
        // If the colors are different, update the widget.
        if(color != paletteColors[index])
        {
            QRegion region;

            // Replace the color
            paletteColors[index] = color;

            // Redraw the color in the appropriate manner.
            if(index == currentSelectedColor)
                region = drawSelectedColor(0, index);
            else if(index == activeColorIndex())
                region = drawHighlightedColor(0, index);
            else
            {
                int x, y, w, h;
                getColorRect(index, x, y, w, h);
                region = QRegion(x, y, w, h);

                if(drawPixmap)
                {
                    QPainter paint(drawPixmap);
                    drawColor(paint, index);
                }
            }

            // Repaint the region that was changed.
            if(isVisible())
                repaint(region);
            else if(drawPixmap)
            {
                delete drawPixmap;
                drawPixmap = 0;
            }
        }
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::setSelectedColorIndex
//
// Purpose:
//   Sets the selected color for the widget. If the new selected color differs
//   from the old one, the selectedColor signal is emitted.
//
// Arguments:
//   index : The index of the new selected color. 0..rows*cols.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:56:36 PST 2000
//
// Modifications:
//   Brad Whitlock, Fri Apr 26 11:49:58 PDT 2002
//   I fixed an error that cropped up on windows.
//
//   Brad Whitlock, Thu Nov 21 10:47:42 PDT 2002
//   I made it emit a signal that contains the color as well as the row and
//   column of the color that changed.
//
//   Brad Whitlock, Wed Feb 26 12:51:24 PDT 2003
//   I renamed the method and made it emit another selectedColor signal.
//
// ****************************************************************************

void
QvisColorGridWidget::setSelectedColorIndex(int index)
{
    if(index >= -1 && index < numPaletteColors)
    {
        QRegion region;

        // If we currently have a selected color, unhighlight it.
        if(currentSelectedColor != -1)
            region = drawUnHighlightedColor(0, currentSelectedColor);

        // Set the new value.
        currentSelectedColor = index;

        // If the selected color that we set is a real color, highlight
        // the new selected color.
        if(currentSelectedColor != -1)
        {
            region = region + drawSelectedColor(0, currentSelectedColor);
        }

        // Update the widget.
        if(isVisible())
            repaint(region);
        else if(drawPixmap)
        {
            delete drawPixmap;
            drawPixmap = 0;
        }

        // emit the selectedColor signal.
        if(currentSelectedColor != -1)
        {
            emit selectedColor(paletteColors[currentSelectedColor]);
            emit selectedColor(paletteColors[currentSelectedColor],
                               currentSelectedColor);
            int row, column;
            getRowColumnFromIndex(currentSelectedColor, row, column);
            emit selectedColor(paletteColors[currentSelectedColor], row, column);
        }
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::setSelectedColor
//
// Purpose:
//   Sets the selected color for the widget. If the new selected color differs
//   from the old one, the selectedColor signal is emitted.
//
// Arguments:
//   color : The color to select.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:56:36 PST 2000
//
// Modifications:
//   Brad Whitlock, Wed Feb 26 12:50:49 PDT 2003
//   Changed the name of a method that gets called.
//
// ****************************************************************************

void
QvisColorGridWidget::setSelectedColor(const QColor &color)
{
    // Figure out the index of the color. If it is not in the palette, we'll
    // end up unselecting the currently selected color.
    int index = -1;
    bool notFound = true;
    for(int i = 0; i < numPaletteColors && notFound; ++i)
    {
        if(color == paletteColors[i])
        {
            index = i;
            notFound = false;
        }
    }

    // Set the selected color.
    setSelectedColorIndex(index);
}

// ****************************************************************************
// Method: QvisColorGridWidget::setFrame
//
// Purpose:
//   Turns the frame around the widget on or off and causes the widget to
//   redraw itself accordingly.
//
// Arguments:
//   val : A boolean value indicating whether or not to draw the frame.
//
// Programmer: Brad Whitlock
// Creation:   Tue Dec 5 10:35:25 PDT 2000
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::setFrame(bool val)
{
    if(val != drawFrame)
    {
        drawFrame = val;

        if(drawPixmap)
        {
            delete drawPixmap;
            drawPixmap = 0;
        }

        if(isVisible())
        {
            // Make the widget redraw itself.
            update();
        }
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::rows
//
// Purpose:
//   Returns the number of color rows.
//
// Returns:    Returns the number of color rows.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:57:44 PST 2000
//
// Modifications:
//
// ****************************************************************************

int
QvisColorGridWidget::rows() const
{
    return numRows;
}

// ****************************************************************************
// Method: QvisColorGridWidget::columns
//
// Purpose:
//   Returns the number of color columns.
//
// Returns:    Returns the number of color columns.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:57:44 PST 2000
//
// Modifications:
//
// ****************************************************************************

int
QvisColorGridWidget::columns() const
{
    return numColumns;
}

// ****************************************************************************
// Method: QvisColorGridWidget::selectedColor
//
// Purpose:
//   Returns the selected color.
//
// Returns:    Returns the index of the selected color.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:59:16 PST 2000
//
// Modifications:
//
// ****************************************************************************

QColor
QvisColorGridWidget::selectedColor() const
{
    QColor retval;

    if(currentSelectedColor != -1)
        retval = paletteColors[currentSelectedColor];

    return retval;
}

// ****************************************************************************
// Method: QvisColorGridWidget::selectedColorIndex
//
// Purpose:
//   Returns the row and column of the selected color.
//
// Arguments:
//   row : The row of the selected color.
//   column : The column of the selected color.
//
// Returns:    The row and column of the selected color.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Thu Nov 21 10:54:04 PDT 2002
//
// Modifications:
//   Brad Whitlock, Wed Feb 26 12:47:46 PDT 2003
//   I renamed it and made it return the single color index.
//
// ****************************************************************************

int
QvisColorGridWidget::selectedColorIndex() const
{
    return currentSelectedColor;
}

// ****************************************************************************
// Method: QvisColorGridWidget::paletteColor
//
// Purpose:
//   Returns the color at the specified row and column.
//
// Returns:    The color at the specified row and column.
//
// Programmer: Brad Whitlock
// Creation:   Tue Dec 5 13:16:12 PST 2000
//
// Modifications:
//   Brad Whitlock, Wed Feb 26 12:47:06 PDT 2003
//   Made it take a single argument.
//
// ****************************************************************************

QColor
QvisColorGridWidget::paletteColor(int index) const
{
    QColor retval;

    if(index >= 0 && index < numPaletteColors)
    {
        retval = paletteColors[index];
    }

    return retval;
}

// ****************************************************************************
// Method: QvisColorGridWidget::containsColor
//
// Purpose:
//   Searches the color palette for a specified color.
//
// Returns:    Whether or not the color is in the palette.
//
// Programmer: Brad Whitlock
// Creation:   Tue Dec 5 16:41:47 PST 2000
//
// Modifications:
//
// ****************************************************************************

bool
QvisColorGridWidget::containsColor(const QColor &color) const
{
    bool notFound = true;

    for(int i = 0; i < numPaletteColors && notFound; ++i)
    {
        if(color == paletteColors[i])
            notFound = false;
    }

    return !notFound;
}

// ****************************************************************************
// Method: QvisColorGridWidget::activeColorIndex
//
// Purpose:
//   Returns the index of the active color.
//
// Returns:    Returns the index of the active color.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:57:44 PST 2000
//
// Modifications:
//
// ****************************************************************************

int
QvisColorGridWidget::activeColorIndex() const
{
    return currentActiveColor;
}

// ****************************************************************************
// Method: QvisColorGridWidget::setActiveColor
//
// Purpose:
//   Sets the active color. This is the color that is being considered but is
//   not yet selected.
//
// Arguments:
//   index : The index of the color to select. This is from 0..rows*cols.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:55:11 PST 2000
//
// Modifications:
//   Brad Whitlock, Fri Apr 26 11:49:20 PDT 2002
//   I fixed an error that cropped up on windows.
//
//   Brad Whitlock, Wed Feb 26 13:09:42 PST 2003
//   I made some internal interface changes.
//
// ****************************************************************************

void
QvisColorGridWidget::setActiveColorIndex(int index)
{
    if(index >= -1 && index < numPaletteColors)
    {
        QRegion region;

        // If we currently have an active color, unhighlight it.
        if(activeColorIndex() != -1)
        {
            if(activeColorIndex() == currentSelectedColor)
                region = drawSelectedColor(0, activeColorIndex());
            else
                region = drawUnHighlightedColor(0, activeColorIndex());
        }

        currentActiveColor = index;

        // If the active color that we set is a real color, highlight the new
        // active color.
        if(activeColorIndex() == currentSelectedColor)
            region = region + drawSelectedColor(0, activeColorIndex());
        else if(activeColorIndex() != -1)
            region = region + drawHighlightedColor(0, activeColorIndex());

        // Update the pixmap.
        if(isVisible())
            repaint(region);
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::keyPressEvent
//
// Purpose:
//   This is the event handler for keypresses. It allows the user to operate
//   this widget with the keyboard.
//
// Arguments:
//   e : The key event.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 19:59:53 PST 2000
//
// Modifications:
//   Brad Whitlock, Wed Feb 26 13:10:56 PST 2003
//   Made some internal interface changes.
//
// ****************************************************************************

void
QvisColorGridWidget::keyPressEvent(QKeyEvent *e)
{
    QColor temp;
    int    column = activeColorIndex() % numColumns;
    int    row = activeColorIndex() / numColumns;

    // Handle the key strokes.
    switch(e->key())
    {
    case Qt::Key_Escape:
        // emit an empty color.
        emit selectedColor(temp);
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        setSelectedColorIndex(activeColorIndex());
        break;
    case Qt::Key_Left:
        if(column == 0)
            setActiveColorIndex(getIndex(row, numColumns - 1));
        else
            setActiveColorIndex(getIndex(row, column - 1));
        break;
    case Qt::Key_Right:
        if(column == numColumns - 1)
            setActiveColorIndex(getIndex(row, 0));
        else
            setActiveColorIndex(getIndex(row, column + 1));
        break;
    case Qt::Key_Up:
        if(row == 0)
            setActiveColorIndex(getIndex(numRows - 1, column));
        else
            setActiveColorIndex(getIndex(row - 1, column));
        break;
    case Qt::Key_Down:
        if(row == numRows - 1)
            setActiveColorIndex(getIndex(0, column));
        else
            setActiveColorIndex(getIndex(row + 1, column));
        break;
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::enterEvent
//
// Purpose:
//   Turns on mouse tracking when the mouse enters this widget.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:00:48 PST 2000
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::enterEvent(QEvent *)
{
    // We've entered the widget, turn on mouse tracking.
    setMouseTracking(true);
}

// ****************************************************************************
// Method: QvisColorGridWidget::leaveEvent
//
// Purpose:
//   Turns off mouse tracking when the mouse leaves this widget.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:01:13 PST 2000
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::leaveEvent(QEvent *)
{
    // We've left the widget, turn off mouse tracking.
    setMouseTracking(false);

    // Indicate that no color is active.
    setActiveColorIndex(-1);
}

// ****************************************************************************
// Method: QvisColorGridWidget::mousePressEvent
//
// Purpose:
//   This method is called when the mouse is clicked in the widget. It sets
//   the selected color based on the color that was clicked.
//
// Arguments:
//   e : The mouse event.
//
// Programmer: Brad Whitlock
// Creation:   Thu Nov 21 11:07:54 PDT 2002
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::mousePressEvent(QMouseEvent *e)
{
  if(e->button() == Qt::RightButton)
    {
        int index = getColorIndex(e->x(), e->y());

        // If a valid color index was returned, select the color.
        if(index != -1)
        {
            // Set the selected color.
            setSelectedColorIndex(index);

            // Emit a signal that allows us to activate a menu.
            int row, column;
            QPoint center(e->x(), e->y());
            getRowColumnFromIndex(currentSelectedColor, row, column);
            emit activateMenu(selectedColor(), row, column,
                              mapToGlobal(center));
        }
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::mouseMoveEvent
//
// Purpose:
//   This method is called when the mouse is moved in the widget. Its job is to
//   determine whether or not the active color needs to be updated.
//
// Arguments:
//   e : The mouse event.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:01:46 PST 2000
//
// Modifications:
//   Brad Whitlock, Wed Feb 26 13:12:46 PST 2003
//   Internal interface changes.
//
// ****************************************************************************

void
QvisColorGridWidget::mouseMoveEvent(QMouseEvent *e)
{
    int index = getColorIndex(e->x(), e->y());

    // If we've moved the mouse to a new active color, unhighlight the old one
    // and highlight the new one.
    if(index != activeColorIndex())
    {
        setActiveColorIndex(index);
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::mouseReleaseEvent
//
// Purpose:
//   This method is called when the mouse is clicked in the widget. It sets
//   the selected color based on the color that was clicked.
//
// Arguments:
//   e : The mouse event.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:02:43 PST 2000
//
// Modifications:
//   Brad Whitlock, Wed Feb 26 13:12:59 PST 2003
//   Internal interface changes.
//
// ****************************************************************************

void
QvisColorGridWidget::mouseReleaseEvent(QMouseEvent *e)
{
    int index = getColorIndex(e->x(), e->y());

    // If a valid color index was returned, select the color.
    if(index != -1)
    {
        // Set the selected color.
        setSelectedColorIndex(index);
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::paintEvent
//
// Purpose:
//   This method handles repainting the widget on the screen.
//
// Arguments:
//   e : The paint event.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:03:36 PST 2000
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::paintEvent(QPaintEvent *e)
{
    // If the pixmap has not been created, create it and draw into it.
    if(drawPixmap == 0)
    {
        drawPixmap = new QPixmap(width(), height());
        drawColorArray();
    }

    // Blit the pixmap onto the widget.
    QPainter paint;
    paint.begin(this);
    if(!e->region().isEmpty())
    {
        paint.setClipRegion(e->region());
        paint.setClipping(true);
    }
    paint.drawPixmap(0, 0, *drawPixmap);
    paint.end();
}

// ****************************************************************************
// Method: QvisColorGridWidget::resizeEvent
//
// Purpose:
//   This method deletes the drawing pixmap so it will be redrawn in the new
//   size when the next paint event happens.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:04:15 PST 2000
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::resizeEvent(QResizeEvent *)
{
    // Delete the pixmap so the entire widget will be redrawn.
    if(drawPixmap)
    {
        delete drawPixmap;
        drawPixmap = 0;
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::getColorIndex
//
// Purpose:
//   Computes a color index given an x,y location in the widget.
//
// Arguments:
//   x : The x location.
//   y : The y location.
//
// Returns:   The color index given at the x,y location in the widget.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:04:59 PST 2000
//
// Modifications:
//
// ****************************************************************************

int
QvisColorGridWidget::getColorIndex(int x, int y) const
{
    int index = -1;

    // See if the x,y coordinate is in the widget.
    if(QRect(0, 0, width(), height()).contains(QPoint(x, y)))
    {
        int boxWidth  = (width()  - boxPaddingValue) / numColumns;
        int boxHeight = (height() - boxPaddingValue) / numRows;

        int column = (x - boxPaddingValue) / boxWidth;
        int row = (y - boxPaddingValue) / boxHeight;
        index = getIndex(row, column);
    }

    return index;
}

// ****************************************************************************
// Method: QvisColorGridWidget::getIndex
//
// Purpose:
//   Computes a color index given a row, column.
//
// Arguments:
//   row : The row of the color.
//   column : The column of the color.
//
// Returns:    The color index given the row,column.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:06:05 PST 2000
//
// Modifications:
//
// ****************************************************************************

int
QvisColorGridWidget::getIndex(int row, int column) const
{
    return (row * numColumns) + column;
}

// ****************************************************************************
// Method: QvisColorGridWidget::getRowColumnFromIndex
//
// Purpose:
//   Computes the row and column from a color index.
//
// Arguments:
//   index  : A color index.
//   row    : The row that contains the index.
//   column : The column that contains the index.
//
// Returns: The row and column of the index.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Thu Nov 21 10:45:26 PDT 2002
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::getRowColumnFromIndex(int index, int &row, int &column) const
{
    row = index / numColumns;
    column = index % numColumns;
}

// ****************************************************************************
// Method: QvisColorGridWidget::getColorRect
//
// Purpose:
//   Figures out the location of the color box in the widget for a specified
//   color index.
//
// Arguments:
//   index : The color index for which we want geometry.
//   x : A reference to an int in which we'll store the x value.
//   y : A reference to an int in which we'll store the y value.
//   w : A reference to an int in which we'll store the width value.
//   h : A reference to an int in which we'll store the height value.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:07:12 PST 2000
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::getColorRect(int index, int &x, int &y,
    int &w, int &h)
{
    int column = index % numColumns;
    int row = index / numColumns;

    int boxWidth  = (width() - boxPaddingValue) / numColumns;
    int boxHeight = (height() -  boxPaddingValue) / numRows;

    // Figure out the x,y location.
    x = column * boxWidth + boxPaddingValue;
    y = row * boxHeight + boxPaddingValue;

    // Figure out the width, height.
    w = boxWidth - boxPaddingValue;
    h = boxHeight - boxPaddingValue;
}

// ****************************************************************************
// Method: QvisColorGridWidget::drawColorArray
//
// Purpose:
//   Draws all of the color boxes into the drawing pixmap.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:09:03 PST 2000
//
// Modifications:
//   Brad Whitlock, Mon Mar 11 11:29:27 PDT 2002
//   Rewrote so it does not use styles.
//
//   Brad Whitlock, Fri Apr 26 11:48:22 PDT 2002
//   I fixed an error that cropped up on windows.
//
//   Brad Whitlock, Wed Feb 26 13:00:03 PST 2003
//   I made it capable of drawing an incomplete row of colors.
//
//   Brad Whitlock, Thu Aug 21 15:36:18 PST 2003
//   I changed how the brush to draw the background is selected so it looks
//   better on MacOS X.
//
// ****************************************************************************

void
QvisColorGridWidget::drawColorArray()
{
    // Fill the pixmap with the background color or draw a frame.
    QPainter paint(drawPixmap);

#ifdef Q_WS_MACX
    paint.fillRect(rect(), palette().brush(QPalette::Background));
#else
    paint.fillRect(rect(), palette().brush(QPalette::Button));
#endif

    if(drawFrame)
    {
      drawBox(paint, rect(), palette().light().color(),
                palette().dark().color());
    }

    // Draw all of the color boxes.
    int index = 0;
    for(int i = 0; i < numRows; ++i)
    {
        for(int j = 0; j < numColumns; ++j, ++index)
        {
            if(index < numPaletteColors)
            {
                if(index == currentSelectedColor)
                    drawSelectedColor(&paint, index);
                else if(index == activeColorIndex())
                    drawHighlightedColor(&paint, index);
                else
                    drawColor(paint, index);
            }
        }
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::drawBox
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
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Tue Mar 12 18:45:56 PST 2002
//
// Modifications:
//
// ****************************************************************************

void
QvisColorGridWidget::drawBox(QPainter &paint, const QRect &r,
    const QColor &light, const QColor &dark, int lw)
{
    int i;
    int X  = r.x();
    int X2 = r.x() + r.width() - 1;
    int Y  = r.y();
    int Y2 = r.y() + r.height() - 1;

    // Draw the highlight
    paint.setPen(QPen(light));
    for(i = 0; i < lw; ++i)
    {
        paint.drawLine(QPoint(X + i, Y + i), QPoint(X + i, Y2 - i));
        paint.drawLine(QPoint(X + i, Y + i), QPoint(X2 - i, Y + i));
    }

    // Draw the shadow
    paint.setPen(QPen(dark));
    for(i = 0; i < lw; ++i)
    {
        paint.drawLine(QPoint(X + i + 1, Y2 - i), QPoint(X2, Y2 - i));
        paint.drawLine(QPoint(X2 - i, Y + i + 1), QPoint(X2 - i, Y2));
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::drawColor
//
// Purpose:
//   Draws the specified color box into the drawing pixmap.
//
// Arguments:
//   paint : The painter to use.
//   index : The index of the color to draw.
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:09:29 PST 2000
//
// Modifications:
//   Brad Whitlock, Fri Apr 26 11:47:44 PDT 2002
//   I fixed an error that cropped up on windows.
//
// ****************************************************************************

void
QvisColorGridWidget::drawColor(QPainter &paint, int index)
{
    if(index >= 0)
    {
        // Get the location of the index'th color box.
        int x, y, boxWidth, boxHeight;
        getColorRect(index, x, y, boxWidth, boxHeight);

        paint.setPen(palette().dark().color());
        paint.drawRect(x, y, boxWidth, boxHeight);
        paint.fillRect(x + 1, y + 1, boxWidth - 2, boxHeight - 2,
                       paletteColors[index]);
    }
}

// ****************************************************************************
// Method: QvisColorGridWidget::drawHighlightedColor
//
// Purpose:
//   Draws a highlighted color box.
//
// Arguments:
//   index : The index of the color box to draw.
//
// Returns:    The region that was covered by drawing.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:10:27 PST 2000
//
// Modifications:
//   Brad Whitlock, Tue Mar 12 18:48:55 PST 2002
//   Removed the style coding in favor of a custom drawing routine.
//
// ****************************************************************************

QRegion
QvisColorGridWidget::drawHighlightedColor(QPainter *paint, int index)
{
    QRegion retval;

    if(drawPixmap && index >= 0)
    {
        // Get the location of the index'th color box.
        int x, y, boxWidth, boxHeight;
        getColorRect(index, x, y, boxWidth, boxHeight);

        QRect r(x - boxPaddingValue / 2, y - boxPaddingValue / 2,
                boxWidth + boxPaddingValue, boxHeight + boxPaddingValue);

        // Draw the button and the color over the button.
        if(paint == 0)
        {
            QPainter p2(drawPixmap);
            drawBox(p2, r, palette().light().color(),
                    palette().dark().color());
            drawColor(p2, index);
        }
        else
        {
            drawBox(*paint, r, palette().light().color(),
                    palette().dark().color());
            drawColor(*paint, index);
        }

        // return the region that we drew on.
        retval = QRegion(x - boxPaddingValue / 2, y - boxPaddingValue / 2,
            boxWidth + boxPaddingValue, boxHeight + boxPaddingValue);
    }

    return retval;
}

// ****************************************************************************
// Method: QvisColorGridWidget::drawUnHighlightedColor
//
// Purpose:
//   Draws an unhighlighted color box.
//
// Arguments:
//   paint : The painter to use or 0.
//   index : The index of the color box to draw.
//
// Returns:    The region that was covered by drawing.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:10:27 PST 2000
//
// Modifications:
//   Brad Whitlock, Wed Aug 22 14:49:52 PST 2001
//   I changed the color used to draw the rectange from background to button
//   and it seems to look correct for all of the styles.
//
//   Brad Whitlock, Fri Apr 26 11:37:12 PDT 2002
//   I fixed an error that cropped up on windows.
//
//   Brad Whitlock, Thu Aug 21 15:38:21 PST 2003
//   I changed how the brush is selected so it looks better on MacOS X.
//
// ****************************************************************************

QRegion
QvisColorGridWidget::drawUnHighlightedColor(QPainter *paint, int index)
{
    QRegion retval;

    if(drawPixmap && index >= 0)
    {
        // Get the location of the index'th color box.
        int x, y, boxWidth, boxHeight;
        getColorRect(index, x, y, boxWidth, boxHeight);

#ifdef Q_WS_MACX
        QBrush brush(palette().brush(QPalette::Background));
#else
        QBrush brush(palette().brush(QPalette::Button));
#endif

        // Draw the button and the color over the button.
        if(paint == 0)
        {
            QPainter p2(drawPixmap);
            p2.fillRect(x - boxPaddingValue / 2, y - boxPaddingValue / 2,
                        boxWidth + boxPaddingValue, boxHeight + boxPaddingValue,
                        brush);
            drawColor(p2, index);
        }
        else
        {
            paint->fillRect(x - boxPaddingValue / 2, y - boxPaddingValue / 2,
                            boxWidth + boxPaddingValue, boxHeight + boxPaddingValue,
                            brush);
            drawColor(*paint, index);
        }

        // return the region that we drew on.
        retval = QRegion(x - boxPaddingValue / 2, y - boxPaddingValue / 2,
            boxWidth + boxPaddingValue, boxHeight + boxPaddingValue);
    }

    return retval;
}

// ****************************************************************************
// Method: QvisColorGridWidget::drawSelectedColor
//
// Purpose:
//   Draws a selected color box.
//
// Arguments:
//   index : The index of the color box to draw.
//
// Returns:    The region that was covered by drawing.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 20:10:27 PST 2000
//
// Modifications:
//   Brad Whitlock, Tue Mar 12 18:48:55 PST 2002
//   Removed the style coding in favor of a custom drawing routine.
//
//   Brad Whitlock, Fri Apr 26 11:37:12 PDT 2002
//   I fixed an error that cropped up on windows.
//
// ****************************************************************************

QRegion
QvisColorGridWidget::drawSelectedColor(QPainter *paint, int index)
{
    QRegion retval;

    if(drawPixmap && index >= 0)
    {
        // Get the location of the index'th color box.
        int x, y, boxWidth, boxHeight;
        getColorRect(index, x, y, boxWidth, boxHeight);

        QRect r(x - boxPaddingValue / 2, y - boxPaddingValue / 2,
                boxWidth + boxPaddingValue, boxHeight + boxPaddingValue);

        if(paint == 0)
        {
            // Draw a sunken button.
            QPainter p2(drawPixmap);
            drawBox(p2, r, palette().dark().color(), palette().light().color());

            // Draw the color over the button.
            drawColor(p2, index);
        }
        else
        {
            // Draw a sunken button.
            drawBox(*paint, r, palette().dark().color(), palette().light().color());

            // Draw the color over the button.
            drawColor(*paint, index);
        }

        // return the region that we drew on.
        retval = QRegion(x - boxPaddingValue / 2, y - boxPaddingValue / 2,
            boxWidth + boxPaddingValue, boxHeight + boxPaddingValue);
    }

    return retval;
}
