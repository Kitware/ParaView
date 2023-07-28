// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCTHArraySelectionDecorator.h"

#include "pqPropertyWidget.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
pqCTHArraySelectionDecorator::pqCTHArraySelectionDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  QObject::connect(parentObject, SIGNAL(changeFinished()), this, SLOT(updateSelection()));

  // Scan the config to determine the names of the other selection properties.
  for (unsigned int cc = 0; cc < config->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = config->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Property") == 0 &&
      child->GetAttribute("name"))
    {
      this->PropertyNames << child->GetAttribute("name");
    }
  }
}

//-----------------------------------------------------------------------------
pqCTHArraySelectionDecorator::~pqCTHArraySelectionDecorator() = default;

//-----------------------------------------------------------------------------
void pqCTHArraySelectionDecorator::updateSelection()
{
  vtkSMProperty* curProperty = this->parentWidget()->property();
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  if (vtkSMUncheckedPropertyHelper(curProperty).GetNumberOfElements() == 0)
  {
    return;
  }

  Q_FOREACH (const QString& pname, this->PropertyNames)
  {
    vtkSMProperty* prop = proxy->GetProperty(pname.toUtf8().data());
    if (prop && prop != curProperty)
    {
      vtkSMUncheckedPropertyHelper(prop).SetNumberOfElements(0);
    }
  }
}
