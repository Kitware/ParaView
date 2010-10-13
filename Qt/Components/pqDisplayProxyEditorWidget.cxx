/*=========================================================================

   Program: ParaView
   Module:    pqDisplayProxyEditorWidget.cxx

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
#include "pqDisplayProxyEditorWidget.h"
#include "ui_pqDisplayProxyEditorWidget.h"

#include "vtkSMProxy.h"

#include <QPointer>
#include <QVBoxLayout>

#include <QDebug>

#include "pqApplicationCore.h"
#include "pqDisplayPanelDecoratorInterface.h"
#include "pqDisplayPanelInterface.h"
#include "pqDisplayPolicy.h"
#include "pqDisplayProxyEditor.h"
#include "pqOutputPort.h"
#include "pqParallelCoordinatesChartDisplayPanel.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPluginManager.h"
#include "pqPropertyLinks.h"
#include "pqSpreadSheetDisplayEditor.h"
#include "pqTextDisplayPropertiesWidget.h"
#include "pqTextRepresentation.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "pqXYChartDisplayPanel.h"

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

    if (type == "XYPlotRepresentation" ||
       type == "XYChartRepresentation" ||
       type == "XYBarChartRepresentation" ||
       type == "BarChartRepresentation" ||
       type == "SpreadSheetRepresentation" ||
       qobject_cast<pqTextRepresentation*>(proxy)||
       type == "ScatterPlotRepresentation" ||
       type == "ParallelCoordinatesRepresentation")
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
      qDebug() << "Proxy is null" << proxy;
      return NULL;
      }

    QString type = proxy->getProxy()->GetXMLName();
    if (type == QString("XYChartRepresentation"))
      {
      return new pqXYChartDisplayPanel(proxy, p);
      }
    if (type == QString("XYBarChartRepresentation"))
      {
      return new pqXYChartDisplayPanel(proxy, p);
      }
    if (type == "SpreadSheetRepresentation")
      {
      return new pqSpreadSheetDisplayEditor(proxy, p);
      }

    if (qobject_cast<pqTextRepresentation*>(proxy))
      {
      return new pqTextDisplayPropertiesWidget(proxy, p);
      }
#ifdef FIXME
    if (type == "ScatterPlotRepresentation")
      {
      return new pqScatterPlotDisplayPanel(proxy, p);
      }
#endif
    if (type == QString("ParallelCoordinatesRepresentation"))
      {
      return new pqParallelCoordinatesChartDisplayPanel(proxy, p);
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
  QPointer<pqOutputPort> OutputPort;
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
  this->updatePanel();
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setOutputPort(pqOutputPort* port)
{
  this->Internal->OutputPort = port;
  this->Internal->Source = (port? port->getSource() : 0);
  this->updatePanel();
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
  pqRepresentation* disp = policy->setRepresentationVisibility(
    this->Internal->OutputPort, this->Internal->View, state);
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

  this->Internal->Representation = repr;
  this->updatePanel();

}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::updatePanel()
{
  if(this->Internal->DisplayPanel)
    {
    delete this->Internal->DisplayPanel;
    this->Internal->DisplayPanel = 0;
    }

  pqRepresentation* repr = this->Internal->Representation;

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

    if((this->Internal->Representation || !this->Internal->View ||
       this->Internal->View->canDisplay(this->Internal->OutputPort)) &&
      (this->Internal->OutputPort &&
       this->Internal->OutputPort->getSource()->modifiedState() !=
       pqProxy::UNINITIALIZED) )
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

  foreach(QObject* iface, ifaces)
    {
    pqDisplayPanelDecoratorInterface* piface =
      qobject_cast<pqDisplayPanelDecoratorInterface*>(iface);
    if (piface && piface->canDecorate(this->Internal->DisplayPanel))
      {
      piface->decorate(this->Internal->DisplayPanel);
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
