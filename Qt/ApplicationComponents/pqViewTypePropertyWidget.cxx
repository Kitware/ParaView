/*=========================================================================

   Program: ParaView
   Module:  pqViewTypePropertyWidget.cxx

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
#include "pqViewTypePropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqServer.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"

#include <QComboBox>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqViewTypePropertyWidget::pqViewTypePropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  this->ComboBox = new QComboBox(this);
  this->ComboBox->setObjectName("ComboBox");
  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setMargin(0);
  vbox->addWidget(this->ComboBox);

  this->ComboBox->addItem("None", QVariant("None"));
  this->ComboBox->addItem("Empty", QVariant("Empty"));

  // fill combo-box.
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().activeServer()
    ? pqActiveObjects::instance().activeServer()->proxyManager()
    : nullptr;

  QMap<QString, QString> valuesMap; // <-- used to sort the entries.
  if (pxm)
  {
    vtkPVProxyDefinitionIterator* iter =
      pxm->GetProxyDefinitionManager()->NewSingleGroupIterator("views");
    for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      // add label instead of proxy name to make this more user friendly.
      const char* proxyName = iter->GetProxyName();
      vtkSMProxy* prototype = pxm->GetPrototypeProxy("views", proxyName);
      if (prototype)
      {
        valuesMap.insert(prototype->GetXMLLabel(), iter->GetProxyName());
      }
    }
    iter->Delete();

    for (QMap<QString, QString>::iterator viter = valuesMap.begin(); viter != valuesMap.end();
         ++viter)
    {
      this->ComboBox->addItem(viter.key(), QVariant(viter.value()));
    }

    this->connect(this->ComboBox, SIGNAL(currentIndexChanged(int)), SIGNAL(valueChanged()));
    this->addPropertyLink(this, "value", SIGNAL(valueChanged()), smproperty);
  }
  else
  {
    this->ComboBox->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
pqViewTypePropertyWidget::~pqViewTypePropertyWidget() = default;

//-----------------------------------------------------------------------------
QString pqViewTypePropertyWidget::value() const
{
  return this->ComboBox->itemData(this->ComboBox->currentIndex()).toString();
}

//-----------------------------------------------------------------------------
void pqViewTypePropertyWidget::setValue(const QString& val)
{
  int index = this->ComboBox->findData(val);
  if (index == -1)
  {
    // add the value being specified to the combo-box.
    index = this->ComboBox->count();
    this->ComboBox->addItem(val, val);
  }
  this->ComboBox->setCurrentIndex(index);
}
