// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqIntMaskPropertyWidget.h"

#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"

#include <QAction>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QtDebug>

class pqIntMaskPropertyWidget::pqInternals
{
public:
  QPointer<QPushButton> Button;
  QPointer<QMenu> Menu;
  unsigned int Mask;

  pqInternals(pqIntMaskPropertyWidget* self)
    : Mask(0)
  {
    QHBoxLayout* hbox = new QHBoxLayout(self);
    hbox->setContentsMargins(0, 0, 0, 0);

    this->Button = new QPushButton(self);
    hbox->addWidget(this->Button);
    this->Button->setObjectName("Button");

    this->Menu = new QMenu(self);
    this->Menu->setObjectName("Menu");

    this->Button->setMenu(this->Menu);
  }
};

//-----------------------------------------------------------------------------
pqIntMaskPropertyWidget::pqIntMaskPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals(this))
{
  this->setShowLabel(false);
  this->Internals->Button->setText(
    QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel()));

  vtkPVXMLElement* hints =
    smproperty->GetHints() ? smproperty->GetHints()->FindNestedElementByName("Mask") : nullptr;
  if (!hints)
  {
    qCritical() << "Missing 'Mask' hints for property! pqIntMaskPropertyWidget cannot work.";
    return;
  }
  for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; cc++)
  {
    vtkPVXMLElement* elem = hints->GetNestedElement(cc);
    if (elem && elem->GetName() && strcmp(elem->GetName(), "Item") == 0)
    {
      const char* name = elem->GetAttributeOrDefault("name", nullptr);
      int value;
      if (name == nullptr || name[0] == '\0' || !elem->GetScalarAttribute("value", &value))
      {
        qCritical("'Item' must have a 'name' and an integer 'value'.");
        continue;
      }
      QAction* actn = this->Internals->Menu->addAction(name);
      actn->setData(static_cast<unsigned int>(value));
      actn->setCheckable(true);
      actn->setChecked(false);
      this->connect(actn, SIGNAL(toggled(bool)), SIGNAL(maskChanged()));
    }
  }

  this->addPropertyLink(this, "mask", SIGNAL(maskChanged()), smproperty);
}

//-----------------------------------------------------------------------------
pqIntMaskPropertyWidget::~pqIntMaskPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqIntMaskPropertyWidget::setMask(int ivalue)
{
  unsigned int value = static_cast<unsigned int>(ivalue);
  if (this->Internals->Mask != value)
  {
    this->Internals->Mask = value;
    Q_FOREACH (QAction* actn, this->Internals->Menu->actions())
    {
      unsigned int mask_flag = actn->data().value<unsigned int>();
      actn->setChecked((value & mask_flag) != 0);
    }
    Q_EMIT this->maskChanged();
  }
}

//-----------------------------------------------------------------------------
int pqIntMaskPropertyWidget::mask() const
{
  this->Internals->Mask = 0;
  Q_FOREACH (QAction* actn, this->Internals->Menu->actions())
  {
    if (actn->isChecked())
    {
      unsigned int mask_flag = actn->data().value<unsigned int>();
      this->Internals->Mask |= mask_flag;
    }
  }
  return static_cast<int>(this->Internals->Mask);
}
