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

#include "QvisAbstractOpacityBar.h"
#include "ColorControlPointList.h"

#include <qpainter.h>
#include <qpolygon.h>
#include <qpixmap.h>
#include <qimage.h>

#include <iostream>
#include <cmath>
#include <cstdlib>


// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::QvisAbstractOpacityBar
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************

QvisAbstractOpacityBar::QvisAbstractOpacityBar(QWidget *parent,
  const char* /*name*/)
    : QFrame(parent)
{
    setFrameStyle( QFrame::Panel | QFrame::Sunken );
    setLineWidth( 2 );
    setMinimumHeight(50);
    setMinimumWidth(128);
    pix = new QPixmap;
    backgroundColorControlPoints = 0;
    backgroundPixmap = NULL;
}

// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::~QvisAbstractOpacityBar
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
//  Modifications:
//    Brad Whitlock, Thu Feb 14 13:19:19 PST 2002
//    Deleted pix.
//
// ****************************************************************************

QvisAbstractOpacityBar::~QvisAbstractOpacityBar()
{
    delete pix;
    delete backgroundColorControlPoints;
    pix = 0;
    showBackgroundPixmap = false;
}

void QvisAbstractOpacityBar::SetShowBackgroundPixmap(bool show)
{
    showBackgroundPixmap = show;;
}

// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::SetBackgroundColorControlPoints(const ColorControlPointList *ccp)
//
//  Purpose: Set color control points for color transfer function backdrop
//
//
//  Programmer:  Gunther H. Weber
//  Creation:    April 5, 2007
//
//  Modifications:
//
// ****************************************************************************

void QvisAbstractOpacityBar::SetBackgroundColorControlPoints(const ColorControlPointList *ccp)
{
  if (backgroundColorControlPoints) delete backgroundColorControlPoints;
  backgroundColorControlPoints = NULL;
  if (ccp) {
    backgroundColorControlPoints = new ColorControlPointList(*ccp);
  }
  this->update();
}

void QvisAbstractOpacityBar::SetBackgroundPixmap(QPixmap *background)
{
  if (this->backgroundPixmap) delete this->backgroundPixmap;
  this->backgroundPixmap = NULL;
  if (background) {
    this->backgroundPixmap = new QPixmap(*background);
  }
}

void QvisAbstractOpacityBar::paintBackground(QPainter &painter, int w, int h)
{
  if (this->showBackgroundPixmap && this->backgroundPixmap) {
    painter.drawPixmap(0,0, *this->backgroundPixmap);
  }
/*
  else if (backgroundColorControlPoints && backgroundColorControlPoints->GetNumControlPoints()>1)
  {
    unsigned char *cols = new unsigned char[w*3];
    backgroundColorControlPoints->GetColors(cols, w);
    for (int x=0; x<w; ++x) {
      QRgb bgCols = QColor(cols[x*3+0], cols[x*3+1], cols[x*3+2]).rgb();
      painter.setPen(bgCols);
      painter.drawLine(x,0, x, h-1);
    }
    delete[] cols;
  }
*/
  else
  {
    painter.fillRect(0,0, w,h, QBrush(Qt::black));
  }
}

// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::val2x
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
int
QvisAbstractOpacityBar::val2x(float val)
{
    QRect c = contentsRect();
    int w = c.width();
    int l = c.left();
    int x = int(val*float(w) + l);
    x = qMax(l, qMin(l+w, x));
    return x;
}


// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::x2val
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
float
QvisAbstractOpacityBar::x2val(int x)
{
    QRect c = contentsRect();
    int w = c.width();
    int l = c.left();
    float val = float(x-l)/float(w);
    val = qMax((float)0, qMin((float)1, val));
    return val;
}


// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::val2y
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
int
QvisAbstractOpacityBar::val2y(float val)
{
    QRect c = contentsRect();
    int h = c.height();
    int t = c.top();
    int y = int((1-val)*float(h) + t);
    y = qMax(t, qMin(t+h, y));
    return y;
}


// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::y2val
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
float
QvisAbstractOpacityBar::y2val(int y)
{
    QRect c = contentsRect();
    int h = c.height();
    int t = c.top();
    float val = float(y-t)/float(h);
    val = qMax((float)0, qMin((float)1, ((float)1-val)));
    return val;
}


// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::paintEvent
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisAbstractOpacityBar::paintEvent(QPaintEvent *e)
{
    QFrame::paintEvent(e);
    if (!pix) return;
    //
    QPainter painter(this);
    this->paintToPixmap(contentsRect().width(), contentsRect().height());
    painter.drawPixmap(contentsRect().left(),contentsRect().top(),*pix);
    painter.end();
}



// ****************************************************************************
//  Method:  QvisAbstractOpacityBar::resizeEvent
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisAbstractOpacityBar::resizeEvent(QResizeEvent*)
{
    QRect framerect(rect());
    framerect.setTop(framerect.top()       +5);
    framerect.setBottom(framerect.bottom( )-5);
    framerect.setLeft(framerect.left()     +13);
    framerect.setRight(framerect.right()   -13);
    setFrameRect(framerect);

    int w=contentsRect().width();
    int h=contentsRect().height();

    delete pix;
    pix = new QPixmap(w,h);

    emit resized();

}
