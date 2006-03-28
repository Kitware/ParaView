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
 * \file pqHistogramSelection.h
 *
 * \brief
 *   The pqHistogramSelection and pqHistogramSelectionList classes are
 *   used to define the selection for a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   June 2, 2005
 */

#ifndef _pqHistogramSelection_h
#define _pqHistogramSelection_h

#include "QtChartExport.h"
#include "pqChartValue.h" // Needed for first and second members.

class pqHistogramSelectionListData;
class pqHistogramSelectionIteratorData;
class pqHistogramSelectionConstIteratorData;


/// \class pqHistogramSelection
/// \brief
///   The pqHistogramSelection class is used to store a selection range.
///
/// The selection range includes the first and last values. The
/// selection type is used to distinguish between bin indexes and
/// integer value types.
class QTCHART_EXPORT pqHistogramSelection
{
  friend class pqHistogramSelectionList;

public:
  enum SelectionType {
    None,
    Value,
    Bin
  };

public:
  /// Creates an empty selection object.
  pqHistogramSelection();

  /// \brief
  ///   Creates a copy of another selection object.
  /// \param other The selection to copy.
  pqHistogramSelection(const pqHistogramSelection &other);

  /// \brief
  ///   Creates a selection object with a given range.
  ///
  /// The \c first and \c second values will be included in the
  /// selection range.
  ///
  /// \param first The beginning of the selection range.
  /// \param second The end of the selection range.
  pqHistogramSelection(const pqChartValue &first, const pqChartValue &second);
  virtual ~pqHistogramSelection() {}

  /// \brief
  ///   Sets the selection type.
  /// \param type The new selection type.
  void setType(SelectionType type) {this->Type = type;}

  /// \brief
  ///   Gets the selection type.
  /// \return
  ///   The selection type.
  SelectionType getType() const {return this->Type;}

  /// Switches the direction of the selection range.
  void reverse();

  /// \brief
  ///   Adjusts the selection range to be within the limits.
  /// \param min The minimum selection value allowed.
  /// \param max The maximum selection value allowed.
  void adjustRange(const pqChartValue &min, const pqChartValue &max);

  /// \brief
  ///   Adds the offset to both ends of the range.
  /// \param offset The amount to move the selection range.
  void moveRange(const pqChartValue &offset);

  /// \brief
  ///   Sets the selection range.
  ///
  /// The \c first and \c last values will be included in the
  /// selection range.
  ///
  /// \param first The beginning of the selection range.
  /// \param last The end of the selection range.
  /// \sa pqHistogramSelection::setFirst(const pqChartValue &),
  ///     pqHistogramSelection::setSecond(const pqChartValue &)
  void setRange(const pqChartValue &first, const pqChartValue &last);

  /// \brief
  ///   Sets the beginning of the selection range.
  /// \param first The beginning of the selection range.
  /// \sa pqHistogramSelection::setRange(const pqChartValue &,
  ///       const pqChartValue &),
  ///     pqHistogramSelection::setSecond(const pqChartValue &)
  void setFirst(const pqChartValue &first) {this->First = first;}

  /// \brief
  ///   Sets the end of the selection range.
  /// \param second The end of the selection range.
  /// \sa pqHistogramSelection::setRange(const pqChartValue &,
  ///       const pqChartValue &),
  ///     pqHistogramSelection::setSecond(const pqChartValue &)
  void setSecond(const pqChartValue &second) {this->Second = second;}

  /// \brief
  ///   Gets the beginning of the selection range.
  /// \return
  ///   The beginning of the selection range.
  const pqChartValue &getFirst() const {return this->First;}

  /// \brief
  ///   Gets the end of the selection range.
  /// \return
  ///   The end of the selection range.
  const pqChartValue &getSecond() const {return this->Second;}

  /// \brief
  ///   Makes a copy of another selection.
  /// \param other The selection range to copy.
  /// \return
  ///   A reference the object being assigned.
  pqHistogramSelection &operator=(const pqHistogramSelection &other);

private:
  SelectionType Type;  ///< Stores the type of selection.
  pqChartValue First;  ///< Stores the beginning of the range.
  pqChartValue Second; ///< Stores the end of the range.
};


class pqHistogramSelectionConstIterator;

/// \class pqHistogramSelectionIterator
/// \brief
///   The pqHistogramSelectionIterator class is used to iterate through
///   a pqHistogramSelectionList.
class QTCHART_EXPORT pqHistogramSelectionIterator
{
  friend class pqHistogramSelectionList;
  friend class pqHistogramSelectionConstIterator;

public:
  pqHistogramSelectionIterator();
  pqHistogramSelectionIterator(const pqHistogramSelectionIterator &iter);
  ~pqHistogramSelectionIterator();

  bool operator==(const pqHistogramSelectionIterator &iter) const;
  bool operator!=(const pqHistogramSelectionIterator &iter) const;
  bool operator==(const pqHistogramSelectionConstIterator &iter) const;
  bool operator!=(const pqHistogramSelectionConstIterator &iter) const;

  const pqHistogramSelection *operator*() const;
  pqHistogramSelection *&operator*();
  pqHistogramSelection *operator->();

  pqHistogramSelectionIterator &operator++();
  pqHistogramSelectionIterator operator++(int post);

  pqHistogramSelectionIterator &operator=(
      const pqHistogramSelectionIterator &iter);

private:
  pqHistogramSelectionIteratorData *Data;
};


/// \class pqHistogramSelectionConstIterator
/// \brief
///   The pqHistogramSelectionConstIterator class is used to iterate
///   through a pqHistogramSelectionList.
class QTCHART_EXPORT pqHistogramSelectionConstIterator
{
  friend class pqHistogramSelectionList;
  friend class pqHistogramSelectionIterator;

public:
  pqHistogramSelectionConstIterator();
  pqHistogramSelectionConstIterator(const pqHistogramSelectionIterator &iter);
  pqHistogramSelectionConstIterator(
      const pqHistogramSelectionConstIterator &iter);
  ~pqHistogramSelectionConstIterator();

  bool operator==(const pqHistogramSelectionConstIterator &iter) const;
  bool operator!=(const pqHistogramSelectionConstIterator &iter) const;
  bool operator==(const pqHistogramSelectionIterator &iter) const;
  bool operator!=(const pqHistogramSelectionIterator &iter) const;

  const pqHistogramSelection *operator*() const;
  const pqHistogramSelection *operator->() const;

  pqHistogramSelectionConstIterator &operator++();
  pqHistogramSelectionConstIterator operator++(int post);

  pqHistogramSelectionConstIterator &operator=(
      const pqHistogramSelectionConstIterator &iter);
  pqHistogramSelectionConstIterator &operator=(
      const pqHistogramSelectionIterator &iter);

private:
  pqHistogramSelectionConstIteratorData *Data;
};

/*!
 *  \class pqHistogramSelectionList
 *  \brief
 *    The pqHistogramSelectionList class contains a list of
 *    pqHistogramSelection objects.
 * 
 *  The pqHistogramSelectionList can be used as a simple list or to
 *  perform boolean operations on a list of selection ranges. To
 *  create a simple list, use the \c pushBack method to add items
 *  to the list. The \c unite, \c subtract, and \c Xor methods can
 *  be used to perform the boolean operations.
 * 
 *  The list can be navigated using a selection list iterator. There
 *  are iterators available for mutable and immutable lists. The
 *  following example shows how to iterate through the list:
 * 
 *  \code
 *  pqHistogramSelectionList::Iterator iter = list.begin();
 *  for( ; iter != list.end(); ++iter)
 *  {
 *     ...
 *  }
 *  \endcode
 * 
 *  The pqHistogramSelectionList class does not take responsibility
 *  for any selection object memory. The memory allocated for the
 *  selection items must be deleted by the list user before clearing
 *  the list. All of the boolean operators have a parameter used to
 *  return items that are no longer used in the list.
 * 
 *  \sa pqHistogramSelectionList::pushBack(pqHistogramSelection *),
 *      pqHistogramSelectionList::unite(pqHistogramSelection *,
 *          pqHistogramSelectionList &),
 *      pqHistogramSelectionList::subtract(pqHistogramSelection *,
 *          pqHistogramSelectionList &),
 *      pqHistogramSelectionList::Xor(pqHistogramSelection *,
 *          pqHistogramSelectionList &)
 */
class QTCHART_EXPORT pqHistogramSelectionList
{
public:
  typedef pqHistogramSelectionIterator Iterator;
  typedef pqHistogramSelectionConstIterator ConstIterator;

public:
  /// Creates a new selection list.
  pqHistogramSelectionList();

  /// \brief
  ///   Creates a copy of another selection list.
  ///
  /// This constructor does not allocate memory for the selection
  /// items. The new list shares the memory with the old list. To
  /// make a separate copy of the selection items, use the
  /// pqHistogramSelectionList::makeNewCopy method.
  pqHistogramSelectionList(const pqHistogramSelectionList &list);
  ~pqHistogramSelectionList();

  /// \brief
  ///   Makes a new copy of the specified list.
  /// \param list The list to copy.
  void makeNewCopy(const pqHistogramSelectionList &list);

  /// \brief
  ///   Sets the selection type for the list.
  ///
  /// If the selection type is not set explicitly, it will be set
  /// during the \c sortAndMerge call based on the first selection
  /// item. If \c sortAndMerge has already been called, it must be
  /// called again to change the selection type.
  ///
  /// \param type The selection type.
  /// \sa pqHistogramSelectionList::sortAndMerge(pqHistogramSelectionList &)
  void setType(pqHistogramSelection::SelectionType type) {this->Type = type;}

  /// \brief
  ///   Gets the selection type for the list.
  /// \return
  ///   The type of selections on the list.
  pqHistogramSelection::SelectionType getType() const {return this->Type;}

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
  ///   Clears all the selection items from the list.
  /// \note
  ///   This method does not delete any memory allocated for the
  ///   selection objects.
  void clear();

  /// \brief
  ///   Removes the item at \c position from the list.
  /// \param position An iterator that points to the item to remove.
  /// \return
  ///   An iterator pointing to the item after the one removed.
  Iterator erase(Iterator position);

  /// \brief
  ///   Appends a new selection item to the end of the list.
  ///
  /// Calling this method also changes the sorted state of the list.
  /// The \c sortAndMerge method will need to be called again after
  /// adding an item with this method.
  ///
  /// \param selection The new selection to append.
  /// \sa pqHistogramSelectionList::unite(pqHistogramSelection *,
  ///         pqHistogramSelectionList &),
  ///     pqHistogramSelectionList::sortAndMerge(pqHistogramSelectionList &)
  void pushBack(pqHistogramSelection *selection);

  /// \brief
  ///   Used to sort and merge the selection ranges on the list.
  ///
  /// The selection ranges are sorted according to their first
  /// value. If the selection type has not been set, the first
  /// selection item will be used to determine the type. Items of
  /// the wrong type will be removed from the list and placed on
  /// the \c toDelete list. After the items are sorted, overlapping
  /// items are merged together. The merging process can consolidate
  /// items and leave unused ones. The unused items are placed on
  /// the \c toDelete list.
  ///
  /// \param toDelete Used to return unused or invalid items.
  void sortAndMerge(pqHistogramSelectionList &toDelete);

  /// \brief
  ///   Unites a selection range with the list.
  ///
  /// If the list has not been sorted, \c sortAndMerge will be called.
  /// The new item is added to the list and united with overlapping
  /// ranges.
  ///
  /// \param item The selection item to unite with the list.
  /// \param toDelete Used to return unused or invalid items.
  /// \sa pqHistogramSelectionList::sortAndMerge(pqHistogramSelectionList &)
  void unite(pqHistogramSelection *item, pqHistogramSelectionList &toDelete);

  /// \brief
  ///   Unites a list of selection ranges with the list.
  /// \param list The selection items to unite with the list.
  /// \param toDelete Used to return unused or invalid items.
  /// \sa pqHistogramSelectionList::unite(pqHistogramSelection *,
  ///         pqHistogramSelectionList &)
  void unite(pqHistogramSelectionList &list,
      pqHistogramSelectionList &toDelete);

  /// \brief
  ///   Subtracts a selection range from the list.
  ///
  /// If the list has not been sorted, \c sortAndMerge will be called.
  /// The new item is used to subtract from overlapping ranges in the
  /// list. Any unused items created in the process will be put on
  /// the \c toDelete list.
  ///
  /// \param item The selection item to subtract from the list.
  /// \param toDelete Used to return unused or invalid items.
  /// \sa pqHistogramSelectionList::sortAndMerge(pqHistogramSelectionList &)
  void subtract(pqHistogramSelection *item,
      pqHistogramSelectionList &toDelete);

  /// \brief
  ///   Subtracts a list of selection ranges from the list.
  /// \param list The selection items to subtract from the list.
  /// \param toDelete Used to return unused or invalid items.
  /// \sa pqHistogramSelectionList::subtract(pqHistogramSelection *,
  ///         pqHistogramSelectionList &)
  void subtract(pqHistogramSelectionList &list,
      pqHistogramSelectionList &toDelete);

  /// \brief
  ///   Performs an exclusive or for a selection range and the list.
  ///
  /// If the list has not been sorted, \c sortAndMerge will be called.
  /// An exclusive or is performed with the new item and all the
  /// overlapping items in the list. Any unused items created in the
  /// process will be put on the \c toDelete list.
  ///
  /// \param item The selection item to Xor with the list.
  /// \param toDelete Used to return unused or invalid items.
  /// \sa pqHistogramSelectionList::sortAndMerge(pqHistogramSelectionList &)
  void Xor(pqHistogramSelection *item, pqHistogramSelectionList &toDelete);

  /// \brief
  ///   Performs an exclusive or for a list of selection ranges
  ///   and the list.
  ///
  /// Each item in the supplied list will be Xored one at a time in
  /// the list order. This order may be important if there are
  /// overlapping items in the supplied list.
  ///
  /// \param list The selection items to Xor with the list.
  /// \param toDelete Used to return unused or invalid items.
  /// \sa pqHistogramSelectionList::Xor(pqHistogramSelection *,
  ///         pqHistogramSelectionList &)
  void Xor(pqHistogramSelectionList &list, pqHistogramSelectionList &toDelete);
  //@}

  /// \name Operators
  //@{
  /// \brief
  ///   Makes a copy of another list.
  ///
  /// Like the copy constructor, this operator does not allocate new
  /// memory for the selection items.
  ///
  /// \param list The list of selection items to copy.
  /// \return
  ///   A reference to the selection list.
  /// \sa pqHistogramSelectionList::makeNewCopy(
  ///         const pqHistogramSelectionList &)
  pqHistogramSelectionList &operator=(const pqHistogramSelectionList &list);

  /// \brief
  ///   Appends a list of selection items to this list.
  /// \param list The list of selection items to add.
  /// \return
  ///   A reference to the selection list.
  pqHistogramSelectionList &operator+=(const pqHistogramSelectionList &list);
  //@}

private:
  /// \brief
  ///   Used to prepare a selection item for a boolean operation.
  ///
  /// If the item is null or the list is null, false is returned
  /// immediately. Otherwise, the list is sorted to make the final
  /// checks. If the item's type is not the same as the list's type,
  /// false is returned. If the item is not null, but it is invalid,
  /// it will be placed on the deletion list. If the item passes all
  /// the tests, it will be reversed if needed, and the return value
  /// will be true.
  ///
  /// \param item The selection item to process.
  /// \param toDelete The deletion list in case the item is invalid.
  /// \return
  ///   True if the selection item is valid.
  bool preprocessItem(pqHistogramSelection *item,
      pqHistogramSelectionList &toDelete);

private:
  pqHistogramSelectionListData *Data;       ///< Stores the selection list.
  pqHistogramSelection::SelectionType Type; ///< Stores the list type.
  bool Sorted;                              ///< True if the list is sorted.
};

#endif
