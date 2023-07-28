// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
  l->setContentsMargins(0, 0, 0, 0);

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
  loadButton->setIcon(QIcon(":/pqWidgets/Icons/pqNewFolder.svg"));
  QObject::connect(loadButton, SIGNAL(clicked()), this, SLOT(onLoad()));
  gridLayout->addWidget(loadButton, 0, 1);

  QToolButton* delButton = new QToolButton(this);
  delButton->setText(tr("Delete"));
  delButton->setToolTip(tr("Delete the selected preset from the preset list or clear the current "
                           "shader replacement string."));
  delButton->setIcon(QIcon(":/QtWidgets/Icons/pqDelete.svg"));
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
  std::string newValue = this->Internal->TextEdit->toPlainText().toStdString();
  if (currentValue == newValue)
  {
    // Don't do anything if property and textedit are already synchronized
    return;
  }

  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->proxy());

  BEGIN_UNDO_SET(tr("Shader Replacements Change"));
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
  QString filters = tr("Shader replacements files") + QString(" (*.json);;") + tr("All files (*)");
  pqFileDialog dialog(nullptr, this, tr("Open ShaderReplacements:"), QString(), filters, false);
  dialog.setObjectName("LoadShaderReplacementsDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec())
  {
    QStringList files = dialog.getSelectedFiles();
    if (!files.empty() && this->loadShaderReplacements(files[0]))
    {
      if (this->Internal->ComboBox->getPathIndex(files[0]) == 0)
      {
        // Selected file is not in the current preset list, add it
        vtkSMSettings* settings = vtkSMSettings::GetInstance();
        QString newPaths = QString::fromStdString(settings->GetSettingAsString(
          pqShaderReplacementsComboBox::ShaderReplacementPathsSettings, ""));
        if (!newPaths.isEmpty())
        {
          newPaths += QDir::listSeparator();
        }
        newPaths += files[0];

        settings->SetSetting(pqShaderReplacementsComboBox::ShaderReplacementPathsSettings,
          newPaths.toUtf8().toStdString());

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
    QString paths = QString::fromStdString(settings->GetSettingAsString(
      pqShaderReplacementsComboBox::ShaderReplacementPathsSettings, ""));
    QStringList plist = paths.split(QDir::listSeparator());
    QString newPaths;
    int count = 0;
    Q_FOREACH (QString p, plist)
    {
      if (p != presetPath)
      {
        if (count > 0)
        {
          newPaths += QDir::listSeparator();
        }
        newPaths += p;
      }
    }
    settings->SetSetting(pqShaderReplacementsComboBox::ShaderReplacementPathsSettings,
      newPaths.toUtf8().toStdString());
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
