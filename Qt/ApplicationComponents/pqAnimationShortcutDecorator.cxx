/*=========================================================================

   Program: ParaView
   Module:  pqAnimationShortcutDecorator.cxx

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
  : Superclass(NULL, parentWidget)
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
pqAnimationShortcutDecorator::~pqAnimationShortcutDecorator()
{
}

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
