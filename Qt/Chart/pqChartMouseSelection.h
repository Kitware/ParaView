/*=========================================================================

   Program: ParaView
   Module:    pqChartMouseSelection.h

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

/// \file pqChartMouseSelection.h
/// \date 6/20/2007

#ifndef _pqChartMouseSelection_h
#define _pqChartMouseSelection_h


#include "QtChartExport.h"
#include "pqChartMouseFunction.h"
#include "pqHistogramChart.h" // Needed for enum

class pqChartContentsSpace;
class pqChartMouseBox;
class pqChartMouseSelectionHistogram;
class pqChartMouseSelectionInternal;
class pqHistogramSelectionModel;
class QMouseEvent;
class QPoint;
class QString;
class QStringList;


/// \class pqChartMouseSelection
/// \brief
///   The pqChartMouseSelection class is used to select chart elements
///   based on the current selection mode.
class QTCHART_EXPORT pqChartMouseSelection : public pqChartMouseFunction
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a mouse selection object.
  /// \param parent The parent object.
  pqChartMouseSelection(QObject *parent=0);
  virtual ~pqChartMouseSelection();

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Gets the histogram used for interaction.
  /// \return
  ///   A pointer to the histogram used for interaction.
  pqHistogramChart *getHistogram() const;

  /// \brief
  ///   Sets the histogram to interact with.
  /// \param histogram The histogram to interact with.
  void setHistogram(pqHistogramChart *histogram);

  /// \brief
  ///   Gets the chart mouse box object.
  /// \return
  ///   A pointer to the chart mouse box object.
  pqChartMouseBox *getMouseBox() const {return this->MouseBox;}

  /// \brief
  ///   Sets the chart mouse box object.
  ///
  /// The mouse box can be used to create a selection box.
  ///
  /// \param box The chart mouse box object to use.
  virtual void setMouseBox(pqChartMouseBox *box) {this->MouseBox = box;}

  virtual bool isCombinable() const {return false;}
  //@}

  /// \name Configuration Methods
  //@{
  const QString &getSelectionMode() const;
  void getAllModes(QStringList &list) const;
  void getAvailableModes(QStringList &list) const;

  /// \brief
  ///   Gets the histogram bin pick style.
  /// \return
  ///   The histogram bin pick style.
  pqHistogramChart::BinPickMode getPickStyle() const;

  /// \brief
  ///   Sets the histogram bin pick style.
  /// \param style The new histogram bin pick style.
  void setPickStyle(pqHistogramChart::BinPickMode style);
  //@}

  /// \name Interaction Methods
  //@{
  virtual bool mousePressEvent(QMouseEvent *e, pqChartContentsSpace *contents);
  virtual bool mouseMoveEvent(QMouseEvent *e, pqChartContentsSpace *contents);
  virtual bool mouseReleaseEvent(QMouseEvent *e,
      pqChartContentsSpace *contents);
  virtual bool mouseDoubleClickEvent(QMouseEvent *e,
      pqChartContentsSpace *contents);
  //@}

public slots:
  void setSelectionMode(const QString &mode);

signals:
  /// Emitted when the list of available modes changes.
  void modeAvailabilityChanged();

  /// \brief
  ///   Emitted when the selection mode changes.
  /// \param mode The new selection mode.
  void selectionModeChanged(const QString &mode);

private:
  void mousePressHistogramBin(pqHistogramSelectionModel *model,
      const QPoint &point, Qt::KeyboardModifiers modifiers);
  void mousePressHistogramValue(pqHistogramSelectionModel *model,
      const QPoint &point, Qt::KeyboardModifiers modifiers);
  void mousePressHistogramMove(const QPoint &point);

  void mouseMoveSelectBox(pqChartContentsSpace *contents, const QPoint &point,
      Qt::KeyboardModifiers modifiers);
  void mouseMoveSelectDrag(pqChartContentsSpace *contents, const QPoint &point,
      Qt::KeyboardModifiers modifiers);
  void mouseMoveDragMove(const QPoint &point);

private:
  /// Stores the mode data and delay timer.
  pqChartMouseSelectionInternal *Internal;

  /// Stores the histogram mode data.
  pqChartMouseSelectionHistogram *Histogram;

  pqChartMouseBox *MouseBox; ///< Stores the mouse box.
  int Mode;                  ///< Stores the current mode.
  int State;                 ///< Stores the current state.
};

#endif
