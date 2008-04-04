/*=========================================================================

   Program: ParaView
   Module:    pqVTKHistogramColor.cxx

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
#include "pqVTKHistogramColor.h"

#include "vtkProcessModule.h"
#include "vtkRectilinearGrid.h"
#include "vtkScalarsToColors.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <QColor>
#include <QPointer>
#include <QtDebug>

#include "pqChartValue.h"
#include "pqHistogramModel.h"

//-----------------------------------------------------------------------------
class pqVTKHistogramColor::pqInternals
{
public:
  vtkSmartPointer<vtkScalarsToColors> ScalarToColors;
  QPointer<pqHistogramModel> Model;
};

//-----------------------------------------------------------------------------
pqVTKHistogramColor::pqVTKHistogramColor()
{
  this->Internals = new pqInternals();
  this->MapIndexToColor = false;
}

//-----------------------------------------------------------------------------
pqVTKHistogramColor::~pqVTKHistogramColor()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
QColor pqVTKHistogramColor::getColor(int index, int total) const
{
  QColor color = Qt::gray;

  if (this->Internals->Model && this->Internals->ScalarToColors)
    {
    if (this->MapIndexToColor)
      {
      pqChartValue cvmin, cvmax;
      this->Internals->Model->getRangeX(cvmin, cvmax);
      double min = cvmin.getDoubleValue();
      double max = cvmax.getDoubleValue();
      double value = min + index*(max-min)/total;
      unsigned char* ucolor = 
        this->Internals->ScalarToColors->MapValue(value);
      color.setRgb(ucolor[0], ucolor[1], ucolor[2], ucolor[3]);
      }
    else
      {
      pqChartValue value;
      this->Internals->Model->getBinValue(index, value);
      unsigned char* ucolor = 
        this->Internals->ScalarToColors->MapValue(value.getDoubleValue());
      color.setRgb(ucolor[0], ucolor[1], ucolor[2], ucolor[3]);
      }
    }

  return color;
}

//-----------------------------------------------------------------------------
void pqVTKHistogramColor::setModel(pqHistogramModel* model)
{
  this->Internals->Model = model;
}

//-----------------------------------------------------------------------------
void pqVTKHistogramColor::setScalarsToColors(vtkSMProxy* lut)
{
  if (!lut)
    {
    this->Internals->ScalarToColors = 0;
    return;
    }

  // Get the client side stc and save it.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  this->Internals->ScalarToColors = 
    vtkScalarsToColors::SafeDownCast(pm->GetObjectFromID(lut->GetID()));
  if (this->Internals->ScalarToColors)
    {
    // This ensures that the LUT is up-to-date.
    this->Internals->ScalarToColors->Build();
    }
}

