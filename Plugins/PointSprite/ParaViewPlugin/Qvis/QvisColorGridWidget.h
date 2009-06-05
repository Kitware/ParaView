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

#ifndef QVIS_COLOR_GRID_WIDGET_H
#define QVIS_COLOR_GRID_WIDGET_H

#include <QWidget>

class QPixmap;
class QPainter;

// ****************************************************************************
// Class: QvisColorGridWidget
//
// Purpose:
//   This widget contains an array of colors from which the user may choose.
//
// Notes:
//
// Programmer: Brad Whitlock
// Creation:   Mon Dec 4 14:30:50 PST 2000
//
// Modifications:
//   Brad Whitlock, Tue Mar 12 19:07:21 PST 2002
//   Added an internal method for drawing the button edges.
//
//   Brad Whitlock, Fri Apr 26 11:35:04 PDT 2002
//   I fixed a drawing error that cropped up on windows.
//
//   Brad Whitlock, Thu Nov 21 10:41:40 PDT 2002
//   I added more signals.
//
//   Brad Whitlock, Wed Feb 26 12:37:09 PDT 2003
//   I made it capable of having empty color slots.
//
// ****************************************************************************

class QvisColorGridWidget : public QWidget
{
    Q_OBJECT
public:
    QvisColorGridWidget(QWidget *parent = 0, const char *name = 0);
    virtual ~QvisColorGridWidget();
    virtual QSize sizeHint () const;
    virtual QSize minimumSize() const;

    void setPaletteColors(const QColor *c, int nColors, int suggestedColumns=6);
    void setPaletteColor(const QColor &c, int index);
    void setSelectedColor(const QColor &c);

    void setActiveColorIndex(int index);
    void setSelectedColorIndex(int index);


    void setFrame(bool val);
    void setBoxSize(int val);
    void setBoxPadding(int val);

    int    rows() const;
    int    columns() const;
    QColor selectedColor() const;
    int    selectedColorIndex() const;
    QColor paletteColor(int index) const;
    bool   containsColor(const QColor &color) const;
    int    boxSize() const;
    int    boxPadding() const;
    int    activeColorIndex() const;

signals:
    void selectedColor(const QColor &c);
    void selectedColor(const QColor &c, int colorIndex);
    void selectedColor(const QColor &c, int row, int column);
    void activateMenu(const QColor &c, int row, int column, const QPoint &);
protected:
    virtual void enterEvent(QEvent *);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void leaveEvent(QEvent *);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);

    void drawColorArray();
    void drawColor(QPainter &paint, int index);
    QRegion drawHighlightedColor(QPainter *paint, int index);
    QRegion drawUnHighlightedColor(QPainter *paint, int index);
    QRegion drawSelectedColor(QPainter *paint, int index);

private:
    void getColorRect(int index, int &x, int &y, int &w, int &h);
    int  getColorIndex(int x, int y) const;
    int  getIndex(int row, int col) const;
    void getRowColumnFromIndex(int index, int &row, int &column) const;
    void drawBox(QPainter &p, const QRect &r,
                 const QColor &light, const QColor &dark, int lw = 2);

    QColor  *paletteColors;
    int     numPaletteColors;
    int     numRows;
    int     numColumns;
    int     currentActiveColor;
    int     currentSelectedColor;
    bool    drawFrame;
    int     boxSizeValue;
    int     boxPaddingValue;
    QPixmap *drawPixmap;
};

#endif
