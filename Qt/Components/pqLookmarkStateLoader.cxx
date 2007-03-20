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
#include "vtkSmartPointer.h"

#include <QPointer>
#include <QList>
#include <QString>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStandardItem>

#include "pqPipelineSource.h"
#include "pqPipelineFilter.h"
#include "pqApplicationCore.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServerManagerModel.h"
#include "pqLookmarkSourceDialog.h"
#include "pqPipelineModel.h"

//-----------------------------------------------------------------------------
class pqLookmarkStateLoaderInternal
{
public:
  QList<pqPipelineSource*> *PreferredSources;
  QList<QStandardItem*> LookmarkSources;
  int NumberOfLookmarkSources;
  QStandardItemModel *LookmarkPipelineModel;
  pqPipelineModel *PipelineModel;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(pqLookmarkStateLoader);
vtkCxxRevisionMacro(pqLookmarkStateLoader, "1.4");
//-----------------------------------------------------------------------------
pqLookmarkStateLoader::pqLookmarkStateLoader()
{
  this->Internal = new pqLookmarkStateLoaderInternal;
  this->Internal->NumberOfLookmarkSources = 0;
  this->Internal->PreferredSources = 0;
  this->Internal->LookmarkPipelineModel = 0;
  this->Internal->PipelineModel = 0;

  pqServerManagerModel *model = pqApplicationCore::instance()->getServerManagerModel();
  this->Internal->PipelineModel = new pqPipelineModel(*model);
}

//-----------------------------------------------------------------------------
pqLookmarkStateLoader::~pqLookmarkStateLoader()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetPreferredSources(QList<pqPipelineSource*> *sources)
{
  if(this->Internal->PreferredSources)
    {
    this->Internal->PreferredSources->clear();
    }

  this->Internal->PreferredSources = sources;
}


//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetPipelineHierarchy(vtkPVXMLElement *lookmarkPipeline)
{
  // Determine the number of sources in the lookmmark's state from the pipeline hierarchy
  int numSources = 0;
  for(unsigned int i=0; i<lookmarkPipeline->GetNumberOfNestedElements(); i++)
    {
    vtkPVXMLElement *childElem = lookmarkPipeline->GetNestedElement(i);
    if(strcmp(childElem->GetName(),"Source")==0)
      {
      numSources++;
      }
    }

  this->Internal->NumberOfLookmarkSources = numSources;

  // Set up the pipeline model for this lookmak's state
  this->Internal->LookmarkSources.clear();
  this->Internal->LookmarkPipelineModel = new QStandardItemModel();
  this->AddChildItems(lookmarkPipeline,this->Internal->LookmarkPipelineModel->invisibleRootItem());
}

void pqLookmarkStateLoader::AddChildItems(vtkPVXMLElement *elem, QStandardItem *item)
{
  for(unsigned int i=0; i<elem->GetNumberOfNestedElements(); i++)
    {
    vtkPVXMLElement *childElem = elem->GetNestedElement(i);
    // determine icon type:
    QIcon icon = QIcon();
    if(strcmp(childElem->GetName(),"Server")==0)
      {
      icon.addFile(":/pqWidgets/Icons/pqServer16.png");
      }
    if(strcmp(childElem->GetName(),"Source")==0)
      {
      icon.addFile(":/pqWidgets/Icons/pqSource16.png");
      }
    if(strcmp(childElem->GetName(),"Filter")==0)
      {
      icon.addFile(":/pqWidgets/Icons/pqFilter16.png");
      }
    QStandardItem *childItem = new QStandardItem(icon,QString(childElem->GetAttribute("Name")));
    item->setChild(i,0,childItem);
    if(strcmp(childElem->GetName(),"Source")==0)
      {
      this->Internal->LookmarkSources.push_back(childItem);
      }
    this->AddChildItems(childElem,childItem);
    }
}


//---------------------------------------------------------------------------
int pqLookmarkStateLoader::LoadState(vtkPVXMLElement* rootElement, int keep_proxies/*=0*/)
{
  pqServerManagerModel *model = pqApplicationCore::instance()->getServerManagerModel();

  if (!rootElement)
    {
    vtkErrorMacro("Cannot load state from (null) root element.");
    return 0;
    }

  // Do we have enough open sources to accomodate this lookmark's state?
  int numExistingSources = 0;
  for(unsigned int i=0; i<model->getNumberOfSources(); i++)
    {
    if(!dynamic_cast<pqPipelineFilter*>(model->getPQSource(i)))
      {
      numExistingSources++;
      }
    }

  if(numExistingSources<this->Internal->NumberOfLookmarkSources)
    {
    QMessageBox::warning(NULL, "Error Loading Lookmark",
          "There are not enough existing readers or sources in the pipeline to accomodate this lookmark.");
    return 0;
    }

  return this->Superclass::LoadState(rootElement, keep_proxies);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqLookmarkStateLoader::NewProxyInternal(
  const char* xml_group, const char* xml_name)
{
  if(!xml_group || !xml_name || strcmp(xml_group, "sources")!=0)
    {
    // let superclass handle it
    return this->Superclass::NewProxyInternal(xml_group, xml_name);
    }

  // If this lookmark has one source, use the first item in the preferred source list, if any
  if(this->Internal->NumberOfLookmarkSources==1 && this->Internal->PreferredSources->size()>=1)
    {
    vtkSMProxy *proxy = this->Internal->PreferredSources->at(0)->getProxy();
    proxy->Register(this);
    return proxy;
    }

  // If it has multiple sources, prompt user
  pqLookmarkSourceDialog *srcDialog = new pqLookmarkSourceDialog(this->Internal->LookmarkPipelineModel,this->Internal->PipelineModel);
  srcDialog->setLookmarkSource(this->Internal->LookmarkSources.takeFirst());
  if(srcDialog->exec() == QDialog::Accepted)
    {
    // return the source the user selected to use for this proxy
    pqPipelineSource *src = srcDialog->getSelectedSource();
    if(src)
      {
    //  this->Internal->Sources->removeAll(src);
      //this->Internal->PipelineModel->removeSource(src);
      vtkSMProxy *proxy = src->getProxy();
      proxy->Register(this);
      return proxy;
      }
    }
  delete srcDialog;

  return 0;
}


//---------------------------------------------------------------------------
void pqLookmarkStateLoader::RegisterProxyInternal(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  if (proxy->GetXMLGroup() && strcmp(proxy->GetXMLGroup(), "sources")==0 )
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    if (pxm->GetProxyName(group, proxy))
      {
      // source is registered, don't re-register it.
      return;
      }
    }
  this->Superclass::RegisterProxyInternal(group, name, proxy);
}

//-----------------------------------------------------------------------------
int pqLookmarkStateLoader::LoadProxyState(vtkPVXMLElement* proxyElement, 
  vtkSMProxy* proxy)
{
  if (strcmp(proxy->GetXMLGroup(), "sources")==0 )
    {
    // If this is a source, don't load the state
    return 1;
    }
  else if (strcmp(proxy->GetXMLGroup(), "rendermodules")==0 )
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
