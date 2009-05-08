/*=========================================================================

   Program: ParaView
   Module:    pqTwoDRenderViewOptions.cxx

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
#include "pqTwoDRenderViewOptions.h"
#include "ui_pqTwoDRenderViewOptions.h"

// Server Manager Includes.
#include "vtkSMProxy.h"

// Qt Includes.
#include <QPointer>

// ParaView Includes.
#include "pqTwoDRenderView.h"
#include "pqSignalAdaptors.h"
#include "pqPropertyManager.h"

class pqTwoDRenderViewOptions::pqInternal : public Ui::pqTwoDRenderViewOptions
{
public:
  QPointer<pqTwoDRenderView> View;
  pqPropertyManager Links;
  pqSignalAdaptorColor* ColorAdaptor;

};

//-----------------------------------------------------------------------------
pqTwoDRenderViewOptions::pqTwoDRenderViewOptions(QWidget* _parent):
  Superclass(_parent)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);

  this->Internal->ColorAdaptor = 
    new pqSignalAdaptorColor(this->Internal->backgroundColor,
      "chosenColor", SIGNAL(chosenColorChanged(const QColor&)), false);

  // enable the apply button when things are changed
  QObject::connect(&this->Internal->Links, SIGNAL(modified()),
          this, SIGNAL(changesAvailable()));
  
  QObject::connect(this->Internal->restoreDefault,
    SIGNAL(clicked(bool)), this, SLOT(restoreDefaultBackground()));
}

//-----------------------------------------------------------------------------
pqTwoDRenderViewOptions::~pqTwoDRenderViewOptions()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqTwoDRenderViewOptions::setPage(const QString &page)
{
  int count = this->Internal->stackedWidget->count();
  for (int i=0; i<count; i++)
    {
    if(this->Internal->stackedWidget->widget(i)->objectName() == page)
      {
      this->Internal->stackedWidget->setCurrentIndex(i);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
QStringList pqTwoDRenderViewOptions::getPageList()
{
  QStringList pages;
  int count = this->Internal->stackedWidget->count();
  for (int i=0; i<count; i++)
    {
    pages << this->Internal->stackedWidget->widget(i)->objectName();
    }
  return pages;
}
  
//-----------------------------------------------------------------------------
void pqTwoDRenderViewOptions::setView(pqView* view)
{
  if (this->Internal->View)
    {
    // disconnect widgets from current render view
    this->disconnectGUI();
    }

  this->Internal->View = qobject_cast<pqTwoDRenderView*>(view);
  if(this->Internal->View)
    {
    // connect widgets to current render view
    this->connectGUI();
    }
}

//-----------------------------------------------------------------------------
void pqTwoDRenderViewOptions::applyChanges()
{
  if(!this->Internal->View)
    {
    return;
    }

  this->Internal->Links.accept();
  this->Internal->View->saveSettings();

  // update the view after changes
  this->Internal->View->render();
}

//-----------------------------------------------------------------------------
void pqTwoDRenderViewOptions::resetChanges()
{
  this->Internal->Links.reject();
}

//-----------------------------------------------------------------------------
void pqTwoDRenderViewOptions::connectGUI()
{
  this->blockSignals(true);

  vtkSMProxy* proxy = this->Internal->View->getProxy();

  // link stuff on the general tab
  this->Internal->Links.registerLink(this->Internal->ColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy, proxy->GetProperty("Background"));
  this->Internal->Links.registerLink(this->Internal->showAxes, "checked",
    SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("AxesVisibility"));
  this->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqTwoDRenderViewOptions::disconnectGUI()
{
  vtkSMProxy* proxy = this->Internal->View->getProxy();

  // link stuff on the general tab
  this->Internal->Links.unregisterLink(this->Internal->ColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy, proxy->GetProperty("Background"));
  this->Internal->Links.unregisterLink(this->Internal->showAxes, "checked",
    SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("AxesVisibility"));
}

//-----------------------------------------------------------------------------
void pqTwoDRenderViewOptions::restoreDefaultBackground()
{
  if (this->Internal->View)
    {
    const int* col = this->Internal->View->defaultBackgroundColor();
    this->Internal->backgroundColor->setChosenColor(
               QColor(col[0], col[1], col[2]));
    }
}


