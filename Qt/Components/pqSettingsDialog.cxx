/*=========================================================================

   Program: ParaView
   Module:    pqSettingsDialog.cxx

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
#include "pqSettingsDialog.h"
#include "ui_pqSettingsDialog.h"

// ParaView Server Manager includes.
#include "vtkSMProxy.h"

// Qt includes.
#include <QPointer>

// ParaView Client includes.
#include "pqRenderModule.h"
#include "pq3DViewPropertiesWidget.h"

//-----------------------------------------------------------------------------
class pqSettingsDialogInternal : public Ui::pqSettingsDialog
{
public:
  pq3DViewPropertiesWidget* ViewProperties;
  QPointer<pqRenderModule> RenderModule;
  pqSettingsDialogInternal()
    {
    this->ViewProperties = 0;
    }
  ~pqSettingsDialogInternal()
    {
    delete this->ViewProperties;
    }
};



//-----------------------------------------------------------------------------
pqSettingsDialog::pqSettingsDialog(QWidget* _p/*=null*/, 
  Qt::WFlags f/*=0*/): QDialog(_p, f)
{
  this->Internal = new pqSettingsDialogInternal;
  this->Internal->setupUi(this);
  this->Internal->ViewProperties = new pq3DViewPropertiesWidget();
  

  QObject::connect(this, SIGNAL(finished(int)), this, SLOT(onFinished(int)));
}

//-----------------------------------------------------------------------------
pqSettingsDialog::~pqSettingsDialog()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::setRenderModule(pqRenderModule* ren)
{
  this->Internal->RenderModule = ren;
  this->setupGUI();
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::setupGUI()
{
  if (this->Internal->RenderModule)
    {
    // Add settings tab.
    this->Internal->ViewProperties->setRenderModule(
      this->Internal->RenderModule->getProxy());
    this->Internal->tabWidget->addTab(
      this->Internal->ViewProperties, "Active View Properties");
    }
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::onFinished(int end_result)
{
  if (end_result != QDialog::Accepted)
    {
    return;
    }
  this->Internal->ViewProperties->accept();
}

