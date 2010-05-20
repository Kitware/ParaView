/*=========================================================================

   Program: ParaView
   Module:    pqHelperProxyRegisterUndoElement.cxx

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
#include "pqHelperProxyRegisterUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"

#include <QList>
#include <QString>

#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"

vtkStandardNewMacro(pqHelperProxyRegisterUndoElement);
//-----------------------------------------------------------------------------
pqHelperProxyRegisterUndoElement::pqHelperProxyRegisterUndoElement()
{
}

//-----------------------------------------------------------------------------
pqHelperProxyRegisterUndoElement::~pqHelperProxyRegisterUndoElement()
{
}

//-----------------------------------------------------------------------------
bool pqHelperProxyRegisterUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "HelperProxyRegister") == 0);
}

//-----------------------------------------------------------------------------
void pqHelperProxyRegisterUndoElement::RegisterHelperProxies(pqProxy* proxy)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("HelperProxyRegister");
  elem->AddAttribute("id", proxy->getProxy()->GetSelfIDAsString());

  QList<QString> keys = proxy->getHelperKeys();
  for (int cc=0; cc < keys.size(); cc++)
    {
    QString key = keys[cc];
    QList<vtkSMProxy*> helpers = proxy->getHelperProxies(key);
    foreach (vtkSMProxy* helper, helpers)
      {
      vtkPVXMLElement* child = vtkPVXMLElement::New();
      child->SetName("Item");
      child->AddAttribute("id", helper->GetSelfIDAsString());
      child->AddAttribute("name", key.toAscii().data());
      elem->AddNestedElement(child);
      child->Delete();
      }
    }
  this->SetXMLElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
int pqHelperProxyRegisterUndoElement::Redo()
{
  vtkPVXMLElement* element = this->XMLElement;
  
  int id = 0;
  element->GetScalarAttribute("id",&id);
  if (!id)
    {
    vtkErrorMacro("Failed to locate proxy id.");
    return 0;
    }

  vtkSmartPointer<vtkSMProxyLocator> locator = 
   this->GetProxyLocator(); 

  locator->SetConnectionID(this->GetConnectionID());
  vtkSMProxy* proxy = locator->LocateProxy(id);

  if (!proxy)
    {
    vtkErrorMacro("Failed to locate the proxy.");
    return 0;
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();

  pqProxy* pq_proxy = smModel->findItem<pqProxy*>(proxy);
  if (!pq_proxy)
    {
    vtkErrorMacro("Failed to located pqProxy for the proxy.");
    return 0;
    }

  for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if (!child->GetScalarAttribute("id", &id))
      {
      vtkErrorMacro("Missing id.");
      continue;
      }

    const char* name = child->GetAttribute("name");
    if (!name)
      {
      vtkErrorMacro("Missing name.");
      continue;
      }

    vtkSMProxy* helper = locator->LocateProxy(id);
    if (!helper)
      {
      vtkErrorMacro("Failed to locate the helper.");
      continue;
      }
    pq_proxy->addHelperProxy(name, helper);
    }
  return 1;
}

//-----------------------------------------------------------------------------
void pqHelperProxyRegisterUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
