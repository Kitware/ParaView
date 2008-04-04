/*=========================================================================

   Program: ParaView
   Module:    pqChartInteractor.h

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
 *    The pqChartInteractor class is used to interact with a chart.
 *
 *  The contents space and mouse box object used by the chart are
 *  shared among the mouse functions. The contents space object is
 *  used to convert widget coordinates to contents coordinates. It is
 *  also used to pan and zoom the contents. The chart uses the mouse
 *  box to draw a dashed rectangle on top of the chart. Mouse
 *  functions can use this rectangle for selection or zooming.
 * 
 *  The keyboard shortcuts are as follows:
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
 */
class QTCHART_EXPORT pqChartInteractor : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart interactor instance.
  /// \param parent The parent object.
  pqChartInteractor(QObject *parent=0);
  virtual ~pqChartInteractor();

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Gets the chart's contents space object.
  /// \return
  ///   A pointer to the chart's contents space object.
  pqChartContentsSpace *getContentsSpace() const {return this->Contents;}

  /// \brief
  ///   Sets the contents space object used by the chart.
  /// \param space The chart's contents space object.
  void setContentsSpace(pqChartContentsSpace *space);

  /// \brief
  ///   Gets the chart's mouse box object.
  /// \return
  ///   A pointer to the chart's mouse box object.
  pqChartMouseBox *getMouseBox() const {return this->MouseBox;}

  /// \brief
  ///   Sets the mouse box object used by the chart.
  /// \param box The chart's mouse box object.
  void setMouseBox(pqChartMouseBox *box);
  //@}

  /// \name Configuration Methods
  //@{
  /// \brief
  ///   Sets the given function on the indicated mouse button.
  ///
  /// This method clears any functions currently assigned to the
  /// given button before adding the new function.
  ///
  /// \param function The mouse function to add.
  /// \param button The mouse button to assign the function to.
  /// \param modifiers The keyboard modifiers used to activate the
  ///   function.
  void setFunction(pqChartMouseFunction *function, Qt::MouseButton button,
      Qt::KeyboardModifiers modifiers=Qt::NoModifier);

  /// \brief
  ///   Adds the given function to the indicated mouse button.
  ///
  /// If the new function is not combinable, it will be added to its
  /// own interaction mode. If the function is combinable, it is
  /// added to the first mode that does not have the given modifiers.
  ///
  /// \param function The mouse function to add.
  /// \param button The mouse button to assign the function to.
  /// \param modifiers The keyboard modifiers used to activate the
  ///   function.
  void addFunction(pqChartMouseFunction *function, Qt::MouseButton button,
      Qt::KeyboardModifiers modifiers=Qt::NoModifier);

  /// \brief
  ///   Removes the given function from its assigned button.
  /// \param function The mouse function to remove.
  void removeFunction(pqChartMouseFunction *function);

  /// \brief
  ///   Removes all the functions assigned to the given button.
  /// \param button The mouse button to clear.
  void removeFunctions(Qt::MouseButton button);

  /// Removes all the functions from all the buttons.
  void removeAllFunctions();

  /// \brief
  ///   Gets the number of modes on a mouse button.
  /// \param button The mouse button.
  /// \return
  ///   The number of modes on a mouse button.
  int getNumberOfModes(Qt::MouseButton button) const;

  /// \brief
  ///   Gets the current mode for the given button.
  /// \param button The mouse button.
  /// \return
  ///   The current mode for the given button.
  int getMode(Qt::MouseButton button) const;

  /// \brief
  ///   Sets the current mode for the given button.
  /// \param button The mouse button.
  /// \param index The new interaction mode.
  void setMode(Qt::MouseButton button, int index);
  //@}

  /// \name Interaction Methods
  //@{
  /// \brief
  ///   Handles the key press events for the chart.
  /// \param e Event specific information.
  virtual bool keyPressEvent(QKeyEvent *e);

  /// \brief
  ///   Calls the appropriate function to handle the mouse press.
  ///
  /// The mouse button and that button's current mode are used to
  /// determine the function to call. If a function on another button
  /// owns the mouse, the event will be ignored.
  ///
  /// \param e Event specific information.
  virtual void mousePressEvent(QMouseEvent *e);

  /// \brief
  ///   Calls the appropriate function to handle the mouse move.
  /// \param e Event specific information.
  virtual void mouseMoveEvent(QMouseEvent *e);

  /// \brief
  ///   Calls the appropriate function to handle the mouse release.
  /// \param e Event specific information.
  virtual void mouseReleaseEvent(QMouseEvent *e);

  /// \brief
  ///   Calls the appropriate function to handle the double click.
  /// \param e Event specific information.
  virtual void mouseDoubleClickEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse wheel events for the chart.
  /// \param e Event specific information.
  virtual void wheelEvent(QWheelEvent *e);
  //@}

signals:
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

private slots:
  /// \brief
  ///   Called to begin a new mouse state.
  ///
  /// Only one mouse function can own the mouse at one time.
  ///
  /// \param owner The mouse function requesting the mouse state.
  void beginState(pqChartMouseFunction *owner);

  /// \brief
  ///   Called to end the current mouse state.
  ///
  /// Only the current owner should end the current state.
  ///
  /// \param owner The mouse function releasing the mouse state.
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
