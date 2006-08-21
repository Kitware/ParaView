/*=========================================================================

   Program: ParaView
   Module:    pqStateLoader.cxx

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

========================================================================*/
#include "pqStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"

#include <QPointer>

#include "pqMainWindowCore.h"
#include "pqRenderWindowManager.h"

//-----------------------------------------------------------------------------
class pqStateLoaderInternal
{
public:
  QPointer<pqMainWindowCore> MainWindowCore;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(pqStateLoader);
vtkCxxRevisionMacro(pqStateLoader, "1.2");
//-----------------------------------------------------------------------------
pqStateLoader::pqStateLoader()
{
  this->Internal = new pqStateLoaderInternal;
}

//-----------------------------------------------------------------------------
pqStateLoader::~pqStateLoader()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqStateLoader::SetMainWindowCore(pqMainWindowCore* core)
{
  this->Internal->MainWindowCore = core;
}

//-----------------------------------------------------------------------------
int pqStateLoader::LoadState(vtkPVXMLElement* root, int keep_proxies/*=0*/)
{
  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; ++cc)
    {
    vtkPVXMLElement* curElement = root->GetNestedElement(cc);
    const char* name = curElement->GetName();
    if (!name)
      {
      continue;
      }
    if (strcmp(name, "ServerManagerState") == 0)
      {
      if (!this->Superclass::LoadState(curElement, 1))
        {
        return 0;
        }
      }
    else if (strcmp(name, "RenderViewManager") == 0)
      {
      if (!this->Internal->MainWindowCore->multiViewManager().loadState(
          curElement, this))
        {
        return 0;
        }
      }
    }

  if (!keep_proxies)
    {
    this->ClearCreatedProxies();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int pqStateLoader::LoadProxyState(vtkPVXMLElement* proxyElement, 
  vtkSMProxy* proxy)
{
  if (strcmp(proxy->GetXMLGroup(), "rendermodules")==0)
    {
    unsigned int max = proxyElement->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < max; ++cc)
      {
      vtkPVXMLElement* element = proxyElement->GetNestedElement(cc);
      if (element->GetName() == QString("Property") and
        element->GetAttribute("name") == QString("Displays"))
        {
        element->SetAttribute("clear", "0");
        // This will ensure that when the state for Displays property is loaded
        // all already present displays won't be cleared.
        break;
        }
      }
    }
  return this->Superclass::LoadProxyState(proxyElement, proxy);
}

//-----------------------------------------------------------------------------
void pqStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
