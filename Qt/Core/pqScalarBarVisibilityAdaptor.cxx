/*=========================================================================

   Program: ParaView
   Module:    pqScalarBarVisibilityAdaptor.cxx

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
#include "pqScalarBarVisibilityAdaptor.h"

#include <QtDebug>
#include <QAction>
#include <QPointer>

#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqScalarsToColors.h"
#include "pqScalarBarDisplay.h"
#include "pqRenderViewModule.h"

//-----------------------------------------------------------------------------
class pqScalarBarVisibilityAdaptor::pqInternal
{
public:
  QPointer<pqPipelineDisplay> ActiveDisplay;
  QPointer<pqRenderViewModule> ActiveRenderModule;
};

//-----------------------------------------------------------------------------
pqScalarBarVisibilityAdaptor::pqScalarBarVisibilityAdaptor(QAction* p)
  : QObject(p)
{
  this->Internal = new pqInternal();
  QObject::connect(p, SIGNAL(toggled(bool)),
    this, SLOT(setScalarBarVisibility(bool)));
  QObject::connect(this, SIGNAL(canChangeVisibility(bool)),
    p, SLOT(setEnabled(bool)));
  QObject::connect(this, SIGNAL(scalarBarVisible(bool)),
    p, SLOT(setChecked(bool)));
}

//-----------------------------------------------------------------------------
pqScalarBarVisibilityAdaptor::~pqScalarBarVisibilityAdaptor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityAdaptor::setActiveDisplay(pqConsumerDisplay *display,
    pqGenericViewModule *view)
{
  if(display != this->Internal->ActiveDisplay)
    {
    if(this->Internal->ActiveDisplay)
      {
      this->disconnect(this->Internal->ActiveDisplay, 0, this, 0);
      }

    this->Internal->ActiveDisplay = dynamic_cast<pqPipelineDisplay *>(display);
    this->Internal->ActiveRenderModule =
        dynamic_cast<pqRenderViewModule *>(view);
    if(this->Internal->ActiveDisplay)
      {
      this->connect(this->Internal->ActiveDisplay,
          SIGNAL(visibilityChanged(bool)), 
          this, SLOT(updateEnableState()), Qt::QueuedConnection);
      this->connect(this->Internal->ActiveDisplay, SIGNAL(colorChanged()),
          this, SLOT(updateEnableState()), Qt::QueuedConnection);
      }

    this->updateEnableState();
    }
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityAdaptor::setScalarBarVisibility(bool visible)
{
  if (!this->Internal->ActiveDisplay)
    {
    qDebug() << "No active display found, cannot change scalar bar visibility.";
    return;
    }

  pqScalarsToColors* lut = this->Internal->ActiveDisplay->getLookupTable();
  if (!lut)
    {
    qDebug() << "No Lookup Table found for the active display.";
    return;
    }
  
  pqScalarBarDisplay* sb = 
    lut->getScalarBar(this->Internal->ActiveRenderModule);
  if (!sb)
    {
    pqPipelineBuilder* builder = pqApplicationCore::instance()->getPipelineBuilder();
    sb = builder->createScalarBar(lut, this->Internal->ActiveRenderModule);
    sb->makeTitle(this->Internal->ActiveDisplay);
    }
  if (!sb)
    {
    qDebug() << "Failed to locate/create scalar bar.";
    return;
    }
  sb->setVisible(visible);
  sb->renderAllViews();

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityAdaptor::updateEnableState()
{
  // No display, no scalar bar.
  if (!this->Internal->ActiveDisplay)
    {
    emit this->canChangeVisibility(false);
    return;
    }

  // Is the display colored with some array? Scalar bar only
  // is scalar coloring used.
  QString colorField = this->Internal->ActiveDisplay->getColorField();
  if (colorField == "" || colorField == "Solid Color")
    {
    emit this->canChangeVisibility(false);
    return;
    }

  pqScalarsToColors* lut = this->Internal->ActiveDisplay->getLookupTable();
  if (!lut)
    {
    emit this->canChangeVisibility(false);
    return;
    }

  emit this->canChangeVisibility(true);

  // update check state.
  pqScalarBarDisplay* sb = 
    lut->getScalarBar(this->Internal->ActiveRenderModule);
  if (sb)
    {
    emit this->scalarBarVisible(sb->isVisible());
    }
  else
    {
    emit this->scalarBarVisible(false);
    }
}

