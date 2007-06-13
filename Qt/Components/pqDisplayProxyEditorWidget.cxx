/*=========================================================================

   Program: ParaView
   Module:    pqDisplayProxyEditorWidget.cxx

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
#include "pqDisplayProxyEditorWidget.h"
#include "ui_pqDisplayProxyEditorWidget.h"

#include "vtkSMProxy.h"

#include <QPointer>
#include <QVBoxLayout>

#include "pqApplicationCore.h"
#include "pqBarChartDisplayProxyEditor.h"
#include "pqDisplayPanelInterface.h"
#include "pqDisplayPolicy.h"
#include "pqDisplayProxyEditor.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPluginManager.h"
#include "pqPropertyLinks.h"
#include "pqTextDisplay.h"
#include "pqTextDisplayPropertiesWidget.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "pqXYPlotDisplayProxyEditor.h"

/// standard display panels
class pqStandardDisplayPanels : public QObject,
                                public pqDisplayPanelInterface
{
public:
  /// constructor
  pqStandardDisplayPanels(){}
  /// destructor
  virtual ~pqStandardDisplayPanels(){}

  /// Returns true if this panel can be created for the given the proxy.
  virtual bool canCreatePanel(pqRepresentation* proxy) const
    {
    if(!proxy || !proxy->getProxy())
      {
      return false;
      }

    QString type = proxy->getProxy()->GetXMLName();

    if(type == "BarChartDisplay" ||
       type == "XYPlotDisplay2" ||
       qobject_cast<pqTextDisplay*>(proxy))
      {
      return true;
      }

    return false;
    }
  /// Creates a panel for the given proxy
  virtual pqDisplayPanel* createPanel(pqRepresentation* proxy, QWidget* p)
    {
    if(!proxy || !proxy->getProxy())
      {
      return NULL;
      }

    QString type = proxy->getProxy()->GetXMLName();
    if(type == QString("XYPlotDisplay2"))
      {
      return new pqXYPlotDisplayProxyEditor(proxy, p);
      }
    
    if(type == QString("BarChartDisplay"))
      {
      return new pqBarChartDisplayProxyEditor(proxy, p);
      }
    
    if (qobject_cast<pqTextDisplay*>(proxy))
      {
      return new pqTextDisplayPropertiesWidget(proxy, p);
      }
    return NULL;
    }
};


class pqDefaultDisplayPanel::pqInternal 
 : public Ui::DisplayProxyEditorWidget
{
public:
  pqPropertyLinks Links;
};

pqDefaultDisplayPanel::pqDefaultDisplayPanel(pqRepresentation* repr, QWidget* p)
  : pqDisplayPanel(repr, p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);
  if(repr)
    {
    this->Internal->Links.addPropertyLink(
      this->Internal->ViewData, "checked", SIGNAL(stateChanged(int)),
      repr->getProxy(), repr->getProxy()->GetProperty("Visibility"));
    }
  else
    {
    this->Internal->ViewData->setCheckState(Qt::Unchecked);
    }
  QObject::connect(this->Internal->ViewData, SIGNAL(stateChanged(int)), 
                   this, SLOT(onStateChanged(int)));
}

pqDefaultDisplayPanel::~pqDefaultDisplayPanel()
{
  delete this->Internal;
}

void pqDefaultDisplayPanel::onStateChanged(int s)
{
  this->updateAllViews();
  emit this->visibilityChanged(s == Qt::Checked);
}


class pqDisplayProxyEditorWidget::pqInternal
{
public:
  QPointer<pqPipelineSource> Source;
  QPointer<pqView> View;
  QPointer<pqRepresentation> Representation;
  QPointer<pqDisplayPanel> DisplayPanel;
  pqStandardDisplayPanels StandardPanels;
};

//-----------------------------------------------------------------------------
pqDisplayProxyEditorWidget::pqDisplayProxyEditorWidget(QWidget* p /*=0*/)
  : QWidget(p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  l->setMargin(0);
  this->Internal = new pqDisplayProxyEditorWidget::pqInternal;

  this->Internal->DisplayPanel = new pqDefaultDisplayPanel(NULL, this);
  l->addWidget(this->Internal->DisplayPanel);

  pqUndoStack* ustack = pqApplicationCore::instance()->getUndoStack();
  if (ustack)
    {
    QObject::connect(this, SIGNAL(beginUndo(const QString&)),
      ustack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this, SIGNAL(endUndo()),
      ustack, SLOT(endUndoSet()));
    }
}

//-----------------------------------------------------------------------------
pqDisplayProxyEditorWidget::~pqDisplayProxyEditorWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setView(pqView* view)
{
  this->Internal->View = view;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setSource(pqPipelineSource* source)
{
  this->Internal->Source = source;
}

//-----------------------------------------------------------------------------
pqRepresentation* pqDisplayProxyEditorWidget::getRepresentation() const
{
  return this->Internal->Representation;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::onVisibilityChanged(bool state)
{
  if (!this->Internal->Source)
    {
    return;
    }

  emit this->beginUndo(QString("Change Visibility of %1").arg(
      this->Internal->Source->getSMName()));
  pqDisplayPolicy* policy = pqApplicationCore::instance()->getDisplayPolicy();
  pqRepresentation* disp = policy->setDisplayVisibility(this->Internal->Source, 
    this->Internal->View, state);
  emit this->endUndo();
  
  if (disp)
    {
    disp->renderViewEventually();
    }
  this->setRepresentation(disp);
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setRepresentation(pqRepresentation* repr)
{
  if(repr && this->Internal->Representation == repr)
    {
    return;
    }

  if(this->Internal->DisplayPanel)
    {
    delete this->Internal->DisplayPanel;
    }
  
  this->Internal->Representation = repr;
  
  // search for a custom panels
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  QObjectList ifaces = pm->interfaces();
  foreach(QObject* iface, ifaces)
    {
    pqDisplayPanelInterface* piface =
      qobject_cast<pqDisplayPanelInterface*>(iface);
    if (piface && piface->canCreatePanel(repr))
      {
      this->Internal->DisplayPanel = piface->createPanel(repr, this);
      break;
      }
    }

  if (!this->Internal->DisplayPanel &&
    this->Internal->StandardPanels.canCreatePanel(repr))
    {
    this->Internal->DisplayPanel =
      this->Internal->StandardPanels.createPanel(repr, this);
    }

  pqPipelineRepresentation* pd = qobject_cast<pqPipelineRepresentation*>(repr);
  if (!this->Internal->DisplayPanel && pd)
    {
    this->Internal->DisplayPanel = new pqDisplayProxyEditor(pd, this);
    }
  else if(!this->Internal->DisplayPanel)
    {
    this->Internal->DisplayPanel = new pqDefaultDisplayPanel(repr, this);
    
    if(this->Internal->Representation || !this->Internal->View ||
       this->Internal->View->canDisplaySource(this->Internal->Source))
      {
      // connect to visibility so we can create a view for it
      QObject::connect(this->Internal->DisplayPanel,
                       SIGNAL(visibilityChanged(bool)),
                       this, 
                       SLOT(onVisibilityChanged(bool)), Qt::QueuedConnection);
      }
    else
      {
      this->Internal->DisplayPanel->setEnabled(false);
      }
    }
  this->layout()->addWidget(this->Internal->DisplayPanel);
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::reloadGUI()
{
  if(this->Internal->DisplayPanel)
    {
    this->Internal->DisplayPanel->reloadGUI();
    }
}
