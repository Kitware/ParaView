/*=========================================================================

   Program: ParaView
   Module:    pqChartZoomPanAlt.h

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
/*!
 * \file pqChartZoomPanAlt.cxx
 *
 * \brief
 *   An alternative to pqChartZoomPan.
 *   Used to handle the zoom and
 *   pan interaction for a chart widget by manipulating properties of axes.
 *
 * \author Eric Stanton
 * \date   June 23, 2006
 */

#ifndef _pqChartZoomPanAlt_h
#define _pqChartZoomPanAlt_h

#include "QtChartExport.h"
#include <QObject>
#include <QPoint> // Needed for QPoint member.
#include "pqChartAxis.h"

class QRect;
class pqLineChartWidget;

class QTCHART_EXPORT pqChartZoomPanAlt : public QObject
{
  Q_OBJECT

public:
  enum InteractMode {
    NoMode,
    Zoom,
    Pan
  };

  enum InteractFlags {
    NoFlags,
    ZoomBoth,
    ZoomXOnly,
    ZoomYOnly
  };

public:
  /// \brief
  ///   Creates a chart zoom/pan instance.
  /// \param parent The parent object.
  pqChartZoomPanAlt(pqLineChartWidget *parent=0);
  virtual ~pqChartZoomPanAlt();

  /// \brief
  ///   Zooms chart to the given rectangle.
  ///
  ///  Not yet supported.
  ///
  /// \param area The zoom area in current content coordinates.
  /// \sa pqChartZoomPanAlt::zoomToPercent(int, int)
  void zoomToRectangle(const QRect *area);

  /// \brief
  ///   Zooms in by a predetermined percentage.
  /// \param flags Used to change the interaction slightly.
  void zoomIn(InteractFlags flags);

  /// \brief
  ///   Zooms out by a predetermined percentage.
  /// \param flags Used to change the interaction slightly.
  void zoomOut(InteractFlags flags);
  //@}

  /// \name Mouse Interactions
  //@{
  /// Sets the parent's cursor to the zoom cursor.
  void setZoomCursor();

  /// \brief
  ///   Signals the start of a mouse move interaction.
  ///
  /// When an interaction mode is started, no other interaction
  /// mode can be used until the current mode is finished. The
  /// parent's cursor will be set to the appropriate cursor.
  ///
  /// \param mode The interaction mode to begin.
  /// \sa pqChartZoomPanAlt::finishInteraction(),
  ///     pqChartZoomPanAlt::interact(const QPoint &, const QPoint &,
  ///         InteractFlags)
  void startInteraction(InteractMode mode);

  /// \brief
  ///   Used to incrementally modify the chart zoom/pan.
  ///
  /// The interaction is based on the current interaction mode. The
  /// \c flags parameter can be used to modify the interaction, such
  /// as zooming only in the horizontal or vertical directions. The
  /// \c pos parameter should be passed in global coordinates to
  /// avoid repaint shake.
  ///
  /// \param pos The current mouse position in global coordinates.
  /// \param flags Used to change the interaction slightly.
  /// \sa pqChartZoomPanAlt::startInteraction(InteractMode),
  ///     pqChartZoomPanAlt::finishInteraction()
  void interact(const QPoint &pos, InteractFlags flags);

  /// \brief
  ///   Signals the end of a mouse move interaction.
  ///
  /// The parent's cursor will be set back to the arrow cursor. The
  /// current interaction type will be cleared so another interaction
  /// type can be started. If the interaction was a zoom operation,
  /// the current position will be added to the zoom history.
  ///
  /// \sa pqChartZoomPanAlt::startInteraction(),
  ///     pqChartZoomPanAlt::interact(const QPoint &, const QPoint &,
  ///         InteractFlags)
  void finishInteraction();

  /// \brief
  ///   Zooms in or out and senters on the position indicated.
  ///
  ///  Not yet supported.
  ///
  /// \param delta The wheel delta parameter from the event.
  /// \param pos The current mouse position in contents coordinates.
  /// \param flags Used to change the interaction slightly.
  bool handleWheelZoom(int delta, const QPoint &pos, InteractFlags flags);
  //@}

public slots:
  /// Pans up by a predetermined amount.
  void panUp();

  /// Pans down by a predetermined amount.
  void panDown();

  /// Pans left by a predetermined amount.
  void panLeft();

  /// Pans right by a predetermined amount.
  void panRight();

public:
  /// Stores the last mouse location in global coordinates.
  QPoint Last;

protected:

  /// \brief
  ///   adjusts the min/max values of the axes to produce the effect of zooming or panning.
  /// \param deltaMinX The amount to move the min x-axis value left (neg value) or right (pos value).
  /// \param deltaMaxX The amount to move the max x-axis value left (neg value) or right (pos value).
  /// \param deltaMinY The amount to move the min y-axis value down (neg value) or up (pos value).
  /// \param deltaMinY The amount to move the min y-axis value down (neg value) or up (pos value).
  void setAxesBounds(double deltaMinX, double deltaMaxX, double deltaMinY, double deltaMaxY);

private:
  InteractMode Current;        ///< Stores the current interact mode.
  pqChartAxis::AxisLayout oldXAxisLayout;  ///< Stores the layout type of the x axis before interaction began
  pqChartAxis::AxisLayout oldYAxisLayout;  ///< Stores the layout type of the y axis before interaction began
  pqLineChartWidget *Parent; ///< Stores a pointer to the parent.
};

#endif
