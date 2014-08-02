/*=========================================================================

   Program: ParaView
   Module:  pqGlyphScaleFactorPropertyWidget.cxx

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
#include "pqGlyphScaleFactorPropertyWidget.h"

#include "pqCoreUtilities.h"
#include "pqLineEdit.h"
#include "vtkCommand.h"
#include "vtkGlyph3D.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqGlyphScaleFactorPropertyWidget::pqGlyphScaleFactorPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproperty, smproxy, parentObject)
{
  QLayout* layout = this->layout();
  Q_ASSERT(layout);

  QPushButton* button = new QPushButton(this);
  button->setObjectName("Reset");
  button->setToolTip("Reset using current data values");
  button->setIcon(button->style()->standardIcon(QStyle::SP_BrowserReload));
  layout->addWidget(button);

  this->ResetButton = button;
  this->ResetPalette = this->ResetButton->palette();

  this->connect(button, SIGNAL(clicked()), SLOT(resetClicked()));

  pqCoreUtilities::connect(smproperty, vtkCommand::DomainModifiedEvent,
    this, SLOT(highlightResetButton()));
  pqCoreUtilities::connect(smproperty, vtkCommand::UncheckedPropertyModifiedEvent,
    this, SLOT(highlightResetButton()));

  if (vtkSMProperty* scaleMode = smproxy->GetProperty("ScaleMode"))
    {
    pqCoreUtilities::connect(scaleMode, vtkCommand::UncheckedPropertyModifiedEvent,
      this, SLOT(highlightResetButton()));
    }
}

//-----------------------------------------------------------------------------
pqGlyphScaleFactorPropertyWidget::~pqGlyphScaleFactorPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqGlyphScaleFactorPropertyWidget::apply()
{
  this->Superclass::apply();
  this->highlightResetButton(false);
}

//-----------------------------------------------------------------------------
void pqGlyphScaleFactorPropertyWidget::reset()
{
  this->Superclass::reset();
  this->highlightResetButton(false);
}

//-----------------------------------------------------------------------------
void pqGlyphScaleFactorPropertyWidget::highlightResetButton(bool highlight)
{
  QPalette palette = this->ResetPalette;
  if (highlight)
    {
    palette.setColor(QPalette::Active, QPalette::Button, QColor(161, 213, 135));
    palette.setColor(QPalette::Inactive, QPalette::Button, QColor(161, 213, 135));
    }
  this->ResetButton->setPalette(palette);
}

//-----------------------------------------------------------------------------
void pqGlyphScaleFactorPropertyWidget::resetClicked()
{
  // Now this logic to hardcoded for the Glyph filter (hence the name of this
  // widget).

  // This logic has been ported directly from the old pqGlyphPanel class for the
  // most part.

  vtkSMProxy* smproxy = this->proxy();
  vtkSMProperty* smproperty = this->property();

  double scaledExtent = 1.0;
  if (vtkSMBoundsDomain* domain = vtkSMBoundsDomain::SafeDownCast(
      smproperty->GetDomain("bounds")))
    {
    if (domain->GetMaximumExists(0))
      {
      scaledExtent = domain->GetMaximum(0);
      }
    }

  double divisor = 1.0;
  switch (vtkSMUncheckedPropertyHelper(smproxy, "ScaleMode").GetAsInt())
    {
  case VTK_SCALE_BY_SCALAR:
    if (vtkSMArrayRangeDomain* domain = vtkSMArrayRangeDomain::SafeDownCast(
        smproperty->GetDomain("scalar_range")))
      {
      if (domain->GetMaximumExists(0) /*&& domain->GetMinimumExists(0)*/)
        {
        divisor = domain->GetMaximum(0)/*-domain->GetMinimum(0)*/;
        }
      }
    break;

  case VTK_SCALE_BY_VECTOR:
  case VTK_SCALE_BY_VECTORCOMPONENTS:
    if (vtkSMArrayRangeDomain* domain = vtkSMArrayRangeDomain::SafeDownCast(
        smproperty->GetDomain("vector_range")))
      {
      if (domain->GetMaximumExists(3)/* && domain->GetMinimumExists(3)*/)
        {
        // we use the vector magnitude.
        divisor = domain->GetMaximum(3)/*-domain->GetMinimum(3)*/;
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
  divisor = (divisor < 0.000000001)? 1 : divisor;
  double scalefactor = scaledExtent / divisor;

  vtkSMUncheckedPropertyHelper helper(smproperty);
  if (helper.GetAsDouble() != scalefactor)
    {
    vtkSMUncheckedPropertyHelper(smproperty).Set(scalefactor);
    this->highlightResetButton(false);
    emit this->changeAvailable();
    emit this->changeFinished();
    }
}
