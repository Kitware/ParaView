/*=========================================================================

   Program: ParaView
   Module:    pqHistogramSelectionModel.cxx

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

/// \file pqHistogramSelectionModel.cxx
/// \date 8/17/2006

#include "pqHistogramSelectionModel.h"

#include "pqChartValue.h"
#include "pqHistogramModel.h"


pqHistogramSelectionModel::pqHistogramSelectionModel(QObject *parentObject)
  : QObject(parentObject), List()
{
  this->Type = pqHistogramSelection::None;
  this->Model = 0;
  this->PendingSignal = false;
  this->InInteractMode = false;
}

void pqHistogramSelectionModel::setModel(pqHistogramModel *model)
{
  this->Model = model;
}

void pqHistogramSelectionModel::beginInteractiveChange()
{
  this->InInteractMode = true;
}

void pqHistogramSelectionModel::endInteractiveChange()
{
  if(this->InInteractMode == true)
    {
    this->InInteractMode = false;
    emit this->interactionFinished();
    }
}

bool pqHistogramSelectionModel::hasSelection() const
{
  return this->List.size() > 0;
}

void pqHistogramSelectionModel::getRange(pqChartValue &min,
    pqChartValue &max) const
{
  if(!this->List.isEmpty())
    {
    // The total list range is the first value of the first selection
    // and the second value of the last selection.
    min = this->List.first().getFirst();
    max = this->List.last().getSecond();
    }
}

void pqHistogramSelectionModel::selectAllBins()
{
  if(this->Model && this->Model->getNumberOfBins() > 0)
    {
    pqHistogramSelection selection;
    selection.setType(pqHistogramSelection::Bin);
    selection.setRange((int)0, this->Model->getNumberOfBins() - 1);
    this->setSelection(selection);
    }
}

void pqHistogramSelectionModel::selectAllValues()
{
  if(this->Model)
    {
    pqChartValue min, max;
    pqHistogramSelection selection;
    selection.setType(pqHistogramSelection::Value);
    this->Model->getRangeX(min, max);
    selection.setRange(min, max);
    this->setSelection(selection);
    }
}

void pqHistogramSelectionModel::selectNone()
{
  if(!this->List.isEmpty())
    {
    this->clearSelections();
    emit this->selectionChanged(this->List);
    }
}

void pqHistogramSelectionModel::selectInverse()
{
  if(this->Model)
    {
    pqHistogramSelection selection;
    selection.setType(pqHistogramSelection::Bin);
    pqChartValue min = (int)0;
    pqChartValue max = this->Model->getNumberOfBins() - 1;
    if(this->Type == pqHistogramSelection::Value)
      {
      selection.setType(pqHistogramSelection::Value);
      this->Model->getRangeX(min, max);
      }
    else if(max < 0)
      {
      max = 0;
      }

    this->xorSelection(selection);
    }
}

void pqHistogramSelectionModel::setSelection(
    const pqHistogramSelectionList &list)
{
  bool hadSelection = !this->List.isEmpty();
  this->clearSelections();
  if(!list.isEmpty())
    {
    this->addSelection(list);
    }
  else if(hadSelection)
    {
    emit this->selectionChanged(this->List);
    }
}

void pqHistogramSelectionModel::setSelection(const pqHistogramSelection &range)
{
  bool hadSelection = !this->List.isEmpty();
  this->clearSelections();
  if(range.getType() != pqHistogramSelection::None ||
      this->Type == pqHistogramSelection::None ||
      this->Type == range.getType())
    {
    this->addSelection(range);
    }
  else if(hadSelection)
    {
    emit this->selectionChanged(this->List);
    }
}

void pqHistogramSelectionModel::addSelection(
    const pqHistogramSelectionList &list)
{
  if(list.isEmpty())
    {
    return;
    }

  // Make a copy of the list in order to sort and merge the ranges.
  pqHistogramSelectionList copy = list;
  pqHistogramSelectionModel::sortAndMerge(copy);

  // Unite the new selection with the current selection.
  this->blockSignals(true);
  pqHistogramSelectionList::ConstIterator iter = copy.begin();
  for( ; iter != copy.end(); ++iter)
    {
    this->addSelection(*iter);
    }

  this->blockSignals(false);
  emit this->selectionChanged(this->List);
}

void pqHistogramSelectionModel::addSelection(const pqHistogramSelection &range)
{
  if(range.getType() == pqHistogramSelection::None)
    {
    return;
    }

  // Make sure the selection type matches. If the selection type
  // hasn't been set, use the range's type.
  if(this->Type == pqHistogramSelection::None)
    {
    this->Type = range.getType();
    }
  else if(this->Type != range.getType())
    {
    return;
    }

  // Make sure the range is valid.
  pqHistogramSelection copy = range;
  this->validateRange(copy);

  // Search through the sorted list for an intersection.
  bool addItem = true;
  pqChartValue v1, v2;
  pqHistogramSelectionList::Iterator iter = this->List.begin();
  for( ; iter != this->List.end(); iter++)
    {
    v1 = iter->getFirst();
    v2 = iter->getSecond();
    if(copy.getSecond() < --v1)
      {
      this->List.insert(iter, copy);
      addItem = false;
      break;
      }
    else if(copy.getFirst() <= ++v2)
      {
      if(copy.getFirst() < iter->getFirst())
        {
        iter->setFirst(copy.getFirst());
        }
      if(copy.getSecond() > iter->getSecond())
        {
        // If the new item's right boundary is greater than the
        // current item's right boundary, the newly created union
        // could intersect with subsequent items.
        iter->setSecond(copy.getSecond());
        pqHistogramSelection next;
        pqHistogramSelection prev = *iter;
        for(iter++; iter != this->List.end(); )
          {
          v1 = iter->getFirst();
          if(prev.getSecond() < --v1)
            {
            break;
            }
          else
            {
            next = *iter;
            iter = this->List.erase(iter);
            if(prev.getSecond() <= next.getSecond())
              {
              prev.setSecond(next.getSecond());
              break;
              }
            }
          }
        }

      addItem = false;
      break;
      }
    }

  if(addItem)
    {
    this->List.append(copy);
    }

  emit this->selectionChanged(this->List);
}

void pqHistogramSelectionModel::subtractSelection(
    const pqHistogramSelectionList &list)
{
  if(list.isEmpty())
    {
    return;
    }

  // Make a copy of the list in order to sort and merge the ranges.
  pqHistogramSelectionList copy = list;
  pqHistogramSelectionModel::sortAndMerge(copy);

  // Subtract the specified selection from the current selection.
  bool changed = false;
  this->blockSignals(true);
  pqHistogramSelectionList::ConstIterator iter = copy.begin();
  for( ; iter != copy.end(); ++iter)
    {
    bool temp = this->subtractSelection(*iter);
    changed = temp || changed;
    }

  this->blockSignals(false);
  if(changed)
    {
    emit this->selectionChanged(this->List);
    }
}

bool pqHistogramSelectionModel::subtractSelection(
    const pqHistogramSelection &range)
{
  // If the list is empty, there's nothing to subtract. The range
  // must be of the same type as well.
  if(this->List.isEmpty() || range.getType() == pqHistogramSelection::None ||
    this->Type != range.getType())
    {
    return false;
    }

  // Make sure the range is valid.
  pqHistogramSelection copy = range;
  this->validateRange(copy);

  // Search through the sorted list for an intersection.
  bool changed = false;
  pqChartValue v1, v2;
  pqHistogramSelectionList::Iterator iter = this->List.begin();
  while(iter != this->List.end())
    {
    if(copy.getSecond() < iter->getFirst())
      {
      break;
      }
    else if(copy.getFirst() <= iter->getSecond())
      {
      if(copy.getSecond() <= iter->getSecond())
        {
        if(copy.getSecond() == iter->getSecond())
          {
          changed = true;
          if(copy.getFirst() > iter->getFirst())
            {
            v1 = copy.getFirst();
            iter->setSecond(--v1);
            }
          else
            {
            iter = this->List.erase(iter);
            }
          }
        else if(copy.getFirst() > iter->getFirst())
          {
          changed = true;
          v1 = copy.getFirst();
          copy.setFirst(iter->getFirst());
          v2 = copy.getSecond();
          iter->setFirst(++v2);
          copy.setSecond(--v1);
          iter = this->List.insert(iter, copy);
          }
        else
          {
          changed = true;
          v1 = copy.getSecond();
          iter->setFirst(++v1);
          }

        break;
        }
      else if(copy.getFirst() > iter->getFirst())
        {
        changed = true;
        v1 = copy.getFirst();
        v2 = iter->getSecond();
        copy.setFirst(++v2);
        iter->setSecond(--v1);
        ++iter;
        }
      else
        {
        changed = true;
        v1 = iter->getSecond();
        copy.setFirst(++v1);
        iter = this->List.erase(iter);
        }
      }
    else
      {
      ++iter;
      }
    }

  if(this->List.isEmpty())
    {
    // The selection may be empty after a subtract operation.
    this->Type = pqHistogramSelection::None;
    }

  if(changed)
    {
    emit this->selectionChanged(this->List);
    }

  return changed;
}

void pqHistogramSelectionModel::xorSelection(
    const pqHistogramSelectionList &list)
{
  if(list.isEmpty())
    {
    return;
    }

  // Make a copy of the list in order to sort and merge the ranges.
  pqHistogramSelectionList copy = list;
  pqHistogramSelectionModel::sortAndMerge(copy);

  // Xor the specified selection with the current selection.
  this->blockSignals(true);
  pqHistogramSelectionList::ConstIterator iter = copy.begin();
  for( ; iter != copy.end(); ++iter)
    {
    this->xorSelection(*iter);
    }

  this->blockSignals(false);
  emit this->selectionChanged(this->List);
}

void pqHistogramSelectionModel::xorSelection(const pqHistogramSelection &range)
{
  if(range.getType() == pqHistogramSelection::None)
    {
    return;
    }

  // Make sure the selection type matches. If the selection type
  // hasn't been set, use the range's type.
  if(this->Type == pqHistogramSelection::None)
    {
    this->Type = range.getType();
    }
  else if(this->Type != range.getType())
    {
    return;
    }

  // Make sure the range is valid.
  pqHistogramSelection copy = range;
  this->validateRange(copy);

  // Search through the sorted list for an intersection.
  bool addItem = true;
  pqChartValue v1, v2;
  pqHistogramSelectionList::Iterator iter = this->List.begin();
  for( ; iter != this->List.end(); ++iter)
    {
    v1 = iter->getFirst();
    v2 = iter->getSecond();
    if(copy.getSecond() < --v1)
      {
      this->List.insert(iter, copy);
      addItem = false;
      break;
      }
    else if(copy.getFirst() <= ++v2)
      {
      if(copy.getSecond() <= iter->getSecond())
        {
        if(copy.getSecond() == v1)
          {
          iter->setFirst(copy.getFirst());
          }
        else if(copy.getSecond() == iter->getSecond())
          {
          if(copy.getFirst() == iter->getFirst())
            {
            this->List.erase(iter);
            }
          else if(copy.getFirst() > iter->getFirst())
            {
            v1 = copy.getFirst();
            iter->setSecond(--v1);
            }
          else
            {
            v1 = iter->getFirst();
            iter->setSecond(--v1);
            iter->setFirst(copy.getFirst());
            }
          }
        else if(copy.getFirst() == iter->getFirst())
          {
          v1 = copy.getSecond();
          iter->setFirst(++v1);
          }
        else
          {
          // Move the left-most range to the new item so it can
          // be inserted before the current item.
          if(copy.getFirst() > iter->getFirst())
            {
            v1 = iter->getFirst();
            v2 = copy.getSecond();
            iter->setFirst(++v2);
            v2 = copy.getFirst();
            copy.setSecond(--v2);
            copy.setFirst(v1);
            }
          else
            {
            v1 = iter->getFirst();
            v2 = copy.getSecond();
            iter->setFirst(++v2);
            copy.setSecond(--v1);
            }

          this->List.insert(iter, copy);
          }

        addItem = false;
        break;
        }
      else
        {
        if(copy.getFirst() == v2)
          {
          iter->setSecond(copy.getSecond());
          addItem = false;
          }
        else if(copy.getFirst() == iter->getFirst())
          {
          v1 = iter->getSecond();
          iter->setFirst(++v1);
          iter->setSecond(copy.getSecond());
          addItem = false;
          }
        else if(copy.getFirst() > iter->getFirst())
          {
          v1 = iter->getSecond();
          v2 = copy.getFirst();
          iter->setSecond(--v2);
          copy.setFirst(++v1);
          }
        else
          {
          v1 = iter->getFirst();
          v2 = iter->getSecond();
          iter->setFirst(copy.getFirst());
          copy.setFirst(++v2);
          iter->setSecond(--v1);
          }

        // If the new item's right boundary is greater than the
        // current item's right boundary, the leftover range could
        // intersect with subsequent items.
        bool usingCopy = addItem;
        pqHistogramSelection *prev = &(*iter);
        if(addItem)
          {
          prev = &copy;
          }

        for(++iter; iter != this->List.end(); ++iter)
          {
          if(prev->getSecond() <= iter->getSecond())
            {
            v1 = iter->getFirst();
            if(prev->getSecond() < --v1)
              {
              if(usingCopy)
                {
                this->List.insert(iter, copy);
                addItem = false;
                }
              }
            else if(prev->getSecond() == v1)
              {
              if(usingCopy)
                {
                iter->setFirst(prev->getFirst());
                addItem = false;
                }
              else
                {
                prev->setSecond(iter->getSecond());
                this->List.erase(iter);
                }
              }
            else if(prev->getSecond() == iter->getSecond())
              {
              if(usingCopy)
                {
                v1 = iter->getFirst();
                iter->setSecond(--v1);
                iter->setFirst(prev->getFirst());
                addItem = false;
                }
              else
                {
                v1 = iter->getFirst();
                prev->setSecond(--v1);
                this->List.erase(iter);
                }
              }
            else
              {
              v1 = iter->getFirst();
              v2 = prev->getSecond();
              iter->setFirst(++v2);
              prev->setSecond(--v1);
              if(usingCopy)
                {
                this->List.insert(iter, copy);
                addItem = false;
                }
              }

            break;
            }
          else
            {
            v1 = iter->getFirst();
            v2 = iter->getSecond();
            if(usingCopy)
              {
              iter->setFirst(prev->getFirst());
              prev->setFirst(++v2);
              iter->setSecond(--v1);
              }
            else
              {
              iter->setFirst(++v2);
              iter->setSecond(prev->getSecond());
              prev->setSecond(--v1);
              prev = &(*iter);
              }
            }
          }

        break;
        }
      }
    }

  if(addItem)
    {
    this->List.append(copy);
    }
  else if(this->List.isEmpty())
    {
    // The selection may be empty after an xor operation.
    this->Type = pqHistogramSelection::None;
    }

  emit this->selectionChanged(this->List);
}

void pqHistogramSelectionModel::moveSelection(
    const pqHistogramSelection &range, const pqChartValue &offset)
{
  if(offset == 0 || range.getType() == pqHistogramSelection::None)
    {
    return;
    }

  if(this->Type != pqHistogramSelection::None && this->Type != range.getType())
    {
    return;
    }

  // Find the given selection range and remove it from the list.
  bool found = false;
  pqHistogramSelection item;
  pqHistogramSelectionList::Iterator iter = this->List.begin();
  for( ; iter != this->List.end(); ++iter)
    {
    if(iter->getFirst() == range.getFirst() &&
        iter->getSecond() == range.getSecond())
      {
      found = true;
      item = *iter;
      this->List.erase(iter);
      break;
      }
    }

  if(found)
    {
    // Add the offset to the selection range and add it back to the
    // selection list.
    item.moveRange(offset);
    this->addSelection(item);
    }
}

void pqHistogramSelectionModel::sortAndMerge(pqHistogramSelectionList &list)
{
  if(list.size() < 2)
    {
    return;
    }

  // Use a temporary list to sort the items according to the
  // first value of each selection range.
  pqHistogramSelection::SelectionType listType = pqHistogramSelection::None;
  pqHistogramSelectionList temp;
  pqHistogramSelectionList::Iterator iter = list.begin();
  pqHistogramSelectionList::Iterator jter;
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the selection range is in order.
    if(iter->getSecond() < iter->getFirst())
      {
      iter->reverse();
      }

    if(listType == pqHistogramSelection::None)
      {
      listType = iter->getType();
      }
    else if(listType != iter->getType())
      {
      continue;
      }

    for(jter = temp.begin(); jter != temp.end(); ++jter)
      {
      if(iter->getFirst() < jter->getFirst())
        {
        temp.insert(jter, *iter);
        break;
        }
      }

    if(jter == temp.end())
      {
      temp.append(*iter);
      }
    }

  // Put the items back on the list while uniting overlapping selection
  // ranges.
  list.clear();
  iter = temp.begin();
  if(iter != temp.end())
    {
    pqChartValue v;
    list.append(*iter);
    pqHistogramSelection *prev = &list.last();
    for(++iter; iter != temp.end(); ++iter)
      {
      v = iter->getFirst();
      if(--v <= prev->getSecond())
        {
        if(prev->getSecond() < iter->getSecond())
          {
          prev->setSecond(iter->getSecond());
          }
        }
      else
        {
        list.append(*iter);
        prev = &list.last();
        }
      }
    }
}

void pqHistogramSelectionModel::beginModelReset()
{
  if(this->List.size() > 0)
    {
    // Reset the selection, but let the chart finish the layout before
    // sending the selection changed signal.
    this->clearSelections();
    this->PendingSignal = true;
    }
}

void pqHistogramSelectionModel::endModelReset()
{
  if(this->PendingSignal)
    {
    this->PendingSignal = false;
    emit this->selectionChanged(this->List);
    }
}

void pqHistogramSelectionModel::beginInsertBinValues(int first, int last)
{
  if(this->Type == pqHistogramSelection::Bin)
    {
    // Adjust the selected indexes to account for the change. Don't
    // signal the change until the insert is complete.
    pqChartValue offset = last - first + 1;
    pqHistogramSelectionList::Iterator iter = this->List.begin();
    for( ; iter != this->List.end(); ++iter)
      {
      if(iter->getFirst() >= first)
        {
        iter->moveRange(offset);
        this->PendingSignal = true;
        }
      else if(iter->getSecond() >= first)
        {
        pqChartValue value = iter->getSecond();
        value += offset;
        iter->setSecond(value);
        this->PendingSignal = true;
        }
      }
    }
}

void pqHistogramSelectionModel::endInsertBinValues()
{
  if(this->Type == pqHistogramSelection::Bin && this->PendingSignal)
    {
    // Send the selection change signal.
    this->PendingSignal = false;
    emit this->selectionChanged(this->List);
    }
}

void pqHistogramSelectionModel::beginRemoveBinValues(int first, int last)
{
  if(this->Type == pqHistogramSelection::Bin)
    {
    // Adjust the selected indexes to account for the change. Don't
    // signal the change until the remove is complete.
    pqChartValue offset = first - last - 1;
    pqHistogramSelectionList::Iterator iter = this->List.begin();
    for( ; iter != this->List.end(); ++iter)
      {
      if(iter->getFirst() >= first)
        {
        iter->moveRange(offset);
        this->PendingSignal = true;
        }
      else if(iter->getSecond() >= first)
        {
        pqChartValue value = iter->getSecond();
        value += offset;
        iter->setSecond(value);
        this->PendingSignal = true;
        }
      }
    }
}

void pqHistogramSelectionModel::endRemoveBinValues()
{
  if(this->Type == pqHistogramSelection::Bin && this->PendingSignal)
    {
    // Send the selection change signal.
    this->PendingSignal = false;
    emit this->selectionChanged(this->List);
    }
}

void pqHistogramSelectionModel::beginRangeChange(const pqChartValue &min,
    const pqChartValue &max)
{
  // This is only relevant when using value selection mode.
  if(this->Type == pqHistogramSelection::Value && !this->List.isEmpty())
    {
    // Make sure the selection is within the new range. Don't send the
    // selection changed signal until the range change is complete.
    this->blockSignals(true);
    bool changed = false;
    pqChartValue currentMin = this->List.first().getFirst();
    pqChartValue currentMax = this->List.last().getSecond();
    if(currentMin < min)
      {
      pqChartValue newMin = min;
      pqHistogramSelection left(currentMin, --newMin);
      left.setType(this->Type);
      changed = this->subtractSelection(left);
      }

    if(currentMax > max)
      {
      pqChartValue newMax = max;
      pqHistogramSelection right(++newMax, currentMax);
      right.setType(this->Type);
      bool temp = this->subtractSelection(right);
      changed = temp || changed;
      }

    this->blockSignals(false);
    this->PendingSignal = changed;
    }
}

void pqHistogramSelectionModel::endRangeChange()
{
  if(this->Type == pqHistogramSelection::Value && this->PendingSignal)
    {
    // Send the selection change signal.
    this->PendingSignal = false;
    emit this->selectionChanged(this->List);
    }
}

void pqHistogramSelectionModel::validateRange(pqHistogramSelection &range)
{
  // The first range value should be less than or equal to the second
  // value.
  if(range.getSecond() < range.getFirst())
    {
    range.reverse();
    }

  // The range must also lie within the model's bounds.
  if(this->Model)
    {
    pqChartValue min = (int)0;
    pqChartValue max = this->Model->getNumberOfBins() - 1;
    if(range.getType() == pqHistogramSelection::Value)
      {
      this->Model->getRangeX(min, max);
      }
    else if(max < 0)
      {
      max = 0;
      }

    range.adjustRange(min, max);
    }
}

void pqHistogramSelectionModel::clearSelections()
{
  this->List.clear();
  this->Type = pqHistogramSelection::None;
}


