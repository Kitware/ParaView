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

#include "QvisScribbleOpacityBar.h"
#include "ColorControlPointList.h"

#include <qpainter.h>
#include <QPolygon>
#include <qpixmap.h>
#include <qimage.h>
#include <QMouseEvent>

#include <iostream>
#include <cmath>
#include <cstdlib>

// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::QvisScribbleOpacityBar
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
//  Modifications:
//    Brad Whitlock, Thu Feb 14 12:57:49 PDT 2002
//    Added deletion of values array and the internal pixmap.
//
// ****************************************************************************

QvisScribbleOpacityBar::QvisScribbleOpacityBar(QWidget *parentObject, const char *name)
    : QvisAbstractOpacityBar(parentObject, name)
{
    setFrameStyle( QFrame::Panel | QFrame::Sunken );
    setLineWidth( 2 );
    setMinimumHeight(50);
    setMinimumWidth(128);

    nvalues = 256; //contentsRect().width();
    values = new float[nvalues];
    for (int i=0; i<nvalues; ++i)
    {
        values[i] = float(i)/float(nvalues-1);
    }
    mousedown = false;
}

// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::~QvisScribbleOpacityBar
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
//  Modifications:
//    Brad Whitlock, Thu Feb 14 12:57:49 PDT 2002
//    Added deletion of values array.
//
// ****************************************************************************

QvisScribbleOpacityBar::~QvisScribbleOpacityBar()
{
    delete [] values;
}


// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::paintToPixmap
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisScribbleOpacityBar::paintToPixmap(int w, int h)
{
    if (w != nvalues)
    {
        int nvalues2 = w;
        float *values2 = new float[nvalues2];
        if (nvalues2 > nvalues)
        {
            for (int i=0; i<nvalues2; i++)
                values2[i] = values[(i * nvalues) / nvalues2];
        }
        else
        {
            for (int i=0; i<nvalues; i++)
                values2[(i * nvalues2) / nvalues] = values[i];
        }
        delete[] values;
        values = values2;
        nvalues = nvalues2;
    }


  QColor white(255, 255, 255 );
  QColor black(0,   0,   0 );
  QPen   whitepen(Qt::white, 2);
  QPainter painter(pix);
  this->paintBackground(painter,w,h);

  painter.setPen(whitepen);
  for (int _x=0; _x<w; _x++)
  {
      float yval = values[_x];
      painter.drawLine(_x, h-1, _x, int((h-1) - float(yval)*(h-1)));
  }
}


// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::mousePressEvent
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisScribbleOpacityBar::mousePressEvent(QMouseEvent *e)
{
    int _x = e->x();
    int _y = e->y();
    setValue(x2val(_x), y2val(_y));
    lastx = _x;
    lasty = _y;
    mousedown = true;

    this->repaint();
}


// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::mouseMoveEvent
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisScribbleOpacityBar::mouseMoveEvent(QMouseEvent *e)
{
    if (!mousedown)
        return;

    int _x = e->x();
    int _y = e->y();
    setValues(lastx, lasty, _x, _y);
    lastx = _x;
    lasty = _y;

    this->repaint();
    emit mouseMoved();
}


// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::mouseReleaseEvent
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisScribbleOpacityBar::mouseReleaseEvent(QMouseEvent *e)
{
    int _x = e->x();
    int _y = e->y();
    setValues(lastx, lasty, _x, _y);
    mousedown = false;

    this->repaint();
    emit mouseReleased();
}


// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::setValues
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisScribbleOpacityBar::setValues(int x1, int y1, int x2, int y2)
{
    if (x1==x2)
    {
        setValue(x2val(x2), y2val(y2));
        return;
    }

    int   xdiff = abs(x2 - x1) + 1;
    int   step  = (x1 < x2) ? 1 : -1;
    float slope = float(y2 - y1) / float (x2 - x1);
    for (int i=0; i<xdiff; i++)
        setValue(x2val(x1 + i*step),
                 y2val(y1 + int(float(i)*slope*step)));

}


// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::setValue
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisScribbleOpacityBar::setValue(float xval, float yval)
{
    int _x = int(xval * float(nvalues-1));
    values[_x] = yval;
}


// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::getRawOpacities
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisScribbleOpacityBar::getRawOpacities(int n, float *opacity)
{
    int nvalues2 = n;
    if (nvalues2 > nvalues)
    {
        for (int i=0; i<nvalues2; i++)
            opacity[i] = values[(i * nvalues) / nvalues2];
    }
    else
    {
        for (int i=0; i<nvalues; i++)
            opacity[(i * nvalues2) / nvalues] = values[i];
    }
}


// ****************************************************************************
//  Method:  QvisScribbleOpacityBar::setRawOpacities
//
//  Purpose:
//    Sets all of the opacities in the widget.
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
//  Modifications:
//    Brad Whitlock, Fri Apr 6 12:27:22 PDT 2001
//    I added code to emit a valueChanged signal.
//
// ****************************************************************************
void
QvisScribbleOpacityBar::setRawOpacities(int n, float *v)
{
    if (n < nvalues)
    {
        for (int i=0; i<nvalues; i++)
            values[i] = v[(i * n) / nvalues];
    }
    else
    {
        for (int i=0; i<n; i++)
            values[(i * nvalues) / n] = v[i];
    }
    this->update();

    // Emit a signal indicating that the values changed.
    emit opacitiesChanged();
}

// ****************************************************************************
// Method: QvisScribbleOpacityBar::makeTotallyZero
//
// Purpose:
//   This is a Qt slot function that sets all of the alpha values to zero
//   and updates the widget.
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb 5 13:41:25 PST 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisScribbleOpacityBar::makeTotallyZero()
{
    // Set all the alphas to zero.
    for(int i = 0; i < nvalues; ++i)
        values[i] = 0.;

    //this->paintToPixmap(contentsRect().width(), contentsRect().height());
    this->update();

    // Emit a signal indicating that the values changed.
    emit opacitiesChanged();
}

// ****************************************************************************
// Method: QvisScribbleOpacityBar::makeLinearRamp
//
// Purpose:
//   This is a Qt slot function that sets the alpha values to be a linear ramp.
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb 5 13:42:01 PST 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisScribbleOpacityBar::makeLinearRamp()
{
    // Make a ramp.
    for(int i = 0; i < nvalues; ++i)
        values[i] = float(i) * float(1. / nvalues);

    //this->paintToPixmap(contentsRect().width(), contentsRect().height());
    this->update();

    // Emit a signal indicating that the values changed.
    emit opacitiesChanged();
}

void
QvisScribbleOpacityBar::makeInverseLinearRamp()
{
    // Make a ramp.
    for(int i = 0; i < nvalues; ++i)
        values[nvalues-i-1] = float(i) * float(1. / nvalues);

    //this->paintToPixmap(contentsRect().width(), contentsRect().height());
    this->update();

    // Emit a signal indicating that the values changed.
    emit opacitiesChanged();
}

// ****************************************************************************
// Method: QvisScribbleOpacityBar::makeTotallyOne
//
// Purpose:
//   This is a Qt slot function that sets all of the alpha values to
//   maximum strength.
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb 5 13:42:35 PST 2001
//
// Modifications:
//
// ****************************************************************************

void
QvisScribbleOpacityBar::makeTotallyOne()
{
    // Set all the alphas to 255.
    for(int i = 0; i < nvalues; ++i)
        values[i] = 1.;

    //this->paintToPixmap(contentsRect().width(), contentsRect().height());
    this->update();

    // Emit a signal indicating that the values changed.
    emit opacitiesChanged();
}

// ****************************************************************************
// Method: QvisScribbleOpacityBar::smoothCurve
//
// Purpose:
//   This is a Qt slot function that applies a simple filter to the alphas
//   in order to smooth them out.
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb 5 13:43:15 PST 2001
//
// Modifications:
//   Brad Whitlock, Thu Feb 14 13:07:26 PST 2002
//   Fixed an ABR.
//
// ****************************************************************************

void
QvisScribbleOpacityBar::smoothCurve()
{
    // Smooth the curve
    for(int i = 1; i < nvalues - 1; ++i)
    {
        // 1 3 1 filter.
        float smooth = (0.2 * values[i - 1]) +
                       (0.6 * values[i]) +
                       (0.2 * values[i + 1]);
        values[i] = (smooth > 1.) ? 1. : smooth;
    }

    //this->paintToPixmap(contentsRect().width(), contentsRect().height());
    this->update();

    // Emit a signal indicating that the values changed.
    emit opacitiesChanged();
}
