/*=========================================================================

   Program: ParaView
   Module:    pqCPExportStateWizard.cxx

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
#include "pqCPExportStateWizard.h"
#include <QMessageBox>
#include <QPointer>
#include <QRegExp>
#include <QRegExpValidator>
#include <QWizardPage>

#include <pqApplicationCore.h>
#include <pqFileDialog.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqPythonDialog.h>
#include <pqPythonManager.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>

#include <vtkPVXMLElement.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtksys/SystemTools.hxx>

extern const char* cp_export_py;

// HACK.
static QPointer<pqCPExportStateWizard> ActiveWizard;

class pqCPExportStateWizardPage2 : public QWizardPage
{
  pqCPExportStateWizard::pqInternals* Internals;
public:
  pqCPExportStateWizardPage2(QWidget* _parent=0)
    : QWizardPage(_parent)
    {
    this->Internals = ::ActiveWizard->Internals;
    }

  virtual void initializePage();

  virtual bool isComplete() const;

  void emitCompleteChanged()
    { emit this->completeChanged(); }
};

class pqCPExportStateWizardPage3: public QWizardPage
{
  pqCPExportStateWizard::pqInternals* Internals;
public:
  pqCPExportStateWizardPage3(QWidget* _parent=0)
    : QWizardPage(_parent)
    {
    this->Internals = ::ActiveWizard->Internals;
    }

  virtual void initializePage();
};

#include "ui_ExportStateWizard.h"

class pqCPExportStateWizard::pqInternals : public Ui::ExportStateWizard
{
};

//-----------------------------------------------------------------------------
void pqCPExportStateWizardPage2::initializePage()
{
  this->Internals->simulationInputs->clear();
  this->Internals->allInputs->clear();
  QList<pqPipelineSource*> sources =
    pqApplicationCore::instance()->getServerManagerModel()->
    findItems<pqPipelineSource*>();
  foreach (pqPipelineSource* source, sources)
    {
    if (qobject_cast<pqPipelineFilter*>(source))
      {
      continue;
      }
    this->Internals->allInputs->addItem(source->getSMName());
    }
}

//-----------------------------------------------------------------------------
bool pqCPExportStateWizardPage2::isComplete() const
{
  return this->Internals->simulationInputs->count() > 0;
}

//-----------------------------------------------------------------------------
void pqCPExportStateWizardPage3::initializePage()
{
  this->Internals->nameWidget->clearContents();
  this->Internals->nameWidget->setRowCount(
    this->Internals->simulationInputs->count());
  for (int cc=0; cc < this->Internals->simulationInputs->count(); cc++)
    {
    QListWidgetItem* item = this->Internals->simulationInputs->item(cc);
    QString text = item->text();
    this->Internals->nameWidget->setItem(cc, 0, new QTableWidgetItem(text));
    this->Internals->nameWidget->setItem(cc, 1, new QTableWidgetItem(text));
    QTableWidgetItem* tableItem = this->Internals->nameWidget->item(cc, 1);
    tableItem->setFlags(tableItem->flags()|Qt::ItemIsEditable);

    tableItem = this->Internals->nameWidget->item(cc, 0);
    tableItem->setFlags(tableItem->flags() & ~Qt::ItemIsEditable);
    }
}

//-----------------------------------------------------------------------------
pqCPExportStateWizard::pqCPExportStateWizard(
  QWidget *parentObject, Qt::WindowFlags parentFlags)
: Superclass(parentObject, parentFlags)
{
  ::ActiveWizard = this;
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  ::ActiveWizard = NULL;
  //this->setWizardStyle(ModernStyle);
  this->setOption(QWizard::NoCancelButton, false);
  this->Internals->imageFileName->hide();
  this->Internals->imageType->hide();
  this->Internals->imageWriteFrequency->hide();

  this->Internals->imageFileNameLabel->hide();
  this->Internals->imageTypeLabel->hide();
  this->Internals->imageWriteFrequencyLabel->hide();

  QObject::connect(this->Internals->allInputs, SIGNAL(itemSelectionChanged()),
    this, SLOT(updateAddRemoveButton()));
  QObject::connect(this->Internals->simulationInputs, SIGNAL(itemSelectionChanged()),
    this, SLOT(updateAddRemoveButton()));
  QObject::connect(this->Internals->addButton, SIGNAL(clicked()),
    this, SLOT(onAdd()));
  QObject::connect(this->Internals->removeButton, SIGNAL(clicked()),
    this, SLOT(onRemove()));

  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->imageType, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->imageFileName, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->imageWriteFrequency, SLOT(setVisible(bool)));

  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->imageTypeLabel, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->imageFileNameLabel, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->imageWriteFrequencyLabel, SLOT(setVisible(bool)));

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  this->NumberOfViews = smModel->getNumberOfItems<pqRenderView*>();
  if(this->NumberOfViews > 1)
    {
    this->Internals->imageFileName->setText("image_%v_%t.png");
    }
  QObject::connect(
    this->Internals->imageFileName, SIGNAL(editingFinished()),
    this, SLOT(updateImageFileName()));

  QObject::connect(
    this->Internals->imageType, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updateImageFileNameExtension(const QString&)));
}

//-----------------------------------------------------------------------------
pqCPExportStateWizard::~pqCPExportStateWizard()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqCPExportStateWizard::updateAddRemoveButton()
{
  this->Internals->addButton->setEnabled(
    this->Internals->allInputs->selectedItems().size() > 0);
  this->Internals->removeButton->setEnabled(
    this->Internals->simulationInputs->selectedItems().size() > 0);
}

//-----------------------------------------------------------------------------
void pqCPExportStateWizard::onAdd()
{
  foreach (QListWidgetItem* item, this->Internals->allInputs->selectedItems())
    {
    QString text = item->text();
    this->Internals->simulationInputs->addItem(text);
    delete this->Internals->allInputs->takeItem(
      this->Internals->allInputs->row(item));
    }
  dynamic_cast<pqCPExportStateWizardPage2*>(
    this->currentPage())->emitCompleteChanged();
}

//-----------------------------------------------------------------------------
void pqCPExportStateWizard::onRemove()
{
  foreach (QListWidgetItem* item, this->Internals->simulationInputs->selectedItems())
    {
    QString text = item->text();
    this->Internals->allInputs->addItem(text);
    delete this->Internals->simulationInputs->takeItem(
      this->Internals->simulationInputs->row(item));
    }
  dynamic_cast<pqCPExportStateWizardPage2*>(this->currentPage())->emitCompleteChanged();
}

//-----------------------------------------------------------------------------
void pqCPExportStateWizard::updateImageFileName()
{
  QString fileName = this->Internals->imageFileName->displayText();
  if(fileName.isNull() || fileName.isEmpty())
    {
    fileName = "image";
    }
  QRegExp regExp("\\.(png|bmp|ppm|tif|tiff|jpg|jpeg)$");
  if(fileName.contains(regExp) == 0)
    {
    fileName.append(".");
    fileName.append(this->Internals->imageType->currentText());
    }
  else
    {  // update imageType if it is different
    int extensionIndex = fileName.lastIndexOf(".");
    QString anExtension = fileName.right(fileName.size()-extensionIndex-1);
    int index = this->Internals->imageType->findText(anExtension);
    this->Internals->imageType->setCurrentIndex(index);
    fileName = this->Internals->imageFileName->displayText();
    }

  if(this->NumberOfViews > 1 && fileName.contains("%v") == 0)
    {
    fileName.insert(fileName.lastIndexOf("."), "_%v");
    }
  if(fileName.contains("%t") == 0)
    {
    fileName.insert(fileName.lastIndexOf("."), "_%t");
    }

  this->Internals->imageFileName->setText(fileName);
}

//-----------------------------------------------------------------------------
void pqCPExportStateWizard::updateImageFileNameExtension(
  const QString& fileExtension)
{
  QString displayText = this->Internals->imageFileName->text();
  vtkstd::string newFileName = vtksys::SystemTools::GetFilenameWithoutExtension(
    displayText.toLocal8Bit().constData());

  newFileName.append(".");
  newFileName.append(fileExtension.toLocal8Bit().constData());
  this->Internals->imageFileName->setText(QString::fromStdString(newFileName));
}

//-----------------------------------------------------------------------------
bool pqCPExportStateWizard::validateCurrentPage()
{
  if (!this->Superclass::validateCurrentPage())
    {
    return false;
    }

  if (this->nextId() != -1)
    {
    // not yet done with the wizard.
    return true;
    }

  QString image_file_name("");
  int image_write_frequency = 0;
  QString export_rendering = "True";
  if (this->Internals->outputRendering->isChecked() == 0)
    {
    export_rendering = "False";
    // check to make sure that there is a writer hooked up since we aren't
    // exporting an image
    vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
    pqServerManagerModel* smModel =
      pqApplicationCore::instance()->getServerManagerModel();
    bool haveSomeWriters = false;
    QStringList filtersWithoutConsumers;
    for(unsigned int i=0;i<proxyManager->GetNumberOfProxies("sources");i++)
      {
      if(vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(
           proxyManager->GetProxy("sources", proxyManager->GetProxyName("sources", i))))
        {
        vtkPVXMLElement* coProcessingHint = proxy->GetHints();
        if(coProcessingHint && coProcessingHint->FindNestedElementByName("CoProcessing"))
          {
          haveSomeWriters = true;
          }
        else
          {
          pqPipelineSource* input = smModel->findItem<pqPipelineSource*>(proxy);
          if(input && input->getNumberOfConsumers() == 0)
            {
            filtersWithoutConsumers << proxyManager->GetProxyName("sources", i);
            }
          }
        }
      }
    if(!haveSomeWriters)
      {
      QMessageBox messageBox;
      QString message(tr("No output writers specified. Either add writers in the pipeline or check <b>Output rendering components</b>."));
      messageBox.setText(message);
      messageBox.exec();
      return false;
      }
    if(filtersWithoutConsumers.size() != 0)
      {
      QMessageBox messageBox;
      QString message(tr("The following filters have no consumers and will not be saved:\n"));
      for(QStringList::const_iterator iter=filtersWithoutConsumers.constBegin();
          iter!=filtersWithoutConsumers.constEnd();iter++)
        {
        message.append("  ");
        message.append(iter->toLocal8Bit().constData());
        message.append("\n");
        }
      messageBox.setText(message);
      messageBox.exec();
      }
    }
  else // we are creating an image so we need to get the proper information from there
    {
    image_file_name = this->Internals->imageFileName->text();
    image_write_frequency = this->Internals->imageWriteFrequency->value();
    }

  QString filters ="ParaView Python State Files (*.py);;All files (*)";

  pqFileDialog file_dialog (NULL, this,
    tr("Save Server State:"), QString(), filters);
  file_dialog.setObjectName("ExportCoprocessingStateFileDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (!file_dialog.exec())
    {
    return false;
    }

  QString filename = file_dialog.getSelectedFiles()[0];

  // Last Page, export the state.
  pqPythonManager* manager = qobject_cast<pqPythonManager*>(
    pqApplicationCore::instance()->manager("PYTHON_MANAGER"));
  pqPythonDialog* dialog = 0;
  if (manager)
    {
    dialog = manager->pythonShellDialog();
    }
  if (!dialog)
    {
    qCritical("Failed to locate Python dialog. Cannot save state.");
    return true;
    }

  QString sim_inputs_map;
  for (int cc=0; cc < this->Internals->nameWidget->rowCount(); cc++)
    {
    QTableWidgetItem* item0 = this->Internals->nameWidget->item(cc, 0);
    QTableWidgetItem* item1 = this->Internals->nameWidget->item(cc, 1);
    sim_inputs_map +=
      QString(" '%1' : '%2',").arg(item0->text()).arg(item1->text());
    }
  // remove last ","
  sim_inputs_map.chop(1);

  QString command =
    QString(cp_export_py).arg(export_rendering).arg(sim_inputs_map).arg(image_file_name).arg(image_write_frequency).arg(filename);

  dialog->runString(command);
  return true;
}
