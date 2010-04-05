/*=========================================================================

   Program: ParaView
   Module:    pqScalarBarVisibilityAdaptor.cxx

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
#include "pqScalarBarVisibilityAdaptor.h"

#include <QtDebug>
#include <QAction>
#include <QPointer>

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderViewBase.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqUndoStack.h"
#include "pqLookupTableManager.h"
//-----------------------------------------------------------------------------
class pqScalarBarVisibilityAdaptor::pqInternal
{
public:
  QPointer<pqPipelineRepresentation> ActiveDisplay;
  QPointer<pqRenderViewBase> ActiveRenderView;
  QPointer<pqScalarsToColors> ActiveLUT;
};

//-----------------------------------------------------------------------------
pqScalarBarVisibilityAdaptor::pqScalarBarVisibilityAdaptor(QAction* p)
  : QObject(p)
{
  this->Internal = new pqInternal();
  QObject::connect(p, SIGNAL(toggled(bool)),
    this, SLOT(setScalarBarVisibility(bool)));
  QObject::connect(this, SIGNAL(canChangeVisibility(bool)),
    p, SLOT(setEnabled(bool)), Qt::QueuedConnection);
  QObject::connect(this, SIGNAL(scalarBarVisible(bool)),
    p, SLOT(setChecked(bool)));
}

//-----------------------------------------------------------------------------
pqScalarBarVisibilityAdaptor::~pqScalarBarVisibilityAdaptor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityAdaptor::setActiveRepresentation(
  pqDataRepresentation *display)
{
  if(display != this->Internal->ActiveDisplay)
    {
    if(this->Internal->ActiveDisplay)
      {
      QObject::disconnect(this->Internal->ActiveDisplay, 0, this, 0);
      }

    this->Internal->ActiveDisplay = qobject_cast<pqPipelineRepresentation *>(display);
    this->Internal->ActiveRenderView = 0;
    if(this->Internal->ActiveDisplay)
      {
      this->Internal->ActiveRenderView =  
        qobject_cast<pqRenderViewBase *>(display->getView());
      QObject::connect(this->Internal->ActiveDisplay,
          SIGNAL(visibilityChanged(bool)), 
          this, SLOT(updateState()), Qt::QueuedConnection);
      QObject::connect(this->Internal->ActiveDisplay, SIGNAL(colorChanged()),
          this, SLOT(updateState()), Qt::QueuedConnection);
      }

    this->updateState();
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

  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  if (!lut_mgr)
    {
    qCritical() << "pqScalarBarVisibilityAdaptor needs a pqLookupTableManager";
    return;
    }

  BEGIN_UNDO_SET( "Toggle Color Legend Visibility");
  pqScalarBarRepresentation* scalar_bar =
    lut_mgr->setScalarBarVisibility(this->Internal->ActiveDisplay, visible);
  END_UNDO_SET();
  if (scalar_bar)
    {
    scalar_bar->renderViewEventually();
    }
  this->updateState();
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityAdaptor::updateState()
{
  if (this->Internal->ActiveLUT)
    {
    QObject::disconnect(this->Internal->ActiveLUT, 0, this, 0);
    this->Internal->ActiveLUT = 0;
    }

  // We block the action signals so that we don't call
  // setScalarBarVisibility() slot as a consequence of updating
  // the state of the action.
  bool old = this->parent()->blockSignals(true);
  this->updateStateInternal();
  this->parent()->blockSignals(old);

  if (this->Internal->ActiveLUT)
    {
    QObject::connect(this->Internal->ActiveLUT, SIGNAL(scalarBarsChanged()),
      this, SLOT(updateState()), Qt::QueuedConnection);
    }
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityAdaptor::updateStateInternal()
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
  if (colorField == "" || colorField == pqPipelineRepresentation::solidColor())
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

  this->Internal->ActiveLUT = lut;

  // update check state.
  pqScalarBarRepresentation* sb = 
    lut->getScalarBar(this->Internal->ActiveRenderView);
  if (sb)
    {
    emit this->scalarBarVisible(sb->isVisible());
    }
  else
    {
    emit this->scalarBarVisible(false);
    }
}

