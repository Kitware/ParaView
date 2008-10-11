/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientChartDisplay.h"
#include "ui_ClientChartDisplay.h"

#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QLabel>

#include <pqComboBoxDomain.h>
#include <pqPropertyLinks.h>
#include <pqPropertyManager.h>
#include <pqSignalAdaptors.h>
#include <pqSignalAdaptorSelectionTreeWidget.h>

#include <vtkSMProxy.h>
#include <vtkSMProperty.h>

#include <vtkLookupTable.h>

class ClientChartDisplay::implementation
{
public:
  implementation()
  {
  }

  ~implementation()
  {
    delete this->PropertyManager;
  }

  Ui::ClientChartDisplay Widgets;

  pqPropertyManager* PropertyManager;
  pqPropertyLinks Links;
};

ClientChartDisplay::ClientChartDisplay(pqRepresentation* representation, QWidget* p) :
  pqDisplayPanel(representation, p),
  Implementation(new implementation)
{
  this->Implementation->PropertyManager = new pqPropertyManager(this);

  vtkSMProxy* const proxy = vtkSMProxy::SafeDownCast(representation->getProxy());

  this->Implementation->Widgets.setupUi(this);

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.columnsAsSeries,
    "checked",
    SIGNAL(toggled(bool)),
    proxy,
    proxy->GetProperty("ColumnsAsSeries"));

  this->Implementation->Links.addPropertyLink(this->Implementation->Widgets.UseArrayIndex,
      "checked", SIGNAL(toggled(bool)), proxy, proxy->GetProperty("UseYArrayIndex"));

  pqSignalAdaptorComboBox* xAxisArrayAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.XAxisArray);
  xAxisArrayAdaptor->setObjectName("ComboBoxAdaptor");

  pqComboBoxDomain* const xAxisArrayDomain = new pqComboBoxDomain(
    this->Implementation->Widgets.XAxisArray,
    proxy->GetProperty("XAxisArrayName"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    xAxisArrayAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("XAxisArrayName"));

  proxy->GetProperty("SeriesInfo")->UpdateDependentDomains();
  pqSignalAdaptorSelectionTreeWidget* seriesAdaptor = 
    new pqSignalAdaptorSelectionTreeWidget(this->Implementation->Widgets.SeriesList, proxy->GetProperty("SeriesStatus"));
  seriesAdaptor->setObjectName("SelectionTreeWidgetAdaptor");

  this->Implementation->PropertyManager->registerLink(
    seriesAdaptor, "values", SIGNAL(valuesChanged()),
    proxy, proxy->GetProperty("SeriesStatus"));

  // Assign colors to series, using the same lookup table given to the chart
  // TODO: Store lookup table as a display property

  vtkLookupTable *lut = vtkLookupTable::New();
  lut->SetHueRange(0.0, 1.0);
  lut->SetValueRange(0.8, 0.8);
  lut->SetRange(0.0, 1.0);
  lut->Build();

  int numSeries = this->Implementation->Widgets.SeriesList->topLevelItemCount();
  for(int i=0; i<numSeries; ++i)
    {
    double seriesValue = 1;
    if (numSeries > 1)
      {
      seriesValue = static_cast<double>(i) / (numSeries-1);
      }
    if(seriesValue == 1)
      {
      seriesValue = 1 - 0.5 * (static_cast<double>(1)/numSeries);
      }
    QColor c;
    double rgb[3];
    double opacity;
    lut->GetColor(seriesValue, rgb);
    opacity = lut->GetOpacity(seriesValue);
    c.setRgbF(rgb[0], rgb[1], rgb[2], opacity);
    
    QTreeWidgetItem *item = this->Implementation->Widgets.SeriesList->topLevelItem(i);
    QPixmap pixmap(10,10);
    pixmap.fill(c);
    item->setIcon(0,QIcon(pixmap));
    }

  lut->Delete();

  this->Implementation->Links.addPropertyLink(
    seriesAdaptor,
    "values",
    SIGNAL(valuesChanged()),
    proxy,
    proxy->GetProperty("SeriesStatus"));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()));
}

ClientChartDisplay::~ClientChartDisplay()
{
  delete this->Implementation;
}

