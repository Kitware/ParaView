/*=========================================================================

   Program: ParaView
   Module:    pqGlyphPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqGlyphPanel.h"

// Server Manager Includes.
#include "vtkGlyph3D.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

// Qt Includes.
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QtDebug>

// ParaView Includes.
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
pqGlyphPanel::pqGlyphPanel(pqProxy* object_proxy, QWidget* _parent)
  : Superclass(object_proxy, _parent), LockScaleFactor(0), ScaleFactorWidget(0),
  ScaleModeWidget(0)
{
  // By now, superclass will have created the default panel correctly.
  // Now we tweak the panel a bit to add the "lock" button.

  // Get the entry widget created of "SetScaleFactor" property and add a "lock"
  // button next to it. If lock button is set, we don't auto-adjust the scale
  // factor.
  QWidget* scaleFactor = this->findChild<QWidget*>("SetScaleFactor");
  if (!scaleFactor)
    {
    this->findChild<QWidget*>("ScaleFactor");
    }

  if (!scaleFactor)
    {
    qDebug() << "Failed to locate ScaleFactor widget.";
    return;
    }

  this->ScaleFactorWidget = scaleFactor; 

  int oldIndex = this->PanelLayout->indexOf(scaleFactor);
  int row, column, rowSpan, columnSpan;
  this->PanelLayout->getItemPosition(oldIndex, &row, &column, &rowSpan, &columnSpan);
  this->PanelLayout->removeWidget(scaleFactor);

  QCheckBox* lockButton = new QCheckBox(this);
  lockButton->setObjectName("LockScaleFactor");
  lockButton->setCheckable(true);
  lockButton->setTristate(false);
  lockButton->setText("Edit");
  lockButton->setToolTip(tr("<html>Edit the scale factor. "
      "Otherwise, the scale factor will be computed automatically when the scale mode"
      "changes.</html>")); 
  this->LockScaleFactor = lockButton;

  QHBoxLayout* subLayout = new QHBoxLayout();
  subLayout->addWidget(scaleFactor, 1);
  subLayout->addWidget(lockButton,0, Qt::AlignRight);
  subLayout->setMargin(0);
  subLayout->setSpacing(4);

  this->PanelLayout->addLayout(subLayout, row, column, rowSpan, columnSpan);

  QObject::connect(this->propertyManager(), SIGNAL(modified()),
    this, SLOT(updateScaleFactor()), Qt::QueuedConnection);

  this->ScaleModeWidget = this->findChild<QComboBox*>("SetScaleMode");

  QObject::connect(lockButton, SIGNAL(toggled(bool)),
    this->ScaleFactorWidget, SLOT(setEnabled(bool)));
  lockButton->toggle();
  lockButton->toggle();

  if (object_proxy->modifiedState() == pqProxy::UNINITIALIZED)
    {
    this->updateScaleFactor();
    }
}

//-----------------------------------------------------------------------------
pqGlyphPanel::~pqGlyphPanel()
{
}

//-----------------------------------------------------------------------------
void pqGlyphPanel::updateScaleFactor()
{
  if (!this->LockScaleFactor  ||
    this->LockScaleFactor->isChecked())
    {
    return;
    }

  vtkSMProxy* filterProxy = this->proxy();
  filterProxy->GetProperty("Input")->UpdateDependentDomains();

  vtkSMEnumerationDomain* enumDomain = vtkSMEnumerationDomain::SafeDownCast(
    filterProxy->GetProperty("SetScaleMode")->GetDomain("enum"));

  int valid;
  int scale_mode = enumDomain->GetEntryValue(
    this->ScaleModeWidget->currentText().toAscii().data(), valid);
  if (!valid)
    {
    return;
    }

  vtkSMProperty* scaleFactorProperty = filterProxy->GetProperty("SetScaleFactor");
  vtkSMArrayRangeDomain* domain=0;
  vtkSMBoundsDomain* boundsDomain = vtkSMBoundsDomain::SafeDownCast(
    scaleFactorProperty->GetDomain("bounds"));

  double scaled_extent = boundsDomain->GetMaximumExists(0)?
    boundsDomain->GetMaximum(0) : 1.0;

  double scalefactor = scaled_extent;
  double divisor = 1.0;
  switch (scale_mode)
    {
  case VTK_SCALE_BY_SCALAR:
      {
      domain = vtkSMArrayRangeDomain::SafeDownCast(
        scaleFactorProperty->GetDomain("scalar_range"));
      if (domain->GetMaximumExists(0) && domain->GetMinimumExists(0))
        {
        divisor = domain->GetMaximum(0)-domain->GetMinimum(0);
        }
      }
    break;

  case VTK_SCALE_BY_VECTOR:
  case VTK_SCALE_BY_VECTORCOMPONENTS:
      {
      domain = vtkSMArrayRangeDomain::SafeDownCast(
        scaleFactorProperty->GetDomain("vector_range"));
      if (domain->GetMaximumExists(3) && domain->GetMinimumExists(3))
        {
        // we use the vector magnitude.
        divisor = domain->GetMaximum(3)-domain->GetMinimum(3);
        }
      }
    break;

  case VTK_DATA_SCALING_OFF:
  default:
    break;
    }

  divisor = (divisor < 1.0)? 1 : divisor;
  scalefactor /= divisor;

  if (this->ScaleFactorWidget->property("text").toDouble() != scalefactor)
    {
    this->ScaleFactorWidget->setProperty("text", scalefactor);
    }
}

