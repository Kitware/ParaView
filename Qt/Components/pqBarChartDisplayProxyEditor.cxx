/*=========================================================================

   Program: ParaView
   Module:    pqBarChartDisplayProxyEditor.cxx

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
#include "pqBarChartDisplayProxyEditor.h"
#include "ui_pqBarChartDisplayEditor.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QPointer>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqBarChartRepresentation.h"
#include "pqColorScaleToolbar.h"
#include "pqComboBoxDomain.h"
#include "pqRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"


//-----------------------------------------------------------------------------
class pqBarChartDisplayProxyEditor::pqInternal : public Ui::BarCharDisplayEditor
{
public:
  QPointer<pqRepresentation> Representation;
  QPointer<pqComboBoxDomain> XDomain;
  QPointer<pqComboBoxDomain> YDomain;
  vtkEventQtSlotConnect* VTKConnect;
  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* XArrayNameAdaptor;
  pqSignalAdaptorComboBox* PointComponentAdaptor;
  pqSignalAdaptorComboBox* YArrayNameAdaptor;
};

//-----------------------------------------------------------------------------
pqBarChartDisplayProxyEditor::pqBarChartDisplayProxyEditor(pqRepresentation* repr, QWidget* _parent)
  : pqDisplayPanel(repr, _parent)
{
  this->Internal = new pqInternal;
  this->Internal->VTKConnect = vtkEventQtSlotConnect::New();
  this->Internal->setupUi(this);
 
  this->Internal->XArrayNameAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->XArrayName);
  this->Internal->PointComponentAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->PointComponent);
  this->Internal->YArrayNameAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->YArrayName);

  QObject::connect(
    this->Internal->EditColorMapButton, SIGNAL(clicked()),
    this, SLOT(openColorMapEditor()));
  QObject::connect(
    this->Internal->RescaleButton, SIGNAL(clicked()),
    this, SLOT(rescaleToDataRange()));
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()));

  this->Internal->UsePoints->setCheckState(Qt::Checked);

  this->setRepresentation(repr);
}

//-----------------------------------------------------------------------------
pqBarChartDisplayProxyEditor::~pqBarChartDisplayProxyEditor()
{
  this->cleanup();

  this->Internal->VTKConnect->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayProxyEditor::setRepresentation(pqRepresentation* repr) 
{
  if (this->Internal->Representation == repr)
    {
    return;
    }
  this->setEnabled(false);
  this->cleanup();

  this->Internal->Representation = repr;
  if (!repr)
    {
    // Null repr, just return.
    return;
    }
  vtkSMProxy* proxy = repr->getProxy();
  if (!proxy || proxy->GetXMLName() != QString("BarChartRepresentation"))
    {
    qCritical() << "Proxy is not a BarChartRepresentation proxy. Cannot "
      << " edit it in pqBarChartDisplayProxyEditor.";
    return;
    }

  this->setEnabled(true);

  // Initialize domains.
  // Domains must be intialized before links are created otherwise,
  // we may loose current property values.
  this->Internal->XDomain = new pqComboBoxDomain(
    this->Internal->XArrayName, proxy->GetProperty("XArrayName"));
  this->Internal->YDomain = new pqComboBoxDomain(
    this->Internal->YArrayName, proxy->GetProperty("YArrayName"));
  this->Internal->XDomain->forceDomainChanged();
  this->Internal->YDomain->forceDomainChanged();

  // Initialize links.
  this->Internal->Links.addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));
  this->Internal->Links.addPropertyLink(this->Internal->XArrayNameAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("XArrayName"));
  this->Internal->Links.addPropertyLink(this->Internal->YArrayNameAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("YArrayName"));
  this->Internal->Links.addPropertyLink(this->Internal->PointComponentAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("XAxisPointComponent"));
  this->Internal->Links.addPropertyLink(this->Internal->UsePoints,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("XAxisUsePoints"));

  this->Internal->VTKConnect->Connect(proxy->GetProperty("XArrayName"),
    vtkCommand::ModifiedEvent, repr, SLOT(updateLookupTable()), 0, 0,
    Qt::QueuedConnection);
  this->Internal->VTKConnect->Connect(proxy->GetProperty("YArrayName"),
    vtkCommand::ModifiedEvent, repr, SLOT(updateLookupTable()), 0, 0,
    Qt::QueuedConnection);
  this->Internal->VTKConnect->Connect(proxy->GetProperty("XAxisUsePoints"),
    vtkCommand::ModifiedEvent, repr, SLOT(updateLookupTable()), 0, 0,
    Qt::QueuedConnection);
  this->Internal->VTKConnect->Connect(proxy->GetProperty("XAxisPointComponent"),
    vtkCommand::ModifiedEvent, repr, SLOT(updateLookupTable()), 0, 0,
    Qt::QueuedConnection);

  this->reloadGUI();
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayProxyEditor::cleanup()
{
  this->Internal->Links.removeAllPropertyLinks();
  this->Internal->VTKConnect->Disconnect();

  delete this->Internal->XDomain;
  this->Internal->XDomain = 0;

  delete this->Internal->YDomain;
  this->Internal->YDomain = 0;
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayProxyEditor::reloadGUI()
{
  if (!this->Internal->Representation)
    {
    return;
    }
  this->Internal->Representation->getProxy()->GetProperty("Input")->
    UpdateDependentDomains();
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayProxyEditor::openColorMapEditor()
{
  pqBarChartRepresentation *histogram =
    qobject_cast<pqBarChartRepresentation *>(this->Internal->Representation);

  // Get the color scale editor from the application core's registry.
  pqColorScaleToolbar *colorScale = qobject_cast<pqColorScaleToolbar *>(
      pqApplicationCore::instance()->manager("COLOR_SCALE_EDITOR"));
  if(colorScale)
    {
    colorScale->editColorMap(histogram);
    }
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayProxyEditor::rescaleToDataRange()
{
  pqBarChartRepresentation *histogram =
    qobject_cast<pqBarChartRepresentation *>(this->Internal->Representation);
  if(histogram)
    {
    histogram->resetLookupTableScalarRange();
    this->updateAllViews();
    }
}

