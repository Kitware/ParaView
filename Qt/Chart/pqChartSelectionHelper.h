/*=========================================================================

   Program: ParaView
   Module:    pqChartSelectionHelper.h

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

/// \file pqChartSelectionHelper.h
/// \date 11/8/2006

#ifndef _pqChartSelectionHelper_h
#define _pqChartSelectionHelper_h


#include "QtChartExport.h"
#include <QObject>

class pqChartContentsSpace;
class pqChartMouseBox;
class QCursor;
class QKeyEvent;
class QMouseEvent;
class QRect;


/// \class pqChartSelectionHelper
/// \brief
///   The pqChartSelectionHelper class can be extended to add user
///   interaction to a chart layer.
///
/// The chart mouse-box and zoom-pan objects are set by the chart.
/// They are set when the selection helper is added to the chart. A
/// sub-class only needs to implement the event handler methods.
class QTCHART_EXPORT pqChartSelectionHelper : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Constructs a chart selection helper.
  /// \param parent The parent object.
  pqChartSelectionHelper(QObject *parent=0);
  virtual ~pqChartSelectionHelper() {}

  /// \brief
  ///   Sets the chart mouse-box object.
  ///
  /// The mouse-box can be used to create a selection box. The chart
  /// uses the mouse-box when zooming to a rectangle. The same object
  /// is shared between the chart and the chart layers.
  ///
  /// \param mouse The chart mouse-box object to use.
  void setMouseBox(pqChartMouseBox *mouse);

  /// \brief
  ///   Gets the chart mouse-box object.
  /// \return
  ///   A pointer to the chart mouse-box object.
  pqChartMouseBox *getMouseBox() const {return this->Mouse;}

  /// \brief
  ///   Sets the chart contents space object.
  ///
  /// The contents space object holds the contents size and position.
  ///
  /// \param zoomPan The chart contents space object to use.
  void setContentsSpace(pqChartContentsSpace *contents);

  /// \brief
  ///   Gets the chart contents space object.
  /// \return
  ///   A pointer to the chart contents space object.
  pqChartContentsSpace *getContentSpace() const {return this->Contents;}

  /// \brief
  ///   Called when a key is pressed.
  /// \return
  ///   True if the event was handled by the selection helper.
  virtual bool handleKeyPress(QKeyEvent *e)=0;

  /// \brief
  ///   Called when a mouse button is pressed.
  /// \return
  ///   True if the event was handled by the selection helper.
  virtual bool handleMousePress(QMouseEvent *e)=0;

  /// \brief
  ///   Called when the mouse is dragged.
  /// \return
  ///   True if the event was handled by the selection helper.
  virtual bool handleMouseMove(QMouseEvent *e)=0;

  /// \brief
  ///   Called when a mouse button is released.
  /// \return
  ///   True if the event was handled by the selection helper.
  virtual bool handleMouseRelease(QMouseEvent *e)=0;

  /// \brief
  ///   Called when a mouse button is double clicked.
  /// \return
  ///   True if the event was handled by the selection helper.
  virtual bool handleMouseDoubleClick(QMouseEvent *e)=0;

signals:
  /// Emitted when the entire chart area needs to be repainted.
  void repaintNeeded();

  /// Emitted when a portion of the chart area needs to be repainted.
  void repaintNeeded(const QRect &area);

  /// \brief
  ///   Emitted when the mouse cursor needs to be changed.
  /// \param cursor The cursor to use.
  void cursorChangeNeeded(const QCursor &cursor);

private:
  pqChartMouseBox *Mouse;         ///< Stores the mouse-box.
  pqChartContentsSpace *Contents; ///< Stores the contents space.
};

#endif
