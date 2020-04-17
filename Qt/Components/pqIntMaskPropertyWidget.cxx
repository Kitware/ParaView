/*=========================================================================

   Program: ParaView
   Module:  pqIntMaskPropertyWidget.cxx

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
#include "pqIntMaskPropertyWidget.h"

#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"

#include <QAction>
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
    hbox->setMargin(0);

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
  this->Internals->Button->setText(smproperty->GetXMLLabel());

  vtkPVXMLElement* hints =
    smproperty->GetHints() ? smproperty->GetHints()->FindNestedElementByName("Mask") : NULL;
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
      const char* name = elem->GetAttributeOrDefault("name", NULL);
      int value;
      if (name == NULL || name[0] == '\0' || !elem->GetScalarAttribute("value", &value))
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
pqIntMaskPropertyWidget::~pqIntMaskPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqIntMaskPropertyWidget::setMask(int ivalue)
{
  unsigned int value = static_cast<unsigned int>(ivalue);
  if (this->Internals->Mask != value)
  {
    this->Internals->Mask = value;
    foreach (QAction* actn, this->Internals->Menu->actions())
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
  foreach (QAction* actn, this->Internals->Menu->actions())
  {
    if (actn->isChecked())
    {
      unsigned int mask_flag = actn->data().value<unsigned int>();
      this->Internals->Mask |= mask_flag;
    }
  }
  return static_cast<int>(this->Internals->Mask);
}
