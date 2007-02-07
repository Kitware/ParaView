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
#include "pqDisplayPolicy.h"
#include "pqDisplayProxyEditor.h"
#include "pqGenericViewModule.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqTextDisplayPropertiesWidget.h"
#include "pqTextWidgetDisplay.h"
#include "pqXYPlotDisplayProxyEditor.h"
#include "pqPropertyLinks.h"

class pqDisplayProxyEditorWidgetInternal : public Ui::DisplayProxyEditorWidget
{
public:
  QPointer<pqPipelineSource> Source;
  QPointer<pqGenericViewModule> View;
  QPointer<pqDisplay> Display;
  QPointer<QWidget> DisplayWidget;
  QWidget* DefaultWidget;
  pqPropertyLinks Links;
};

//-----------------------------------------------------------------------------
pqDisplayProxyEditorWidget::pqDisplayProxyEditorWidget(QWidget* p /*=0*/)
  : QWidget(p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  l->setMargin(0);
  this->Internal = new pqDisplayProxyEditorWidgetInternal;

  this->Internal->DefaultWidget = new QWidget(this);
  this->Internal->setupUi(this->Internal->DefaultWidget);
  l->addWidget(this->Internal->DefaultWidget);
}

//-----------------------------------------------------------------------------
pqDisplayProxyEditorWidget::~pqDisplayProxyEditorWidget()
{
  delete this->Internal->DefaultWidget;
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
void pqDisplayProxyEditorWidget::showDefaultWidget()
{
  if (this->Internal->DisplayWidget)
    {
    this->Internal->DisplayWidget->hide();
    }
  this->Internal->DefaultWidget->show();
  pqDisplayPolicy* policy = pqApplicationCore::instance()->getDisplayPolicy();
  if (this->Internal->Display || 
      !this->Internal->View || 
      (policy && policy->canDisplay(this->Internal->Source, 
                                    this->Internal->View)))
    {
    this->Internal->DefaultWidget->setEnabled(true);
    if (this->Internal->Display)
      {
      vtkSMProxy* display = this->Internal->Display->getProxy();
      this->Internal->Links.addPropertyLink(
        this->Internal->ViewData, "checked", SIGNAL(stateChanged(int)),
        display, display->GetProperty("Visibility"));
      }
    else
      {
      this->Internal->ViewData->setCheckState(Qt::Unchecked);
      }
    QObject::connect(this->Internal->ViewData, SIGNAL(stateChanged(int)),
      this, SLOT(onVisibilityChanged(int)), Qt::QueuedConnection);
    }
  else
    {
    this->Internal->DefaultWidget->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::onVisibilityChanged(int state)
{
  if (this->Internal->Display)
    {
    this->Internal->Display->renderAllViews();
    }
  else
    {
    pqDisplayPolicy* policy = pqApplicationCore::instance()->getDisplayPolicy();
    pqDisplay* disp = policy->setDisplayVisibility(this->Internal->Source, 
      this->Internal->View, (state == Qt::Checked));
    if (disp)
      {
      disp->renderAllViews();
      }
    this->setDisplay(disp);
    }
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setDisplay(pqDisplay* display)
{
  QObject::disconnect(this->Internal->ViewData, 0, this, 0);
  this->Internal->Links.removeAllPropertyLinks();
  this->Internal->Display = display;
  if (!display)
    {
    this->showDefaultWidget();
    emit this->requestSetDisplay((pqDisplay*)0);
    emit this->requestSetDisplay((pqPipelineDisplay*)0);
    return;
    }

  pqPipelineDisplay* pd = qobject_cast<pqPipelineDisplay*>(display);
  if (pd)
    {
    pqDisplayProxyEditor* editor = qobject_cast<pqDisplayProxyEditor*>(
      this->Internal->DisplayWidget);

    if (!editor)
      {
      editor = new pqDisplayProxyEditor(this);
      this->layout()->addWidget(editor);
      if (this->Internal->DisplayWidget)
        {
        this->layout()->removeWidget(this->Internal->DisplayWidget);
        delete this->Internal->DisplayWidget;
        }      
      this->Internal->DisplayWidget = editor;
      QObject::connect(this, SIGNAL(requestReload()),
        editor, SLOT(reloadGUI()));
      QObject::connect(this, SIGNAL(requestSetDisplay(pqPipelineDisplay*)),
        editor, SLOT(setDisplay(pqPipelineDisplay*)));
      }
    editor->setDisplay(pd);
    }
  else if (display->getProxy() && 
    display->getProxy()->GetXMLName() == QString("XYPlotDisplay2"))
    {
    pqXYPlotDisplayProxyEditor* editor = 
      qobject_cast<pqXYPlotDisplayProxyEditor*>(
        this->Internal->DisplayWidget);
    if (!editor)
      {
      editor = new pqXYPlotDisplayProxyEditor(this);
      this->layout()->addWidget(editor);
      if (this->Internal->DisplayWidget)
        {
        this->layout()->removeWidget(this->Internal->DisplayWidget);
        delete this->Internal->DisplayWidget;
        }
      this->Internal->DisplayWidget = editor;
      QObject::connect(this, SIGNAL(requestSetDisplay(pqDisplay*)),
        editor, SLOT(setDisplay(pqDisplay*)));
      QObject::connect(this, SIGNAL(requestReload()),
        editor, SLOT(reloadGUI()));
      }
    editor->setDisplay(display);
    }
  else if (qobject_cast<pqTextWidgetDisplay*>(display))
    {
    pqTextDisplayPropertiesWidget* editor = 
      qobject_cast<pqTextDisplayPropertiesWidget*>(
        this->Internal->DisplayWidget);
    if (!editor)
      {
      editor = new pqTextDisplayPropertiesWidget(this);
      this->layout()->addWidget(editor);
      if (this->Internal->DisplayWidget)
        {
        this->layout()->removeWidget(this->Internal->DisplayWidget);
        delete this->Internal->DisplayWidget;
        }
      this->Internal->DisplayWidget = editor;
      QObject::connect(this, SIGNAL(requestSetDisplay(pqDisplay*)),
        editor, SLOT(setDisplay(pqDisplay*)));
      QObject::connect(this, SIGNAL(requestReload()),
        editor, SLOT(reloadGUI()));
      }
    editor->setDisplay(display);
    }
  else
    {
    this->showDefaultWidget(); 
    emit this->requestSetDisplay((pqDisplay*)0);
    emit this->requestSetDisplay((pqPipelineDisplay*)0);
    return;
    }

  if (this->Internal->DisplayWidget)
    {
    this->Internal->DisplayWidget->show();

    // NOTE: We make is disabled before we hide it. If the DefaultWidget children
    // had the focus, then hiding the widget before disabling it leads to the
    // focus getting transferred to one of the view modules which may lead to 
    // unnecessary active-view change. 
    this->Internal->DefaultWidget->setEnabled(false);
    this->Internal->DefaultWidget->hide();
    }
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::reloadGUI()
{
  emit this->requestReload();
}
