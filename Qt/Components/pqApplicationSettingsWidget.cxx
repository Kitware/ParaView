/*=========================================================================

   Program: ParaView
   Module:    pqApplicationSettingsWidget.cxx

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
#include "pqApplicationSettingsWidget.h"
#include "ui_pqApplicationSettingsWidget.h"

// Server Manager Includes.

// Qt Includes.

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqPluginManager.h"
#include "pqSettings.h"
#include "pqViewModuleInterface.h"

class pqApplicationSettingsWidget::pqInternal : public Ui::ApplicationSettingsWidget
{
};

//-----------------------------------------------------------------------------
pqApplicationSettingsWidget::pqApplicationSettingsWidget(QWidget* _parent)
: Superclass(_parent)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);

  this->Internal->DefaultViewType->addItem("None");
  // Get available view types.
  QObjectList ifaces =
    pqApplicationCore::instance()->getPluginManager()->interfaces();
  foreach(QObject* iface, ifaces)
    {
    pqViewModuleInterface* vi = qobject_cast<pqViewModuleInterface*>(iface);
    if(vi)
      {
      QStringList viewtypes = vi->viewTypes();
      QStringList::iterator iter;
      for(iter = viewtypes.begin(); iter != viewtypes.end(); ++iter)
        {
        if ((*iter) == "TableView")
          {
          // Ignore this view for now.
          continue;
          }

        this->Internal->DefaultViewType->addItem(
          vi->viewTypeName(*iter), *iter);
        }
      }
    } 

  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString curView = settings->value("/defaultViewType", "None").toString();
  int index = this->Internal->DefaultViewType->findData(curView);
  index = (index==-1)? 0 : index;
  this->Internal->DefaultViewType->setCurrentIndex(index);
}

//-----------------------------------------------------------------------------
pqApplicationSettingsWidget::~pqApplicationSettingsWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqApplicationSettingsWidget::accept()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("/defaultViewType", 
    this->Internal->DefaultViewType->itemData(
      this->Internal->DefaultViewType->currentIndex()));
}

