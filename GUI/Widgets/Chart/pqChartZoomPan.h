/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
 * \file pqChartZoomPan.h
 *
 * \brief
 *   The pqChartZoomPan class is used to handle the zoom and pan
 *   interaction for a chart widget.
 *
 * \author Mark Richardson
 * \date   August 9, 2005
 */

#ifndef _pqChartZoomPan_h
#define _pqChartZoomPan_h

#include "pqChartExport.h"
#include <QObject>
#include <QPoint> // Needed for QPoint member.

class QRect;
class QAbstractScrollArea;
class pqChartZoomHistory;

/*!
    \class pqChartZoomPan
    \brief
      The pqChartZoomPan class is used to handle the zoom and pan
      interaction for a chart widget.
   
    The zoom factors and contents positions can be saved in a
    history list for user navigation. The zoom/pan object does not
    dictate the key or mouse events that initiate the zooming or
    panning. Those decisions are left to the object using the
    zoom/pan class.
   
    The pqChartZoomPan object works with a QAbstractScrollArea object
    to provide the functionality. The QAbstractScrollArea should be
    set in the constructor. Without the scroll view, the zoom/pan object
    will not function correctly. The \c contentsSizeChanging signal
    should be used to update the chart layout.
    \code
    SomeChart::SomeChart(QWidget *parent)
      : QAbstractScrollArea(parent)
    {
       this->zoomPan = new pqChartZoomPan(this);
       ...
       connect(this->zoomPan, SIGNAL(contentsSizeChanging(int, int)),
          this, SLOT(layoutChart(int, int)));
    }
    \endcode
   
    The pqChartZoomPan object will store the contents position and
    width for the scroll view. It will automatically adjust the
    scrollbars when the contents position or size has changed. The
    \c updateContentSize() method should be called when the widget
    is shown and when the viewport is resized.
   
    After creating and connecting the zoom/pan object, the keyboard
    and mouse interactions can be hooked up. The following shows the
    key press event handler. The arrow keys will be hooked up to the
    pan functions. The +/- keys will be hooked up to the zoom
    functions. The Alt+Left and Alt+Right will be hooked up to the
    zoom history.
    \code
    void SomeChart::keyPressEvent(QKeyEvent *e)
    {
       if(e->key() == Qt::Key_Plus || e->key() == Qt::Key_Equal ||
          e->key() == Qt::Key_Minus)
       {
          pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
          int state = e->modifiers() & (Qt::ControlModifier |
             Qt::AltModifier | Qt::MetaModifier);
          if(state == Qt::ControlModifier)
             flags = pqChartZoomPan::ZoomXOnly;
          else if(state == Qt::AltModifier)
             flags = pqChartZoomPan::ZoomYOnly;
   
          if(e->key() == Qt::Key_Minus)
             this->zoomPan->zoomOut(flags);
          else
             this->zoomPan->zoomIn(flags);
       }
       else if(e->key() == Qt::Key_Up)
          this->zoomPan->panUp();
       else if(e->key() == Qt::Key_Down)
          this->zoomPan->panDown();
       else if(e->key() == Qt::Key_Left)
       {
          if(e->modifiers() == Qt::AltModifier)
             this->zoomPan->historyPrevious();
          else
             this->zoomPan->panLeft();
       }
       else if(e->key() == Qt::Key_Right)
       {
          if(e->modifiers() == Qt::AltModifier)
             this->zoomPan->historyNext();
          else
             this->zoomPan->panRight();
       }
    }
    \endcode
   
    The mouse interactions need to be put in the mouse press, mouse
    move, and mouse release events. The following example doesn't
    show the zoom box interaction. For details on that see the
    pqChartMouseBox class.
    \code
    void SomeChart::mousePressEvent(QMouseEvent *e)
    {
       this->zoomPan->Last = e->globalPos();
    }
    \endcode
   
    Store the mouse position, in global coordinates, during the
    mouse press event. The zoom/pan object will update the last
    position as it needs to after that. The mouse move event will
    indicate the start and continuation of a zoom/pan interaction.
    The following example uses the middle and right mouse buttons
    for zoom and pan respectively. This frees up the left mouse
    button for picking.
    \code
    void SomeChart::mouseMoveEvent(QMouseEvent *e)
    {
       if(this->mouseMode == NoMode)
       {
          if(e->buttons() == Qt::MidButton)
          {
             this->mouseMode = ZoomMode;
             this->zoomPan->startInteraction(pqChartZoomPan::Zoom);
          }
          else if(e->buttons() == Qt::RightButton)
          {
             this->mouseMode = PanMode;
             this->zoomPan->startInteraction(pqChartZoomPan::Pan);
          }
       }
   
       if(this->mouseMode == ZoomMode)
       {
          pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
          if(e->modifiers() == Qt::ControlModifier)
             flags = pqChartZoomPan::ZoomXOnly;
          else if(e->modifiers() == Qt::AltModifier)
             flags = pqChartZoomPan::ZoomYOnly;
          this->zoomPan->interact(e->globalPos(), flags);
       }
       else if(this->mouseMode == PanMode)
          this->zoomPan->interact(e->globalPos(), pqChartZoomPan::NoFlags);
    }
    \endcode
   
    The mouse release event signals the end of an interaction mode.
    clear the state of the zoom/pan object in this method.
    \code
    void SomeChart::mouseReleaseEvent(QMouseEvent *e)
    {
       if(this->mouseMode == ZoomMode || this->mouseMode == PanMode)
       {
          this->mouseMode = NoMode;
          this->zoomPan->finishInteraction();
       }
    }
    \endcode
 */
class QTCHART_EXPORT pqChartZoomPan : public QObject
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
  pqChartZoomPan(QAbstractScrollArea *parent=0);
  virtual ~pqChartZoomPan();

  /// \name Contents Methods
  //@{
  /// \brief
  ///   Gets the contents x position.
  /// \return
  ///   The contents x position.
  int contentsX() const {return this->ContentsX;}

  /// \brief
  ///   Gets the contents y position.
  /// \return
  ///   The contents y position.
  int contentsY() const {return this->ContentsY;}

  /// \brief
  ///   Gets the contents width.
  /// \return
  ///   The contents width.
  int contentsWidth() const {return this->ContentsWidth;}

  /// \brief
  ///   Gets the contents height.
  /// \return
  ///   The contents height.
  int contentsHeight() const {return this->ContentsHeight;}
  //@}

  /// \name Zoom Methods
  //@{
  /// \brief
  ///   Gets the x-axis zoom factor.
  /// \return
  ///   The x-axis zoom factor as a percentage.
  int getXZoomFactor() const {return this->XZoomFactor;}

  /// \brief
  ///   Gets the y-axis zoom factor.
  /// \return
  ///   The y-axis zoom factor as a percentage.
  int getYZoomFactor() const {return this->YZoomFactor;}

  /// \brief
  ///   Zooms the chart to the given percentage.
  /// \param percent The new zoom factor for both axes.
  /// \sa pqChartZoomPan::zoomToPercent(int, int)
  void zoomToPercent(int percent);

  /// \brief
  ///   Zooms the chart to the given percentages.
  ///
  /// The zoom factors of the chart are independent of each other.
  /// In other words, the x-axis can be zoomed to a different
  /// factor than the y-axis.
  ///
  /// When the zoom factors are changed, the new zoom viewport will
  /// be added to the zoom history. The zoom history can be navigated
  /// using the \c historyNext and \c historyPrevious methods. The
  /// user can also navigate through the history using the keyboard
  /// shortcuts.
  ///
  /// \param percentX The x-axis zoom factor as a percentage.
  /// \param percentY The y-axis zoom factor as a percentage.
  void zoomToPercent(int percentX, int percentY);

  /// \brief
  ///   Zooms only the x-axis to a percentage.
  /// \param percent The x-axis zoom factor as a percentage.
  /// \sa pqChartZoomPan::zoomToPercent(int, int)
  void zoomToPercentX(int percent);

  /// \brief
  ///   Zooms only the y-axis to a percentage.
  /// \param percent The y-axis zoom factor as a percentage.
  /// \sa pqChartZoomPan::zoomToPercent(int, int)
  void zoomToPercentY(int percent);

  /// \brief
  ///   Zooms chart to the given rectangle.
  ///
  /// The rectangle should be given in content coordinates. If
  /// the rectangle aspect ratio does not match the viewport
  /// aspect ratio, the longest length will be used to ensure
  /// that everything in the rectangle will be shown.
  ///
  /// \param area The zoom area in current content coordinates.
  /// \sa pqChartZoomPan::zoomToPercent(int, int)
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
  /// \sa pqChartZoomPan::finishInteraction(),
  ///     pqChartZoomPan::interact(const QPoint &, const QPoint &,
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
  /// \sa pqChartZoomPan::startInteraction(InteractMode),
  ///     pqChartZoomPan::finishInteraction()
  void interact(const QPoint &pos, InteractFlags flags);

  /// \brief
  ///   Signals the end of a mouse move interaction.
  ///
  /// The parent's cursor will be set back to the arrow cursor. The
  /// current interaction type will be cleared so another interaction
  /// type can be started. If the interaction was a zoom operation,
  /// the current position will be added to the zoom history.
  ///
  /// \sa pqChartZoomPan::startInteraction(),
  ///     pqChartZoomPan::interact(const QPoint &, const QPoint &,
  ///         InteractFlags)
  void finishInteraction();

  /// \brief
  ///   Zooms in or out and senters on the position indicated.
  /// \param delta The wheel delta parameter from the event.
  /// \param pos The current mouse position in contents coordinates.
  /// \param flags Used to change the interaction slightly.
  bool handleWheelZoom(int delta, const QPoint &pos, InteractFlags flags);
  //@}

public slots:
  /// \brief
  ///   Sets the contents x position.
  /// \param x The new contents x position.
  void setContentsX(int x);

  /// \brief
  ///   Sets the contents y position.
  /// \param y The new contents y position.
  void setContentsY(int y);

  /// \brief
  ///   Sets the contents  position.
  /// \param x The new contents x position.
  /// \param y The new contents y position.
  void setContentsPos(int x, int y);

  /// \brief
  ///   Updates the contents size based on the viewport size.
  void updateContentSize();

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

signals:
  /// brief
  ///   Called before changing the contents size.
  /// \param width The new contents width.
  /// \param height The new contents height.
  void contentsSizeChanging(int width, int height);

  /// brief
  ///   Called after changing the contents size.
  /// \param width The new contents width.
  /// \param height The new contents height.
  void contentsSizeChanged(int width, int height);

public:
  /// Stores the last mouse location in global coordinates.
  QPoint Last;

private:
  InteractMode Current;        ///< Stores the current interact mode.
  QAbstractScrollArea *Parent; ///< Stores a pointer to the parent.
  pqChartZoomHistory *History; ///< Stores the viewport zoom history.
  int ContentsX;               ///< Stores the x contents position.
  int ContentsY;               ///< Stores the y contents position.
  int ContentsWidth;           ///< Stores the contents width.
  int ContentsHeight;          ///< Stores the contents height.
  int XZoomFactor;             ///< Stores the x-axis zoom factor.
  int YZoomFactor;             ///< Stores the y-axis zoom factor.
  bool InHistory;              ///< Used for zoom history processing.
  bool InPosition;             ///< Used when setting the position.  
};

#endif
