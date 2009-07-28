/*=========================================================================

   Program: ParaView
   Module:    pqChartMouseBox.h

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

/// \file pqChartMouseBox.h
/// \date 9/28/2005

#ifndef _pqChartMouseBox_h
#define _pqChartMouseBox_h


#include "QtChartExport.h"

class pqChartMouseBoxInternal;
class QPoint;
class QRect;


/*!
 *  \class pqChartMouseBox
 *  \brief
 *    The pqChartMouseBox class stores the data for a mouse box that
 *    can be used for zooming or selection.
 * 
 *  To use the pqChartMouseBox, code needs to be added to several
 *  key methods. The drag box interaction starts in the mouse press
 *  event. The box grows or shrinks in the mouse move event. In the
 *  mouse release event, the box is finalized and used for its
 *  intent (zoom, select, etc.).
 * 
 *  In the mouse press event, the mouse location needs to be saved.
 *  The coordinate system used to set the mouse press location must
 *  be the same coordinate system used to adjust the box.
 *  \code
 *  void SomeClass::mousePressEvent(QMouseEvent *e)
 *  {
 *     this->mouseBox->setStartingPosition(e->pos());
 *  }
 *  \endcode
 * 
 *  In the mouse move event, the drag box needs to be updated. The
 *  point set in the mouse press event should remain unchanged until
 *  the mouse release event. If your class watches all mouse move
 *  events, make sure the box is only updated for drag events.
 *  \code
 *  void SomeClass::mouseMoveEvent(QMouseEvent *e)
 *  {
 *     QRect area;
 *     this->mouseBox->getRectangle(area);
 *     this->mouseBox->adjustRectangle(e->pos());
 * 
 *     // Repaint the mouse box. Unite the previous area with the new
 *     // area to ensure all the changes get repainted.
 *     this->mouseBox->getUnion(area);
 *     update(area);
 *  }
 *  \endcode
 * 
 *  In the mouse release event, the drag box needs to be updated
 *  with the release location before using it. After using the box,
 *  it should be reset for the next time.
 *  \code
 *  void SomeClass::mouseReleaseEvent(QMouseEvent *e)
 *  {
 *     QRect area;
 *     this->mouseBox->adjustRectangle(e->pos());
 *     this->mouseBox->getRectangle(area);
 *     ...
 *     this->mouseBox->resetRectangle();
 *     update(area);
 *  }
 *  \endcode
 * 
 *  The paint event should also be modified to display the drag box.
 *  Only the rectangle is stored in the pqChartMouseBox, so you can
 *  display the box any way you like. The following code draws a
 *  dashed line around the box.
 *  \code
 *  void SomeClass::paintEvent(QPaintEvent *e)
 *  {
 *     ...
 *     if(this->mouseBox->isValid())
 *     {
 *        QRect box;
 *        painter->setPen(Qt::black);
 *        painter->setPen(Qt::DotLine);
 *        this->mouseBox->getPaintRectangle(box);
 *        painter->drawRect(box);
 *     }
 *     ...
 *  }
 *  \endcode
 */
class QTCHART_EXPORT pqChartMouseBox
{
public:
  pqChartMouseBox();
  ~pqChartMouseBox();

  /// \brief
  ///   Sets the mouse box starting position.
  ///
  /// The starting position should be set before calling
  /// \c adjustRectangle. The starting position and adjustment
  /// positions should be in the same coordinate space.
  ///
  /// \param start The original mouse press location.
  /// \sa pqChartMouseBox::adjustRectangle(const QPoint &)
  void setStartingPosition(const QPoint &start);

  /// \brief
  ///   Gets whether or not the rectangle is valid.
  /// \return
  ///   True if the rectangle is valid.
  bool isValid() const;

  /// \brief
  ///   Gets the current mouse box.
  /// \param area Used to return the mouse box.
  void getRectangle(QRect &area) const;

  /// \brief
  ///   Gets the rectangle suitable for painting the mouse box.
  /// \param area Used to return the painting rectangle.
  void getPaintRectangle(QRect &area) const;

  /// \brief
  ///   Gets the union of the mouse box and the given area.
  ///
  /// If the mouse box is not valid, this method does nothing. If the
  /// area passed in is not valid, the mouse box area will be returned,
  /// which is equivalent to calling \c getRectangle.
  ///
  /// \param area The rectangle to union with the mouse box. The
  ///   variable is also used to return the new rectangle.
  void getUnion(QRect &area) const;

  /// \brief
  ///   Adjusts the boundary of the mouse box.
  ///
  /// The selection or zoom box should contain the original mouse
  /// down location and the current mouse location. This method
  /// is used to adjust the box based on the current mouse location.
  ///
  /// \param current The current position of the mouse.
  /// \sa pqChartMouseBox::setStartingPosition(const QPoint &)
  void adjustRectangle(const QPoint &current);

  /// Resets the mouse box to an invalid rectangle.
  void resetRectangle();

private:
  pqChartMouseBoxInternal *Internal; ///< Stores the mouse box data.
};

#endif
