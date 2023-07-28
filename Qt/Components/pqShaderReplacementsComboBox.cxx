// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqShaderReplacementsComboBox.h"

// Server Manager Includes.
#include "vtkSMSettings.h"

// Qt Includes.
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStringList>

const char* pqShaderReplacementsComboBox::ShaderReplacementPathsSettings =
  "ShaderReplacements.Paths";

//-----------------------------------------------------------------------------
pqShaderReplacementsComboBox::pqShaderReplacementsComboBox(QWidget* _parent)
  : Superclass(_parent)
{
  this->populate();
}

//-----------------------------------------------------------------------------
int pqShaderReplacementsComboBox::getPathIndex(const QString& path) const
{
  for (int i = 1; i < this->count(); i++)
  {
    if (this->itemData(i).toString() == path)
    {
      return i;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsComboBox::setPath(const QString& path)
{
  this->setCurrentIndex(this->getPathIndex(path));
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsComboBox::populate()
{
  // Refresh the whole content of the combobox
  this->blockSignals(true);
  this->clear();

  // Get and process the preset paths saved in the user settings
  QString paths(
    vtkSMSettings::GetInstance()
      ->GetSettingAsString(pqShaderReplacementsComboBox::ShaderReplacementPathsSettings, "")
      .c_str());
  QStringList plist = paths.split(QDir::listSeparator());
  if (plist.empty() || (plist.size() == 1 && plist[0] == ""))
  {
    this->addItem(QString("(%1)").arg("No preset"), "");
  }
  else
  {
    this->addItem(QString("(%1)").arg("Select a preset"), "");
    QSet<QString> pathSet;
    Q_FOREACH (QString p, plist)
    {
      if (p != "" && !pathSet.contains(p))
      {
        this->addItem(QFileInfo(p).fileName(), QVariant(p));
        pathSet.insert(p);
      }
    }
  }

  this->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsComboBox::showPopup()
{
  this->populate();
  QComboBox::showPopup();
}
