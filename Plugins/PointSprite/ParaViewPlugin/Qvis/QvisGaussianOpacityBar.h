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

#ifndef QVIS_GAUSSIAN_OPACITY_BAR_H
#define QVIS_GAUSSIAN_OPACITY_BAR_H

#include "QvisAbstractOpacityBar.h"

class QPixmap;

// ****************************************************************************
//  Class:  QvisGaussianOpacityBar
//
//  Purpose:
//    Gaussian-max implementation of QvisAbstractOpacityBar
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 30, 2001
//
//  Modifications:
//
// ****************************************************************************

class QvisGaussianOpacityBar : public QvisAbstractOpacityBar
{
    Q_OBJECT
  public:
                  QvisGaussianOpacityBar(QWidget *parent=NULL, const char *name=NULL);
                 ~QvisGaussianOpacityBar();
    void          getRawOpacities(int, float*);
    int           getNumberOfGaussians();
    void          getGaussian(int, float*,float*,float*,float*,float*);
    void          setGaussian(int, float*,float*,float*,float*,float*);
    void          setAllGaussians(int, float*);
    void          setMaximumNumberOfGaussians(int);
    void          setMinimumNumberOfGaussians(int);

  protected:
    void          mouseMoveEvent(QMouseEvent*);
    void          mousePressEvent(QMouseEvent*);
    void          mouseReleaseEvent(QMouseEvent*);
    void          paintToPixmap(int,int);
    void          drawControlPoints(QPainter &painter);

  signals:
    void          mouseReleased();
    void          mouseMoved();

  private:
    enum Mode     {modeNone, modeX, modeH, modeW, modeWR, modeWL, modeB};
    // encapsulation of gaussian parameters
    class Gaussian
    {
      public:
        float x;
        float h;
        float w;
        float bx;
        float by;
      public:
        Gaussian(float x_,float h_,float w_,float bx_,float by_) : x(x_),h(h_),w(w_),bx(bx_),by(by_) {};
        Gaussian() {};
        ~Gaussian() {};
    };

    // the list of gaussians
    int         ngaussian;
    Gaussian    gaussian[200];

    // the current interaction mode and the current gaussian
    Mode        currentMode;
    int         currentGaussian;

    // GUI interaction variables
    bool        mousedown;
    int         lastx;
    int         lasty;

    // User specified constraints
    int         maximumNumberOfGaussians;
    int         minimumNumberOfGaussians;

    // helper functions
    bool findGaussianControlPoint(int,int, int*,Mode*);
    void removeGaussian(int);
    void addGaussian(float,float,float,float,float);
};

#endif
