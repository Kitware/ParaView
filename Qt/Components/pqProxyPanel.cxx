/*=========================================================================

   Program: ParaView
   Module:    pqProxyPanel.cxx

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

=========================================================================*/

// this include
#include "pqProxyPanel.h"

// Qt includes
#include <QApplication>
#include <QStyle>
#include <QStyleOption>

// VTK includes
#include "QVTKWidget.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"

// ParaView Server Manager includes
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"

// ParaView includes
#include "pqServerManagerObserver.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"
#include "pqRenderViewModule.h"

//-----------------------------------------------------------------------------

class pqProxyPanel::pqImplementation
{
public:
  pqImplementation(pqProxy* refProxy, vtkSMProxy* pxy) :
    ReferenceProxy(refProxy), Proxy(pxy)
  {
  this->RenderModule = NULL;
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->InformationObsolete = true;
  this->Selected = false;
  }
  
  ~pqImplementation()
  {
    delete this->PropertyManager;
  }
  
  pqProxy* ReferenceProxy;
  vtkSmartPointer<vtkSMProxy> Proxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqPropertyManager* PropertyManager;
  QPointer<pqRenderViewModule> RenderModule;

  // Flag indicating to the best of our knowledge, the information properties
  // and domains are not up-to-date since the proxy was modified since the last
  // time we updated the properties and their domains. This is just a guess, 
  // to reduce number of updates information calls.
  bool InformationObsolete;
 
  // Indicates if the panel is currently selected.
  bool Selected;
};

//-----------------------------------------------------------------------------
pqProxyPanel::pqProxyPanel(pqProxy* refProxy, vtkSMProxy* pxy, QWidget* p) :
  QWidget(p),
  Implementation(new pqImplementation(refProxy, pxy))
{
  // Just make sure that the proxy is setup properly.
  this->Implementation->Proxy->UpdateVTKObjects();

  this->Implementation->PropertyManager = new pqPropertyManager(this);

  QObject::connect(this->Implementation->PropertyManager,
                   SIGNAL(modified()),
                   this, SLOT(setModified()));

  this->Implementation->VTKConnect->Connect(
    this->Implementation->Proxy, vtkCommand::ModifiedEvent,
    this, SLOT(proxyModifiedEvent()));
}

//-----------------------------------------------------------------------------
pqProxyPanel::~pqProxyPanel()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqProxyPanel::proxy() const
{
  return this->Implementation->Proxy;
}

//-----------------------------------------------------------------------------
pqProxy* pqProxyPanel::referenceProxy() const
{
  return this->Implementation->ReferenceProxy;
}

//-----------------------------------------------------------------------------
pqPropertyManager* pqProxyPanel::propertyManager()
{
  return this->Implementation->PropertyManager;
}

//-----------------------------------------------------------------------------
pqRenderViewModule* pqProxyPanel::renderModule() const
{
  return this->Implementation->RenderModule;
}

//-----------------------------------------------------------------------------
QSize pqProxyPanel::sizeHint() const
{
  // return a size hint that would reasonably fit several properties
  ensurePolished();
  QFontMetrics fm(font());
  int h = qMax(fm.lineSpacing(), 14);
  int w = fm.width('x') * 25;
  QStyleOptionFrame opt;
  opt.rect = rect();
  opt.palette = palette();
  opt.state = QStyle::State_None;
  return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                    expandedTo(QApplication::globalStrut()), 
                                    this));
}

//-----------------------------------------------------------------------------
/// accept the changes made to the properties
/// changes will be propogated down to the server manager
void pqProxyPanel::accept()
{
  this->Implementation->PropertyManager->accept();
  this->Implementation->ReferenceProxy->setModified(false);

  if (this->Implementation->Selected)
    {
    this->updateInformationAndDomains();
    }

  emit this->onaccept();
}

//-----------------------------------------------------------------------------
/// reset the changes made
/// editor will query properties from the server manager
void pqProxyPanel::reset()
{
  this->Implementation->Proxy->UpdatePropertyInformation();
  this->Implementation->PropertyManager->reject();
  this->Implementation->ReferenceProxy->setModified(false);
  emit this->onreset();
}

//-----------------------------------------------------------------------------
void pqProxyPanel::select()
{
  this->Implementation->Selected = true;

  this->updateInformationAndDomains();
  emit this->onselect();
}

//-----------------------------------------------------------------------------
void pqProxyPanel::deselect()
{
  this->Implementation->Selected = false;
  emit this->ondeselect();
}
  
//-----------------------------------------------------------------------------
void pqProxyPanel::setRenderModule(pqRenderViewModule* rm)
{
  if(this->Implementation->RenderModule == rm)
    {
    return;
    }

  this->Implementation->RenderModule = rm;
  emit this->renderModuleChanged(this->Implementation->RenderModule);
}

//-----------------------------------------------------------------------------
void pqProxyPanel::setModified()
{
  this->Implementation->ReferenceProxy->setModified(true);
  emit this->modified();
}

//-----------------------------------------------------------------------------
// Called when vtkSMProxy fires vtkCommand::ModifiedEvent which implies that
// the proxy was modified. If that's the case,  the information properties
// need to be updated. We mark them as obsolete so that next
// time the panel becomes active (or is accepted when it is active),
// we'll refresh the information properties and domains.
// Note thate vtkCommand::ModifiedEvent is fired by a proxy when it is
// updated or anyone upstream for it is updated.
void pqProxyPanel::proxyModifiedEvent()
{
  this->Implementation->InformationObsolete = true;
}

//-----------------------------------------------------------------------------
// Update information properties and domains. Since this is not a
// particularly fast operation, we update the information and domains
// only when the panel is selected or an already active panel is 
// accepted. 
void pqProxyPanel::updateInformationAndDomains()
{
  if (this->Implementation->InformationObsolete)
    {
    vtkSMSourceProxy* sp;
    vtkSMCompoundProxy* cp;
    sp = vtkSMSourceProxy::SafeDownCast(this->Implementation->Proxy);
    cp = vtkSMCompoundProxy::SafeDownCast(this->Implementation->Proxy);
    if(sp)
      {
      sp->UpdatePipelineInformation();
      }
    else if(cp)
      {
      // TODO --  this is a workaround for a bug in the server manager
      //          fix it the right way
      //          does that mean calling UpdatePipelineInformation() above
      //          for a source proxy goes away?
      int num = cp->GetNumberOfProxies();
      for(int i=0; i<num; i++)
        {
        sp = vtkSMSourceProxy::SafeDownCast(cp->GetProxy(i));
        if(sp)
          {
          sp->UpdatePipelineInformation();
          }
        }
      }
    else
      {
      this->Implementation->Proxy->UpdatePropertyInformation();
      }
    vtkSMProperty* inputProp = this->Implementation->Proxy->GetProperty("Input");
    if (inputProp)
      {
      inputProp->UpdateDependentDomains();
      }
    this->Implementation->InformationObsolete = false;
    }
}
