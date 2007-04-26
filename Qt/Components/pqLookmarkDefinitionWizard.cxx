/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkDefinitionWizard.cxx

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

/// \file pqLookmarkDefinitionWizard.cxx
/// \date 6/19/2006

#include "pqLookmarkDefinitionWizard.h"
#include "ui_pqLookmarkDefinitionWizard.h"

#include "pqLookmarkManagerModel.h"
#include "pqLookmarkModel.h"
#include "pqPipelineSource.h"
#include "pqRenderViewModule.h"
#include "pqGenericViewModule.h"
#include "pqPipelineFilter.h"
#include "pqConsumerDisplay.h"
#include "pqDisplay.h"
#include "pqPipelineModel.h"
#include "pqFlatTreeView.h"
#include "pqApplicationCore.h"
#include "pqPipelineBrowser.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"
#include "pqImageUtil.h"

#include <QMessageBox>
#include <QModelIndex>
#include <QStringList>
#include <QList>
#include <QLineEdit>
#include <QImage>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QtDebug>

#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkImageData.h"

#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "QVTKWidget.h"
#include "vtkPVXMLElement.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"

#include "assert.h"

class pqLookmarkDefinitionWizardForm : public Ui::pqLookmarkDefinitionWizard{};


pqLookmarkDefinitionWizard::pqLookmarkDefinitionWizard(pqLookmarkManagerModel *model,
    pqGenericViewModule *viewModule, QWidget *widgetParent)
  : QDialog(widgetParent)
{ 
  this->Model = model;
  this->OverwriteOK = false;
  this->ViewModule = viewModule;
  this->PipelineHierarchy = vtkPVXMLElement::New();
  this->PipelineHierarchy->SetName("PipelineHierarchy");
  this->Form = new pqLookmarkDefinitionWizardForm();
  this->Form->setupUi(this);

  this->createPipelinePreview();

  // Listen to the button click events.
  QObject::connect(this->Form->CancelButton, SIGNAL(clicked()),
      this, SLOT(reject()));
  QObject::connect(this->Form->FinishButton, SIGNAL(clicked()),
      this, SLOT(finishWizard()));

  // Listen for name changes.
  QObject::connect(this->Form->LookmarkName,
      SIGNAL(textEdited(const QString &)),
      this, SLOT(clearNameOverwrite(const QString &)));

  this->Form->LookmarkName->setFocus();
}

pqLookmarkDefinitionWizard::~pqLookmarkDefinitionWizard()
{
  if(this->Form)
    {
    delete this->Form;
    }

  if(this->PipelineHierarchy)
    {
    this->PipelineHierarchy->Delete();
    }

  if(this->PipelineModel)
    {
    delete this->PipelineModel;
    }

}


void pqLookmarkDefinitionWizard::createPipelinePreview()
{
  // Make a copy of the model for the user to select sources.
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  this->PipelineModel = new pqPipelineModel(*smModel);
  this->PipelineModel->setEditable(false);

  // Save visible displays and their sources, also any display/source pair upstream from a visible one in the pipeline:
  QList<pqDisplay*> displays = this->ViewModule->getDisplays();
  QList<pqDisplay *>::Iterator iter;
  pqConsumerDisplay *consDisp;
  vtkCollection *proxies = vtkCollection::New();
  for(iter = displays.begin(); iter != displays.end(); ++iter)
    {
    // if a display is visible, add it, its pipeline source, and all its upstream inputs to the collection of proxies
    if( (consDisp = dynamic_cast<pqConsumerDisplay*>(*iter)))
      {
      if(consDisp->isVisible() )
        {
        this->addToProxyCollection(consDisp->getInput(),proxies);
        }
      }
    }

  pqPipelineSource *src;
  for(int i=smModel->getNumberOfSources()-1; i>=0; i--)
    {
    src = smModel->getPQSource(i);
    if( src )
      {
      if(!proxies->IsItemPresent(src->getProxy()))
        {
        this->PipelineModel->removeSource(src);
        }
      }
    }

  // assume there's only one server for now
  pqServer *server = smModel->getServerByIndex(0);
  
  // Populate the xml elements with the name and type of each pipeline item
  this->addChildItems(this->PipelineModel->getIndexFor(server),this->PipelineHierarchy);

  proxies->Delete();
}



void pqLookmarkDefinitionWizard::addChildItems(const QModelIndex &index, vtkPVXMLElement *elem)
{
  // Get the number of children from the model. The model may
  // delay loading information. Force the model to load the
  // child information if the item can't be made expandable.
  if(this->PipelineModel->canFetchMore(index))
    {
    this->PipelineModel->fetchMore(index);
    }

  int count = this->PipelineModel->rowCount(index);

  // Set up the parent and model index for each added child.
  // The model's hierarchical data should be in column 0.
  QModelIndex childIndex;
  pqServerManagerModelItem *smItem;
  for(int i = 0; i < count; i++)
    {
    childIndex = this->PipelineModel->index(i, 0, index);
    if(childIndex.isValid())
      {
      vtkPVXMLElement *childElem = vtkPVXMLElement::New();
      QString name = this->PipelineModel->data(childIndex,Qt::EditRole).toString();
      smItem = this->PipelineModel->getItemFor(childIndex);
      if(dynamic_cast<pqServer*>(smItem))
        {
        childElem->SetName("Server");
        }
      else if(dynamic_cast<pqPipelineFilter*>(smItem))
        {
        childElem->SetName("Filter");
        }
      else
        {
        childElem->SetName("Source");
        }
      childElem->SetAttribute("Name",name.toAscii().data());
      elem->AddNestedElement(childElem);
      this->addChildItems(childIndex,childElem);
      childElem->Delete();
      }
    }
}

void pqLookmarkDefinitionWizard::createLookmark()
{
  if(this->Form->LookmarkName->text().isEmpty())
    {
    return;
    }

  pqRenderViewModule* renderModule = qobject_cast<pqRenderViewModule*>(this->ViewModule);
  if(!renderModule)
    {
    qCritical() << "Can only create lookmarks of render views at this time.";
    return;
    }

  vtkSMRenderModuleProxy *smRen = renderModule->getRenderModuleProxy();
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();

  // Save a screenshot of the view to store with the lookmark
  QWidget* w = renderModule->getWidget();
  QSize old = w->size();
  w->resize(150,150);
  vtkImageData* imageData = smRen->CaptureWindow(1);
  w->resize(old);
  QImage image;
  pqImageUtil::fromImageData(imageData, image);
  imageData->Delete();
  
  vtkCollection *proxies = vtkCollection::New();
  // Save visible displays and their sources, also any display/source pair 
  // upstream from a visible one in the pipeline:
  QList<pqDisplay*> displays = renderModule->getDisplays();
  QList<pqDisplay *>::Iterator iter;
  pqConsumerDisplay *consDisp;
  for(iter = displays.begin(); iter != displays.end(); ++iter)
    {
    // if a display is visible, add it, its pipeline source, and all its 
    //  upstream inputs to the collection of proxies
    if( (consDisp = dynamic_cast<pqConsumerDisplay*>(*iter)))
      {
      if(consDisp->isVisible() )
        {
        this->addToProxyCollection(consDisp->getInput(),proxies);
        }
      }
    }

  // Get the XML representation of the contents of "proxies", as well as 
  // their referred proxies
  vtkPVXMLElement *stateElement = proxyManager->SaveState(proxies, true);

  // Collect all referred (proxy property) proxies of the render module EXCEPT 
  // its "Displays" These have been handled separately.
  proxies->RemoveAllItems();
  vtkSmartPointer<vtkSMPropertyIterator> pIter;
  pIter.TakeReference(smRen->NewPropertyIterator());
  for (pIter->Begin(); !pIter->IsAtEnd(); pIter->Next())
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      pIter->GetProperty());

    for (unsigned int cc=0; pp && (pp->GetNumberOfProxies() > cc); cc++)
      {
      vtkSMProxy* referredProxy = pp->GetProxy(cc);
      if (referredProxy && QString::compare(referredProxy->GetXMLGroup(),"displays")!=0)
        {
        proxies->AddItem(referredProxy);
        }
      }
    }
  
  // Save all referred proxies of the render module's non-display referred proxies
  vtkPVXMLElement* childElement = proxyManager->SaveState(proxies, true);
  unsigned int cc;
  for (cc=0; cc < childElement->GetNumberOfNestedElements(); cc++)
    {
    stateElement->AddNestedElement(childElement->GetNestedElement(cc));
    }
  childElement->Delete();

  // Now add the render module, but don't save its referred proxies, because 
  // we've dealt with them separately
  proxies->RemoveAllItems();
  proxies->AddItem(smRen);
  childElement = proxyManager->SaveState(proxies, false);
  for (cc=0; cc < childElement->GetNumberOfNestedElements(); cc++)
    {
    stateElement->AddNestedElement(childElement->GetNestedElement(cc));
    }
  childElement->Delete();

  ostrstream stateString;
  stateElement->PrintXML(stateString,vtkIndent(1));
  stateString << ends;
  stateString.freeze();

  QString lmkState = stateString.str();

  // Create a lookmark with the given name, image, and state
  pqLookmarkModel *lmkModel = new pqLookmarkModel(this->Form->LookmarkName->text(), lmkState);
  lmkModel->setDescription(this->Form->LookmarkComments->toPlainText());
  lmkModel->setIcon(image);
  //lmkModel->setPipelinePreview(pipeline);
  lmkModel->setPipelineHierarchy(this->PipelineHierarchy);

  //this->Lookmarks->addLookmark(lmkModel);
  this->Model->addLookmark(lmkModel);
  //pqApplicationCore::instance()->getLookmarkManagerModel()->addLookmark(lmkModel);

  proxies->Delete();
  stateElement->Delete();
}



void pqLookmarkDefinitionWizard::addToProxyCollection(pqPipelineSource *src, vtkCollection *proxies)
{
  pqPipelineFilter *filter;
  //pqPipelineSource *src;
  pqPipelineSource *input;
  pqConsumerDisplay *disp;

//  if(!disp || !disp->getInput())
//    {
//    return;
//    }

  //src = disp->getInput();

  // Add this display/source's proxy to the list if it has not already been added

  if(!proxies->IsItemPresent(src->getProxy()))
    {
    // The source may or may not have a display in the view
    if( (disp = src->getDisplay(this->ViewModule)) )
      {
      proxies->AddItem(disp->getProxy());
      }
    proxies->AddItem(src->getProxy());
    }

  // If this is a filter, recurse on its inputs
  if( (filter = dynamic_cast<pqPipelineFilter*>(src)) )
    {
    for(int i=0; i<filter->getInputCount(); i++)
      {
      input = filter->getInput(i);
      this->addToProxyCollection(input,proxies);
      }
    }
}

//-----------------------------------------------------------------------------
bool pqLookmarkDefinitionWizard::validateLookmarkName()
{
  // Make sure the user has entered a name for the lookmark.
  QString lookmarkName = this->Form->LookmarkName->text();
  if(lookmarkName.isEmpty())
    {
    QMessageBox::warning(this, "No Name",
        "The lookmark name field is empty.\n"
        "Please enter a unique name for the lookmark.",
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->LookmarkName->setFocus();
    return false;
    }

  // Make sure the name is unique.
  if(!this->OverwriteOK)
    {
    if(this->Model->getLookmark(lookmarkName))
      {
      int button = QMessageBox::warning(this, "Duplicate Name",
          "The lookmark name already exists.\n"
          "Do you want to overwrite the lookmark?",
          QMessageBox::Yes | QMessageBox::Default, QMessageBox::No);
      if(button != QMessageBox::Yes)
        {
        return false;
        }

      this->Model->removeLookmark(lookmarkName);
      this->OverwriteOK = true;
      }
    }

  return true;
}

void pqLookmarkDefinitionWizard::finishWizard()
{
  // Make sure the name has been entered and is unique.
  if(this->validateLookmarkName())
    {
    this->accept();
    }
}

void pqLookmarkDefinitionWizard::clearNameOverwrite(const QString &)
{
  this->OverwriteOK = false;
}

