/*=========================================================================

   Program: ParaView
   Module:    pqChartLayer.cxx

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

/// \file pqChartLayer.cxx
/// \date 11/7/2006

#include "pqChartLayer.h"

#include "pqChartArea.h"
#include "pqChartAxis.h"
#include "pqChartContentsSpace.h"
#include "pqChartValue.h"

#include <QPainter>
#include <QRect>


pqChartLayer::pqChartLayer(QObject *parentObject)
  : QObject(parentObject)
{
  this->ChartArea = 0;
}

bool pqChartLayer::getAxisRange(const pqChartAxis *, pqChartValue &,
    pqChartValue &, bool &, bool &) const
{
  return false;
}

bool pqChartLayer::isAxisControlPreferred(const pqChartAxis *) const
{
  return false;
}

void pqChartLayer::generateAxisLabels(pqChartAxis *)
{
}

void pqChartLayer::drawBackground(QPainter &, const QRect &)
{
}

pqChartContentsSpace *pqChartLayer::getContentsSpace() const
{
  if(this->ChartArea)
    {
    return this->ChartArea->getContentsSpace();
    }

  return 0;
}


