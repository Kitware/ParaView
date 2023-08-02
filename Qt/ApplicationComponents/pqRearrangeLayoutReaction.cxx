// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqRearrangeLayoutReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "vtkSMViewLayoutProxy.h"

#include <QMainWindow>

//-----------------------------------------------------------------------------
pqRearrangeLayoutReaction::pqRearrangeLayoutReaction(Orientation orientation, QAction* parentObject)
  : Superclass(parentObject)
  , ActionOrientation(orientation)
{
}

//-----------------------------------------------------------------------------
void pqRearrangeLayoutReaction::onTriggered()
{
  vtkSMViewLayoutProxy* currentLayout = pqActiveObjects::instance().activeLayout();

  switch (this->ActionOrientation)
  {
    case Orientation::HORIZONTAL:
      currentLayout->RearrangeViews(vtkSMViewLayoutProxy::HORIZONTAL);
      break;
    case Orientation::VERTICAL:
      currentLayout->RearrangeViews(vtkSMViewLayoutProxy::VERTICAL);
      break;
    case Orientation::BOTH:
      currentLayout->RearrangeViews();
      break;
    default:
      break;
  }
}
