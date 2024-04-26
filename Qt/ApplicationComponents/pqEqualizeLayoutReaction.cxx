// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEqualizeLayoutReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "vtkSMTrace.h"
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
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("EqualizeViewsHorizontally")
        .arg("layout", currentLayout)
        .arg("comment", "equalize view sizes in layout horizontally");
      currentLayout->EqualizeViews(vtkSMViewLayoutProxy::HORIZONTAL);
    }
    break;
    case Orientation::VERTICAL:
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("EqualizeViewsVertically")
        .arg("layout", currentLayout)
        .arg("comment", "equalize view sizes in layout vertically");
      currentLayout->EqualizeViews(vtkSMViewLayoutProxy::VERTICAL);
    }
    break;
    case Orientation::BOTH:
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("EqualizeViewsBoth")
        .arg("layout", currentLayout)
        .arg("comment", "equalize view sizes in layout both vertically and horizontally");
      currentLayout->EqualizeViews();
    }
    break;
    default:
      break;
  }
}
