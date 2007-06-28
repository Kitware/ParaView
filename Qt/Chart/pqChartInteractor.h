/*=========================================================================

   Program: ParaView
   Module:    pqChartInteractor.h

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

/// \file pqChartInteractor.h
/// \date 5/2/2007

#ifndef _pqChartInteractor_h
#define _pqChartInteractor_h


#include "QtChartExport.h"
#include <QObject>

class pqChartContentsSpace;
class pqChartInteractorInternal;
class pqChartMouseBox;
class pqChartMouseFunction;
class QCursor;
class QKeyEvent;
class QMouseEvent;
class QRect;
class QWheelEvent;


/*!
 *  \class pqChartInteractor
 *  \brief
 *    The pqChartInteractor class is used to interact with chart.
 * 
 *  The chart can be zoomed in and out. The axes can be zoomed
 *  independently as well. The zoom factors and viewport location can
 *  be set programatically. The user can use the mouse and keyboard
 *  to zoom and pan. The keyboard shortcuts are as follows:
 *  \code
 *  Plus...................Zoom in.
 *  Minus..................Zoom out.
 *  Ctrl+Plus..............Horizontally zoom in.
 *  Ctrl+minus.............Horizontally zoom out.
 *  Alt+Plus...............Vertically zoom in.
 *  Alt+minus..............Vertically zoom out.
 *  Up.....................Pan up.
 *  Down...................Pan down.
 *  Left...................Pan left.
 *  Right..................Pan right.
 *  Alt+Left...............Go to previous view in the history.
 *  Alt+Right..............Go to next view in the history.
 *  \endcode
 * 
 *  Based on the interaction mode, the histogram can highlight values
 *  or bins. When switching between selection modes, the current
 *  selection will be erased. This prevents errors when combining bin
 *  and value selection. When calling any of the selection methods,
 *  the selection mode will be maintained. If you try to set a value
 *  selection during bin mode, it will be ignored.
 */
class QTCHART_EXPORT pqChartInteractor : public QObject
{
  Q_OBJECT

public:
  pqChartInteractor(QObject *parent=0);
  virtual ~pqChartInteractor();

  /// \name Setup Methods
  //@{
  pqChartContentsSpace *getContentsSpace() const {return this->Contents;}
  void setContentsSpace(pqChartContentsSpace *space);

  pqChartMouseBox *getMouseBox() const {return this->MouseBox;}
  void setMouseBox(pqChartMouseBox *box);
  //@}

  /// \name Configuration Methods
  //@{
  void setFunction(pqChartMouseFunction *function, Qt::MouseButton button,
      Qt::KeyboardModifiers modifiers=Qt::NoModifier);
  void addFunction(pqChartMouseFunction *function, Qt::MouseButton button,
      Qt::KeyboardModifiers modifiers=Qt::NoModifier);
  void removeFunction(pqChartMouseFunction *function);
  void removeFunctions(Qt::MouseButton button);
  void removeAllFunctions();

  int getNumberOfModes(Qt::MouseButton button) const;
  int getMode(Qt::MouseButton button) const;
  void setMode(Qt::MouseButton button, int index);
  //@}

  /// \name Interaction Methods
  //@{
  virtual bool keyPressEvent(QKeyEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mouseDoubleClickEvent(QMouseEvent *e);
  virtual void wheelEvent(QWheelEvent *e);
  //@}

signals:
  void repaintNeeded();
  void repaintNeeded(const QRect &area);
  void cursorChangeRequested(const QCursor &cursor);

private slots:
  void beginState(pqChartMouseFunction *owner);
  void endState(pqChartMouseFunction *owner);

private:
  /// Stores the mouse function configuration.
  pqChartInteractorInternal *Internal;
  pqChartContentsSpace *Contents; ///< Stores the contents space.
  pqChartMouseBox *MouseBox;      ///< Stores the mouse box.
  Qt::KeyboardModifier XModifier; ///< Stores the zoom x-only modifier.
  Qt::KeyboardModifier YModifier; ///< Stores the zoom y-only modifier.
};

#endif
