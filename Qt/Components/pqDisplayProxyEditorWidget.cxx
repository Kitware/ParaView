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

#include "vtkSMProxy.h"

#include <QPointer>
#include <QVBoxLayout>

#include "pqApplicationCore.h"
#include "pqBarChartDisplayProxyEditor.h"
#include "pqDisplayPolicy.h"
#include "pqDisplayProxyEditor.h"
#include "pqGenericViewModule.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqTextDisplayPropertiesWidget.h"
#include "pqTextWidgetDisplay.h"
#include "pqXYPlotDisplayProxyEditor.h"
#include "pqPropertyLinks.h"
#include "ui_pqDisplayProxyEditorWidget.h"

class pqDefaultDisplayPanel::pqInternal 
 : public Ui::DisplayProxyEditorWidget
{
public:
  pqPropertyLinks Links;
};

pqDefaultDisplayPanel::pqDefaultDisplayPanel(pqDisplay* display, QWidget* p)
  : pqDisplayPanel(display, p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);
  if(display)
    {
    this->Internal->Links.addPropertyLink(
      this->Internal->ViewData, "checked", SIGNAL(stateChanged(int)),
      display->getProxy(), display->getProxy()->GetProperty("Visibility"));
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


class pqDisplayProxyEditorWidgetInternal
{
public:
  QPointer<pqPipelineSource> Source;
  QPointer<pqGenericViewModule> View;
  QPointer<pqDisplay> Display;
  QPointer<pqDisplayPanel> DisplayPanel;
};

//-----------------------------------------------------------------------------
pqDisplayProxyEditorWidget::pqDisplayProxyEditorWidget(QWidget* p /*=0*/)
  : QWidget(p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  l->setMargin(0);
  this->Internal = new pqDisplayProxyEditorWidgetInternal;

  this->Internal->DisplayPanel = new pqDefaultDisplayPanel(NULL, this);
  l->addWidget(this->Internal->DisplayPanel);
}

//-----------------------------------------------------------------------------
pqDisplayProxyEditorWidget::~pqDisplayProxyEditorWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setView(pqGenericViewModule* view)
{
  this->Internal->View = view;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setSource(pqPipelineSource* source)
{
  this->Internal->Source = source;
}

//-----------------------------------------------------------------------------
pqDisplay* pqDisplayProxyEditorWidget::getDisplay() const
{
  return this->Internal->Display;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::onVisibilityChanged(bool state)
{
  pqDisplayPolicy* policy = pqApplicationCore::instance()->getDisplayPolicy();
  pqDisplay* disp = policy->setDisplayVisibility(this->Internal->Source, 
    this->Internal->View, state);
  
  if (disp)
    {
    disp->renderAllViews();
    }
  this->setDisplay(disp);
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setDisplay(pqDisplay* display)
{
  if(display && this->Internal->Display == display)
    {
    return;
    }

  if(this->Internal->DisplayPanel)
    {
    delete this->Internal->DisplayPanel;
    }
  
  this->Internal->Display = display;

  pqPipelineDisplay* pd = qobject_cast<pqPipelineDisplay*>(display);
  if (pd)
    {
    this->Internal->DisplayPanel = new pqDisplayProxyEditor(pd, this);
    }
  else if (display && display->getProxy() && 
    display->getProxy()->GetXMLName() == QString("XYPlotDisplay2"))
    {
    this->Internal->DisplayPanel = new pqXYPlotDisplayProxyEditor(display, this);
    }
  else if (display && display->getProxy() && 
    display->getProxy()->GetXMLName() == QString("BarChartDisplay"))
    {
    this->Internal->DisplayPanel = new pqBarChartDisplayProxyEditor(display, this);
    }
  else if (qobject_cast<pqTextWidgetDisplay*>(display))
    {
    this->Internal->DisplayPanel = new pqTextDisplayPropertiesWidget(display, this);
    }
  else
    {
    this->Internal->DisplayPanel = new pqDefaultDisplayPanel(display, this);
    
    if(this->Internal->Display || !this->Internal->View ||
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
