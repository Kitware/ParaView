/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkStateLoader.cxx

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
#include "pqLookmarkStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <QPointer>
#include <QList>
#include <QString>

#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqMainWindowCore.h"
#include "pqViewManager.h"
#include "pqPipelineSource.h"

//-----------------------------------------------------------------------------
class pqLookmarkStateLoaderInternal
{
public:
  QPointer<pqMainWindowCore> MainWindowCore;
  bool UseDataFlag;
  bool UseCameraFlag;
  bool MultipleSourcesFlag;
  vtkSMProxy *ActiveSourceProxy;
  QList<pqPipelineSource*> Sources;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(pqLookmarkStateLoader);
vtkCxxRevisionMacro(pqLookmarkStateLoader, "1.1");
//-----------------------------------------------------------------------------
pqLookmarkStateLoader::pqLookmarkStateLoader()
{
  this->Internal = new pqLookmarkStateLoaderInternal;
  this->Internal->MainWindowCore = 0;
  this->Internal->ActiveSourceProxy = 0;
  this->Internal->MultipleSourcesFlag = false;
  this->Internal->UseDataFlag = true;
  this->Internal->UseCameraFlag = true;
}

//-----------------------------------------------------------------------------
pqLookmarkStateLoader::~pqLookmarkStateLoader()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
int pqLookmarkStateLoader::LoadState(vtkPVXMLElement* state, int keep_proxies/*=0*/)
{
  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->AddNestedElement(state);
  int ret = this->Superclass::LoadState(root, keep_proxies);
  root->Delete();

  return ret;
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetMainWindowCore(pqMainWindowCore* core)
{
  this->Internal->MainWindowCore = core;
}


//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetUseDataFlag(bool state)
{
  if(state==false && this->Internal->MainWindowCore)
    {
    pqPipelineSource *activeSource = this->Internal->MainWindowCore->getActiveSource();
    if(!activeSource)
      {
      state = true;
      }
    this->Internal->MainWindowCore->getRootSources(&this->Internal->Sources,activeSource);
    if(this->Internal->Sources.size()>1)
      {
      this->Internal->MultipleSourcesFlag = true;
      }
    }

  this->Internal->UseDataFlag = state;
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetUseCameraFlag(bool state)
{
  this->Internal->UseCameraFlag = state;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqLookmarkStateLoader::NewProxyInternal(
  const char* xml_group, const char* xml_name)
{
  if (xml_group && xml_name && strcmp(xml_group, "sources")==0 && !this->Internal->UseDataFlag && !this->Internal->MultipleSourcesFlag)
    {
    return this->Internal->Sources.at(0)->getProxy();
    }
  return this->Superclass::NewProxyInternal(xml_group, xml_name);
}

//---------------------------------------------------------------------------
void pqLookmarkStateLoader::RegisterProxyInternal(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  if (proxy->GetXMLGroup() 
    && strcmp(proxy->GetXMLGroup(), "sources")==0 && !this->Internal->UseDataFlag && !this->Internal->MultipleSourcesFlag)
    {
    return;
    }
  this->Superclass::RegisterProxyInternal(group, name, proxy);
}

//-----------------------------------------------------------------------------
int pqLookmarkStateLoader::LoadProxyState(vtkPVXMLElement* proxyElement, 
  vtkSMProxy* proxy)
{
  if (strcmp(proxy->GetXMLGroup(), "rendermodules")==0 && !this->Internal->UseCameraFlag)
    {
    unsigned int max = proxyElement->GetNumberOfNestedElements();
    QList<vtkPVXMLElement*> toRemove;
    QString name;
    for (unsigned int cc=0; cc < max; ++cc)
      {
      vtkPVXMLElement* element = proxyElement->GetNestedElement(cc);
      name = element->GetAttribute("name");
      if (element->GetName() == QString("Property") &&
         name.contains("Camera"))
        {
        toRemove.push_back(element);
        }
      }
    QList<vtkPVXMLElement*>::iterator iter;
    for(iter=toRemove.begin(); iter!=toRemove.end(); iter++)
      {
      proxyElement->RemoveNestedElement(*iter);
      }
    }

  return this->Superclass::LoadProxyState(proxyElement, proxy);
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
