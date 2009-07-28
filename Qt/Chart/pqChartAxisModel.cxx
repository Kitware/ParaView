/*=========================================================================

   Program: ParaView
   Module:    pqChartAxisModel.cxx

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

/// \file pqChartAxisModel.cxx
/// \date 12/1/2006

#include "pqChartAxisModel.h"

#include "pqChartValue.h"
#include <QList>


class pqChartAxisModelInternal : public QList<pqChartValue> {};


//----------------------------------------------------------------------------
pqChartAxisModel::pqChartAxisModel(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqChartAxisModelInternal();
  this->InModify = false;
}

pqChartAxisModel::~pqChartAxisModel()
{
  delete this->Internal;
}

void pqChartAxisModel::addLabel(const pqChartValue &label)
{
  // Find the insertion point.
  int index = 0;
  QList<pqChartValue>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter, ++index)
    {
    if(*iter == label)
      {
      // Ignore duplicate entries.
      return;
      }

    if(label < *iter)
      {
      break;
      }
    }

  // Add the label to the list.
  if(iter == this->Internal->end())
    {
    this->Internal->append(label);
    }
  else
    {
    this->Internal->insert(iter, label);
    }

  if(!this->InModify)
    {
    emit this->labelInserted(index);
    }
}

void pqChartAxisModel::removeLabel(int index)
{
  if(index >= 0 && index < this->Internal->size())
    {
    if(!this->InModify)
      {
      emit this->removingLabel(index);
      }

    this->Internal->removeAt(index);
    if(!this->InModify)
      {
      emit this->labelRemoved(index);
      }
    }
}

void pqChartAxisModel::removeAllLabels()
{
  if(this->Internal->size() > 0)
    {
    this->Internal->clear();
    if(!this->InModify)
      {
      emit this->labelsReset();
      }
    }
}

void pqChartAxisModel::startModifyingData()
{
  this->InModify = true;
}

void pqChartAxisModel::finishModifyingData()
{
  if(this->InModify)
    {
    this->InModify = false;
    emit this->labelsReset();
    }
}

int pqChartAxisModel::getNumberOfLabels() const
{
  return this->Internal->size();
}

void pqChartAxisModel::getLabel(int index, pqChartValue &label) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    label = (*this->Internal)[index];
    }
}


