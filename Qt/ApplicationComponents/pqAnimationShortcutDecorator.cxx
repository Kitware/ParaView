// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnimationShortcutDecorator.h"

#include "pqAnimationShortcutWidget.h"
#include "pqCoreUtilities.h"
#include "pqDoubleVectorPropertyWidget.h"

#include "vtkPVGeneralSettings.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMVectorProperty.h"

#include <QLayout>

//-----------------------------------------------------------------------------
pqAnimationShortcutDecorator::pqAnimationShortcutDecorator(pqPropertyWidget* parentWidget)
  : Superclass(nullptr, parentWidget)
{
  QLayout* layout = parentWidget->layout();
  // Adding pqAnimationShortcutWidget
  this->Widget =
    new pqAnimationShortcutWidget(parentWidget, parentWidget->proxy(), parentWidget->property());
  layout->addWidget(this->Widget);

  // Get the current ShowShortcutAnimation setting
  vtkPVGeneralSettings* generalSettings = vtkPVGeneralSettings::GetInstance();
  this->Widget->setVisible(generalSettings->GetShowAnimationShortcuts());

  // Connect to a slot to catch when the settings changes
  pqCoreUtilities::connect(
    generalSettings, vtkCommand::ModifiedEvent, this, SLOT(generalSettingsChanged()));
}

//-----------------------------------------------------------------------------
pqAnimationShortcutDecorator::~pqAnimationShortcutDecorator() = default;

//-----------------------------------------------------------------------------
bool pqAnimationShortcutDecorator::accept(pqPropertyWidget* parentWidget)
{
  if (parentWidget)
  {
    // Test the property widget
    vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(parentWidget->property());
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(parentWidget->proxy());
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(parentWidget->proxy());
    // Its proxy should be a source, but not a representation.
    // Its property should be a VectorProperty of a single element
    if (source && !repr && vp && vp->GetNumberOfElements() == 1 && vp->GetAnimateable())
    {
      bool hasRange = true;
      vtkSMDomain* domain = vp->FindDomain<vtkSMDoubleRangeDomain>();
      if (!domain)
      {
        domain = vp->FindDomain<vtkSMIntRangeDomain>();
        if (!domain)
        {
          hasRange = false;
        }
      }

      // And it must contain a range domain
      if (hasRange)
      {
        return true;
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqAnimationShortcutDecorator::generalSettingsChanged()
{
  this->Widget->setVisible(vtkPVGeneralSettings::GetInstance()->GetShowAnimationShortcuts());
}
