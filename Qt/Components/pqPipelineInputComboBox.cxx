/*=========================================================================

   Program: ParaView
   Module:    pqPipelineInputComboBox.cxx

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

========================================================================*/
#include "pqPipelineInputComboBox.h"

#include "vtkSMInputProperty.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"

#include "pqApplicationCore.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerObserver.h"
#include "pqTimer.h"
#include "pqUndoStack.h"

#include <QDebug>

namespace
{
// Functions used to save and obtain a proxy from a QVariant.
QVariant DATA(vtkSMProxy* proxy)
{
  return QVariant::fromValue<void*>(proxy);
}

vtkSMProxy* PROXY(const QVariant& variant)
{
  return reinterpret_cast<vtkSMProxy*>(variant.value<void*>());
}
}

//-----------------------------------------------------------------------------
pqPipelineInputComboBox::pqPipelineInputComboBox(
  vtkSMProxy* proxy, vtkSMProperty* _property, QWidget* _parent)
  : Superclass(_parent)
  , Proxy(proxy)
  , Property(_property)
  , UpdatePending(false)
  , InOnActivate(false)
{

  this->connect(this, SIGNAL(currentIndexChanged(int)), SLOT(onActivated(int)));

  pqServerManagerObserver* obs = pqApplicationCore::instance()->getServerManagerObserver();
  this->connect(obs, SIGNAL(proxyRegistered(QString, QString, vtkSMProxy*)),
    SLOT(proxyRegistered(QString, QString, vtkSMProxy*)));
  this->connect(obs, SIGNAL(proxyUnRegistered(QString, QString, vtkSMProxy*)),
    SLOT(proxyUnRegistered(QString, QString, vtkSMProxy*)));

  this->reload();
}

//-----------------------------------------------------------------------------
pqPipelineInputComboBox::~pqPipelineInputComboBox()
{
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineInputComboBox::currentProxy() const
{
  QVariant currentData = this->itemData(this->currentIndex());
  return PROXY(currentData);
}

//-----------------------------------------------------------------------------
void pqPipelineInputComboBox::setCurrentProxy(vtkSMProxy* proxy)
{
  for (int idx = 0; idx < this->count(); idx++)
  {
    if (PROXY(this->itemData(idx)) == proxy)
    {
      this->setCurrentIndex(idx);
      return;
    }
  }
}

//-----------------------------------------------------------------------------
void pqPipelineInputComboBox::proxyRegistered(const QString& group, const QString&, vtkSMProxy*)
{
  if (!this->UpdatePending && group == QLatin1String("sources"))
  {
    this->UpdatePending = true;
    pqTimer::singleShot(100, this, SLOT(reload()));
  }
}

//-----------------------------------------------------------------------------
void pqPipelineInputComboBox::proxyUnRegistered(const QString& group, const QString&, vtkSMProxy*)
{
  if (!this->UpdatePending && group == QLatin1String("sources"))
  {
    this->UpdatePending = true;
    pqTimer::singleShot(100, this, SLOT(reload()));
  }
}

//-----------------------------------------------------------------------------
void pqPipelineInputComboBox::reload()
{
  this->UpdatePending = false;
  this->blockSignals(true);
  this->clear();

  // Get all proxies under "sources" group and add them to the menu.
  vtkSMProxyIterator* proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetModeToOneGroup();

  // Look for proxies in the current active server/session
  proxyIter->SetSession(pqApplicationCore::instance()->getActiveServer()->session());

  for (proxyIter->Begin("sources"); !proxyIter->IsAtEnd(); proxyIter->Next())
  {
    this->addItem(proxyIter->GetKey(), DATA(proxyIter->GetProxy()));
  }
  proxyIter->Delete();

  this->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqPipelineInputComboBox::onActivated(int index)
{
  if (this->InOnActivate)
  {
    return;
  }

  QVariant sourceData = this->itemData(index);
  vtkSMProxy* sourceProxy = PROXY(sourceData);

  vtkSMInputProperty* inputProperty = vtkSMInputProperty::SafeDownCast(this->Property);

  if (!inputProperty)
  {
    qWarning() << "Missing input property.";
    return;
  }

  BEGIN_UNDO_SET("Input Change");
  this->InOnActivate = true;
  inputProperty->SetInputConnection(0, sourceProxy, 0);
  this->Proxy->UpdateVTKObjects();
  this->InOnActivate = false;
  END_UNDO_SET();
}
