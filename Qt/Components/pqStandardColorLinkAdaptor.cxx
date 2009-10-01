/*=========================================================================

   Program: ParaView
   Module:    pqStandardColorLinkAdaptor.cxx

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
#include "pqStandardColorLinkAdaptor.h"

// Server Manager Includes.
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkEventQtSlotConnect.h"

// Qt Includes.
#include "pqStandardColorButton.h"
#include "pqApplicationCore.h"
// ParaView Includes.

//-----------------------------------------------------------------------------
pqStandardColorLinkAdaptor::pqStandardColorLinkAdaptor(
  pqStandardColorButton* button, vtkSMProxy* proxy, const char* propname)
:Superclass(button)
{
  this->IgnoreModifiedEvents = false;
  this->Proxy = proxy;
  this->PropertyName = propname;
  this->VTKConnect = vtkEventQtSlotConnect::New();

  QObject::connect(button, SIGNAL(standardColorChanged(const QString&)),
    this, SLOT(onStandardColorChanged(const QString&)));
  this->VTKConnect->Connect(
    pqApplicationCore::instance()->getGlobalPropertiesManager(),
    vtkCommand::ModifiedEvent,
    this, SLOT(onGlobalPropertiesChanged()));

  this->onGlobalPropertiesChanged();
}

//-----------------------------------------------------------------------------
pqStandardColorLinkAdaptor::~pqStandardColorLinkAdaptor()
{
  this->VTKConnect->Delete();
}

//-----------------------------------------------------------------------------
void pqStandardColorLinkAdaptor::onGlobalPropertiesChanged()
{
  if (this->IgnoreModifiedEvents)
    {
    return;
    }
  vtkSMGlobalPropertiesManager* mgr =
    pqApplicationCore::instance()->getGlobalPropertiesManager();
  const char* name = mgr->GetGlobalPropertyName(this->Proxy,
    this->PropertyName.toAscii().data());
  qobject_cast<pqStandardColorButton*>(this->parent())->setStandardColor(name);
}

//-----------------------------------------------------------------------------
void pqStandardColorLinkAdaptor::breakLink(vtkSMProxy* proxy, const char* pname)
{
  vtkSMGlobalPropertiesManager* mgr =
    pqApplicationCore::instance()->getGlobalPropertiesManager();
  const char* oldname = mgr->GetGlobalPropertyName(proxy, pname);
  if (oldname)
    {
    mgr->RemoveGlobalPropertyLink(oldname, proxy, pname);
    }
}

//-----------------------------------------------------------------------------
void pqStandardColorLinkAdaptor::onStandardColorChanged(const QString& name)
{
  this->IgnoreModifiedEvents = true;
  vtkSMGlobalPropertiesManager* mgr =
    pqApplicationCore::instance()->getGlobalPropertiesManager();
  if (name.isEmpty())
    {
    pqStandardColorLinkAdaptor::breakLink(this->Proxy,
      this->PropertyName.toAscii().data());
    }
  else
    {
    mgr->SetGlobalPropertyLink(
      name.toStdString().c_str(),
      this->Proxy,
      this->PropertyName.toStdString().c_str());
    }
  this->IgnoreModifiedEvents = false;
}

