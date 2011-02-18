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

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkSMStateLocator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"

#include <QList>
#include <QString>
#include <vtkstd/vector>

#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

//*****************************************************************************
//                  Internal class
//*****************************************************************************
struct HelperProxy
{
  HelperProxy(QString name, vtkTypeUInt32 id)
    {
    this->Name = name;
    this->Id = id;
    }

  QString Name;
  vtkTypeUInt32 Id;
};
//*****************************************************************************
struct pqHelperProxyRegisterUndoElement::vtkInternals
{
  vtkTypeUInt32 ProxyGlobalID;
  vtkstd::vector<HelperProxy> HelperList;
};
//*****************************************************************************
vtkStandardNewMacro(pqHelperProxyRegisterUndoElement);
//-----------------------------------------------------------------------------
pqHelperProxyRegisterUndoElement::pqHelperProxyRegisterUndoElement()
{
  this->Internal = new vtkInternals();
}

//-----------------------------------------------------------------------------
pqHelperProxyRegisterUndoElement::~pqHelperProxyRegisterUndoElement()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqHelperProxyRegisterUndoElement::RegisterHelperProxies(pqProxy* proxy)
{
  this->Internal->ProxyGlobalID = proxy->getProxy()->GetGlobalID();
  this->Internal->HelperList.clear();
  this->SetSession(proxy->getProxy()->GetSession());

  QList<QString> keys = proxy->getHelperKeys();
  for (int cc=0; cc < keys.size(); cc++)
    {
    QString key = keys[cc];
    QList<vtkSMProxy*> helpers = proxy->getHelperProxies(key);
    foreach (vtkSMProxy* helper, helpers)
      {
      this->Internal->HelperList.push_back(HelperProxy(key, helper->GetGlobalID()));
      }
    }
}

//-----------------------------------------------------------------------------
int pqHelperProxyRegisterUndoElement::DoTheJob()
{
  if (!this->Session)
    {
    vtkErrorMacro("Undo element not properly set");
    return 0;
    }

  vtkSMProxy* proxy =
      vtkSMProxy::SafeDownCast(
          this->Session->GetRemoteObject(this->Internal->ProxyGlobalID));

  if (!proxy)
    {
    vtkErrorMacro( "Failed to locate the proxy "
                   << this->Internal->ProxyGlobalID << endl);
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

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  for (unsigned int cc=0; cc < this->Internal->HelperList.size(); cc++)
    {
    HelperProxy item = this->Internal->HelperList[cc];
    vtkSMProxy* helper =
        vtkSMProxy::SafeDownCast(
            this->Session->GetRemoteObject(item.Id));

    // As the helper was not found yet, just recreate it and keep a reference
    // till the undo set complete. In that undo set that proxy should be register
    // otherwise it will be automatically removed.
    if(this->UndoSetWorkingContext && !helper)
      {
      helper = pxm->ReNewProxy(item.Id, proxy->GetSession()->GetStateLocator());
      this->UndoSetWorkingContext->AddItem(helper);
      helper->Delete();
      }

    if (!helper)
      {
      vtkErrorMacro("Failed to locate the helper.");
      continue;
      }
    pq_proxy->addHelperProxy(item.Name, helper);
    }
  return 1;
}

//-----------------------------------------------------------------------------
void pqHelperProxyRegisterUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
