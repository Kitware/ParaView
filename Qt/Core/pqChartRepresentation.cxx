/*=========================================================================

   Program: ParaView
   Module:    pqChartRepresentation.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqChartRepresentation.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QString>
#include <QStringList>
#include <QRegExp>

//-----------------------------------------------------------------------------
pqChartRepresentation::pqChartRepresentation(
  const QString& group, const QString& name,
  vtkSMProxy* reprProxy, pqServer* server,
  QObject* parentObject)
: Superclass(group, name, reprProxy, server, parentObject)
{
}

//-----------------------------------------------------------------------------
pqChartRepresentation::~pqChartRepresentation()
{
}

//-----------------------------------------------------------------------------
void pqChartRepresentation::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();
  if (!this->isVisible())
    {
    return;
    }

  // Set default arrays and lookup table.
  vtkSMRepresentationProxy* proxy = vtkSMRepresentationProxy::SafeDownCast(
    this->getProxy());

  // This update is needed since when resetting default property values, the
  // superclass may have changed some property such as the "composite_index"
  // which requires the representation to be re-updated to get correct data
  // information.
  proxy->UpdatePipeline();
  proxy->UpdatePropertyInformation();

  // * Determine the x-axis array: are we using index or do we have some array
  //   that suits out purpose better?
  QList<QVariant> series_arrays = pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty("SeriesNamesInfo"));

  QString x_array;
  QString y_array;
  if (series_arrays.contains("bin_values"))
    {
    // typically implies that the output is from a histogram filter.
    x_array = series_arrays[0].toString();
    y_array = "bin_values";
    }
  else if (series_arrays.contains(QVariant("Time")))
    {
    x_array = "Time";
    }
  else if (series_arrays.contains(QVariant("arc_length")))
    {
    x_array = "arc_length";
    }


  if (!x_array.isEmpty())
    {
    vtkSMPropertyHelper(proxy, "XArrayName").Set(x_array.toAscii().data());
    vtkSMPropertyHelper(proxy, "UseIndexForXAxis").Set(0);
    }

  // * Turn off Y-series that don't make much sense to show by default
  if (proxy->GetProperty("SeriesVisibility"))
    {
    vtkSMPropertyHelper helper(proxy, "SeriesVisibility");
    foreach (QVariant varray, series_arrays)
      {
      QString array = varray.toString();
      if (!y_array.isNull() && (array != y_array))
        {
        // when y_array is set, the only visible array is the y-array.
        helper.SetStatus(array.toAscii().data(), 0);
        }
      else if (!x_array.isNull() && (array == x_array))
        {
        // No point in plotting the series used as the x-array.
        helper.SetStatus(array.toAscii().data(), 0);
        }
      else if (this->queryHideSeries(array))
        {
        helper.SetStatus(array.toAscii().data(), 0);
        }
      }
    if (!y_array.isEmpty())
      {
      helper.SetStatus(y_array.toAscii().data(), 1);
      }
    }

  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqChartRepresentation::setHiddenSeriesSetting(QStringList list)
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  settings->setValue("Charting/HiddenSeries", list);
}

QStringList pqChartRepresentation::getHiddenSeriesSetting()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  QVariant hiddenSeries = settings->value("Charting/HiddenSeries",
                           pqChartRepresentation::defaultHiddenSeriesSetting());
  return hiddenSeries.toStringList();
}

QStringList pqChartRepresentation::defaultHiddenSeriesSetting()
{
  QStringList hiddenSeries;

  // Arrays of special meaning that typically are not plotted.
  hiddenSeries << "Time";
  hiddenSeries << "arc_length";
  hiddenSeries << "Points.*";
  hiddenSeries << "vtkValidPointMask";
  hiddenSeries << "Pedigree.*";

  // Typical arrays of types that have no need to be plotted.
  hiddenSeries << "ObjectId";
  hiddenSeries << "FileId";

  // Show only the vector magnitudes.  Hide the components.
  hiddenSeries << ".*\\(\\d+\\)";

  // Hide vtkOriginalRowIds.
  hiddenSeries << "vtkOriginalRowIds";
  hiddenSeries << "vtkOriginalIndices";

  return hiddenSeries;
}

//-----------------------------------------------------------------------------
bool pqChartRepresentation::queryHideSeries(QString array)
{
  foreach (QString hideExpr, pqChartRepresentation::getHiddenSeriesSetting())
    {
    if (QRegExp(hideExpr).exactMatch(array)) return true;
    }

  return false;
}
