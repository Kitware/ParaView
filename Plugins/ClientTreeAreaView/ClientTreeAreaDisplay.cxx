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

#include "ClientTreeAreaDisplay.h"
#include "ui_ClientTreeAreaDisplay.h"

#include "IconDialog.h"

#include <pqApplicationCore.h>
#include <pqComboBoxDomain.h>
#include <pqPipelineSource.h>
#include <pqPluginManager.h>
#include <pqPropertyHelper.h>
#include <pqPropertyLinks.h>
#include <pqServerManagerModel.h>
#include <pqSignalAdaptors.h>
#include <pqTreeLayoutStrategyInterface.h>

#include <vtkCommand.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSmartPointer.h>

#include <iostream>

class ClientTreeAreaDisplay::implementation
{
public:
  implementation() :
    AreaColorAdaptor(0),
    EdgeColorAdaptor(0)
  {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  }

  ~implementation()
  {
    delete this->EdgeColorAdaptor;
    delete this->AreaColorAdaptor;
  }

  Ui::ClientTreeAreaDisplay Widgets;

  pqPropertyLinks Links;
  pqSignalAdaptorColor* AreaColorAdaptor;
  pqSignalAdaptorColor* EdgeColorAdaptor;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

ClientTreeAreaDisplay::ClientTreeAreaDisplay(pqRepresentation* representation, QWidget* p) :
  pqDisplayPanel(representation, p),
  Implementation(new implementation())
{
  vtkSMProxy* const proxy = vtkSMProxy::SafeDownCast(representation->getProxy());

  this->Implementation->Widgets.setupUi(this);

  this->Implementation->AreaColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->Widgets.areaColor,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)),
    false);

  this->Implementation->EdgeColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->Widgets.edgeColor,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)),
    false);
    


  QObjectList ifaces =
    pqApplicationCore::instance()->getPluginManager()->interfaces();
  foreach(QObject* iface, ifaces)
    {
    pqTreeLayoutStrategyInterface* glsi = qobject_cast<pqTreeLayoutStrategyInterface*>(iface);
    if(glsi)
      {
      this->Implementation->Widgets.layoutStrategy->addItems(glsi->treeLayoutStrategies());
      }
    }

  pqSignalAdaptorComboBox* layoutStrategyAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.layoutStrategy);
  layoutStrategyAdaptor->setObjectName("ComboBoxAdaptor");

  pqComboBoxDomain* const layout_strategy_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.layoutStrategy,
    proxy->GetProperty("LayoutStrategy"),
    "domain");

  this->Implementation->Links.addPropertyLink(
    layoutStrategyAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("LayoutStrategy"));

  this->Implementation->Widgets.layoutStrategy->setCurrentIndex(
    this->Implementation->Widgets.layoutStrategy->
    findText(pqPropertyHelper(proxy, "LayoutStrategy").GetAsString()));



  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.areaLabels,
    "checked",
    SIGNAL(toggled(bool)),
    proxy,
    proxy->GetProperty("AreaLabels"));

  pqSignalAdaptorComboBox* areaLabelAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.areaLabelArray);
  areaLabelAdaptor->setObjectName("areaComboBoxAdaptor");

  pqComboBoxDomain* const area_label_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.areaLabelArray,
    proxy->GetProperty("AreaLabelArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    areaLabelAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("AreaLabelArray"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.areaLabelFontSize,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("AreaLabelFontSize"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->AreaColorAdaptor,
    "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy,
    proxy->GetProperty("AreaColor"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.areaColorByArray,
    "checked",
    SIGNAL(toggled(bool)),
    proxy,
    proxy->GetProperty("AreaColorByArray"));

  pqSignalAdaptorComboBox* areaColorAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.areaColorArray);
  areaColorAdaptor->setObjectName("areaColorComboBoxAdaptor");

  pqComboBoxDomain* const area_color_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.areaColorArray,
    proxy->GetProperty("AreaColorArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    areaColorAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("AreaColorArray"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.edgeLabels,
    "checked",
    SIGNAL(toggled(bool)),
    proxy,
    proxy->GetProperty("EdgeLabels"));

  pqSignalAdaptorComboBox* edgeLabelAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.edgeLabelArray);
  edgeLabelAdaptor->setObjectName("edgeLabelComboBoxAdaptor");

  pqComboBoxDomain* const edge_label_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.edgeLabelArray,
    proxy->GetProperty("EdgeLabelArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    edgeLabelAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("EdgeLabelArray"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.edgeLabelFontSize,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("EdgeLabelFontSize"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->EdgeColorAdaptor,
    "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy,
    proxy->GetProperty("EdgeColor"));

  //vtkSMIntVectorProperty::SafeDownCast(proxy->GetProperty("EdgeColorByArray"))->SetElement(0,0);
  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.edgeColorByArray,
    "checked",
    SIGNAL(toggled(bool)),
    proxy,
    proxy->GetProperty("EdgeColorByArray"));

  pqSignalAdaptorComboBox* edgeColorAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.edgeColorArray);
  edgeColorAdaptor->setObjectName("edgeColorComboBoxAdaptor");

  pqComboBoxDomain* const edge_color_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.edgeColorArray,
    proxy->GetProperty("EdgeColorArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    edgeColorAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("EdgeColorArray"));


  pqSignalAdaptorComboBox* areaLabelPriorityAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.areaLabelPriorityArray);
  areaLabelPriorityAdaptor->setObjectName("areaLabelPriorityComboBoxAdaptor");

  pqComboBoxDomain* const area_label_priority_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.areaLabelPriorityArray,
    proxy->GetProperty("AreaLabelPriorityArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    areaLabelPriorityAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("AreaLabelPriorityArray"));


  pqSignalAdaptorComboBox* areaLabelHoverAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.areaLabelHoverArray);
  areaLabelHoverAdaptor->setObjectName("areaLabelHoverComboBoxAdaptor");

  pqComboBoxDomain* const area_label_Hover_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.areaLabelHoverArray,
    proxy->GetProperty("AreaLabelHoverArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    areaLabelHoverAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("AreaLabelHoverArray"));


  pqSignalAdaptorComboBox* areaSizeAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.areaSizeArray);
  areaSizeAdaptor->setObjectName("areaSizeComboBoxAdaptor");

  pqComboBoxDomain* const area_size_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.areaSizeArray,
    proxy->GetProperty("AreaSizeArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    areaSizeAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("AreaSizeArray"));


  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.edgeBundlingStrength,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("EdgeBundlingStrength"));


  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.shrinkPercentage,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("ShrinkPercentage"));


  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.useDomainMap,
    "checked",
    SIGNAL(toggled(bool)),
    proxy,
    proxy->GetProperty("UseDomainMap"));

  pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smm,
    SIGNAL(sourceAdded(pqPipelineSource*)),
    this->Implementation->Widgets.domainMap,
    SLOT(addSource(pqPipelineSource*)));
  QObject::connect(smm,
    SIGNAL(sourceRemoved(pqPipelineSource*)),
    this->Implementation->Widgets.domainMap,
    SLOT(removeSource(pqPipelineSource*)));

  // Populate the domain map combo box with sources that produce tables.
  this->Implementation->Widgets.domainMap->setAllowedDataType("vtkTable");
  this->Implementation->Widgets.domainMap->populateComboBox();
  this->Implementation->VTKConnect->Connect(
    proxy->GetProperty("DomainMap"),
    vtkCommand::ModifiedEvent, this, SLOT(onProxyDomainMapChanged()));
  // First load domain map from proxy, if any.
  this->onProxyDomainMapChanged();
  // If there is no domain map in the proxy, set the current one.
  this->onComboBoxDomainMapChanged();

  QObject::connect(this->Implementation->Widgets.domainMap,
    SIGNAL(currentIndexChanged(vtkSMProxy*)),
    this, SLOT(onComboBoxDomainMapChanged()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()));
}

ClientTreeAreaDisplay::~ClientTreeAreaDisplay()
{
  delete this->Implementation;
}

void ClientTreeAreaDisplay::onProxyDomainMapChanged()
{
  vtkSMProxy* const proxy = vtkSMProxy::SafeDownCast(this->getRepresentation()->getProxy());
  if (vtkSMPropertyHelper(proxy, "DomainMap").GetNumberOfElements() > 0)
    {
    vtkSMProxy* const p = vtkSMPropertyHelper(proxy, "DomainMap").GetAsProxy();
    this->Implementation->Widgets.domainMap->setCurrentSource(p);
    }
  else
    {
    this->Implementation->Widgets.domainMap->setCurrentIndex(-1);
    }
}

void ClientTreeAreaDisplay::onComboBoxDomainMapChanged()
{
  vtkSMProxy* const proxy = vtkSMProxy::SafeDownCast(this->getRepresentation()->getProxy());
  pqPipelineSource* const pqsource = this->Implementation->Widgets.domainMap->currentSource();
  if (pqsource)
    {
    vtkSMProxy* const source = pqsource->getProxy();
    vtkSMPropertyHelper(proxy, "DomainMap").Set(source);
    }
  else
    {
    vtkSMPropertyHelper(proxy, "DomainMap").RemoveAllValues();
    }
  this->updateAllViews();
}

