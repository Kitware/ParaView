/*=========================================================================

   Program: ParaView
   Module:    MantaViewOptions.cxx

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

#include "MantaViewOptions.h"
#include "ui_MantaViewOptions.h"

#include "vtkSMRenderViewProxy.h"
#include "vtkSMPropertyHelper.h"

#include <QHBoxLayout>

#include "MantaView.h"

#include "pqActiveView.h"

class MantaViewOptions::pqInternal
{
public:
  Ui::MantaViewOptions ui;
};

//----------------------------------------------------------------------------
MantaViewOptions::MantaViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  //TODO: This should have an associated ActiveViewOptions Dialog
  //To make the controls accessible from the View's control strip.

  this->Internal = new pqInternal();
  this->Internal->ui.setupUi(this);

  QObject::connect(this->Internal->ui.threads,
                   SIGNAL(valueChanged(int)),
                   this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->ui.shadows,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->ui.samples,
                   SIGNAL(valueChanged(int)),
                   this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->ui.maxDepth,
                   SIGNAL(valueChanged(int)),
                   this, SIGNAL(changesAvailable()));
}

//----------------------------------------------------------------------------
MantaViewOptions::~MantaViewOptions()
{
}

//----------------------------------------------------------------------------
void MantaViewOptions::setPage(const QString&)
{
}

//----------------------------------------------------------------------------
QStringList MantaViewOptions::getPageList()
{
  QStringList ret;
  ret << "Manta View";
  return ret;
}

//----------------------------------------------------------------------------
void MantaViewOptions::applyChanges()
{
  pqView* view = pqActiveView::instance().current();
  pqRenderView *rView = qobject_cast<pqRenderView*>(view);
/*
  if(!this->rView)
    {
    return;
    }
*/
  //TODO:These should be saved across sessions
  //pqSettings* settings = pqApplicationCore::instance()->settings();
  //settings->beginGroup("MantaView");

  int intSetting;
  bool boolSetting;

  vtkSMRenderViewProxy *proxy = rView->getRenderViewProxy();
  intSetting = this->Internal->ui.threads->value();
  vtkSMPropertyHelper(proxy, "Threads").Set(intSetting);

  boolSetting = this->Internal->ui.shadows->isChecked();
  vtkSMPropertyHelper(proxy, "EnableShadows").Set(boolSetting);

  intSetting = this->Internal->ui.samples->value();
  vtkSMPropertyHelper(proxy, "Samples").Set(intSetting);

  intSetting = this->Internal->ui.maxDepth->value();
  vtkSMPropertyHelper(proxy, "MaxDepth").Set(intSetting);
}

//----------------------------------------------------------------------------
void MantaViewOptions::resetChanges()
{
}
