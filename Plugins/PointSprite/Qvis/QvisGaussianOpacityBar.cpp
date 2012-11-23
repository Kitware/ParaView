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

#include "QvisGaussianOpacityBar.h"

#include <qpainter.h>
#include <qpolygon.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qnamespace.h>
#include <QMouseEvent>

#include <iostream>
#include <cmath>
#include <cstdlib>

// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::QvisGaussianOpacityBar
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************

QvisGaussianOpacityBar::QvisGaussianOpacityBar(QWidget *parentObject, const char *name)
    : QvisAbstractOpacityBar(parentObject, name)
{
    setFrameStyle( QFrame::Panel | QFrame::Sunken );
    setLineWidth( 2 );
    setMinimumHeight(50);
    setMinimumWidth(128);
    ngaussian = 0;
    currentGaussian = 0;
    currentMode     = modeNone;
    maximumNumberOfGaussians = -1; // unlimited
    minimumNumberOfGaussians =  0;

    // set a default:
    addGaussian(0.5f, 0.5f, 0.1f, 0.0f, 0);

    mousedown = false;
    setMouseTracking(true);
}

// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::~QvisGaussianOpacityBar
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************

QvisGaussianOpacityBar::~QvisGaussianOpacityBar()
{
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::drawControlPoints
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::drawControlPoints(QPainter &painter)
{
    int pw = pix->width();
    int ph = pix->height();
    QPen bluepen(QColor(100,100,255), 2);
    QPen greenpen(QColor(100,255,0),  2);;
    QPen cyanpen(QColor(100,255,255), 2);;
    QPen graypen(QColor(100,100,100), 2);
    QPolygon pts;
    for (int p=0; p<ngaussian; p++)
    {
        int _x  = int(float(gaussian[p].x+gaussian[p].bx)*float(pw));
        int xr = int(float(gaussian[p].x+gaussian[p].w)*float(pw));
        int xl = int(float(gaussian[p].x-gaussian[p].w)*float(pw));
        int _y  = int(float(1-gaussian[p].h)*float(ph));
        int y0 = int(float(1-0)*float(ph));
        int yb = int(float(1-gaussian[p].h/4. - gaussian[p].by*gaussian[p].h/4.)*float(ph));

        // lines:
        painter.setPen(graypen);
        painter.drawLine(_x,y0-2, _x,_y);
        painter.drawLine(xl,y0-2, xr,y0-2);

        // square: position
        if (currentGaussian == p && currentMode == modeX)
        {
            if (mousedown)
                painter.setPen(greenpen);
            else
                painter.setPen(cyanpen);
        }
        else
            painter.setPen(bluepen);
        pts.setPoints(4, _x-4,y0, _x-4,y0-4, _x+4,y0-4, _x+4,y0);
        painter.drawPolyline(pts);

        // diamond: bias (horizontal and vertical)
        if (currentGaussian == p && currentMode == modeB)
        {
            if (mousedown)
                painter.setPen(greenpen);
            else
                painter.setPen(cyanpen);
        }
        else
            painter.setPen(bluepen);
        float bx = gaussian[p].bx;
        float by = gaussian[p].by;
        painter.drawLine(_x,yb, _x,yb+5);
        if (bx > 0)
        {
            painter.drawLine(_x,yb-5, _x+5,yb);
            painter.drawLine(_x,yb+5, _x+5,yb);
        }
        else
        {
            painter.drawLine(_x,yb, _x+5,yb);
        }
        if (bx < 0)
        {
            painter.drawLine(_x,yb-5, _x-5,yb);
            painter.drawLine(_x,yb+5, _x-5,yb);
        }
        else
        {
            painter.drawLine(_x-5,yb, _x,yb);
        }
        if (by > 0)
        {
            painter.drawLine(_x,yb-5, _x-5,yb);
            painter.drawLine(_x,yb-5, _x+5,yb);
        }
        else
        {
            painter.drawLine(_x,yb-5, _x,yb);
        }

        // up triangle: height
        if (currentGaussian == p && currentMode == modeH)
        {
            if (mousedown)
                painter.setPen(greenpen);
            else
                painter.setPen(cyanpen);
        }
        else
            painter.setPen(bluepen);
        pts.setPoints(4, _x+5,_y, _x,_y-5, _x-5,_y, _x+5,_y);
        painter.drawPolyline(pts);

        // triangle: width (R)
        if (currentGaussian == p && (currentMode == modeWR || currentMode == modeW))
        {
            if (mousedown)
                painter.setPen(greenpen);
            else
                painter.setPen(cyanpen);
        }
        else
            painter.setPen(bluepen);
        pts.setPoints(3, xr,y0, xr,y0-6, xr+6,y0);
        painter.drawPolyline(pts);

        // triangle: width (L)
        if (currentGaussian == p && (currentMode == modeWL  || currentMode == modeW))
        {
            if (mousedown)
                painter.setPen(greenpen);
            else
                painter.setPen(cyanpen);
        }
        else
            painter.setPen(bluepen);
        pts.setPoints(3, xl,y0, xl,y0-6, xl-6,y0);
        painter.drawPolyline(pts);
    }
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::paintToPixmap
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::paintToPixmap(int w,int h)
{
  float *values = new float[w];
  getRawOpacities(w,values);

  QColor white(255, 255, 255 );
  QColor black(0,   0,   0 );
  QPen   whitepen(Qt::white, 2);

  QPainter painter(pix);
  this->paintBackground(painter,w,h);

  float dy = 1.0/float(h-1);
  for (int _x=0; _x<w; _x++)
  {
    float yval1 = values[_x];
    float yval2 = values[_x+1];
    painter.setPen(whitepen);
    for (int _y=0; _y<h; _y++)
    {
      float yvalc = 1 - float(_y)/float(h-1);
      if (yvalc >= qMin(yval1,yval2)-dy && yvalc < qMax(yval1,yval2))
      {
        painter.drawPoint(_x,_y);
      }
    }
  }
  delete[] values;

  this->drawControlPoints(painter);
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::mousePressEvent
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::mousePressEvent(QMouseEvent *e)
{
    int _x = e->x();
    int _y = e->y();

    if (e->button() == Qt::RightButton)
    {
        if (findGaussianControlPoint(_x,_y, &currentGaussian, &currentMode))
        {
            if (getNumberOfGaussians()>minimumNumberOfGaussians)
            {
                removeGaussian(currentGaussian);
            }
        }
    }
    else if (e->button() == Qt::LeftButton)
    {
        if (! findGaussianControlPoint(_x,_y,
                                       &currentGaussian, &currentMode))
        {
            currentGaussian = ngaussian;
            currentMode     = modeW;
            if (maximumNumberOfGaussians==-1 || getNumberOfGaussians()<maximumNumberOfGaussians)
            {
                addGaussian(x2val(_x), y2val(_y), 0.001f, 0,0);
            }
        }
        lastx = _x;
        lasty = _y;
        mousedown = true;
    }


//    this->paintToPixmap(contentsRect().width(), contentsRect().height());
    this->repaint();
//    QPainter p(this);
//    p.drawPixmap(contentsRect().left(),contentsRect().top(),*pix);
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::mouseMoveEvent
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::mouseMoveEvent(QMouseEvent *e)
{
    int _x = e->x();
    int _y = e->y();

    if (!mousedown)
    {
        int  oldGaussian = currentGaussian;
        Mode oldMode     = currentMode;
        findGaussianControlPoint(_x,_y, &currentGaussian, &currentMode);
        if (oldGaussian != currentGaussian ||
            oldMode     != currentMode)
        {
//            drawControlPoints();
    this->update();
//            QPainter p(this);
//            p.drawPixmap(contentsRect().left(),contentsRect().top(),*pix);
        }
        return;
    }

    switch (currentMode)
    {
      case modeX:
        gaussian[currentGaussian].x = x2val(_x) - gaussian[currentGaussian].bx;
        break;
      case modeH:
        gaussian[currentGaussian].h = y2val(_y);
        break;
      case modeW:
        gaussian[currentGaussian].w = qMax((float)fabs(x2val(_x) - gaussian[currentGaussian].x),(float)0.01);
        break;
      case modeWR:
        gaussian[currentGaussian].w = qMax((float)(x2val(_x) - gaussian[currentGaussian].x),(float)0.01);
        if (gaussian[currentGaussian].w < fabs(gaussian[currentGaussian].bx))
            gaussian[currentGaussian].w = fabs(gaussian[currentGaussian].bx);
        break;
      case modeWL:
        gaussian[currentGaussian].w = qMax((float)(gaussian[currentGaussian].x - x2val(_x)),(float)0.01);
        if (gaussian[currentGaussian].w < fabs(gaussian[currentGaussian].bx))
            gaussian[currentGaussian].w = fabs(gaussian[currentGaussian].bx);
        break;
      case modeB:
        gaussian[currentGaussian].bx = x2val(_x) - gaussian[currentGaussian].x;
        if (gaussian[currentGaussian].bx > gaussian[currentGaussian].w)
            gaussian[currentGaussian].bx = gaussian[currentGaussian].w;
        if (gaussian[currentGaussian].bx < -gaussian[currentGaussian].w)
            gaussian[currentGaussian].bx = -gaussian[currentGaussian].w;
        if (fabs(gaussian[currentGaussian].bx) < .001)
            gaussian[currentGaussian].bx = 0;

        gaussian[currentGaussian].by = 4*(y2val(_y) - gaussian[currentGaussian].h/4.)/gaussian[currentGaussian].h;
        if (gaussian[currentGaussian].by > 2)
            gaussian[currentGaussian].by = 2;
        if (gaussian[currentGaussian].by < 0)
            gaussian[currentGaussian].by = 0;
        break;
      default:
        break;
    }
    lastx = _x;
    lasty = _y;

//    this->paintToPixmap(contentsRect().width(), contentsRect().height());
    this->repaint();
//    QPainter p(this);
//    p.drawPixmap(contentsRect().left(),contentsRect().top(),*pix);

//    emit mouseMoved();
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::mouseReleaseEvent
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::mouseReleaseEvent(QMouseEvent *)
{
    mousedown = false;

//    this->paintToPixmap(contentsRect().width(), contentsRect().height());
    this->repaint();
//    QPainter p(this);
//    p.drawPixmap(contentsRect().left(),contentsRect().top(),*pix);

    emit mouseReleased();
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::getRawOpacities
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::getRawOpacities(int n, float *opacity)
{
    for (int i=0; i<n; i++) opacity[i] = 0;

    for (int p=0; p<ngaussian; p++)
    {
        float _pos    = gaussian[p].x;
        float _width  = gaussian[p].w;
        float _height = gaussian[p].h;
        float xbias  = gaussian[p].bx;
        float ybias  = gaussian[p].by;
        for (int i=0; i<n/*+1*/; i++)
        {
            float _x = float(i)/float(n-1);

            // clamp non-zero values to _pos +/- _width
            if (_x > _pos+_width || _x < _pos-_width)
            {
                opacity[i] = qMax(opacity[i],(float)0);
                continue;
            }

            // non-zero _width
            if (_width == 0)
                _width = .00001f;

            // translate the original x to a new x based on the xbias
            float x0;
            if (xbias==0 || _x == _pos+xbias)
            {
                x0 = _x;
            }
            else if (_x > _pos+xbias)
            {
                if (_width == xbias)
                    x0 = _pos;
                else
                    x0 = _pos+(_x-_pos-xbias)*(_width/(_width-xbias));
            }
            else // (_x < _pos+xbias)
            {
                if (-_width == xbias)
                    x0 = _pos;
                else
                    x0 = _pos-(_x-_pos-xbias)*(_width/(_width+xbias));
            }

            // center around 0 and normalize to -1,1
            float x1 = (x0-_pos)/_width;

            // do a linear interpolation between:
            //    a gaussian and a parabola        if 0<ybias<1
            //    a parabola and a step function   if 1<ybias<2
            float h0a = exp(-(4*x1*x1));
            float h0b = 1. - x1*x1;
            float h0c = 1.;
            float h1;
            if (ybias < 1)
                h1 = ybias*h0b + (1-ybias)*h0a;
            else
                h1 = (2-ybias)*h0b + (ybias-1)*h0c;
            float h2 = _height * h1;

            // perform the MAX over different guassians, not the sum
            opacity[i] = qMax(opacity[i], h2);
        }
    }
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::addGaussian
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::addGaussian(float _x,float h,float w,float bx,float by)
{
    gaussian[ngaussian++] = Gaussian(_x,h,w,bx,by);
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::removeGaussian
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::removeGaussian(int n)
{
    for (int i=n; i<ngaussian-1; i++)
        gaussian[i] = gaussian[i+1];
    ngaussian--;
}

// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::setMaximumNumberOfGaussians
//
//  Purpose: Limit the number of Gaussians the user can add/remove
//
//
//  Programmer:  John Biddiscombe
//  Creation:    January 31, 2005
//
// ****************************************************************************
void
QvisGaussianOpacityBar::setMaximumNumberOfGaussians(int n)
{
  this->maximumNumberOfGaussians = n;
}

// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::setMaximumNumberOfGaussians
//
//  Purpose: Limit the number of Gaussians the user can add/remove
//
//
//  Programmer:  John Biddiscombe
//  Creation:    January 31, 2005
//
// ****************************************************************************
void
QvisGaussianOpacityBar::setMinimumNumberOfGaussians(int n)
{
  this->minimumNumberOfGaussians = n;
}

#define dist2(x1,y1,x2,y2) (((x2)-(x1))*((x2)-(x1)) + ((y2)-(y1))*((y2)-(y1)))
// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::findGaussianControlPoint
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
bool
QvisGaussianOpacityBar::findGaussianControlPoint(int _x,int _y,
                                             int *newgaussian, Mode *newmode)
{
    *newgaussian = -1;
    *newmode     = modeNone;
    bool  found   = false;
    float mindist = 100000;  // it's okay, it's pixels
    for (int p=0; p<ngaussian; p++)
    {
        int xc = val2x(gaussian[p].x+gaussian[p].bx);
        int xr = val2x(gaussian[p].x+gaussian[p].w);
        int xl = val2x(gaussian[p].x-gaussian[p].w);
        int yc = val2y(gaussian[p].h);
        int y0 = val2y(0);
        int yb = val2y(gaussian[p].h/4. + gaussian[p].by*gaussian[p].h/4.);

        float d1 = dist2(_x,_y, xc,y0);
        float d2 = dist2(_x,_y, xc,yc);
        float d3 = dist2(_x,_y, xr,y0);
        float d4 = dist2(_x,_y, xl,y0);
        float d5 = dist2(_x,_y, xc,yb);

        float rad = 8*8;

        if (d1 < rad && mindist > d1)
        {
            *newgaussian = p;
            *newmode     = modeX;
            mindist          = d1;
            found = true;
        }
        if (d2 < rad && mindist > d2)
        {
            *newgaussian = p;
            *newmode     = modeH;
            mindist          = d2;
            found = true;
        }
        if (d3 < rad && mindist > d3)
        {
            *newgaussian = p;
            *newmode     = modeWR;
            mindist          = d3;
            found = true;
        }
        if (d4 < rad && mindist > d4)
        {
            *newgaussian = p;
            *newmode     = modeWL;
            mindist          = d4;
            found = true;
        }
        if (d5 < rad && mindist > d5)
        {
            *newgaussian = p;
            *newmode     = modeB;
            mindist          = d5;
            found = true;
        }
    }
    return found;
}



// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::getNumberOfGaussians
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
int
QvisGaussianOpacityBar::getNumberOfGaussians()
{
    return ngaussian;
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::getGaussian
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::getGaussian(int i,
                                float *_x,
                                float *h,
                                float *w,
                                float *bx,
                                float *by)
{
    *_x  = gaussian[i].x;
    *h  = gaussian[i].h;
    *w  = gaussian[i].w;
    *bx = gaussian[i].bx;
    *by = gaussian[i].by;
}

// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::setGaussian
//
//  Purpose: Set the parameters of a particular Gaussian
//
//
//  Programmer:  John Biddiscombe
//  Creation:    January 31, 2005
//
// ****************************************************************************
void
QvisGaussianOpacityBar::setGaussian(int i,
                                float *_x,
                                float *h,
                                float *w,
                                float *bx,
                                float *by)
{
    gaussian[i].x = *_x;
    gaussian[i].h = *h;
    gaussian[i].w = *w;
    gaussian[i].bx = *bx;
    gaussian[i].by = *by;
}


// ****************************************************************************
//  Method:  QvisGaussianOpacityBar::setAllGaussians
//
//  Purpose:
//
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 31, 2001
//
// ****************************************************************************
void
QvisGaussianOpacityBar::setAllGaussians(int n, float *gaussdata)
{
    ngaussian = 0;
    for (int i=0; i<n; i++)
    {
        addGaussian(gaussdata[i*5 + 0],
                    gaussdata[i*5 + 1],
                    gaussdata[i*5 + 2],
                    gaussdata[i*5 + 3],
                    gaussdata[i*5 + 4]);
    }
    this->update();
}
