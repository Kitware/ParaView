/*!
 * \file pqChartZoomHistory.h
 *
 * \brief
 *   The pqChartZoomItem and pqChartZoomHistory classes are used to
 *   keep track of the user's zoom position(s).
 *
 * \author Mark Richardson
 * \date   June 10, 2005
 */

#ifndef _pqChartZoomHistory_h
#define _pqChartZoomHistory_h


class pqChartZoomHistoryData;


/// \class pqChartZoomItem
/// \brief
///   The pqChartZoomItem class stores the position and zoom factors
///   for a particular viewport.
///
/// The position stores the top-left corner of the viewport in
/// content coordinates. These coordinates are used to set the
/// content's position in the scroll view. The zoom factors are
/// stored as percentages.
class pqChartZoomItem
{
public:
  pqChartZoomItem();
  ~pqChartZoomItem() {}

  /// \brief
  ///   Sets the viewport position coordinates.
  /// \param x The x coordinate.
  /// \param y The y coordinate.
  /// \sa pqChartZoomItem::getXPosition(),
  ///     pqChartZoomItem::getYPosition()
  void setPosition(int x, int y);

  /// \brief
  ///   Sets the xoom percentages.
  /// \param x The x-axis zoom factor.
  /// \param y The y-axis zoom factor.
  /// \sa pqChartZoomItem::getXZoom(),
  ///     pqChartZoomItem::getYZoom()
  void setZoom(int x, int y);

  /// \brief
  ///   Gets the x coordinate of the viewport.
  /// \return
  ///   The x coordinate of the viewport.
  /// \sa pqChartZoomItem::setPosition(int, int)
  int getXPosition() const {return this->X;}

  /// \brief
  ///   Gets the y coordinate of the viewport.
  /// \return
  ///   The y coordinate of the viewport.
  /// \sa pqChartZoomItem::setPosition(int, int)
  int getYPosition() const {return this->Y;}

  /// \brief
  ///   Gets the x-axis zoom factor.
  /// \return
  ///   The x-axis zoom factor.
  /// \sa pqChartZoomItem::setZoom(int, int)
  int getXZoom() const {return this->XPercent;}

  /// \brief
  ///   Gets the y-axis zoom factor.
  /// \return
  ///   The y-axis zoom factor.
  /// \sa pqChartZoomItem::setZoom(int, int)
  int getYZoom() const {return this->YPercent;}

private:
  int X;        ///< Stores the x position coordinate.
  int Y;        ///< Stores the y position coordinate.
  int XPercent; ///< Stores the x-axis zoom factor.
  int YPercent; ///< Stores the y-axis zoom factor.
};


/// \class pqChartZoomHistory
/// \brief
///   The pqChartZoomHistory class stores a list of pqChartZoomItem
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
class pqChartZoomHistory
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
  ///   Gets the current zoom viewport.
  /// \return
  ///   A pointer to the current zoom viewport or null if the list
  ///   is empty.
  const pqChartZoomItem *getCurrent() const;

  /// \brief
  ///   Gets the previous zoom viewport in the history.
  /// \return
  ///   A pointer to the previous zoom viewport or null if the
  ///   beginning of the list is reached.
  /// \sa pqChartZoomHistory::getNext()
  const pqChartZoomItem *getPrevious();

  /// \brief
  ///   Gets the next zoom viewport in the history.
  /// \return
  ///   A pointer to the next zoom viewport or null if the end
  ///   of the list is reached.
  /// \sa pqChartZoomHistory::getPrevious()
  const pqChartZoomItem *getNext();

private:
  pqChartZoomHistoryData *Data; ///< Stores the zoom viewport list.
  int Current;                  ///< Stores the current item index.
  int Allowed;                  ///< Stores the list length limit.
};

#endif
