// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqFileNamePropertyWidget.h"

#include "vtkCommand.h"
#include "vtkSMInputFileNameDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSmartPointer.h"

#include "pqCoreUtilities.h"
#include "pqDoubleRangeWidget.h"
#include "pqHighlightableToolButton.h"
#include "pqLineEdit.h"
#include "pqListPropertyWidget.h"
#include "pqPropertiesPanel.h"
#include "pqSignalAdaptors.h"
#include "pqWidgetRangeDomain.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QStyle>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqFileNamePropertyWidget::pqFileNamePropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : pqPropertyWidget(smproxy, parentObject)
{
  this->setChangeAvailableAsChangeFinished(false);

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(smproperty);
  if (!svp)
  {
    return;
  }

  // find the domain
  vtkSmartPointer<vtkSMInputFileNameDomain> domain = svp->FindDomain<vtkSMInputFileNameDomain>();
  if (!domain)
  {
    domain = vtkSmartPointer<vtkSMInputFileNameDomain>::New();
  }

  QHBoxLayout* layoutLocal = new QHBoxLayout;
  layoutLocal->setContentsMargins(0, 0, 0, 0);
  layoutLocal->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

  QLineEdit* lineEdit = new pqLineEdit(this);
  lineEdit->setObjectName(smproxy->GetPropertyName(smproperty));
  this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(const QString&)), smproperty);
  this->connect(lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));
  this->setChangeAvailableAsChangeFinished(false);

  layoutLocal->addWidget(lineEdit);

  // add a "reset" button.
  pqHighlightableToolButton* resetButton = new pqHighlightableToolButton(this);
  resetButton->setObjectName("Reset");
  QAction* resetActn = new QAction(resetButton);
  resetActn->setToolTip(tr("Reset using current data values"));
  resetActn->setIcon(QIcon(":/pqWidgets/Icons/pqReset.svg"));
  resetButton->addAction(resetActn);
  resetButton->setDefaultAction(resetActn);

  pqCoreUtilities::connect(
    svp, vtkCommand::DomainModifiedEvent, this, SIGNAL(highlightResetButton()));
  pqCoreUtilities::connect(
    svp, vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(highlightResetButton()));

  this->connect(resetButton, SIGNAL(clicked()), SLOT(resetButtonClicked()));
  resetButton->connect(this, SIGNAL(highlightResetButton()), SLOT(highlight()));
  resetButton->connect(this, SIGNAL(clearHighlight()), SLOT(clear()));

  layoutLocal->addWidget(resetButton);

  this->setLayout(layoutLocal);
}

//-----------------------------------------------------------------------------
pqFileNamePropertyWidget::~pqFileNamePropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqFileNamePropertyWidget::resetButtonClicked()
{
  // Logic is hardcoded for the Environment Annotation filter (hence the name of this
  // widget).

  vtkSMProxy* smproxy = this->proxy();
  vtkSMProperty* smproperty = this->property();

  const char* fileName = "";
  if (auto domain = smproperty->FindDomain<vtkSMInputFileNameDomain>())
  {
    if (!domain->GetFileName().empty())
    {
      fileName = domain->GetFileName().c_str();
    }
  }

  vtkSMUncheckedPropertyHelper helper(smproperty);
  if (strcmp(helper.GetAsString(), fileName) != 0)
  {
    vtkSMUncheckedPropertyHelper(smproxy, "FileName").Set(fileName);
    Q_EMIT this->changeAvailable();
    Q_EMIT this->changeFinished();
    return;
  }
}
