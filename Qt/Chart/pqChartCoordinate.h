/*!
 * \file pqChartCoordinate.h
 *
 * \brief
 *   The pqChartCoordinate class stores an x, y coordinate for a chart.
 *
 * \author Mark Richardson
 * \date   August 19, 2005
 */

#ifndef _pqChartCoordinate_h
#define _pqChartCoordinate_h

#include "pqChartExport.h"
#include "pqChartValue.h" // Needed for x and y members.

class pqChartCoordinateListData;
class pqChartCoordinateIteratorData;
class pqChartCoordinateConstIteratorData;


/// \class pqChartCoordinate
/// \brief
///   The pqChartCoordinate class is used to store a pair of values.
///
/// The x and y coordinates are stored as pqChartValue objects, which
/// allows any combination of int, float, or double. This is useful
/// when the chart axes use different types. If the axis uses a list
/// of string labels, the coordinate should use an int for the index.
class QTCHART_EXPORT pqChartCoordinate
{
public:
  pqChartCoordinate();

  /// \brief
  ///   Creates a coordinate from two pqChartValue objects.
  /// \param px The x coordinate of the point.
  /// \param py The y coordinate of the point.
  pqChartCoordinate(const pqChartValue &px, const pqChartValue &py);

  /// \brief
  ///   Makes a copy of an existing coordinate.
  /// \param other The coordinate to copy.
  pqChartCoordinate(const pqChartCoordinate &other);
  ~pqChartCoordinate() {}

  /// \name Operators
  //@{
  bool operator==(const pqChartCoordinate &other) const;
  bool operator!=(const pqChartCoordinate &other) const;
  pqChartCoordinate &operator=(const pqChartCoordinate &other);
  //@}

public:
  pqChartValue X; ///< Stores the x coordinate.
  pqChartValue Y; ///< Stores the y coordinate.
};


class pqChartCoordinateConstIterator;

/// \class pqChartCoordinateIterator
/// \brief
///   The pqChartCoordinateIterator class is used to iterate through
///   a pqChartCoordinateList.
class QTCHART_EXPORT pqChartCoordinateIterator
{
  friend class pqChartCoordinateList;
  friend class pqChartCoordinateConstIterator;

public:
  pqChartCoordinateIterator();
  pqChartCoordinateIterator(const pqChartCoordinateIterator &iter);
  ~pqChartCoordinateIterator();

  bool operator==(const pqChartCoordinateIterator &iter) const;
  bool operator!=(const pqChartCoordinateIterator &iter) const;
  bool operator==(const pqChartCoordinateConstIterator &iter) const;
  bool operator!=(const pqChartCoordinateConstIterator &iter) const;

  const pqChartCoordinate &operator*() const;
  pqChartCoordinate &operator*();
  pqChartCoordinate *operator->();

  pqChartCoordinateIterator &operator++();
  pqChartCoordinateIterator operator++(int post);

  pqChartCoordinateIterator &operator=(const pqChartCoordinateIterator &iter);

private:
  pqChartCoordinateIteratorData *Data;
};


/// \class pqChartCoordinateConstIterator
/// \brief
///   The pqChartCoordinateConstIterator class is used to iterate
///   through a pqChartCoordinateList.
class QTCHART_EXPORT pqChartCoordinateConstIterator
{
  friend class pqChartCoordinateList;
  friend class pqChartCoordinateIterator;

public:
  pqChartCoordinateConstIterator();
  pqChartCoordinateConstIterator(const pqChartCoordinateIterator &iter);
  pqChartCoordinateConstIterator(const pqChartCoordinateConstIterator &iter);
  ~pqChartCoordinateConstIterator();

  bool operator==(const pqChartCoordinateConstIterator &iter) const;
  bool operator!=(const pqChartCoordinateConstIterator &iter) const;
  bool operator==(const pqChartCoordinateIterator &iter) const;
  bool operator!=(const pqChartCoordinateIterator &iter) const;

  const pqChartCoordinate &operator*() const;
  const pqChartCoordinate *operator->() const;

  pqChartCoordinateConstIterator &operator++();
  pqChartCoordinateConstIterator operator++(int post);

  pqChartCoordinateConstIterator &operator=(
      const pqChartCoordinateConstIterator &iter);
  pqChartCoordinateConstIterator &operator=(
      const pqChartCoordinateIterator &iter);

private:
  pqChartCoordinateConstIteratorData *Data;
};


/// \class pqChartCoordinateList
/// \brief
///   The pqChartCoordinateList class contains a list of pqChartCoordinate
///   objects.
///
/// The list can be navigated using a coordinate list iterator. There
/// are iterators available for mutable and immutable lists. The
/// following example shows how to iterate through the list:
///
/// \code
/// pqChartCoordinateList::Iterator iter = list.begin();
/// for( ; iter != list.end(); ++iter)
/// {
///    ...
/// }
/// \endcode
class QTCHART_EXPORT pqChartCoordinateList
{
public:
  typedef pqChartCoordinateIterator Iterator;
  typedef pqChartCoordinateConstIterator ConstIterator;

public:
  pqChartCoordinateList();
  pqChartCoordinateList(const pqChartCoordinateList&);
  ~pqChartCoordinateList();

  /// \name List Navigation Methods
  //@{
  /// \brief
  ///   Gets an iterator to the beginning of the list.
  /// \return
  ///   An iterator to the beginning of the list.
  Iterator begin();

  /// \brief
  ///   Gets an iterator to the end of the list.
  /// \return
  ///   An iterator to the end of the list.
  Iterator end();

  /// \brief
  ///   Gets an iterator to the beginning of an immutable list.
  /// \return
  ///   An iterator to the beginning of an immutable list.
  ConstIterator begin() const;

  /// \brief
  ///   Gets an iterator to the end of an immutable list.
  /// \return
  ///   An iterator to the end of an immutable list.
  ConstIterator end() const;

  /// \brief
  ///   Gets an iterator to the beginning of an immutable list.
  /// \return
  ///   An iterator to the beginning of an immutable list.
  ConstIterator constBegin() const;

  /// \brief
  ///   Gets an iterator to the end of an immutable list.
  /// \return
  ///   An iterator to the end of an immutable list.
  ConstIterator constEnd() const;

  /// \return
  ///   True if the list is empty.
  bool isEmpty() const;

  /// \return
  ///   The number of items in the list.
  int getSize() const;
  //@}

  /// \name List Modification Methods
  //@{
  /// \brief
  ///   Clears all the coordinate items from the list.
  void clear();

  /// \brief
  ///   Appends a new coordinate item to the end of the list.
  /// \param coord The new coordinate to append.
  void pushBack(const pqChartCoordinate &coord);
  //@}

  /// \name Operators
  //@{
  /// \brief
  ///   Makes a copy of another list.
  /// \param list The list of coordinate items to copy.
  /// \return
  ///   A reference to the coordinate list.
  pqChartCoordinateList &operator=(const pqChartCoordinateList &list);

  /// \brief
  ///   Appends a list of coordinate items to this list.
  /// \param list The list of coordinate items to add.
  /// \return
  ///   A reference to the coordinate list.
  pqChartCoordinateList &operator+=(const pqChartCoordinateList &list);

  /// \brief
  ///   Gets a coordinate from the list for the given index.
  /// \param index The index of the item to retrieve.
  /// \return
  ///   The coordinate at the given index.
  pqChartCoordinate &operator[](int index);

  /// \brief
  ///   Gets a coordinate from the list for the given index.
  /// \param index The index of the item to retrieve.
  /// \return
  ///   The coordinate at the given index.
  const pqChartCoordinate &operator[](int index) const;
  //@}

private:
  pqChartCoordinateListData* const Data; ///< Stores the list of coordinates.
};

#endif
