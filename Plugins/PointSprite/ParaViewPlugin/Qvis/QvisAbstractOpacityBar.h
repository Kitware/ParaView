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

#ifndef QvisAbstractOpacityBar_H
#define QvisAbstractOpacityBar_H

#include <qframe.h>

class QPixmap;
class ColorControlPointList;

// ****************************************************************************
//  Class:  QvisAbstractOpacityBar
//
//  Purpose:
//    Abstract base for an opacity map editor
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 30, 2001
//
// ****************************************************************************
class QvisAbstractOpacityBar : public QFrame
{
    Q_OBJECT
  public:
                   QvisAbstractOpacityBar(QWidget *parent=NULL, const char *name=NULL);
    virtual       ~QvisAbstractOpacityBar();
    virtual void   getRawOpacities(int, float *) = 0;
    void           SetBackgroundColorControlPoints(const ColorControlPointList *ccp);
    void           SetBackgroundPixmap(QPixmap *background);
    void           SetShowBackgroundPixmap(bool show);

  protected:
    int            val2x(float);
    float          x2val(int);
    int            val2y(float);
    float          y2val(int);

    virtual void   paintEvent(QPaintEvent*);
    virtual void   resizeEvent(QResizeEvent*);
    virtual void   paintToPixmap(int,int) = 0;
    virtual void   paintBackground(QPainter &painter, int w, int h);

    QPixmap       *pix;
    QPixmap       *backgroundPixmap;
    bool           showBackgroundPixmap;
    const ColorControlPointList *backgroundColorControlPoints;

  signals:
    void           mouseReleased();
    void           mouseMoved();
    void           resized();
};

#endif
