/*=========================================================================

   Program: ParaView
   Module:    pqScalarBarRepresentation.cxx

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
#include "pqScalarBarRepresentation.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUndoElement.h"
#include "vtkSMPropertyModificationUndoElement.h"

#include <QtDebug>
#include <QPointer>
#include <QRegExp>

#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqProxy.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
class pqScalarBarRepresentation::pqInternal
{
public:
  QPointer<pqScalarsToColors> LookupTable;
  vtkEventQtSlotConnect* VTKConnect;
};

//-----------------------------------------------------------------------------
pqScalarBarRepresentation::pqScalarBarRepresentation(const QString& group,
                                                     const QString& name,
                                                     vtkSMProxy* scalarbar,
                                                     pqServer* server,
                                                     QObject* _parent)
  : Superclass(group, name, scalarbar, server, _parent)
{
  this->AutoHidden = false;
  this->Internal = new pqScalarBarRepresentation::pqInternal();

  this->Internal->VTKConnect = vtkEventQtSlotConnect::New();
  this->Internal->VTKConnect->Connect(scalarbar->GetProperty("LookupTable"),
    vtkCommand::ModifiedEvent, this, SLOT(onLookupTableModified()));

  // Listen to start/end interactions to update the application undo-redo stack
  // correctly.
  this->Internal->VTKConnect->Connect(scalarbar, vtkCommand::StartInteractionEvent,
    this, SLOT(startInteraction()));
  this->Internal->VTKConnect->Connect(scalarbar, vtkCommand::EndInteractionEvent,
    this, SLOT(endInteraction()));

  // load default values.
  this->onLookupTableModified();

  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (stack)
    {
    QObject::connect(this, SIGNAL(begin(const QString&)),
      stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this, SIGNAL(addToActiveUndoSet(vtkUndoElement*)),
      stack, SLOT(addToActiveUndoSet(vtkUndoElement*)));
    QObject::connect(this, SIGNAL(end()),
      stack, SLOT(endUndoSet()));
    }
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::setVisible(bool visible)
{
  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("Enabled"),
    (visible? 1 : 0));
  this->Superclass::setVisible(visible);
}

//-----------------------------------------------------------------------------
pqScalarBarRepresentation::~pqScalarBarRepresentation()
{
  if (this->Internal->LookupTable)
    {
    this->Internal->LookupTable->removeScalarBar(this);
    this->Internal->LookupTable = 0;
    }

  this->Internal->VTKConnect->Disconnect();
  this->Internal->VTKConnect->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqScalarBarRepresentation::getLookupTable() const
{
  return this->Internal->LookupTable;
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::onLookupTableModified()
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();

  vtkSMProxy* curLUTProxy = 
    pqSMAdaptor::getProxyProperty(this->getProxy()->GetProperty("LookupTable"));
  pqScalarsToColors* curLUT = smmodel->findItem<pqScalarsToColors*>(curLUTProxy);

  if (curLUT == this->Internal->LookupTable)
    {
    return;
    }

  if (this->Internal->LookupTable)
    {
    this->Internal->LookupTable->removeScalarBar(this);
    }

  this->Internal->LookupTable = curLUT;
  if (this->Internal->LookupTable)
    {
    this->Internal->LookupTable->addScalarBar(this);
    }
}

//-----------------------------------------------------------------------------
QPair<QString, QString> pqScalarBarRepresentation::getTitle() const
{
  QString title = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("Title")).toString();

  QString compTitle = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("ComponentTitle")).toString();

  return QPair<QString, QString>(title.trimmed(), compTitle.trimmed());
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::setTitle(const QString& name, const QString& comp)
{
  if (QPair<QString, QString>(name, comp) == this->getTitle())
    {
    return;
    }

  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("Title"),
    name.trimmed());
  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("ComponentTitle"),
    comp.trimmed());    
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();
  if (!this->isVisible())
    {
    // For any non-visible display, we don't set its defaults.
    return;
    }

  // Set default arrays and lookup table.
  vtkSMProxy* proxy = this->getProxy();
  
  pqSMAdaptor::setElementProperty(proxy->GetProperty("Selectable"), 0);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("Enabled"), 1);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("Resizable"), 1);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("Repositionable"), 1);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("TitleFontSize"), 12);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LabelFontSize"), 12);

  // setup global property link. By default, color is linked with
  // TextAnnotationColor.
  vtkSMGlobalPropertiesManager* globalPropertiesManager =
    pqApplicationCore::instance()->getGlobalPropertiesManager();
  globalPropertiesManager->SetGlobalPropertyLink(
    "TextAnnotationColor", proxy, "TitleColor");
  globalPropertiesManager->SetGlobalPropertyLink(
    "TextAnnotationColor", proxy, "LabelColor");

  proxy->UpdateVTKObjects();
}

#define PUSH_PROPERTY(name) \
{\
  vtkSMPropertyModificationUndoElement* elem =\
  vtkSMPropertyModificationUndoElement::New();\
  elem->ModifiedProperty(proxy, name);\
  emit this->addToActiveUndoSet(elem);\
  elem->Delete();\
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::startInteraction()
{
  emit this->begin("Move Color Legend");

  vtkSMProxy* proxy = this->getProxy();
  PUSH_PROPERTY("Position");
  PUSH_PROPERTY("Position2");
  PUSH_PROPERTY("Orientation");
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::endInteraction()
{
  vtkSMProxy* proxy = this->getProxy();
  PUSH_PROPERTY("Position");
  PUSH_PROPERTY("Position2");
  PUSH_PROPERTY("Orientation");
  emit this->end();
}

