/*=========================================================================

   Program: ParaView
   Module: pqShaderReplacementsSelectorPropertyWidget.h

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

=========================================================================*/

#include "pqShaderReplacementsSelectorPropertyWidget.h"

#include <vtkEventQtSlotConnect.h>
#include <vtkNew.h>

#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QToolButton>
#include <QVBoxLayout>

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqPipelineRepresentation.h"
#include "pqServerManagerModel.h"
#include "pqShaderReplacementsComboBox.h"
#include "pqTextEdit.h"
#include "pqUndoStack.h"
#include "pqView.h"

#include "vtkSMSettings.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"

//-----------------------------------------------------------------------------
class pqShaderReplacementsSelectorPropertyWidget::pqInternal
{
public:
  pqInternal(vtkSMStringVectorProperty* property)
    : Property(property)
  {
  }

  vtkNew<vtkEventQtSlotConnect> VTKConnect;
  vtkSMStringVectorProperty* Property;

  pqShaderReplacementsComboBox* ComboBox;
  pqTextEdit* TextEdit;
};

//-----------------------------------------------------------------------------
pqShaderReplacementsSelectorPropertyWidget::pqShaderReplacementsSelectorPropertyWidget(
  vtkSMProxy* smProxy, vtkSMProperty* smProperty, QWidget* pWidget)
  : pqPropertyWidget(smProxy, pWidget)
{
  this->Internal = new pqInternal(vtkSMStringVectorProperty::SafeDownCast(smProperty));

  QVBoxLayout* l = new QVBoxLayout;
  l->setMargin(0);

  QGridLayout* gridLayout = new QGridLayout;
  gridLayout->setColumnStretch(0, 0);
  gridLayout->setColumnStretch(1, 1);
  gridLayout->setColumnStretch(2, 1);

  this->Internal->ComboBox = new pqShaderReplacementsComboBox(this);
  this->Internal->ComboBox->setToolTip(tr("List of previously loaded Json files"));
  QObject::connect(
    this->Internal->ComboBox, SIGNAL(activated(int)), this, SLOT(onPresetChanged(int)));

  gridLayout->addWidget(this->Internal->ComboBox, 0, 0);
  QToolButton* loadButton = new QToolButton(this);
  loadButton->setText(tr("Load"));
  loadButton->setToolTip(tr("Load a Json preset file"));
  loadButton->setIcon(QIcon(":/pqWidgets/Icons/pqFolderNew16.png"));
  QObject::connect(loadButton, SIGNAL(clicked()), this, SLOT(onLoad()));
  gridLayout->addWidget(loadButton, 0, 1);

  QToolButton* delButton = new QToolButton(this);
  delButton->setText(tr("Delete"));
  delButton->setToolTip(tr("Delete the selected preset from the preset list or clear the current "
                           "shader replacement string."));
  delButton->setIcon(QIcon(":/pqWidgets/Icons/pqQuit22.png"));
  QObject::connect(delButton, SIGNAL(clicked()), this, SLOT(onDelete()));
  gridLayout->addWidget(delButton, 0, 2);

  this->Internal->TextEdit = new pqTextEdit(this);
  QObject::connect(this->Internal->TextEdit, SIGNAL(textChangedAndEditingFinished()), this,
    SLOT(textChangedAndEditingFinished()));
  QObject::connect(this->Internal->TextEdit, SIGNAL(editingFinished()), this,
    SLOT(textChangedAndEditingFinished()));

  l->addLayout(gridLayout);
  l->addWidget(this->Internal->TextEdit);
  this->setLayout(l);

  this->Internal->VTKConnect->Connect(
    smProperty, vtkCommand::ModifiedEvent, this, SLOT(updateShaderReplacements()));

  this->updateShaderReplacements();
}

//-----------------------------------------------------------------------------
pqShaderReplacementsSelectorPropertyWidget::~pqShaderReplacementsSelectorPropertyWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsSelectorPropertyWidget::refreshView()
{
  pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineRepresentation* repr = smm->findItem<pqPipelineRepresentation*>(proxy());
  if (repr)
  {
    pqView* view = repr->getView();
    if (view)
    {
      view->render();
    }
  }
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsSelectorPropertyWidget::updateShaderReplacements()
{
  this->Internal->TextEdit->setText(this->Internal->Property->GetElement(0));
  this->refreshView();
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsSelectorPropertyWidget::textChangedAndEditingFinished()
{
  std::string currentValue = this->Internal->Property->GetElement(0);
  std::string newValue = this->Internal->TextEdit->toPlainText().toStdString().c_str();
  if (currentValue == newValue)
  {
    // Don't do anything if property and textedit are already synchronized
    return;
  }

  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->proxy());

  BEGIN_UNDO_SET("Shader Replacements Change");
  this->Internal->Property->SetElement(0, newValue.c_str());
  proxy()->UpdateVTKObjects();
  END_UNDO_SET();

  // Reset the combobox and select the first entry (i.e. unselect current preset)
  this->Internal->ComboBox->populate();
  this->Internal->ComboBox->setCurrentIndex(0);

  this->refreshView();
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsSelectorPropertyWidget::onPresetChanged(int index)
{
  QString presetPath = this->Internal->ComboBox->itemData(index).toString();

  if (presetPath != "")
  {
    this->loadShaderReplacements(presetPath);
    // Select the selected current preset
    this->Internal->ComboBox->setPath(presetPath);
  }
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsSelectorPropertyWidget::onLoad()
{
  dynamic_cast<QToolButton*>(this->sender())->setChecked(false);
  // Popup load texture dialog.
  QString filters = "Shader replacements files (*.json);;All files (*)";
  pqFileDialog dialog(0, this, tr("Open ShaderReplacements:"), QString(), filters);
  dialog.setObjectName("LoadShaderReplacementsDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec())
  {
    QStringList files = dialog.getSelectedFiles();
    if (files.size() > 0 && this->loadShaderReplacements(files[0]))
    {
      if (this->Internal->ComboBox->getPathIndex(files[0]) == 0)
      {
        // Selected file is not in the current preset list, add it
        vtkSMSettings* settings = vtkSMSettings::GetInstance();
        std::string newPaths = settings->GetSettingAsString(
          pqShaderReplacementsComboBox::ShaderReplacementPathsSettings, "");
        if (newPaths != "")
        {
          newPaths += std::string(1, QDir::listSeparator().toLatin1());
        }
        newPaths += files[0].toStdString();
        settings->SetSetting(
          pqShaderReplacementsComboBox::ShaderReplacementPathsSettings, newPaths);

        // Refresh the combobox with new preset entry
        this->Internal->ComboBox->populate();
      }
      // Select file as current preset
      this->Internal->ComboBox->setPath(files[0]);
      return;
    }
  }
}

//-----------------------------------------------------------------------------
bool pqShaderReplacementsSelectorPropertyWidget::loadShaderReplacements(const QString& filename)
{
  // Read the whole content of the file and set it as text of the textedit
  QFile f(filename);
  if (!f.open(QFile::ReadOnly | QFile::Text))
  {
    return false;
  }
  QTextStream in(&f);
  this->Internal->TextEdit->setText(in.readAll());
  this->textChangedAndEditingFinished();

  return true;
}

//-----------------------------------------------------------------------------
void pqShaderReplacementsSelectorPropertyWidget::onDelete()
{
  QVariant presetPath =
    this->Internal->ComboBox->itemData(this->Internal->ComboBox->currentIndex()).toString();

  if (presetPath != "")
  {
    // A preset is currently selected, remove it from the saved presets
    vtkSMSettings* settings = vtkSMSettings::GetInstance();
    QString paths(
      settings->GetSettingAsString(pqShaderReplacementsComboBox::ShaderReplacementPathsSettings, "")
        .c_str());
    QStringList plist = paths.split(QDir::listSeparator());
    std::string newPaths;
    int count = 0;
    foreach (QString p, plist)
    {
      if (p != presetPath)
      {
        if (count > 0)
        {
          newPaths += std::string(1, QDir::listSeparator().toLatin1());
        }
        newPaths += p.toStdString();
      }
    }
    settings->SetSetting(pqShaderReplacementsComboBox::ShaderReplacementPathsSettings, newPaths);
  }
  else
  {
    // No preset was selected, clear the text box
    this->Internal->TextEdit->setText("");
    this->textChangedAndEditingFinished();
  }

  // Update the combobox
  this->Internal->ComboBox->populate();
  this->Internal->ComboBox->setCurrentIndex(0);
}
