/*=========================================================================

   Program: ParaView
   Module:    pqHistogramSelectionHelper.h

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

/// \file pqHistogramSelectionHelper.h
/// \date 11/8/2006

#ifndef _pqHistogramSelectionHelper_h
#define _pqHistogramSelectionHelper_h


#include "QtChartExport.h"
#include "pqChartSelectionHelper.h"

#include "pqHistogramChart.h" // Needed for enum.

class pqHistogramSelectionHelperInternal;
class QKeyEvent;
class QMouseEvent;
class QTimer;


/// \class pqHistogramSelectionHelper
/// \brief
///   The pqHistogramSelectionHelper class enables the user to select
///   data on a histogram chart.
class QTCHART_EXPORT pqHistogramSelectionHelper : public pqChartSelectionHelper
{
  Q_OBJECT

public:
  enum InteractMode
    {
    Bin,      ///< Select bins
    Value,    ///< Select values
    ValueMove ///< Move value selection ranges
    };

public:
  /// \brief
  ///   Creates a histogram selection helper.
  /// \param histogram The histogram to interact with.
  /// \param parent The parent object.
  pqHistogramSelectionHelper(pqHistogramChart *histogram, QObject *parent=0);
  virtual ~pqHistogramSelectionHelper();

  /// \name pqChartSelectionHelper Methods
  //@{
  virtual bool handleKeyPress(QKeyEvent *e);
  virtual bool handleMousePress(QMouseEvent *e);
  virtual bool handleMouseMove(QMouseEvent *e);
  virtual bool handleMouseRelease(QMouseEvent *e);
  virtual bool handleMouseDoubleClick(QMouseEvent *e);
  //@}

  /// \brief
  ///   Gets the current interaction mode.
  /// \return
  ///   The current interaction mode.
  /// \sa pqHistogramSelectionHelper::setInteractMode(InteractMode)
  InteractMode getInteractMode() const {return this->Interact;}

  /// \brief
  ///   Sets the interaction mode.
  /// \param mode The new interaction mode.
  void setInteractMode(InteractMode mode);

private slots:
  /// Called when the mouse move timer expires.
  void moveTimeout();

private:
  enum MouseMode {
    NoMode,    ///< No mouse interaction mode.
    MoveWait,  ///< Waiting for a mouse drag.
    SelectBox, ///< Selecting bins with a mouse box.
    ValueDrag  ///< Dragging the mouse to select values.
  };

  /// Keeps track of previous selection state for modified selection.
  pqHistogramSelectionHelperInternal *Internal;

  /// Stores the bin pick mode for histogram selection.
  pqHistogramChart::BinPickMode PickStyle;

  InteractMode Interact;       ///< Stores the current interaction mode.
  InteractMode SelectMode;     ///< Stores the current selection type.
  MouseMode Mode;              ///< Stores the current mouse state.
  pqHistogramChart *Histogram; ///< A pointer to the histogram chart.
  QTimer *MoveTimer;           ///< Used for the mouse interaction.
};

#endif
