/*=========================================================================

   Program: ParaView
   Module:    pqView.cxx

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
#include "pqView.h"

// ParaView Server Manager includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkPVXMLElement.h"

// Qt includes.
#include <QList>
#include <QPointer>
#include <QtDebug>
#include <QTimer>
#include <QWidget>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqProgressManager.h"
#include "pqRepresentation.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"

inline int pqCeil(double val)
{
  if (static_cast<int>(val) == val)
    {
    return static_cast<int>(val);
    }
  return static_cast<int>(vtkMath::Round(val+0.5));
}

template<class T>
inline uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}

class pqViewInternal
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  // List of representation shown by this view.
  QList<QPointer<pqRepresentation> > Representations;

  // The annotation link for linking selections and annotations
  // between views.
  vtkSMSourceProxy* AnnotationLink;

  pqViewInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->AnnotationLink = 0;
    }

  ~pqViewInternal()
    {
    if (this->AnnotationLink)
      {
      this->AnnotationLink->Delete();
      }
    }

  QTimer RenderTimer;
};

//-----------------------------------------------------------------------------
pqView::pqView( const QString& type,
                const QString& group, 
                const QString& name, 
                vtkSMViewProxy* view, 
                pqServer* server, 
                QObject* _parent/*=null*/) : 
  pqProxy(group, name, view, server, _parent)
{
  this->ViewType = type;
  this->Internal = new pqViewInternal();

  // Listen to updates on the Representations property.
  this->Internal->VTKConnect->Connect(
    view->GetProperty("Representations"),
    vtkCommand::ModifiedEvent, this, SLOT(onRepresentationsChanged()));

  // Fire start/end render signals when the underlying proxy
  // fires appropriate events.
  this->Internal->VTKConnect->Connect(view,
    vtkCommand::StartEvent, this, SIGNAL(beginRender()));
  this->Internal->VTKConnect->Connect(view,
    vtkCommand::EndEvent, this, SIGNAL(endRender()));

  this->Internal->RenderTimer.setSingleShot(true);
  this->Internal->RenderTimer.setInterval(1);
  QObject::connect(&this->Internal->RenderTimer, SIGNAL(timeout()),
    this, SLOT(tryRender()));

  pqServerManagerModel* smModel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smModel, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(representationCreated(pqRepresentation*)));

  pqProgressManager* pmManager = pqApplicationCore::instance()->getProgressManager();
  if (pmManager)
    {
    QObject::connect(this, SIGNAL(beginProgress()), pmManager, SLOT(beginProgress()));
    QObject::connect(this, SIGNAL(endProgress()), pmManager, SLOT(endProgress()));
    QObject::connect(this, SIGNAL(progress(const QString&, int)),
      pmManager, SLOT(setProgress(const QString&, int)));
    }
}

//-----------------------------------------------------------------------------
pqView::~pqView()
{
  foreach(pqRepresentation* disp, this->Internal->Representations)
    {
    if (disp)
      {
      disp->setView(0);
      }
    }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqView::getViewProxy() const
{
  return vtkSMViewProxy::SafeDownCast(this->getProxy());
}

vtkView* pqView::getClientSideView() const
{
  return 0;
}

//-----------------------------------------------------------------------------
void pqView::initialize()
{
  this->Superclass::initialize();

  // If the render module already has some representations in it when it is
  // registered, this method will detect them and sync the GUI state with the 
  // SM state.
  this->onRepresentationsChanged();
}

//-----------------------------------------------------------------------------
void pqView::render()
{
  this->Internal->RenderTimer.start();
}

//-----------------------------------------------------------------------------
void pqView::tryRender()
{
  if (this->getProxy()->GetSession()->GetPendingProgress())
    {
    this->render();
    }
  else
    {
    this->forceRender();
    }
}

//-----------------------------------------------------------------------------
void pqView::forceRender()
{
  vtkSMViewProxy* view = this->getViewProxy();
  if (view)
    {
    view->GetSession()->PrepareProgress();
    view->StillRender();
    view->GetSession()->CleanupPendingProgress();
    }
}

//-----------------------------------------------------------------------------
bool pqView::hasRepresentation(pqRepresentation* repr) const
{
  return this->Internal->Representations.contains(repr);
}

//-----------------------------------------------------------------------------
int pqView::getNumberOfRepresentations() const
{
  return this->Internal->Representations.size();
}

//-----------------------------------------------------------------------------
int pqView::getNumberOfVisibleRepresentations() const
{
  int count = 0;
  for (int i=0; i<this->Internal->Representations.size(); i++)
    {
    pqRepresentation *repr = this->Internal->Representations[i];
    if (repr && repr->isVisible())
      {
      count++;
      }
    }
  return count;
}

//-----------------------------------------------------------------------------
pqRepresentation* pqView::getRepresentation(int index) const
{
  if(index >= 0 && index < this->Internal->Representations.size())
    {
    return this->Internal->Representations[index];
    }

  return 0;
}

//-----------------------------------------------------------------------------
QList<pqRepresentation*> pqView::getRepresentations() const
{
  QList<pqRepresentation*> list;
  foreach (pqRepresentation* disp, this->Internal->Representations)
    {
    if (disp)
      {
      list.push_back(disp);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
void pqView::onRepresentationsChanged()
{
  // Determine what changed. Add the new Representations and remove the old
  // ones. Make sure new Representations have a reference to this render module.
  // Remove the reference to this render module in the removed Representations.
  QList<QPointer<pqRepresentation> > currentReprs;
  vtkSMProxyProperty* prop = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Representations"));
  pqServerManagerModel* smModel = 
    pqApplicationCore::instance()->getServerManagerModel();

  unsigned int max = prop->GetNumberOfProxies();
  for (unsigned int cc=0; cc < max; ++cc)
    {
    vtkSMProxy* proxy = prop->GetProxy(cc);
    if (!proxy)
      {
      continue;
      }
    pqRepresentation* repr =  smModel->findItem<pqRepresentation*>(proxy);
    if (!repr)
      {
      continue;
      }
    currentReprs.append(QPointer<pqRepresentation>(repr));
    if(!this->Internal->Representations.contains(repr))
      {
      // Update the render module pointer in the repr.
      repr->setView(this);
      this->Internal->Representations.append(QPointer<pqRepresentation>(repr));
      QObject::connect(repr, SIGNAL(visibilityChanged(bool)),
        this, SLOT(onRepresentationVisibilityChanged(bool)));
      emit this->representationAdded(repr);
      emit this->representationVisibilityChanged(repr, repr->isVisible());
      }
    }

  QList<QPointer<pqRepresentation> >::Iterator iter =
      this->Internal->Representations.begin();
  while(iter != this->Internal->Representations.end())
    {
    if(*iter && !currentReprs.contains(*iter))
      {
      pqRepresentation* repr = (*iter);
      // Remove the render module pointer from the repr.
      repr->setView(0);
      iter = this->Internal->Representations.erase(iter);
      QObject::disconnect(repr, 0, this, 0);
      emit this->representationVisibilityChanged(repr, false);
      emit this->representationRemoved(repr);
      }
    else
      {
      ++iter;
      }
    }
}

//-----------------------------------------------------------------------------
void pqView::representationCreated(pqRepresentation* repr)
{
  vtkSMProxyProperty* prop = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Representations"));
  if (prop->IsProxyAdded(repr->getProxy()))
    {
    repr->setView(this);
    this->Internal->Representations.append(repr);
    QObject::connect(repr, SIGNAL(visibilityChanged(bool)),
      this, SLOT(onRepresentationVisibilityChanged(bool)));
    emit this->representationAdded(repr);
    }
}

//-----------------------------------------------------------------------------
void pqView::onRepresentationVisibilityChanged(bool visible)
{
  pqRepresentation* disp = qobject_cast<pqRepresentation*>(this->sender());
  if (disp)
    {
    emit this->representationVisibilityChanged(disp, visible);
    }
}

//-----------------------------------------------------------------------------
QSize pqView::getSize()
{
  QWidget* widget = this->getWidget();
  return widget? widget->size(): QSize(0, 0);
}

//-----------------------------------------------------------------------------
int pqView::computeMagnification(const QSize& fullsize, QSize& viewsize)
{
  int magnification = 1;

  // If fullsize > viewsize, then magnification is involved.
  int temp = ::pqCeil(fullsize.width()/static_cast<double>(viewsize.width()));
  magnification = (temp> magnification)? temp: magnification;

  temp = ::pqCeil(fullsize.height()/static_cast<double>(viewsize.height()));
  magnification = (temp > magnification)? temp : magnification;

  viewsize = fullsize/magnification;
  return magnification;
}

//-----------------------------------------------------------------------------
vtkImageData* pqView::captureImage(const QSize& fullsize)
{
  QWidget* vtkwidget = this->getWidget();
  QSize cursize = vtkwidget->size();
  QSize newsize = cursize;
  int magnification = 1;
  if (fullsize.isValid())
    {
    magnification = pqView::computeMagnification(fullsize, newsize);
    vtkwidget->resize(newsize);
    }
  this->render();

  vtkImageData* vtkimage = this->captureImage(magnification);
  if (fullsize.isValid())
    {
    vtkwidget->resize(newsize);
    vtkwidget->resize(cursize);
    this->render();
    }
  return vtkimage;
}

//-----------------------------------------------------------------------------
bool pqView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort ? opPort->getSource() : 0;
  vtkSMSourceProxy* sourceProxy = source ? vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort||
     !source ||
     opPort->getServer()->GetConnectionID() != this->getServer()->GetConnectionID() ||
     !sourceProxy ||
     sourceProxy->GetOutputPortsCreated() == 0)
    {
    return false;
    }

  vtkPVXMLElement* hints = sourceProxy->GetHints();
  if (hints)
    {
    unsigned int elementCount = hints->GetNumberOfNestedElements();
    for(unsigned int i = 0; i < elementCount; i++)
      {
      vtkPVXMLElement* child = hints->GetNestedElement(i);
      const char *childName = child->GetName();

      if(childName && strcmp(childName, "DefaultRepresentations") == 0)
        {
        unsigned int defaultRepsCount = child->GetNumberOfNestedElements();
        for(unsigned int j = 0; j < defaultRepsCount; j++)
          {
          vtkPVXMLElement *defaultRep = child->GetNestedElement(j);

          const char *defaultRepViewType = defaultRep->GetAttribute("view");
          if(defaultRepViewType &&
             this->ViewType == defaultRepViewType)
            {
            return true;
            }
          }
        }
      }
    }

  return false;
}

//-----------------------------------------------------------------------------
void pqView::setAnnotationLink(vtkSMSourceProxy* link)
{
  if (this->Internal->AnnotationLink != link)
    {
    vtkSMSourceProxy* tempSGMacroVar = this->Internal->AnnotationLink;
    this->Internal->AnnotationLink = link;
    if (this->Internal->AnnotationLink != NULL) { this->Internal->AnnotationLink->Register(0); }
    if (tempSGMacroVar != NULL)
      {
      tempSGMacroVar->UnRegister(0);
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMSourceProxy* pqView::getAnnotationLink()
{
  return this->Internal->AnnotationLink;
}
