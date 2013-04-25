/*=========================================================================

   Program: ParaView
   Module:    pqScalarBarVisibilityReaction.cxx

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
#include "pqScalarBarVisibilityReaction.h"

#include "pqActiveObjects.h"
#include "pqLookupTableManager.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderViewBase.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqUndoStack.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqScalarBarVisibilityReaction::pqScalarBarVisibilityReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  parentObject->setCheckable(true);
  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::updateEnableState()
{
  if (this->CachedRepresentation)
    {
    QObject::disconnect(this->CachedRepresentation, 0, this, 0);
    this->CachedRepresentation = 0;
    }
  if (this->CachedLUT)
    {
    QObject::disconnect(this->CachedLUT, 0, this, 0);
    this->CachedLUT = 0;
    }

  pqPipelineRepresentation* repr = qobject_cast<pqPipelineRepresentation*>(
    pqActiveObjects::instance().activeRepresentation());

  bool can_show_sb = repr && repr->getColorField() !=
    pqPipelineRepresentation::solidColor();
  bool is_shown = false;

  this->CachedRepresentation = repr;
  if (repr)
    {
    QObject::connect(repr, SIGNAL(colorChanged()), this,
      SLOT(updateEnableState()), Qt::QueuedConnection);

    pqScalarsToColors* lut = repr->getLookupTable();
    this->CachedLUT = lut;
    if (lut)
      {
      QObject::connect(lut, SIGNAL(scalarBarsChanged()), this,
        SLOT(updateEnableState()), Qt::QueuedConnection);

      pqScalarBarRepresentation* sb = lut->getScalarBar(
        qobject_cast<pqRenderViewBase*>(repr->getView()));
      this->CachedScalarBar = sb;
      if (sb)
        {
        QObject::connect(sb, SIGNAL(visibilityChanged(bool)),
          this, SLOT(updateEnableState()), Qt::QueuedConnection);
        is_shown = sb->isVisible();
        }
      }
    }

  QAction* parent_action = this->parentAction();
  parent_action->setEnabled(can_show_sb);
  bool prev = parent_action->blockSignals(true);
  parent_action->setChecked(is_shown);
  parent_action->blockSignals(prev);
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::setScalarBarVisibility(bool visible)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  if (!lut_mgr)
    {
    qCritical() << "Cannot locate pqLookupTableManager.";
    return;
    }

  pqRenderViewBase* view = qobject_cast<pqRenderViewBase*>(
    pqActiveObjects::instance().activeView());
  pqDataRepresentation* repr =
    pqActiveObjects::instance().activeRepresentation();
  if (!view || !repr)
    {
    qCritical() << "Required active objects are not available.";
    return;
    }

  BEGIN_UNDO_SET( "Toggle Color Legend Visibility");
  pqScalarBarRepresentation* scalar_bar =
    lut_mgr->setScalarBarVisibility(repr, visible);
  END_UNDO_SET();
  
  if (scalar_bar)
    {
    scalar_bar->renderViewEventually();
    }
}
