/*=========================================================================

   Program: ParaView
   Module:  vtkVRQueueHandler.cxx

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
#include "pqVRQueueHandler.h"

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVRGrabWorldStyle.h"
#include "vtkVRInteractorStyle.h"
#include "vtkVRInteractorStyleFactory.h"
#include "vtkVRQueue.h"
#include "vtkVRTrackStyle.h"
#include "vtkWeakPointer.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqView.h"

#include <QList>
#include <QTimer>
#include <QtDebug>

#include <cassert>

class pqVRQueueHandler::pqInternals
{
public:
  vtkCollection* Styles;
  vtkWeakPointer<vtkVRQueue> Queue;
  QTimer Timer;
};

//----------------------------------------------------------------------------
QPointer<pqVRQueueHandler> pqVRQueueHandler::Instance;

//----------------------------------------------------------------------------
pqVRQueueHandler* pqVRQueueHandler::instance()
{
  return pqVRQueueHandler::Instance;
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::setInstance(pqVRQueueHandler* inst)
{
  pqVRQueueHandler::Instance = inst;
}

//----------------------------------------------------------------------------
pqVRQueueHandler::pqVRQueueHandler(vtkVRQueue* queue, QObject* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();
  this->Internals->Styles = vtkCollection::New();
  this->Internals->Queue = queue;
  this->Internals->Timer.setInterval(1);
  this->Internals->Timer.setSingleShot(true);
  QObject::connect(&this->Internals->Timer, SIGNAL(timeout()), this, SLOT(processEvents()));

  QObject::connect(pqApplicationCore::instance(),
    SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)), this,
    SLOT(configureStyles(vtkPVXMLElement*, vtkSMProxyLocator*)));
  QObject::connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(saveStylesConfiguration(vtkPVXMLElement*)));
}

//----------------------------------------------------------------------------
pqVRQueueHandler::~pqVRQueueHandler()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::add(vtkVRInteractorStyle* style)
{
  if (!this->Internals->Styles->IsItemPresent(style))
  {
    this->Internals->Styles->AddItem(style);
    emit stylesChanged();
    return;
  }
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::remove(vtkVRInteractorStyle* style)
{
  this->Internals->Styles->RemoveItem(style);
  emit stylesChanged();
}

//----------------------------------------------------------------------public
void pqVRQueueHandler::clear()
{
  if (this->Internals->Styles->GetNumberOfItems() != 0)
  {
    this->Internals->Styles->RemoveAllItems();
    emit stylesChanged();
  }
}

//----------------------------------------------------------------------public
QList<vtkVRInteractorStyle*> pqVRQueueHandler::styles()
{
  vtkVRInteractorStyle* style;
  QList<vtkVRInteractorStyle*> result;
  for (this->Internals->Styles->InitTraversal();
       (style =
           vtkVRInteractorStyle::SafeDownCast(this->Internals->Styles->GetNextItemAsObject()));)
  {
    result << style;
  }

  return result;
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::start()
{
  this->Internals->Timer.start();
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::stop()
{
  this->Internals->Timer.stop();
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::processEvents()
{
  assert(this->Internals->Queue != NULL);
  std::queue<vtkVREventData> events;
  this->Internals->Queue->TryDequeue(events);

  // Loop through the event queue and pass events to InteractorStyles
  while (!events.empty())
  {
    vtkVREventData data = events.front();
    events.pop();
    vtkVRInteractorStyle* style;
    for (this->Internals->Styles->InitTraversal();
         (style =
             vtkVRInteractorStyle::SafeDownCast(this->Internals->Styles->GetNextItemAsObject()));)
    {
      if (style->HandleEvent(data))
      {
        break;
      }
    }
  }

  // There should be an explicit update for each handler. Otherwise the server
  // side updates will not happen
  vtkVRInteractorStyle* style;
  for (this->Internals->Styles->InitTraversal();
       (style =
           vtkVRInteractorStyle::SafeDownCast(this->Internals->Styles->GetNextItemAsObject()));)
  {
    if (style->Update())
    {
      break;
    }
  }

  this->render();

  // since timer is single-shot we start it again.
  this->Internals->Timer.start();
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::render()
{
  vtkSMRenderViewProxy* proxy = 0;
  pqView* view = 0;
  view = pqActiveObjects::instance().activeView();
  if (view)
  {
    proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy)
    {
      proxy->StillRender();
    }
  }
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::configureStyles(vtkPVXMLElement* xml, vtkSMProxyLocator* locator)
{
  if (!xml)
  {
    return;
  }

  if (xml->GetName() && strcmp(xml->GetName(), "VRInteractorStyles") == 0)
  {
    this->clear();
    vtkVRInteractorStyleFactory* factory = vtkVRInteractorStyleFactory::GetInstance();
    for (unsigned cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
    {
      vtkPVXMLElement* child = xml->GetNestedElement(cc);
      if (child && child->GetName() && strcmp(child->GetName(), "Style") == 0)
      {
        const char* class_name = child->GetAttributeOrEmpty("class");
        vtkVRInteractorStyle* style = factory->NewInteractorStyleFromClassName(class_name);
        if (style)
        {
          if (style->Configure(child, locator))
          {
            this->add(style);
          }
          else
          {
            style->Delete();
          }
        }
        else
        {
          qWarning() << "Unknown interactor style: \"" << class_name << "\"";
        }
      }
    }
  }
  else
  {
    this->configureStyles(xml->FindNestedElementByName("VRInteractorStyles"), locator);
  }

  emit stylesChanged();
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::saveStylesConfiguration(vtkPVXMLElement* root)
{
  assert(root != NULL);

  vtkPVXMLElement* tempParent = vtkPVXMLElement::New();
  tempParent->SetName("VRInteractorStyles");
  vtkVRInteractorStyle* style;
  for (this->Internals->Styles->InitTraversal();
       (style =
           vtkVRInteractorStyle::SafeDownCast(this->Internals->Styles->GetNextItemAsObject()));)
  {
    vtkPVXMLElement* child = style->SaveConfiguration();
    if (child)
    {
      tempParent->AddNestedElement(child);
      child->Delete();
    }
  }
  root->AddNestedElement(tempParent);
  tempParent->Delete();
}
