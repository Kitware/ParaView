/*=========================================================================

   Program: ParaView
   Module:  pqPointPickingHelper.cxx

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
#include "pqPointPickingHelper.h"

#include "pqKeySequences.h"
#include "pqModalShortcut.h"
#include "pqRenderView.h"
#include "pqShortcutDecorator.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMRenderViewProxy.h"

#include <QShortcut>
#include <QWidget>

//-----------------------------------------------------------------------------
pqPointPickingHelper::pqPointPickingHelper(
  const QKeySequence& keySequence, bool pick_on_mesh, pqPropertyWidget* propWidget)
  : Superclass(propWidget)
  , PickOnMesh(pick_on_mesh)
{
  if (!propWidget)
  {
    throw std::invalid_argument("Parent property-widget must be non-null.");
  }
  auto* decorator = propWidget->findChild<pqShortcutDecorator*>();
  if (!decorator)
  {
    decorator = new pqShortcutDecorator(propWidget);
  }
  this->Shortcut =
    pqKeySequences::instance().addModalShortcut(keySequence, /* action */ nullptr, propWidget);
  decorator->addShortcut(this->Shortcut);
  this->connect(this->Shortcut, SIGNAL(activated()), SLOT(pickPoint()));
}

//-----------------------------------------------------------------------------
pqPointPickingHelper::~pqPointPickingHelper()
{
  delete this->Shortcut;
}

//-----------------------------------------------------------------------------
void pqPointPickingHelper::setView(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  if (rview == this->View)
  {
    return;
  }
  this->View = rview;
  if (rview)
  {
    this->Shortcut->setContextWidget(view->widget());
  }
}

//-----------------------------------------------------------------------------
void pqPointPickingHelper::pickPoint()
{
  pqRenderView* rview = this->View;
  if (rview && rview->getRenderViewProxy())
  {
    vtkRenderWindowInteractor* rwi = rview->getRenderViewProxy()->GetInteractor();
    if (!rwi)
    {
      return;
    }

    // Get region
    const int* eventpos = rwi->GetEventPosition();
    double position[3];
    if (rview->getRenderViewProxy()->ConvertDisplayToPointOnSurface(
          eventpos, position, this->PickOnMesh))
    {
      Q_EMIT this->pick(position[0], position[1], position[2]);
    }
  }
}
