/*=========================================================================

   Program: ParaView
   Module:    pqHistogramSelection.cxx

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
 * \file pqHistogramSelection.cxx
 *
 * \brief
 *   The pqHistogramSelection and pqHistogramSelectionList classes are
 *   used to define the selection for a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   June 2, 2005
 */

#include "pqHistogramSelection.h"


pqHistogramSelection::pqHistogramSelection()
  : First(), Second()
{
  this->Type = pqHistogramSelection::Value;
}

pqHistogramSelection::pqHistogramSelection(const pqHistogramSelection &other)
  : First(other.First), Second(other.Second)
{
  this->Type = other.Type;
}

pqHistogramSelection::pqHistogramSelection(const pqChartValue &first,
    const pqChartValue &second)
  : First(first), Second(second)
{
  this->Type = pqHistogramSelection::Value;
}

void pqHistogramSelection::reverse()
{
  pqChartValue temp = this->First;
  this->First = this->Second;
  this->Second = temp;
}

void pqHistogramSelection::adjustRange(const pqChartValue &min,
    const pqChartValue &max)
{
  if(this->First < min)
    {
    this->First = min;
    }
  else if(this->First > max)
    {
    this->First = max;
    }
  if(this->Second < min)
    {
    this->Second = min;
    }
  else if(this->Second > max)
    {
    this->Second = max;
    }
}

void pqHistogramSelection::moveRange(const pqChartValue &offset)
{
  this->First += offset;
  this->Second += offset;
}

void pqHistogramSelection::setRange(const pqChartValue &first,
    const pqChartValue &last)
{
  this->First = first;
  this->Second = last;
}

pqHistogramSelection &pqHistogramSelection::operator=(
    const pqHistogramSelection &other)
{
  this->Type = other.Type;
  this->First = other.First;
  this->Second = other.Second;
  return *this;
}

bool pqHistogramSelection::operator==(const pqHistogramSelection &other) const
{
  return this->Type == other.Type && this->First == other.First &&
      this->Second == other.Second;
}


