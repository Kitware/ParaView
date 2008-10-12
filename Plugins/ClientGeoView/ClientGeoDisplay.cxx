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

#include "ClientGeoDisplay.h"
#include "ui_ClientGeoDisplay.h"

#include <vtkCommand.h>
#include <vtkDataObject.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkImageData.h>
#include <vtkPointSet.h>
#include <vtkSmartPointer.h>
#include <vtkSMClientDeliveryRepresentationProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProperty.h>

#include <pqApplicationCore.h>
#include <pqComboBoxDomain.h>
#include <pqPipelineSource.h>
#include <pqPropertyLinks.h>
#include <pqServerManagerModel.h>
#include <pqSignalAdaptors.h>
#include <pqSourceComboBox.h>

class ClientGeoDisplay::implementation
{
public:
  implementation() :
    VertexColorAdaptor(0),
    EdgeColorAdaptor(0)
  {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  }

  ~implementation()
  {
    delete this->EdgeColorAdaptor;
    delete this->VertexColorAdaptor;
  }

  Ui::ClientGeoDisplay Widgets;

  pqPropertyLinks Links;
  pqSignalAdaptorColor* VertexColorAdaptor;
  pqSignalAdaptorColor* EdgeColorAdaptor;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

ClientGeoDisplay::ClientGeoDisplay(pqRepresentation* representation, QWidget* p) :
  pqDisplayPanel(representation, p)
{
  this->Implementation = new ClientGeoDisplay::implementation;

  vtkSMClientDeliveryRepresentationProxy* const proxy = 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(
      representation->getProxy());
  this->Implementation->Widgets.setupUi(this);

  pqComboBoxDomain* latitude_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.latArray,
    proxy->GetProperty("LatitudeArrayName"));
  pqComboBoxDomain* longitude_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.lonArray,
    proxy->GetProperty("LongitudeArrayName"));
  pqComboBoxDomain* vertex_label_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.vertexLabelArray,
    proxy->GetProperty("VertexLabelArrayName"));

  // No need to explicitly set the widget values to the current property values,
  // since the pqPropertyLinks connection initialization takes care of that.

  this->Implementation->VertexColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->Widgets.vertexColor,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)),
    false);

  this->Implementation->EdgeColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->Widgets.edgeColor,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)),
    false);

  // Connect widgets to properties
  pqSignalAdaptorComboBox* latArrayAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.latArray);
  latArrayAdaptor->setObjectName("latComboBoxAdaptor");

  this->Implementation->Links.addPropertyLink(
    latArrayAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("LatitudeArrayName"));

  pqSignalAdaptorComboBox* lonArrayAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.lonArray);
  lonArrayAdaptor->setObjectName("lonComboBoxAdaptor");

  this->Implementation->Links.addPropertyLink(
    lonArrayAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("LongitudeArrayName"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.explodeFactor,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("ExplodeFactor"));
  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.vertexLabels,
    "checked",
    SIGNAL(stateChanged(int)),
    proxy,
    proxy->GetProperty("VertexLabelVisibility"));

  pqSignalAdaptorComboBox* vertexLabelAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.vertexLabelArray);
  vertexLabelAdaptor->setObjectName("vertComboBoxAdaptor");

  this->Implementation->Links.addPropertyLink(
    vertexLabelAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("VertexLabelArrayName"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.vertexLabelFontSize,
    "value",
    SIGNAL(valueChanged(int)),
    proxy,
    proxy->GetProperty("VertexLabelFontSize"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.vertexSize,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("VertexSize"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.vertexOpacity,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("VertexOpacity"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->VertexColorAdaptor,
    "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy,
    proxy->GetProperty("VertexColor"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.vertexColorByArray,
    "checked",
    SIGNAL(toggled(bool)),
    proxy,
    proxy->GetProperty("VertexColorByArray"));

  pqSignalAdaptorComboBox* vertexColorAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.vertexColorArray);
  vertexColorAdaptor->setObjectName("vertColorComboBoxAdaptor");

  pqComboBoxDomain* const vertex_color_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.vertexColorArray,
    proxy->GetProperty("VertexColorArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    vertexColorAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("VertexColorArray"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.edgeLabels,
    "checked",
    SIGNAL(toggled(bool)),
    proxy,
    proxy->GetProperty("EdgeLabels"));

  pqSignalAdaptorComboBox* edgeLabelAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.edgeLabelArray);
  edgeLabelAdaptor->setObjectName("edgeLabelComboBoxAdaptor");

  this->Implementation->Links.addPropertyLink(
    edgeLabelAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("EdgeLabelArray"));

  pqComboBoxDomain* const edge_label_array_domain = new pqComboBoxDomain(
    this->Implementation->Widgets.edgeLabelArray,
    proxy->GetProperty("EdgeLabelArray"),
    "array_list");

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.edgeLabelFontSize,
    "value",
    SIGNAL(valueChanged(int)),
    proxy,
    proxy->GetProperty("EdgeLabelFontSize"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.edgeWidth,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("EdgeWidth"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->Widgets.edgeOpacity,
    "value",
    SIGNAL(valueChanged(double)),
    proxy,
    proxy->GetProperty("EdgeOpacity"));

  this->Implementation->Links.addPropertyLink(
    this->Implementation->EdgeColorAdaptor,
    "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy,
    proxy->GetProperty("EdgeColor"));

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

ClientGeoDisplay::~ClientGeoDisplay()
{
  delete this->Implementation;
}

void ClientGeoDisplay::onProxyDomainMapChanged()
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

void ClientGeoDisplay::onComboBoxDomainMapChanged()
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

