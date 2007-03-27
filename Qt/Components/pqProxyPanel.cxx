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

// ParaView Server Manager includes
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"

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
  }
  
  ~pqImplementation()
  {
    delete this->PropertyManager;
  }
  
  pqProxy* ReferenceProxy;
  vtkSmartPointer<vtkSMProxy> Proxy;
  pqPropertyManager* PropertyManager;
  QPointer<pqRenderViewModule> RenderModule;
};

//-----------------------------------------------------------------------------
pqProxyPanel::pqProxyPanel(pqProxy* refProxy, vtkSMProxy* pxy, QWidget* p) :
  QWidget(p),
  Implementation(new pqImplementation(refProxy, pxy))
{
  this->Implementation->PropertyManager = new pqPropertyManager(this);

  QObject::connect(this->Implementation->PropertyManager,
                   SIGNAL(modified()),
                   this, SLOT(setModified()));

  this->Implementation->Proxy->UpdateVTKObjects();
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
  this->Implementation->Proxy->UpdatePropertyInformation();
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

pqPropertyManager* pqProxyPanel::propertyManager()
{
  return this->Implementation->PropertyManager;
}

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
  vtkSMSourceProxy *source
    = vtkSMSourceProxy::SafeDownCast(this->Implementation->Proxy);
  if (source)
    {
    source->UpdatePipelineInformation();
    }
  this->Implementation->ReferenceProxy->setModified(false);
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

void pqProxyPanel::select()
{
  emit this->onselect();
}

void pqProxyPanel::deselect()
{
  emit this->ondeselect();
}
  
void pqProxyPanel::setRenderModule(pqRenderViewModule* rm)
{
  if(this->Implementation->RenderModule == rm)
    {
    return;
    }

  this->Implementation->RenderModule = rm;
  emit this->renderModuleChanged(this->Implementation->RenderModule);
}

void pqProxyPanel::setModified()
{
  this->Implementation->ReferenceProxy->setModified(true);
  emit this->modified();
}

