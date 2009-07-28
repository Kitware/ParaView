/*=========================================================================

   Program: ParaView
   Module:    pqChartZoomHistory.h

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

/*!
 * \file pqChartZoomHistory.h
 *
 * \brief
 *   The pqChartZoomViewport and pqChartZoomHistory classes are used to
 *   keep track of the user's zoom position(s).
 *
 * \author Mark Richardson
 * \date   June 10, 2005
 */

#ifndef _pqChartZoomHistory_h
#define _pqChartZoomHistory_h


#include "QtChartExport.h"

class pqChartZoomHistoryInternal;


/// \class pqChartZoomViewport
/// \brief
///   The pqChartZoomViewport class stores the position and zoom
///   factors for a viewport.
///
/// The position stores the top-left corner of the viewport in
/// content coordinates. The zoom factors are stored as percentages.
class QTCHART_EXPORT pqChartZoomViewport
{
public:
  pqChartZoomViewport();
  ~pqChartZoomViewport() {}

  /// \brief
  ///   Sets the viewport position coordinates.
  /// \param x The x coordinate.
  /// \param y The y coordinate.
  /// \sa pqChartZoomViewport::getXPosition(),
  ///     pqChartZoomViewport::getYPosition()
  void setPosition(int x, int y);

  /// \brief
  ///   Sets the zoom percentages.
  /// \param x The x-axis zoom factor.
  /// \param y The y-axis zoom factor.
  /// \sa pqChartZoomViewport::getXZoom(),
  ///     pqChartZoomViewport::getYZoom()
  void setZoom(int x, int y);

  /// \brief
  ///   Gets the x coordinate of the viewport.
  /// \return
  ///   The x coordinate of the viewport.
  /// \sa pqChartZoomViewport::setPosition(int, int)
  int getXPosition() const {return this->X;}

  /// \brief
  ///   Gets the y coordinate of the viewport.
  /// \return
  ///   The y coordinate of the viewport.
  /// \sa pqChartZoomViewport::setPosition(int, int)
  int getYPosition() const {return this->Y;}

  /// \brief
  ///   Gets the x-axis zoom factor.
  /// \return
  ///   The x-axis zoom factor.
  /// \sa pqChartZoomViewport::setZoom(int, int)
  int getXZoom() const {return this->XPercent;}

  /// \brief
  ///   Gets the y-axis zoom factor.
  /// \return
  ///   The y-axis zoom factor.
  /// \sa pqChartZoomViewport::setZoom(int, int)
  int getYZoom() const {return this->YPercent;}

private:
  int X;        ///< Stores the x position coordinate.
  int Y;        ///< Stores the y position coordinate.
  int XPercent; ///< Stores the x-axis zoom factor.
  int YPercent; ///< Stores the y-axis zoom factor.
};


/// \class pqChartZoomHistory
/// \brief
///   The pqChartZoomHistory class stores a list of pqChartZoomViewport
///   objects.
///
/// The zoom history contains a list of zoom viewports. The list is
/// ordered chronologically, and contains an index to the current item.
/// The history list is limited to a certain number of items. The
/// default limit is 10, but it can be changed using the \c setLimit
/// method.
///
/// When adding items to the history list, the new item will become
/// the current item. The front of the list may be trimmed to stay
/// within limits. If the current item is in the middle of the list,
/// the subsequent items will be removed before adding the new item
/// to the end of the list.
///
/// The history list is navigated using the \c getPrevious and
/// \c getNext methods. You can also use the \c getCurrent method to
/// get the current item without changing the index.
///
/// \sa pqChartZoomHistory::setLimit(int),
///     pqChartZoomHistory::addHistory(int, int, int, int),
///     pqChartZoomHistory::getNext(),
///     pqChartZoomHistory::getPrevious(),
///     pqChartZoomHistory::getCurrent()
class QTCHART_EXPORT pqChartZoomHistory
{
public:
  pqChartZoomHistory();
  ~pqChartZoomHistory();

  /// \brief
  ///   Sets the maximum number of items in the history.
  /// \param limit The maximum number of entries.
  void setLimit(int limit);

  /// \brief
  ///   Gets the maximum number of items in the history.
  /// \return
  ///   The maximum number of entries.
  int getLimit() const {return this->Allowed;}

  /// \brief
  ///   Adds a zoom viewport to the history list.
  ///
  /// The new item will become the current item in the list. If
  /// the current item is not at the end of the list, all the
  /// subsequent items will be removed. If the list is longer
  /// than the allowed limit, items will be removed from the
  /// front of the list.
  ///
  /// \param x The x position of the viewport.
  /// \param y The y position of the viewport.
  /// \param xZoom The x-axis zoom factor for the viewport.
  /// \param yZoom The y-axis zoom factor for the viewport.
  /// \sa pqChartZoomHistory::updatePosition(int, int)
  void addHistory(int x, int y, int xZoom, int yZoom);

  /// \brief
  ///   Used to update the viewport position for the current
  ///   zoom factors.
  ///
  /// This method allows the current zoom viewport to be updated
  /// when the user changes the viewport position by panning or
  /// scrolling.
  ///
  /// \param x The x position of the viewport.
  /// \param y The y position of the viewport.
  /// \sa pqChartZoomHistory::addHistory(int, int, int, int)
  void updatePosition(int x, int y);

  /// \brief
  ///   Gets whether or not a zoom viewport is before the current.
  /// \return
  ///   True if a zoom viewport is before the current.
  bool isPreviousAvailable() const;

  /// \brief
  ///   Gets whether or not a zoom viewport is after the current.
  /// \return
  ///   True if a zoom viewport is after the current.
  bool isNextAvailable() const;

  /// \brief
  ///   Gets the current zoom viewport.
  /// \return
  ///   A pointer to the current zoom viewport or null if the list
  ///   is empty.
  const pqChartZoomViewport *getCurrent() const;

  /// \brief
  ///   Gets the previous zoom viewport in the history.
  /// \return
  ///   A pointer to the previous zoom viewport or null if the
  ///   beginning of the list is reached.
  /// \sa pqChartZoomHistory::getNext()
  const pqChartZoomViewport *getPrevious();

  /// \brief
  ///   Gets the next zoom viewport in the history.
  /// \return
  ///   A pointer to the next zoom viewport or null if the end
  ///   of the list is reached.
  /// \sa pqChartZoomHistory::getPrevious()
  const pqChartZoomViewport *getNext();

private:
  /// Stores the zoom viewport list.
  pqChartZoomHistoryInternal *Internal;

  int Current; ///< Stores the current item index.
  int Allowed; ///< Stores the list length limit.
};

#endif
