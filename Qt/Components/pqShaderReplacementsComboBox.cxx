/*=========================================================================

   Program: ParaView
   Module:    pqShaderReplacementsComboBox.cxx

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
  if (plist.size() == 0 || (plist.size() == 1 && plist[0] == ""))
  {
    this->addItem(tr("(No preset)"), "");
  }
  else
  {
    this->addItem(tr("(Select a preset)"), "");
    QSet<QString> pathSet;
    foreach (QString p, plist)
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
