/*!
 * \file pqChartMouseBox.h
 *
 * \brief
 *   The pqChartMouseBox class stores the data for a mouse zoom or
 *   selection box.
 *
 * \author Mark Richardson
 * \date   September 28, 2005
 */

#ifndef _pqChartMouseBox_h
#define _pqChartMouseBox_h


#include "pqChartExport.h"
#include <QRect>  // Needed for QRect member.
#include <QPoint> // Needed for QPoint member.


/// \class pqChartMouseBox
/// \brief
///   The pqChartMouseBox class stores the data for a mouse zoom or
///   selection box.
///
/// To use the pqChartMouseBox, code needs to be added to several
/// key methods. The drag box interaction starts in the mouse press
/// event. The box grows or shrinks in the mouse move event. In the
/// mouse release event, the box is finalized and used for its
/// intent (zoom, select, etc.).
///
/// In the mouse press event, the mouse location needs to be saved.
/// The coordinate system used to determine the mouse location must
/// be the same coordinate system used to set up the box.
/// \code
/// void SomeClass::mousePressEvent(QMouseEvent *e)
/// {
///    this->mouseBox->Last = e->pos();
/// }
/// \endcode
///
/// In the mouse move event, the drag box needs to be updated. The
/// point set in the mouse press event should remain unchanged until
/// the mouse release event. If your class watches all mouse move
/// events, make sure the box is only updated for drag events.
/// \code
/// void SomeClass::mouseMoveEvent(QMouseEvent *e)
/// {
///    QRect area = this->mouseBox->Box;
///    this->mouseBox->adjustBox(e->pos());
///
///    // Repaint the zoom box. Unite the previous area with the new
///    // area to ensure all the changes get repainted.
///    if(area.isValid())
///       area = area.unite(this->mouseBox->Box);
///    else
///       area = this->mouseBox->Box;
///    update(area);
/// }
/// \endcode
///
/// In the mouse release event, the drag box needs to be updated
/// with the release location before using it. After using the box,
/// it should be reset for the next time.
/// \code
/// void SomeClass::mouseReleaseEvent(QMouseEvent *e)
/// {
///    this->mouseBox->setBox(e->pos());
///    QRect area = this->mouseBox->Box;
///    ...
///    this->mouseBox->resetBox();
///    update(area);
/// }
/// \endcode
///
/// The paint event should also be modified to display the drag box.
/// Only the rectangle is stored in the pqChartMouseBox, so you can
/// display the box any way you like. The following code draws a
/// dashed line around the box.
/// \code
/// void SomeClass::paintEvent(QPaintEvent *e)
/// {
///    ...
///    if(this->mouseBox->Box.isValid())
///    {
///       painter->setPen(Qt::black);
///       painter->setPen(Qt::DotLine);
///       painter->drawRect(this->data->Box.x(), this->data->Box.y(),
///          this->data->Box.width() - 1, this->data->Box.height() - 1);
///    }
///    ...
/// }
/// \endcode
class QTCHART_EXPORT pqChartMouseBox
{
public:
  pqChartMouseBox();
  ~pqChartMouseBox() {}

  /// \brief
  ///   Adjusts the boundary of the mouse drag box.
  ///
  /// The selection or zoom box should contain the original mouse
  /// down location and the current mouse location. This method
  /// is used to adjust the box based on the current mouse location.
  ///
  /// \param current The current position of the mouse.
  void adjustBox(const QPoint &current);

  /// Resets the drag box to an invalid rectangle.
  void resetBox();

public:
  QRect Box;   ///< Stores the mouse drag box.
  QPoint Last; ///< Stores the last mouse location.
};

#endif
