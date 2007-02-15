/*=========================================================================

   Program: ParaView
   Module:    pqBarChartDisplayProxyEditor.cxx

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
#include "pqBarChartDisplayProxyEditor.h"
#include "ui_pqBarChartDisplayEditor.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QPointer>
#include <QtDebug>

#include "pqComboBoxDomain.h"
#include "pqDisplay.h"
#include "pqPropertyLinks.h"


//-----------------------------------------------------------------------------
class pqBarChartDisplayProxyEditor::pqInternal : public Ui::BarCharDisplayEditor
{
public:
  QPointer<pqDisplay> Display;
  QPointer<pqComboBoxDomain> XDomain;
  QPointer<pqComboBoxDomain> YDomain;
  vtkEventQtSlotConnect* VTKConnect;
  pqPropertyLinks Links;
};

//-----------------------------------------------------------------------------
pqBarChartDisplayProxyEditor::pqBarChartDisplayProxyEditor(pqDisplay* display, QWidget* _parent)
  : pqDisplayPanel(display, _parent)
{
  this->Internal = new pqInternal;
  this->Internal->VTKConnect = vtkEventQtSlotConnect::New();
  this->Internal->setupUi(this);
  
  QObject::connect(
    this->Internal->EditColorMapButton, SIGNAL(clicked()),
    this, SLOT(openColorMapEditor()));
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()), Qt::QueuedConnection);

  this->Internal->UsePoints->setCheckState(Qt::Checked);

  this->setDisplay(display);
}

//-----------------------------------------------------------------------------
pqBarChartDisplayProxyEditor::~pqBarChartDisplayProxyEditor()
{
  this->cleanup();

  this->Internal->VTKConnect->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayProxyEditor::setDisplay(pqDisplay* display) 
{
  if (this->Internal->Display == display)
    {
    return;
    }
  this->setEnabled(false);
  this->cleanup();

  this->Internal->Display = display;
  if (!display)
    {
    // Null display, just return.
    return;
    }
  vtkSMProxy* proxy = display->getProxy();
  if (!proxy || proxy->GetXMLName() != QString("BarChartDisplay"))
    {
    qCritical() << "Proxy is not a BarChartDisplay proxy. Cannot "
      << " edit it in pqBarChartDisplayProxyEditor.";
    return;
    }

  this->setEnabled(true);

  // Initialize links.
  this->Internal->Links.addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));
  this->Internal->Links.addPropertyLink(this->Internal->XArrayName,
    "currentText", SIGNAL(currentIndexChanged(const QString&)),
    proxy, proxy->GetProperty("XArrayName"));
  this->Internal->Links.addPropertyLink(this->Internal->YArrayName,
    "currentText", SIGNAL(currentIndexChanged(const QString&)),
    proxy, proxy->GetProperty("YArrayName"));
  this->Internal->Links.addPropertyLink(this->Internal->PointComponent,
    "currentText", SIGNAL(currentIndexChanged(const QString&)),
    proxy, proxy->GetProperty("XAxisPointComponent"));
  this->Internal->Links.addPropertyLink(this->Internal->UsePoints,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("XAxisUsePoints"));

  // Initialize domains.
  this->Internal->XDomain = new pqComboBoxDomain(
    this->Internal->XArrayName, proxy->GetProperty("XArrayName"), 1);
  this->Internal->YDomain = new pqComboBoxDomain(
    this->Internal->YArrayName, proxy->GetProperty("YArrayName"), 1);
  this->Internal->XDomain->forceDomainChanged();
  this->Internal->YDomain->forceDomainChanged();

  this->Internal->VTKConnect->Connect(proxy->GetProperty("XArrayName"),
    vtkCommand::ModifiedEvent, display, SLOT(updateLookupTable()));
  this->Internal->VTKConnect->Connect(proxy->GetProperty("YArrayName"),
    vtkCommand::ModifiedEvent, display, SLOT(updateLookupTable()));
  this->Internal->VTKConnect->Connect(proxy->GetProperty("XAxisUsePoints"),
    vtkCommand::ModifiedEvent, display, SLOT(updateLookupTable()));
  this->Internal->VTKConnect->Connect(proxy->GetProperty("XAxisPointComponent"),
    vtkCommand::ModifiedEvent, display, SLOT(updateLookupTable()));

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
  if (!this->Internal->Display)
    {
    return;
    }
  this->Internal->Display->getProxy()->GetProperty("Input")->
    UpdateDependentDomains();
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayProxyEditor::openColorMapEditor()
{
}

