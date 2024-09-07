// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorPaletteSelectorWidget.h"

#include "vtkLogger.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QComboBox>
#include <QVBoxLayout>

#include <QCoreApplication>
#include <array>
#include <cassert>
#include <string>

//-----------------------------------------------------------------------------
pqColorPaletteSelectorWidget::pqColorPaletteSelectorWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  // this->setShowLabel(false);

  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setSpacing(0);
  vbox->setContentsMargins(0, 0, 0, 0);

  vtkSMSessionProxyManager* pxm = smproxy->GetSessionProxyManager();
  vtkSMProxyDefinitionManager* pdmgr = pxm->GetProxyDefinitionManager();

  QComboBox* cbbox = new QComboBox(this);
  cbbox->setObjectName("ComboBox");

  // Palette ordering / ban list can be found in issue #20707
  std::array<std::string, 8> mainPalettes = { "WarmGrayBackground", "BlueGrayBackground",
    "DarkGrayBackground", "NeutralGrayBackground", "LightGrayBackground", "WhiteBackground",
    "BlackBackground", "GradientBackground" };

  for (const std::string& str : mainPalettes)
  {
    if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", str.c_str()))
    {
      cbbox->addItem(
        QCoreApplication::translate("ServerManagerXML", prototype->GetXMLLabel()), str.c_str());
    }
    else
    {
      vtkLog(WARNING, "Missing palette: " << str.c_str());
    }
  }

  assert(pdmgr);
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pdmgr->NewSingleGroupIterator("palettes"));

  // If there were any other available palettes, we append them in alphabetical order.
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", iter->GetProxyName());
    if (std::find(mainPalettes.cbegin(), mainPalettes.cend(), iter->GetProxyName()) ==
      mainPalettes.cend())
    {
      cbbox->addItem(QCoreApplication::translate("ServerManagerXML", prototype->GetXMLLabel()),
        QString(prototype->GetXMLName()));
    }
  }

  if (cbbox->count() > 0)
  {
    cbbox->insertItem(0, tr("Select palette to load ..."), -1);
  }
  else
  {
    cbbox->insertItem(0, tr("No palettes available."));
    cbbox->setEnabled(false);
  }
  cbbox->setCurrentIndex(0);
  vbox->addWidget(cbbox);

  this->ComboBox = cbbox;

  // If a SVP is provided, then we will simply change is value to the selected palette,
  // otherwise we'll act as a "loader".
  if (vtkSMStringVectorProperty::SafeDownCast(smproperty))
  {
    this->addPropertyLink(this, "paletteName", SIGNAL(paletteNameChanged()), smproperty);
    this->connect(cbbox, SIGNAL(currentIndexChanged(int)), SIGNAL(paletteNameChanged()));
    cbbox->setItemText(0, "No change");
  }
  else
  {
    this->connect(cbbox, SIGNAL(currentIndexChanged(int)), SLOT(loadPalette(int)));
  }
}

//-----------------------------------------------------------------------------
pqColorPaletteSelectorWidget::~pqColorPaletteSelectorWidget() = default;

//-----------------------------------------------------------------------------
QString pqColorPaletteSelectorWidget::paletteName() const
{
  int index = this->ComboBox->currentIndex();
  if (index <= 0)
  {
    return QString(); // none selected.
  }

  return this->ComboBox->itemData(index).toString();
}

//-----------------------------------------------------------------------------
void pqColorPaletteSelectorWidget::setPaletteName(const QString& pname)
{
  int index = this->ComboBox->findData(QVariant(pname));
  this->ComboBox->setCurrentIndex(index <= 0 ? 0 : index);
}

//-----------------------------------------------------------------------------
void pqColorPaletteSelectorWidget::loadPalette(int index)
{
  vtkSMProxy* smproxy = this->proxy();
  assert(this->ComboBox);
  assert(smproxy);

  if (index <= 0)
  {
    return;
  }
  QString name = this->ComboBox->itemData(index).toString();

  vtkSMSessionProxyManager* pxm = smproxy->GetSessionProxyManager();
  vtkSMProxy* paletteProxy = pxm->GetPrototypeProxy("palettes", name.toUtf8().data());
  assert(paletteProxy);

  smproxy->Copy(paletteProxy);

  // return the combobox back to the "select .." text.
  this->ComboBox->setCurrentIndex(0);

  Q_EMIT this->changeAvailable();
}
