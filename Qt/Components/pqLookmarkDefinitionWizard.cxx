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

#include "pqLookmarkBrowserModel.h"
#include "pqPipelineSource.h"
#include "pqRenderViewModule.h"
#include "pqPipelineFilter.h"
#include "pqConsumerDisplay.h"
#include "pqDisplay.h"

#include <QMessageBox>
#include <QModelIndex>
#include <QStringList>
#include <QList>
#include <QLineEdit>
#include <QImage>

#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPropertyIterator.h"

#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkCollection.h"
#include "QVTKWidget.h"
#include "vtkPVXMLElement.h"

#include "assert.h"

class pqLookmarkDefinitionWizardForm :
    public Ui::pqLookmarkDefinitionWizard
{
public:
  QStringList ListNames; ///< Used to make sure names are unique.
};


pqLookmarkDefinitionWizard::pqLookmarkDefinitionWizard(
    pqRenderViewModule *renderModule, pqLookmarkBrowserModel *model, QWidget *widgetParent)
  : QDialog(widgetParent)
{ 
  this->OverwriteOK = false;
  this->RenderModule = renderModule;
  this->Lookmarks = model;
  this->Form = new pqLookmarkDefinitionWizardForm();
  this->Form->setupUi(this);

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
}

void pqLookmarkDefinitionWizard::createLookmark()
{
  if(this->Form->LookmarkName->text().isEmpty())
    {
    return;
    }

  vtkSMRenderModuleProxy *smRen = this->RenderModule->getRenderModuleProxy();
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();

  // Save a screenshot of the view to store with the lookmark
  // FIXME: Is there a better way to do this? I tried using the vtkWindowToImageFilter but I don't know how to convert its output vtkImageData to an image format that QImage will understand
  QVTKWidget* const widget = qobject_cast<QVTKWidget*>(this->RenderModule->getWidget());
  assert(widget);
  this->hide();
  widget->GetRenderWindow()->Render();
  this->RenderModule->saveImage(48,48,"tempLookmarkImage.png");
  QImage image("tempLookmarkImage.png","PNG");
  remove("tempLookmarkImage.png");

  vtkCollection *proxies = vtkCollection::New();
  vtkPVXMLElement *stateElement = vtkPVXMLElement::New();
  stateElement->SetName("ServerManagerState");

  // Save visible displays and their sources, also any display/source pair upstream from a visible one in the pipeline:

  QList<pqDisplay*> displays = this->RenderModule->getDisplays();
  QList<pqDisplay *>::Iterator iter;
  pqConsumerDisplay *consDisp;
  for(iter = displays.begin(); iter != displays.end(); ++iter)
    {
    // if a display is visible, add it, its pipeline source, and all its upstream inputs to the collection of proxies
    if( (consDisp = dynamic_cast<pqConsumerDisplay*>(*iter)) && (*iter)->isVisible() )
      {
      this->addToProxyCollection(consDisp,proxies);
      }
    }

  // Get the XML representation of the contents of "proxies", as well as their referred proxies
  proxyManager->SaveState(stateElement, proxies, 1);

  // Collect all referred (proxy property) proxies of the render module EXCEPT its "Displays" These have been handled separately.
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
  proxyManager->SaveState(stateElement, proxies, 1);

  // Now add the render module, but don't save its referred proxies, because we've dealt with them separately
  proxies->RemoveAllItems();
  proxies->AddItem(smRen);
  proxyManager->SaveState(stateElement, proxies, 0);

  // Now, stateElement is the root of a complete lookmark state, convert it to a QString that will be stored with the lookmark
  ostrstream os;
  stateElement->PrintXML(os,vtkIndent(0));
  os << ends;
  os.rdbuf()->freeze(0);
  QString state = os.str();
  state.remove('\n');
  state.remove('\t');

  // Create a lookmark with the given name, image, and state
  this->Lookmarks->addLookmark(this->Form->LookmarkName->text(),image,state);

  proxies->Delete();
  stateElement->Delete();
}



void pqLookmarkDefinitionWizard::addToProxyCollection(pqConsumerDisplay *disp, vtkCollection *proxies)
{
  pqPipelineFilter *filter;
  pqPipelineSource *src;
  pqPipelineSource *input;

  if(!disp || !disp->getInput())
    {
    return;
    }

  src = disp->getInput();

  // Add this display/source's proxy to the list if it has not already been added
  if(!proxies->IsItemPresent(disp->getProxy()))
    {
    proxies->AddItem(disp->getProxy());
    proxies->AddItem(src->getProxy());
    }
  
  // If this is a filter, recurse on its inputs
  if( (filter = dynamic_cast<pqPipelineFilter*>(src)) )
    {
    for(int i=0; i<filter->getInputCount(); i++)
      {
      input = filter->getInput(i);
      this->addToProxyCollection(input->getDisplay(this->RenderModule),proxies);
      }
    }
}

QString pqLookmarkDefinitionWizard::getLookmarkName() const
{
  return this->Form->LookmarkName->text();
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
  if(this->Lookmarks && !this->OverwriteOK)
    {
    QModelIndex index = this->Lookmarks->getIndexFor(lookmarkName);
    if(index.isValid())
      {
      int button = QMessageBox::warning(this, "Duplicate Name",
          "The lookmark name already exists.\n"
          "Do you want to overwrite the lookmark?",
          QMessageBox::Yes | QMessageBox::Default, QMessageBox::No);
      if(button != QMessageBox::Yes)
        {
        return false;
        }

      this->Lookmarks->removeLookmark(lookmarkName);
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

