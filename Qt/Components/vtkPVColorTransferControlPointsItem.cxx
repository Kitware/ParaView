// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVColorTransferControlPointsItem.h"

#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkLogger.h"

vtkStandardNewMacro(vtkPVColorTransferControlPointsItem);

bool vtkPVColorTransferControlPointsItem::MouseDoubleClickEvent(const vtkContextMouseEvent& mouse)
{
  // ignore any double-click with the left mouse button.
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    return true;
  }
  return this->Superclass::MouseDoubleClickEvent(mouse);
}

bool vtkPVColorTransferControlPointsItem::KeyPressEvent(const vtkContextKeyEvent& key)
{
  // Enter or Return was pressed, edit the color of the current point.
  if (key.GetInteractor()->GetKeySym() == std::string("Return"))
  {
    this->InvokeEvent(vtkControlPointsItem::CurrentPointEditEvent);
    return true;
  }
  return this->Superclass::KeyPressEvent(key);
}

vtkPVColorTransferControlPointsItem::vtkPVColorTransferControlPointsItem() = default;
vtkPVColorTransferControlPointsItem::~vtkPVColorTransferControlPointsItem() = default;
