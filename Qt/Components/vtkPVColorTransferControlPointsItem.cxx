/*=========================================================================

   Program: ParaView
   Module:    vtkPVColorTransferControlPointsItem.cxx

   Copyright (c) 2005-2022 Sandia Corporation, Kitware Inc.
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
