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

#include "QtChartExport.h"
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

/*!
 *  \class pqChartCoordinateList
 *  \brief
 *    The pqChartCoordinateList class contains a list of pqChartCoordinate
 *    objects.
 * 
 *  The list can be navigated using a coordinate list iterator. There
 *  are iterators available for mutable and immutable lists. The
 *  following example shows how to iterate through the list:
 * 
 *  \code
 *  pqChartCoordinateList::Iterator iter = list.begin();
 *  for( ; iter != list.end(); ++iter)
 *  {
 *     ...
 *  }
 *  \endcode
 */
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
