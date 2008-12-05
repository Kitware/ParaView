/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkStateLoader.cxx

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
#include "pqLookmarkStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#include <QPointer>
#include <QList>
#include <QMap>
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
#include "pqTimeKeeper.h"
#include "pqView.h"
#include "pqDataRepresentation.h"

//-----------------------------------------------------------------------------
class pqLookmarkStateLoaderInternal
{
public:

  pqLookmarkStateLoaderInternal()
    {
    this->NumberOfLookmarkSources = 0;
    this->PreferredSources = 0;
    this->PipelineModel = 0;
    this->RestoreCamera = false;
    this->RestoreTime = false;
    this->TimeKeeper = 0;
    this->SourceProxyCollectionLoaded = false;
    this->View = NULL;

    pqServerManagerModel *model = 
      pqApplicationCore::instance()->getServerManagerModel();
    this->PipelineModel = new pqPipelineModel(*model);
    }

  ~pqLookmarkStateLoaderInternal()
    {
    delete this->PipelineModel;
    }

  int CurrentSourceID;
  int CurrentDisplayID;
  QMap<int, pqPipelineSource*> LookmarkSourceIdToExistingSourceMap;
  QMap<int, int> DisplayIdToSourceIdMap;
  QList<pqPipelineSource*> *PreferredSources;
  QList<QStandardItem*> LookmarkSources;
  int NumberOfLookmarkSources;
  QStandardItemModel LookmarkPipelineModel;
  pqPipelineModel *PipelineModel;
  bool RestoreCamera;
  bool RestoreTime;
  pqTimeKeeper *TimeKeeper;
  vtkPVXMLElement *RootElement;
  QStringList IdsOfProxyElementsToIgnore;
  bool SourceProxyCollectionLoaded;
  pqView *View;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(pqLookmarkStateLoader);
vtkCxxRevisionMacro(pqLookmarkStateLoader, "1.24");
//-----------------------------------------------------------------------------
pqLookmarkStateLoader::pqLookmarkStateLoader()
{
  this->Internal = new pqLookmarkStateLoaderInternal;
}

//-----------------------------------------------------------------------------
pqLookmarkStateLoader::~pqLookmarkStateLoader()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetView(pqView *lmkView)
{
  this->Internal->View = lmkView;
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetRestoreCameraFlag(bool state)
{
  this->Internal->RestoreCamera = state;
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetRestoreTimeFlag(bool state)
{
  this->Internal->RestoreTime = state;
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::SetTimeKeeper(pqTimeKeeper *timekeeper)
{
  this->Internal->TimeKeeper = timekeeper;
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
  this->AddChildItems(lookmarkPipeline,this->Internal->LookmarkPipelineModel.invisibleRootItem());
}

void pqLookmarkStateLoader::AddChildItems(vtkPVXMLElement *elem, QStandardItem *item)
{
  for(unsigned int i=0; i<elem->GetNumberOfNestedElements(); i++)
    {
    vtkPVXMLElement *childElem = elem->GetNestedElement(i);
    QStandardItem *childItem = new QStandardItem(
            QIcon(":/pqWidgets/Icons/pqBundle32.png"),
            QString(childElem->GetAttribute("Name")));
    item->setChild(i,0,childItem);
    // Store the model items of sources for later
    if(strcmp(childElem->GetName(),"Source")==0)
      {
      this->Internal->LookmarkSources.push_back(childItem);
      }
    // recurse...
    this->AddChildItems(childElem,childItem);
    }
}


//---------------------------------------------------------------------------
int pqLookmarkStateLoader::LoadStateInternal(vtkPVXMLElement* rootElement)
{
  pqServerManagerModel *model = pqApplicationCore::instance()->getServerManagerModel();

  if (!rootElement)
    {
    vtkErrorMacro("Cannot load state from (null) root element.");
    return 0;
    }

  this->Internal->RootElement = rootElement;

  // Do we have enough open sources to accomodate this lookmark's state?
  int numSources = model->getNumberOfItems<pqPipelineSource*>();
  if(numSources<this->Internal->NumberOfLookmarkSources)
    {
    QMessageBox::warning(NULL, "Error Loading Lookmark",
       "There are not enough existing sources or filters in the pipeline to "
       "accomodate this lookmark.");
    return 0;
    }

  return this->Superclass::LoadStateInternal(rootElement);
}

//---------------------------------------------------------------------------
int pqLookmarkStateLoader::HandleProxyCollection(vtkPVXMLElement* collectionElement)
{
  const char* groupName = collectionElement->GetAttribute("name");
  if (strcmp(groupName,"sources")!=0 && !this->Internal->SourceProxyCollectionLoaded)
    {
    unsigned int numElems = this->Internal->RootElement->GetNumberOfNestedElements();
    unsigned int i;
    for (i=0; i<numElems; i++)
      {
      vtkPVXMLElement* currentElement = this->Internal->RootElement->GetNestedElement(i);
      const char* name = currentElement->GetName();
      const char* type = currentElement->GetAttribute("name");
      if (name && type)
        {
        if (strcmp(name, "ProxyCollection") == 0 && strcmp(type, "sources") == 0)
          {
          this->HandleProxyCollection(currentElement);
          break;
          }
        }
      }
    }
  else if (strcmp(groupName,"sources")==0)
    {
    QString srcName;
    vtkPVXMLElement *newCollectionElement = vtkPVXMLElement::New();
    newCollectionElement->SetAttribute("name",groupName);
    for(int j=0; j<this->Internal->LookmarkSources.count(); j++)
      {
      srcName = this->Internal->LookmarkSources[j]->text();
      unsigned int numElems = collectionElement->GetNumberOfNestedElements();
      for (unsigned int i=0; i<numElems; i++)
        {
        vtkPVXMLElement* currentElement = collectionElement->GetNestedElement(i);
        if (currentElement->GetName() &&
            strcmp(currentElement->GetName(), "Item") == 0 &&
            srcName == QString(currentElement->GetAttribute("name")))
          {
          newCollectionElement->AddNestedElement(currentElement);
          }
        }
      }
    int ret = this->Superclass::HandleProxyCollection(newCollectionElement);
    newCollectionElement->Delete();
    this->Internal->SourceProxyCollectionLoaded = true;
    return ret;
    }

  return this->Superclass::HandleProxyCollection(collectionElement);
}


//---------------------------------------------------------------------------
vtkSMProxy* pqLookmarkStateLoader::NewProxy(int id, vtkSMProxyLocator* locator)
{
  vtkPVXMLElement* proxyElement = this->LocateProxyElement(id);
  if (!proxyElement)
    {
    return 0;
    }

  const char* group = proxyElement->GetAttribute("group");
  const char* type = proxyElement->GetAttribute("type");
  if (!type || !group)
    {
    vtkErrorMacro("Could not create proxy from element, missing 'type'.");
    return 0;
    }
  
  if (strcmp(proxyElement->GetName(), "Proxy") == 0)
    {
    const char* group = proxyElement->GetAttribute("group");
    const char* type = proxyElement->GetAttribute("type");
    if (!type || !group)
      {
      vtkErrorMacro("Could not create proxy from element.");
      return 0;
      }
    if(strcmp(group,"sources")==0)
      {
      this->Internal->CurrentSourceID = id;
      this->Internal->IdsOfProxyElementsToIgnore.push_back(
            QString(proxyElement->GetAttribute("id")));

      // Find the display in the state that has this source as an input and store for later
      // so we can ignore it when its state is being loaded
      for (unsigned int i=0; i<this->Internal->RootElement->GetNumberOfNestedElements(); i++)
        {
        vtkPVXMLElement* currentElement = 
            this->Internal->RootElement->GetNestedElement(i);
        const char* name = currentElement->GetName();
        const char* groupName = currentElement->GetAttribute("group");
        if (name && groupName)
          {
          if (strcmp(name, "Proxy") == 0 && strcmp(groupName, "representations") == 0)
            {
            for (unsigned int j=0; j<currentElement->GetNumberOfNestedElements(); j++)
              {
              vtkPVXMLElement* inputElement = currentElement->GetNestedElement(j);
              const char* inputTag = inputElement->GetName();
              const char* inputName = inputElement->GetAttribute("name");
              if (inputTag && inputName)
                {
                if (strcmp(inputTag, "Property") == 0 && strcmp(inputName, "Input") == 0)
                  {
                  vtkPVXMLElement *srcElem = inputElement->FindNestedElementByName("Proxy");
                  if(QString::number(id) == QString(srcElem->GetAttribute("value")))
                    {
                    this->Internal->IdsOfProxyElementsToIgnore.push_back(
                          QString(currentElement->GetAttribute("id")));

                    this->Internal->DisplayIdToSourceIdMap[
                          QString(currentElement->GetAttribute("id")).toInt()] = id;
                    }
                  }
                }
              }
            }
          }
        }
      }
    else if(strcmp(group,"representations")==0)
      {
      this->Internal->CurrentDisplayID = id;
      }
    }

  return this->Superclass::NewProxy(id, locator);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqLookmarkStateLoader::CreateProxy(
  const char* xml_group, const char* xml_name, vtkIdType cid)
{
  if(xml_group && xml_name && (strcmp(xml_group, "sources")==0) )
    {
    // If this lookmark has one source and our collection of 
    //  selected sources has only one, use it
    if(this->Internal->NumberOfLookmarkSources==1 && 
        this->Internal->PreferredSources->size()==1)
      {
      pqPipelineSource *src = this->Internal->PreferredSources->at(0);
      this->Internal->LookmarkSourceIdToExistingSourceMap[this->Internal->CurrentSourceID] = src;
      vtkSMProxy *proxy = src->getProxy();
      proxy->Register(this);
      return proxy;
      }

    // If the lookmark has multiple sources 
    //  OR there are more selections than lookmark inputs 
    //  OR there are no selections
    //  prompt the user.
    pqLookmarkSourceDialog *srcDialog = new pqLookmarkSourceDialog(
      &this->Internal->LookmarkPipelineModel,this->Internal->PipelineModel);
    srcDialog->setLookmarkSource(this->Internal->LookmarkSources.takeFirst());
    if(srcDialog->exec() == QDialog::Accepted)
      {
      // return the source the user selected to use for this proxy
      pqPipelineSource *src = srcDialog->getSelectedSource();
      if(src)
        {
        this->Internal->LookmarkSourceIdToExistingSourceMap[this->Internal->CurrentSourceID] = src;
        vtkSMProxy *proxy = src->getProxy();
        proxy->Register(this);
        return proxy;
        }
      }
    }
  else if(xml_group && xml_name && (strcmp(xml_group, "representations")==0) )
    {
    if(this->Internal->DisplayIdToSourceIdMap.keys().contains(
                                    this->Internal->CurrentDisplayID))
      {
      pqPipelineSource *src = this->Internal->LookmarkSourceIdToExistingSourceMap[
                    this->Internal->DisplayIdToSourceIdMap[
                          this->Internal->CurrentDisplayID]];

      QList<pqRepresentation*> displays = this->Internal->View->getRepresentations();
      vtkSMProxy *proxy = NULL;
      for(int i=0; i<displays.count(); i++)
        {
        pqDataRepresentation *rep = dynamic_cast<pqDataRepresentation*>(displays[i]);
        if(rep && rep->getInput() == src)
          {
          proxy = rep->getProxy();
          break;
          }
        } 
      if(proxy)
        {
        proxy->Register(this);
        return proxy;
        }

      // If we get here that means the source has no 
      //    representation in the given view
      this->Internal->IdsOfProxyElementsToIgnore.removeAll(
            QString::number(this->Internal->CurrentDisplayID));
      }
    }

  return this->Superclass::CreateProxy(xml_group, xml_name, cid);
}


//---------------------------------------------------------------------------
void pqLookmarkStateLoader::RegisterProxy(int id, vtkSMProxy* proxy)
{
  // Don't register a proxy that we are going to ignore later
  if(this->Internal->IdsOfProxyElementsToIgnore.contains(QString::number(id)))
    {
    return;
    }

  this->Superclass::RegisterProxy(id, proxy);
}


//---------------------------------------------------------------------------
void pqLookmarkStateLoader::HandleCompoundProxyDefinitions(
  vtkPVXMLElement* vtkNotUsed(element))
{
  // Compound proxy states are not loaded by a lookmark
  return;
}

//-----------------------------------------------------------------------------
int pqLookmarkStateLoader::LoadProxyState(vtkPVXMLElement* proxyElement, 
  vtkSMProxy* proxy, vtkSMProxyLocator* locator)
{
  // Remove all elements of a source/reader's XML unless its a 
  // PointArrayStatus or CellArrayStatus property. But these 
  // should only be turned on, not off (so as not to affect other 
  // views this source is displayed in).
  if (strcmp(proxyElement->GetName(), "Proxy")==0 && 
      strcmp(proxyElement->GetAttribute("group"), "sources")==0 )
    {
    QList<vtkPVXMLElement*> elementsToRemove;
    QList<vtkPVXMLElement*> arrayElementsToRemove;
    QList<vtkPVXMLElement*>::iterator iter;
    unsigned int max = proxyElement->GetNumberOfNestedElements();
    QString name;
    for (unsigned int cc=0; cc < max; ++cc)
      {
      vtkPVXMLElement* element = proxyElement->GetNestedElement(cc);
      name = element->GetAttribute("name");
      if (element->GetName() == QString("Property") &&
         ( name.contains("PointArrayStatus") ||
           name.contains("CellArrayStatus") ||
           name.contains("ResultArrayStatus") ))
        {
        arrayElementsToRemove.clear();
        for(unsigned int cc1=0; cc1<element->GetNumberOfNestedElements(); cc1++)
          {
          vtkPVXMLElement *valueElement = element->GetNestedElement(cc1);
          if(valueElement->GetName() == QString("Element") && 
              strcmp(valueElement->GetAttribute("value"),"0")==0 )
            {
            arrayElementsToRemove.push_back(valueElement);
            }
          }
        for(iter=arrayElementsToRemove.begin(); iter!=arrayElementsToRemove.end(); iter++)
          {
          element->RemoveNestedElement(*iter);
          }
        }
      else
        {
        elementsToRemove.push_back(element);
        }
      }

    for(iter=elementsToRemove.begin(); iter!=elementsToRemove.end(); iter++)
      {
      proxyElement->RemoveNestedElement(*iter);
      }

    // if a filter is being used in place of a source, find the filter's reader
    //    and use its proxy instead
    if(strcmp(proxy->GetXMLGroup(), "filters")==0)
      {
      pqServerManagerModel *model = pqApplicationCore::instance()->getServerManagerModel();
      pqPipelineFilter *filter = model->findItem<pqPipelineFilter*>(proxy);
      if(filter)
        {
        // move up the pipeline until we find a non-filter source
        pqPipelineSource *src;
        while(filter)
          {
          src = filter->getInput(0);
          filter = dynamic_cast<pqPipelineFilter*>(src);
          }
        proxy = src->getProxy();
        }
      }
    }
  else if (strcmp(proxyElement->GetName(), "Proxy")==0 && 
      (proxy->IsA("vtkSMRenderViewProxy") ||
       strcmp(proxyElement->GetAttribute("type"), "ClientGraphView")==0 ) )
    {
    unsigned int max = proxyElement->GetNumberOfNestedElements();
    QString name;
    QString value;
    QList<vtkPVXMLElement*> toRemove;
    QList<vtkPVXMLElement*> displaysToRemove;
    for (unsigned int cc=0; cc < max; ++cc)
      {
      vtkPVXMLElement* element = proxyElement->GetNestedElement(cc);
      name = element->GetAttribute("name");
      if (element->GetName() == QString("Property") &&
         name.contains("Camera") && !this->Internal->RestoreCamera)
        {
        toRemove.push_back(element);
        }
      else if (strcmp(element->GetName(), "Property") == 0 &&
        (name == "GUISize" || name == "ViewPosition" || name == "ViewSize"))
        {
        toRemove.push_back(element);
        }
      else if (element->GetName() == QString("Property") &&
        element->GetAttribute("name") == QString("Representations"))
        {
        // remove unused displays from the view's displays proxyproperty
        QStringList ids = this->Internal->IdsOfProxyElementsToIgnore;
        displaysToRemove.clear();
        for(int k=0;k<ids.size();k++)
          {
          for (unsigned int cc2=0; cc2 < element->GetNumberOfNestedElements(); ++cc2)
            {
            vtkPVXMLElement* childElem = element->GetNestedElement(cc2);
            value = childElem->GetAttribute("value");
            if (value==ids[k])
              {
              displaysToRemove.push_back(childElem);
              }
            }
          }
        int num = QString(element->GetAttribute("number_of_elements")).toInt();
        num -= displaysToRemove.count();
        element->SetAttribute("number_of_elements",QString::number(num).toAscii().data());

        QList<vtkPVXMLElement*>::iterator iter;
        for(iter=displaysToRemove.begin(); iter!=displaysToRemove.end(); iter++)
          {
          element->RemoveNestedElement(*iter);
          }
        }
      else if (element->GetName() == QString("Property") &&
        element->GetAttribute("name") == QString("ViewTime"))
        {
        if(this->Internal->RestoreTime)
          {
          vtkPVXMLElement *valElem = element->FindNestedElementByName("Element");
          if(valElem && this->Internal->TimeKeeper)
            {
            double viewTime;
            valElem->GetScalarAttribute("value",&viewTime);
            this->Internal->TimeKeeper->setTime(viewTime);
            }
          }
        toRemove.push_back(element);
        }
      }

    // Finally, remove those xml elements we flagged
    QList<vtkPVXMLElement*>::iterator iter;
    for(iter=toRemove.begin(); iter!=toRemove.end(); iter++)
      {
      proxyElement->RemoveNestedElement(*iter);
      }
    }
  else if (strcmp(proxyElement->GetName(), "Proxy")==0 && 
      strcmp(proxyElement->GetAttribute("group"), "representations")==0 )
    {
    // If a display has been flagged, then it has a non-filter input. 
    // In this case, we are using the XML state to set up its existing
    // representation proxy for this view. Therefore we do not need 
    // to set the display's input and can remove the input property.
    // Also, we remove any sub-proxies from the XML.

    QString id(proxyElement->GetAttribute("id"));
    QList<vtkPVXMLElement*> toRemove;
    QString name;
    if(this->Internal->IdsOfProxyElementsToIgnore.contains(id))
      {
      int count = proxyElement->GetNumberOfNestedElements();
      for(int i=0; i<count; i++)
        {
        vtkPVXMLElement* element = proxyElement->GetNestedElement(i);
        name = element->GetAttribute("name");
        if (element->GetName() == QString("SubProxy") ||
            (element->GetName() == QString("Property") && name == "Input") )
          {
          toRemove.push_back(element);
          }
        }
      // Finally, remove those xml elements we flagged
      QList<vtkPVXMLElement*>::iterator iter;
      for(iter=toRemove.begin(); iter!=toRemove.end(); iter++)
        {
        proxyElement->RemoveNestedElement(*iter);
        }
      }
    }

  return this->Superclass::LoadProxyState(proxyElement, proxy, locator);
}

//-----------------------------------------------------------------------------
void pqLookmarkStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
