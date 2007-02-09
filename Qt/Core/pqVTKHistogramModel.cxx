/*=========================================================================

   Program: ParaView
   Module:    pqVTKHistogramModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqVTKHistogramModel.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkSMGenericViewDisplayProxy.h"

#include <QtDebug>
#include <QList>
#include <QPointer>

#include "pqBarChartDisplay.h"
#include "pqChartCoordinate.h"
#include "pqSMAdaptor.h"
#include "pqVTKHistogramColor.h"

class pqVTKHistogramModelInternal
{
public:
  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp MTime;
  pqVTKHistogramColor ColorScheme;

  QPointer<pqBarChartDisplay> LastUsedDisplay;
  QList<QPointer<pqBarChartDisplay> > Displays;

  /// returns the display that can be viewed by this
  /// model given the current state of the displays' visibility.
  pqBarChartDisplay* getCurrentDisplay() const
    {
    foreach (pqBarChartDisplay* display, this->Displays)
      {
      if (display && display->isVisible())
        {
        return display;
        }
      }
    return NULL;
    }
};


//----------------------------------------------------------------------------
pqVTKHistogramModel::pqVTKHistogramModel(QObject *parentObject)
  : pqHistogramModel(parentObject)
{
  this->Internal = new pqVTKHistogramModelInternal();
  this->Internal->ColorScheme.setModel(this);
}

//----------------------------------------------------------------------------
pqVTKHistogramModel::~pqVTKHistogramModel()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
pqHistogramColor* pqVTKHistogramModel::getColorScheme() const
{
  return &this->Internal->ColorScheme;
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::addDisplay(pqDisplay* display)
{
  pqBarChartDisplay* cdisplay = qobject_cast<pqBarChartDisplay*>(display);
  if (cdisplay && !this->Internal->Displays.contains(cdisplay))
    {
    this->Internal->Displays.push_back(cdisplay);
    }
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::removeDisplay(pqDisplay* display)
{
  pqBarChartDisplay* cdisplay = qobject_cast<pqBarChartDisplay*>(display);
  if (cdisplay)
    {
    this->Internal->Displays.removeAll(cdisplay);
    if (cdisplay == this->Internal->LastUsedDisplay)
      {
      this->setCurrentDisplay(0);
      }
    }
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::removeAllDisplays()
{
  this->Internal->Displays.clear();
  this->Internal->MTime.Modified();
}

//----------------------------------------------------------------------------
int pqVTKHistogramModel::getNumberOfBins() const
{
  vtkDataArray* yarray = this->getYArray(this->Internal->LastUsedDisplay);
  if (yarray)
    {
    return yarray->GetNumberOfTuples();
    }
  return 0;
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getBinValue(int index, pqChartValue &bin) const
{
  vtkDataArray* yarray = this->getYArray(this->Internal->LastUsedDisplay);
  if(yarray && yarray->GetNumberOfComponents() == 1 && index >= 0 &&
    index < yarray->GetNumberOfTuples())
    {
    bin = yarray->GetTuple1(index);
    }
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.X;
  max = this->Internal->Maximum.X;
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.Y;
  max = this->Internal->Maximum.Y;
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::setCurrentDisplay(pqBarChartDisplay* display)
{
  // Update the lookup table.
  vtkSMProxy* lut = 0;
  if (display)
    {
    lut = pqSMAdaptor::getProxyProperty(
      display->getProxy()->GetProperty("LookupTable"));
    }

  this->Internal->ColorScheme.setMapIndexToColor(true);
  this->Internal->ColorScheme.setScalarsToColors(lut);

  if (this->Internal->LastUsedDisplay == display)
    {
    return;
    }

  this->Internal->LastUsedDisplay = display;
  this->Internal->MTime.Modified();
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::update()
{
  this->setCurrentDisplay(this->Internal->getCurrentDisplay());

  bool force = true; //FIXME: until we fix thses conditions to include LUT.
 
  /*
  // We try to determine if we really need to update the GUI.

  // If the model has been modified since last update.
  force |= this->Internal->MTime > this->Internal->LastUpdateTime;

  // if the display has been modified since last update.
  force |= (this->Internal->LastUsedDisplay) && 
    (this->Internal->LastUsedDisplay->getProxy()->GetMTime() > 
     this->Internal->LastUpdateTime); 

  // if the data object obtained from the display has been modified 
  // since last update.
  vtkRectilinearGrid* data = this->Internal->LastUsedDisplay?
    this->Internal->LastUsedDisplay->getClientSideData() : 0; 
  force |= (data) && (data->GetMTime() > this->Internal->LastUpdateTime);
  */
  if (force)
    {
    this->forceUpdate();
    }
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::forceUpdateEmptyData()
{
  // No data, just show empty chart.
  this->Internal->Minimum.Y = 0;
  this->Internal->Maximum.Y = 0;
  this->Internal->Minimum.X = 0;
  this->Internal->Maximum.X = 0;
  this->Internal->LastUpdateTime.Modified();
  emit this->resetBinValues();
}

//----------------------------------------------------------------------------
vtkDataArray* pqVTKHistogramModel::getYArray(pqBarChartDisplay* display) const
{
  if (!display)
    {
    return 0;
    }
  return display->getYArray();
}

//----------------------------------------------------------------------------
vtkDataArray* pqVTKHistogramModel::getXArray(pqBarChartDisplay* display) const
{
  if (!display)
    {
    return 0;
    }
  return display->getXArray();
}

//----------------------------------------------------------------------------
void pqVTKHistogramModel::forceUpdate()
{
  // ensure that the display to show hasn't changed.
  this->setCurrentDisplay(this->Internal->getCurrentDisplay());

  if (!this->Internal->LastUsedDisplay)
    {
    this->forceUpdateEmptyData();
    return;
    }

  pqBarChartDisplay* display = this->Internal->LastUsedDisplay;
  vtkRectilinearGrid* data = display? display->getClientSideData() : 0;
  if (!data)
    {
    this->forceUpdateEmptyData();
    return;
    }
  
  vtkDataArray* const xarray = this->getXArray(display);
  vtkDataArray* const yarray = this->getYArray(display);
  if (!xarray || !yarray)
    {
    qCritical() << "Failed to locate the data to plot on either axes.";
    this->forceUpdateEmptyData();
    return;
    }

  // Get the overall range for the histogram. The bin ranges are
  // stored in the x coordinate array.
  if(xarray->GetNumberOfTuples() < 2)
    {
    qWarning("The histogram range must have at least two values.");
    }

  double range[2];
  xarray->GetRange(range);
  this->Internal->Minimum.X = range[0];
  this->Internal->Maximum.X = range[1];

  yarray->GetRange(range);
  this->Internal->Minimum.Y = range[0];
  this->Internal->Maximum.Y = range[1];

  this->Internal->LastUpdateTime.Modified();
  emit this->resetBinValues();
}
