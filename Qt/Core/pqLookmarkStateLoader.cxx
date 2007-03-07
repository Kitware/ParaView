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
#include <QMessageBox>

#include "pqPipelineSource.h"
#include "pqPipelineFilter.h"
#include "pqApplicationCore.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
class pqLookmarkStateLoaderInternal
{
public:
  //QPointer<pqMainWindowCore> MainWindowCore;
  bool UseDataFlag;
  bool UseCameraFlag;
  bool MultipleSourcesFlag;
  //vtkSMProxy *ActiveSourceProxy;
  QList<pqPipelineSource*> Sources;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(pqLookmarkStateLoader);
vtkCxxRevisionMacro(pqLookmarkStateLoader, "1.1");
//-----------------------------------------------------------------------------
pqLookmarkStateLoader::pqLookmarkStateLoader()
{
  this->Internal = new pqLookmarkStateLoaderInternal;
  //this->Internal->MainWindowCore = 0;
  //this->Internal->ActiveSourceProxy = 0;
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

  // If we are to use an existing dataset for this lookmark, there are several hoops we need to jump through to try to find one
  if(!this->Internal->UseDataFlag)
    {
    bool abort = false;
    // First make sure the pipeline is not empty
    pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
    if(smModel->getNumberOfSources()==0)
      {
      abort = true;
      }
  
    // Try to get the "active" source in the application because its a good bet this is the data the user means to use with this lookmark
    pqPipelineSource *activeSource = dynamic_cast<pqPipelineSource *>(pqApplicationCore::instance()->getSelectionModel()->currentItem());
    if(activeSource)
      {
      // Get the source data for the active source
      this->GetRootSources(&this->Internal->Sources,activeSource);
      if(this->Internal->Sources.size()>1)
        {
        this->Internal->MultipleSourcesFlag = true;
        }
      else if(this->Internal->Sources.size()==0)
        {
        // This should never happen
        abort = true;
        }
      }
    else
      {
      abort = true;
      }

    if(abort)
      {
      QMessageBox::warning(NULL, "No Data Selected",
            "This lookmark has been configured to use existing data. Please select a source in the Pipeline Browser and try again.");
      return 0;
      }
    }

  return this->Superclass::LoadState(state, keep_proxies);
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::GetRootSources(QList<pqPipelineSource*> *sources, pqPipelineSource *src)
{
  pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter*>(src);
  if(!filter || filter->getInputCount()==0)
    {
    sources->push_back(src);
    return;
    }
  for(int i=0; i<filter->getInputCount(); i++)
    {
    this->GetRootSources(sources, filter->getInput(i));
    }
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetUseDataFlag(bool state)
{
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
  if (strcmp(proxy->GetXMLGroup(), "rendermodules")==0 )
    {
    unsigned int max = proxyElement->GetNumberOfNestedElements();
    QList<vtkPVXMLElement*> toRemove;
    QString name;
    for (unsigned int cc=0; cc < max; ++cc)
      {
      vtkPVXMLElement* element = proxyElement->GetNestedElement(cc);
      name = element->GetAttribute("name");
      if (element->GetName() == QString("Property") &&
         name.contains("Camera") && !this->Internal->UseCameraFlag)
        {
        toRemove.push_back(element);
        }
      else if (element->GetName() == QString("Property") &&
        element->GetAttribute("name") == QString("Displays"))
        {
        element->SetAttribute("clear", "0");
        // This will ensure that when the state for Displays property is loaded
        // all already present displays won't be cleared.
        }
      else if (element->GetName() == QString("Property") &&
        element->GetAttribute("name") == QString("RenderWindowSize"))
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
