// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqGlyphScaleFactorPropertyWidget.h"

#include "pqCoreUtilities.h"
#include "vtkCommand.h"
#include "vtkGlyph3D.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QtDebug>

#include <cmath>

//-----------------------------------------------------------------------------
pqGlyphScaleFactorPropertyWidget::pqGlyphScaleFactorPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproperty, smproxy, parentObject)
{
  // We add this extra dependency for the reset button.
  if (vtkSMProperty* scaleMode = smproxy->GetProperty("ScaleMode"))
  {
    pqCoreUtilities::connect(
      scaleMode, vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(highlightResetButton()));
  }
}

//-----------------------------------------------------------------------------
pqGlyphScaleFactorPropertyWidget::~pqGlyphScaleFactorPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqGlyphScaleFactorPropertyWidget::resetButtonClicked()
{
  // Now this logic to hardcoded for the Glyph filter (hence the name of this
  // widget).

  // This logic has been ported directly from the old pqGlyphPanel class for the
  // most part.

  // It uses the GetDomain("name") method, which is based on named domains.
  // We should rewrite this at some point.

  vtkSMProxy* smproxy = this->proxy();
  vtkSMProperty* smproperty = this->property();

  double scaledExtent = 1.0;
  if (auto domain = smproperty->FindDomain<vtkSMBoundsDomain>())
  {
    if (domain->GetMaximumExists(0))
    {
      scaledExtent = domain->GetMaximum(0);
    }
  }

  double divisor = 1.0;
  switch (vtkSMUncheckedPropertyHelper(smproxy, "ScaleMode", /*quiet*/ true).GetAsInt())
  {
    case VTK_SCALE_BY_SCALAR:
      if (vtkSMArrayRangeDomain* domain =
            vtkSMArrayRangeDomain::SafeDownCast(smproperty->GetDomain("scalar_range")))
      {
        if (domain->GetMaximumExists(0) /*&& domain->GetMinimumExists(0)*/)
        {
          divisor = domain->GetMaximum(0) /*-domain->GetMinimum(0)*/;
        }
      }
      break;

    case VTK_SCALE_BY_VECTOR:
    case VTK_SCALE_BY_VECTORCOMPONENTS:
      if (vtkSMArrayRangeDomain* domain =
            vtkSMArrayRangeDomain::SafeDownCast(smproperty->GetDomain("vector_range")))
      {
        if (domain->GetMaximumExists(3) /* && domain->GetMinimumExists(3)*/)
        {
          // we use the vector magnitude.
          divisor = domain->GetMaximum(3) /*-domain->GetMinimum(3)*/;
        }
      }
      break;

    case VTK_DATA_SCALING_OFF:
    default:
      break;
  }

  divisor = fabs(divisor);
  // the divisor can sometimes be very close to 0, which happens in case the
  // vectors indeed have same value but due to precision issues are not reported
  // as identical. In that case we just treat it as 0.
  divisor = (divisor < 0.000000001) ? 1 : divisor;
  double scalefactor = scaledExtent / divisor;

  vtkSMUncheckedPropertyHelper helper(smproperty);
  if (helper.GetAsDouble() != scalefactor)
  {
    vtkSMUncheckedPropertyHelper(smproperty).Set(scalefactor);
    Q_EMIT this->changeAvailable();
    Q_EMIT this->changeFinished();
  }

  Q_EMIT this->clearHighlight();
}
