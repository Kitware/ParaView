/*=========================================================================

   Program: ParaView
   Module:    pqChartMouseFunction.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/// \file pqChartMouseFunction.h
/// \date 6/21/2007

#ifndef _pqChartMouseFunction_h
#define _pqChartMouseFunction_h


#include "QtChartExport.h"
#include <QObject>

class pqChartContentsSpace;
class pqChartMouseBox;
class QCursor;
class QMouseEvent;
class QRect;


class QTCHART_EXPORT pqChartMouseFunction : public QObject
{
  Q_OBJECT

public:
  pqChartMouseFunction(QObject *parent=0);
  virtual ~pqChartMouseFunction() {}

  bool isMouseOwner() const {return this->OwnsMouse;}
  virtual void setMouseOwner(bool owns) {this->OwnsMouse = owns;}

  virtual void setMouseBox(pqChartMouseBox *box);

  virtual bool isCombinable() const {return true;}

  virtual bool mousePressEvent(QMouseEvent *e,
      pqChartContentsSpace *contents)=0;
  virtual bool mouseMoveEvent(QMouseEvent *e,
      pqChartContentsSpace *contents)=0;
  virtual bool mouseReleaseEvent(QMouseEvent *e,
      pqChartContentsSpace *contents)=0;
  virtual bool mouseDoubleClickEvent(QMouseEvent *e,
      pqChartContentsSpace *contents)=0;

signals:
  void interactionStarted(pqChartMouseFunction *function);
  void interactionFinished(pqChartMouseFunction *function);

  void repaintNeeded();
  void repaintNeeded(const QRect &area);

  /// \brief
  ///   Emitted when the mouse cursor needs to be changed.
  /// \param cursor The cursor to use.
  void cursorChangeRequested(const QCursor &cursor);

private:
  bool OwnsMouse;
};

#endif
