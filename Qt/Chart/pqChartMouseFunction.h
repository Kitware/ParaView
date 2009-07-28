/*=========================================================================

   Program: ParaView
   Module:    pqChartMouseFunction.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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


/// \class pqChartMouseFunction
/// \brief
///   The pqChartMouseFunction class is the base class for all chart
///   mouse functions.
class QTCHART_EXPORT pqChartMouseFunction : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart mouse function instance.
  /// \param parent The parent object.
  pqChartMouseFunction(QObject *parent=0);
  virtual ~pqChartMouseFunction() {}

  /// \brief
  ///   Sets the mouse box used by the chart.
  ///
  /// The mouse box is not stored in the base class since it is not
  /// needed by most functions. Subclasses should store the mouse box
  /// if they need it.
  ///
  /// \param box The chart's mouse box.
  virtual void setMouseBox(pqChartMouseBox *box);

  /// \brief
  ///   Gets whether or not the function is combinable.
  ///
  /// If a function can be combined with other functions on the same
  /// mouse button mode, this method should return true. Functions
  /// are combined using keyboard modifiers. If a function uses the
  /// keyboard modifiers, it should return false.
  ///
  /// \return
  ///   True if the other functions can be combined with this one.
  virtual bool isCombinable() const {return true;}

  /// \brief
  ///   Gets whether or not the function owns the mouse.
  /// \return
  ///   True if the function owns the mouse.
  bool isMouseOwner() const {return this->OwnsMouse;}

  /// \brief
  ///   Sets whether or not the function owns the mouse.
  /// \param owns True if the function owns the mouse.
  /// \sa
  //    pqChartMouseFunction::interactionStarted(pqChartMouseFunction *)
  virtual void setMouseOwner(bool owns) {this->OwnsMouse = owns;}

  /// \brief
  ///   Called to handle the mouse press event.
  /// \param e Event specific information.
  /// \param contents The chart's contents space object.
  /// \return
  ///   True if the event was used.
  virtual bool mousePressEvent(QMouseEvent *e,
      pqChartContentsSpace *contents)=0;

  /// \brief
  ///   Called to handle the mouse move event.
  /// \param e Event specific information.
  /// \param contents The chart's contents space object.
  /// \return
  ///   True if the event was used.
  virtual bool mouseMoveEvent(QMouseEvent *e,
      pqChartContentsSpace *contents)=0;

  /// \brief
  ///   Called to handle the mouse release event.
  /// \param e Event specific information.
  /// \param contents The chart's contents space object.
  /// \return
  ///   True if the event was used.
  virtual bool mouseReleaseEvent(QMouseEvent *e,
      pqChartContentsSpace *contents)=0;

  /// \brief
  ///   Called to handle the double click event.
  /// \param e Event specific information.
  /// \param contents The chart's contents space object.
  /// \return
  ///   True if the event was used.
  virtual bool mouseDoubleClickEvent(QMouseEvent *e,
      pqChartContentsSpace *contents)=0;

signals:
  /// \brief
  ///   Emitted when a function interaction has started.
  ///
  /// A mouse function should not assume it has ownership after
  /// emitting this signal. The interactor will call \c setMouseOwner
  /// if no other function owns the mouse.
  ///
  /// \param function The function requesting mouse ownership.
  void interactionStarted(pqChartMouseFunction *function);

  /// \brief
  ///   Emitted when a function has finished an interaction state.
  /// \param function The function releasing mouse control.
  void interactionFinished(pqChartMouseFunction *function);

  /// Emitted when the entire chart needs to be repainted.
  void repaintNeeded();

  /// \brief
  ///   Emitted when the chart needs to be repainted.
  /// \param area The area that needs to be repainted. The area should
  ///   be in widget coordinates.
  void repaintNeeded(const QRect &area);

  /// \brief
  ///   Emitted when the mouse cursor needs to be changed.
  /// \param cursor The new cursor to use.
  void cursorChangeRequested(const QCursor &cursor);

private:
  bool OwnsMouse; ///< True if the function owns mouse control.
};

#endif
