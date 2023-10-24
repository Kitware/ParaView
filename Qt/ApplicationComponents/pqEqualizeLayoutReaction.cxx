// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEqualizeLayoutReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "vtkSMViewLayoutProxy.h"

#include <QMainWindow>

//-----------------------------------------------------------------------------
pqEqualizeLayoutReaction::pqEqualizeLayoutReaction(Orientation orientation, QAction* parentObject)
  : Superclass(parentObject)
  , ActionOrientation(orientation)
{
}

//-----------------------------------------------------------------------------
void pqEqualizeLayoutReaction::onTriggered()
{
  vtkSMViewLayoutProxy* currentLayout = pqActiveObjects::instance().activeLayout();

  switch (this->ActionOrientation)
  {
    case Orientation::HORIZONTAL:
      currentLayout->EqualizeViews(vtkSMViewLayoutProxy::HORIZONTAL);
      break;
    case Orientation::VERTICAL:
      currentLayout->EqualizeViews(vtkSMViewLayoutProxy::VERTICAL);
      break;
    case Orientation::BOTH:
      currentLayout->EqualizeViews();
      break;
    default:
      break;
  }
}
