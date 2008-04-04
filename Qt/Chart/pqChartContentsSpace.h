/*=========================================================================

   Program: ParaView
   Module:    pqChartContentsSpace.h

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

/// \file pqChartContentsSpace.h
/// \date 3/1/2007

#ifndef _pqChartContentsSpace_h
#define _pqChartContentsSpace_h


#include "QtChartExport.h"
#include <QObject>

class pqChartContentsSpaceInternal;
class QPoint;
class QRect;


/// \class pqChartContentsSpace
/// \brief
///   The pqChartContentsSpace class defines the contents space for a
///   chart.
class QTCHART_EXPORT pqChartContentsSpace : public QObject
{
  Q_OBJECT

public:
  enum InteractFlags {
    NoFlags,   ///< No interaction flags.
    ZoomBoth,  ///< Zoom in both directions.
    ZoomXOnly, ///< Zoom only in the x-direction.
    ZoomYOnly  ///< Zoom only in the y-direction.
  };

public:
  /// \brief
  ///   Creates a chart contents space instance.
  /// \param parent The parent object.
  pqChartContentsSpace(QObject *parent=0);
  virtual ~pqChartContentsSpace();

  /// \name Contents Methods
  //@{
  /// \brief
  ///   Gets the x offset.
  /// \return
  ///   The x offset.
  int getXOffset() const {return this->OffsetX;}

  /// \brief
  ///   Gets the y offset.
  /// \return
  ///   The y offset.
  int getYOffset() const {return this->OffsetY;}

  /// \brief
  ///   Gets the maximum x offset.
  /// \return
  ///   The maximum x offset.
  int getMaximumXOffset() const {return this->MaximumX;}

  /// \brief
  ///   Gets the maximum y offset.
  /// \return
  ///   The maximum y offset.
  int getMaximumYOffset() const {return this->MaximumY;}

  /// \brief
  ///   Gets the contents width.
  /// \return
  ///   The contents width.
  int getContentsWidth() const;

  /// \brief
  ///   Gets the contents height.
  /// \return
  ///   The contents height.
  int getContentsHeight() const;

  /// \brief
  ///   Translates the given point from widget to contents coordinates.
  /// \param point The point to translate.
  void translateToContents(QPoint &point) const;

  /// \brief
  ///   Translates the given area from widget to contents coordinates.
  /// \param area The rectangle to translate.
  void translateToContents(QRect &area) const;

  /// \brief
  ///   Translates the given point from contents to widget coordinates.
  /// \param point The point to translate.
  void translateFromContents(QPoint &point) const;

  /// \brief
  ///   Translates the given area from contents to widget coordinates.
  /// \param area The rectangle to translate.
  void translateFromContents(QRect &area) const;
  //@}

  /// \name Size Methods
  //@{
  /// \brief
  ///   Sets the size of the chart.
  ///
  /// The chart size must be set in order to zoom in or out. The
  /// contents size methods are only valid when the chart size is set.
  ///
  /// \param width The chart width.
  /// \param height The chart height.
  void setChartSize(int width, int height);

  /// \brief
  ///   Sets the chart layer bounds.
  /// \param bounds The chart layer bounds.
  void setChartLayerBounds(const QRect &bounds);
  //@}

  /// \name Zoom Methods
  //@{
  /// \brief
  ///   Gets the x-axis zoom factor.
  /// \return
  ///   The x-axis zoom factor as a percentage.
  int getXZoomFactor() const {return this->ZoomFactorX;}

  /// \brief
  ///   Gets the y-axis zoom factor.
  /// \return
  ///   The y-axis zoom factor as a percentage.
  int getYZoomFactor() const {return this->ZoomFactorY;}

  /// \brief
  ///   Zooms the chart to the given percentage.
  /// \param percent The new zoom factor for both axes.
  /// \sa pqChartContentsSpace::zoomToPercent(int, int)
  void zoomToPercent(int percent);

  /// \brief
  ///   Zooms the chart to the given percentages.
  ///
  /// The zoom factors of the chart are independent of each other. In
  /// other words, the x-axis can be zoomed to a different factor
  /// than the y-axis.
  ///
  /// When the zoom factors are changed, the new zoom viewport will
  /// be added to the zoom history. The zoom history can be navigated
  /// using the \c historyNext and \c historyPrevious methods. The
  /// user can also navigate through the history using the keyboard
  /// shortcuts.
  ///
  /// \param xPercent The x-axis zoom factor as a percentage.
  /// \param yPercent The y-axis zoom factor as a percentage.
  void zoomToPercent(int xPercent, int yPercent);

  /// \brief
  ///   Zooms only the x-axis to a percentage.
  /// \param percent The x-axis zoom factor as a percentage.
  /// \sa pqChartContentsSpace::zoomToPercent(int, int)
  void zoomToPercentX(int percent);

  /// \brief
  ///   Zooms only the y-axis to a percentage.
  /// \param percent The y-axis zoom factor as a percentage.
  /// \sa pqChartContentsSpace::zoomToPercent(int, int)
  void zoomToPercentY(int percent);

  /// \brief
  ///   Zooms chart to the given rectangle.
  ///
  /// The rectangle should be given in contents coordinates. The
  /// chart layer bounds must be set in order to call this method.
  ///
  /// \param area The zoom area in current contents coordinates.
  /// \sa pqChartContentsSpace::zoomToPercent(int, int)
  void zoomToRectangle(const QRect &area);

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
  /// \brief
  ///   Signals the start of a mouse move interaction.
  ///
  /// While an interaction is in progress, the zoom history will not
  /// be updated. When \c finishInteraction is called, the history is
  /// updated if the zoom factors have changed.
  ///
  /// \sa pqChartContentsSpace::finishInteraction()
  void startInteraction();

  /// \brief
  ///   Gets whether or not an interaction is currently in progress.
  /// \return
  ///   True if an interaction is currently in progress.
  /// \sa pqChartContentsSpace::startInteraction(),
  ///     pqChartContentsSpace::finishInteraction()
  bool isInInteraction() const;

  /// \brief
  ///   Signals the end of a mouse move interaction.
  /// \sa pqChartContentsSpace::startInteraction()
  void finishInteraction();

  /// \brief
  ///   Zooms in or out and centers on the position indicated.
  /// \param delta The wheel delta parameter from the event.
  /// \param pos The current mouse position in chart coordinates.
  /// \param flags Used to change the interaction slightly.
  void handleWheelZoom(int delta, const QPoint &pos, InteractFlags flags);
  //@}

  /// \name History Methods
  //@{
  /// \brief
  ///   Gets whether or not a previous zoom viewport is available.
  /// \return
  ///   True if a previous zoom viewport is available.
  bool isHistoryPreviousAvailable() const;

  /// \brief
  ///   Gets whether or not a forward zoom viewport is available.
  /// \return
  ///   True if a forward zoom viewport is available.
  bool isHistoryNextAvailable() const;
  //@}

public slots:
  /// \brief
  ///   Sets the x offset.
  /// \param offset The new x offset.
  void setXOffset(int offset);

  /// \brief
  ///   Sets the y offset.
  /// \param offset The new y offset.
  void setYOffset(int offset);

  /// \brief
  ///   Sets the maximum x offset.
  /// \param maximum The maximum x offset.
  void setMaximumXOffset(int maximum);

  /// \brief
  ///   Sets the maximum y offset.
  /// \param maximum The maximum y offset.
  void setMaximumYOffset(int maximum);

  /// Pans up by a predetermined amount.
  void panUp();

  /// Pans down by a predetermined amount.
  void panDown();

  /// Pans left by a predetermined amount.
  void panLeft();

  /// Pans right by a predetermined amount.
  void panRight();

  /// Resets the zoom factors to 100 percent.
  void resetZoom();

  /// Changes the view to the next one in the history.
  void historyNext();

  /// Changes the view to the previous one in the history.
  void historyPrevious();

public:
  /// \brief
  ///   Gets the zoom factor step.
  /// \return
  ///   The zoom factor step.
  /// \sa pqChartContentsSpace::zoomIn(InteractFlags),
  ///     pqChartContentsSpace::zoomOut(InteractFlags)
  static int getZoomFactorStep();

  /// \brief
  ///   Sets the zoom factor step.
  /// \param step The new zoom factor step.
  static void setZoomFactorStep(int step);

  /// \brief
  ///   Gets the pan step.
  /// \return
  ///   The pan step.
  /// \sa pqChartContentsSpace::panUp(),
  ///     pqChartContentsSpace::panDown(),
  ///     pqChartContentsSpace::panLeft(),
  ///     pqChartContentsSpace::panRight(),
  static int getPanStep();

  /// \brief
  ///   Sets the pan step.
  /// \param step The new pan step.
  static void setPanStep(int step);

signals:
  /// \brief
  ///   Emitted when the x offset has changed.
  /// \param offset The new x offset.
  void xOffsetChanged(int offset);

  /// \brief
  ///   Emitted when the y offset has changed.
  /// \param offset The new y offset.
  void yOffsetChanged(int offset);

  /// \brief
  ///   Emitted when the maximum offsets have changed.
  ///
  /// This signal is sent when either or both of the offsets have
  /// changed. Sending the changes as one improves the chart layout.
  ///
  /// \param xMaximum The maximum x offset.
  /// \param yMaximum The maximum y offset.
  void maximumChanged(int xMaximum, int yMaximum);

  /// \brief
  ///   Emitted when the view history availability changes.
  /// \param available True if there is a history item available
  ///   before the current one.
  void historyPreviousAvailabilityChanged(bool available);

  /// \brief
  ///   Emitted when the view history availability changes.
  /// \param available True if there is a history item available
  ///   after the current one.
  void historyNextAvailabilityChanged(bool available);

private:
  /// Keeps track of mouse position and history.
  pqChartContentsSpaceInternal *Internal;
  int OffsetX;     ///< Stores the x offset.
  int OffsetY;     ///< Stores the y offset.
  int MaximumX;    ///< Stores the maximum x offset.
  int MaximumY;    ///< Stores the maximum y offset.
  int Width;       ///< Stores the chart width.
  int Height;      ///< Stores the chart height.
  int ZoomFactorX; ///< Stores the x-axis zoom factor.
  int ZoomFactorY; ///< Stores the y-axis zoom factor.

  static int ZoomFactorStep; ///< Stores the zoom factor step.
  static int PanStep;        ///< Stores the pan step.
};

#endif
