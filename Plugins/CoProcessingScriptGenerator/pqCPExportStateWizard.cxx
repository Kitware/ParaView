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
#include <pqContextView.h>
#include <pqPipelineRepresentation.h>
#include <pqFileDialog.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqPythonDialog.h>
#include <pqPythonManager.h>
#include <pqRenderViewBase.h>
#include <pqServerManagerModel.h>

#include <vtkPVXMLElement.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>
#include <vtksys/SystemTools.hxx>

#include <QSize>
#include <vtkPNGWriter.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkUnsignedCharArray.h>
#include <QPixmap>
#include <QLabel>

extern const char* cp_export_py;

// HACK.
namespace
{
  static QPointer<pqCPExportStateWizard> ActiveWizard;
}

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
pqImageOutputInfo::pqImageOutputInfo(
  QWidget *parentObject, Qt::WindowFlags parentFlags,
  pqView* view, QString& viewName)
  : QWidget(parentObject, parentFlags), View(view)
{
  this->Info.setupUi(this);

  this->Info.imageFileName->setText(viewName);
  QObject::connect(
    this->Info.imageFileName, SIGNAL(editingFinished()),
    this, SLOT(updateImageFileName()));

  QObject::connect(
    this->Info.imageType, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updateImageFileNameExtension(const QString&)));

  this->setupScreenshotInfo();
};

//-----------------------------------------------------------------------------
void pqImageOutputInfo::updateImageFileName()
{
  QString fileName = this->Info.imageFileName->displayText();
  if(fileName.isNull() || fileName.isEmpty())
    {
    fileName = "image";
    }
  QRegExp regExp("\\.(png|bmp|ppm|tif|tiff|jpg|jpeg)$");
  if(fileName.contains(regExp) == 0)
    {
    fileName.append(".");
    fileName.append(this->Info.imageType->currentText());
    }
  else
    {  // update imageType if it is different
    int extensionIndex = fileName.lastIndexOf(".");
    QString anExtension = fileName.right(fileName.size()-extensionIndex-1);
    int index = this->Info.imageType->findText(anExtension);
    this->Info.imageType->setCurrentIndex(index);
    fileName = this->Info.imageFileName->displayText();
    }

  if(fileName.contains("%t") == 0)
    {
    fileName.insert(fileName.lastIndexOf("."), "_%t");
    }

  this->Info.imageFileName->setText(fileName);
}

//-----------------------------------------------------------------------------
void pqImageOutputInfo::updateImageFileNameExtension(
  const QString& fileExtension)
{
  QString displayText = this->Info.imageFileName->text();
  std::string newFileName = vtksys::SystemTools::GetFilenameWithoutExtension(
    displayText.toLocal8Bit().constData());

  newFileName.append(".");
  newFileName.append(fileExtension.toLocal8Bit().constData());
  this->Info.imageFileName->setText(QString::fromStdString(newFileName));
}

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
void pqImageOutputInfo::setupScreenshotInfo()
{
  this->Info.thumbnailLabel->setVisible(true);
  if(!this->View)
    {
    cerr << "no view available which seems really weird\n";
    return;
    }

  QSize viewSize = this->View->getSize();
  QSize thumbnailSize;
  if(viewSize.width() > viewSize.height())
    {
    thumbnailSize.setWidth(100);
    thumbnailSize.setHeight(100*viewSize.height()/viewSize.width());
    }
  else
    {
    thumbnailSize.setHeight(100);
    thumbnailSize.setWidth(100*viewSize.width()/viewSize.height());
    }
  vtkSmartPointer<vtkImageData> image;
  image.TakeReference(this->View->captureImage(thumbnailSize));
  vtkNew<vtkPNGWriter> pngWriter;
  pngWriter->SetInputData(image);
  pngWriter->WriteToMemoryOn();
  pngWriter->Update();
  pngWriter->Write();
  vtkUnsignedCharArray* result = pngWriter->GetResult();
  QPixmap thumbnail;
  thumbnail.loadFromData(
    result->GetPointer(0),
    result->GetNumberOfTuples()*result->GetNumberOfComponents(), "PNG");

  this->Info.thumbnailLabel->setPixmap(thumbnail);
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
    // if there is only 1 input then call it input, otherwise
    // use the same name as the filter
    if(this->Internals->simulationInputs->count() == 1)
      {
      this->Internals->nameWidget->setItem(cc, 1, new QTableWidgetItem("input"));
      }
    else
      {
      this->Internals->nameWidget->setItem(cc, 1, new QTableWidgetItem(text));
      }
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
  this->CurrentView = 0;
  ::ActiveWizard = this;
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  ::ActiveWizard = NULL;
  //this->setWizardStyle(ModernStyle);
  this->setOption(QWizard::NoCancelButton, false);
  this->Internals->viewsContainer->hide();
  this->Internals->rescaleDataRange->hide();
  this->Internals->previousView->hide();
  this->Internals->nextView->hide();

  QObject::connect(this->Internals->allInputs, SIGNAL(itemSelectionChanged()),
    this, SLOT(updateAddRemoveButton()));
  QObject::connect(this->Internals->simulationInputs, SIGNAL(itemSelectionChanged()),
    this, SLOT(updateAddRemoveButton()));
  QObject::connect(this->Internals->addButton, SIGNAL(clicked()),
    this, SLOT(onAdd()));
  QObject::connect(this->Internals->removeButton, SIGNAL(clicked()),
    this, SLOT(onRemove()));

  QObject::connect(this->Internals->allInputs, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
    this, SLOT(onAdd()));
  QObject::connect(this->Internals->simulationInputs, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
    this, SLOT(onRemove()));

  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->viewsContainer, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
                   this->Internals->rescaleDataRange, SLOT(setVisible(bool)));

  QObject::connect(this->Internals->nextView, SIGNAL(pressed()),
                   this, SLOT(incrementView()));
  QObject::connect(this->Internals->previousView, SIGNAL(pressed()),
                   this, SLOT(decrementView()));

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqRenderViewBase*> renderViews = smModel->findItems<pqRenderViewBase*>();
  QList<pqContextView*> contextViews = smModel->findItems<pqContextView*>();
  int viewCounter = 0;
  int numberOfViews = renderViews.size() + contextViews.size();
  // first do 2D and 3D render views
  for(QList<pqRenderViewBase*>::Iterator it=renderViews.begin();
      it!=renderViews.end();it++)
    {
    QString viewName = (numberOfViews == 1 ? "image_%t.png" :
                        QString("image_%1_%t.png").arg(viewCounter) );
    pqImageOutputInfo* imageOutputInfo = new pqImageOutputInfo(
      this->Internals->viewsContainer, parentFlags, *it,  viewName);
    this->Internals->viewsContainer->addWidget(imageOutputInfo);
    viewCounter++;
    }
  for(QList<pqContextView*>::Iterator it=contextViews.begin();
      it!=contextViews.end();it++)
    {
    QString viewName = (numberOfViews == 1 ? "image_%t.png" :
                        QString("image_%1_%t.png").arg(viewCounter) );
    pqImageOutputInfo* imageOutputInfo = new pqImageOutputInfo(
      this->Internals->viewsContainer, parentFlags, *it, viewName);
    this->Internals->viewsContainer->addWidget(imageOutputInfo);
    viewCounter++;
    }
  if(numberOfViews > 1)
    {
    this->Internals->nextView->setEnabled(true);
    }
  this->Internals->viewsContainer->setCurrentIndex(0);

  // a bit of a hack but we name the finish button here since for testing
  // it's having a hard time finding that button otherwise.
  QAbstractButton* finishButton = this->button(FinishButton);
  QString name("finishButton");
  finishButton->setObjectName(name);
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
void pqCPExportStateWizard::incrementView()
{
  if(this->CurrentView >= this->Internals->viewsContainer->count()-1)
    {
    cerr << "Already on the last view.  Next View button should be disabled.\n";
    this->Internals->nextView->setEnabled(false);
    return;
    }
  if(this->CurrentView == 0)
    {
    this->Internals->previousView->setEnabled(true);
    }
  this->CurrentView++;
  this->Internals->viewsContainer->setCurrentIndex(this->CurrentView);
  if(this->CurrentView >= this->Internals->viewsContainer->count()-1)
    {
    this->Internals->nextView->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqCPExportStateWizard::decrementView()
{
  if(this->CurrentView <= 0)
    {
    cerr << "Already on the first view.  Previous View button should be disabled.\n";
    this->Internals->previousView->setEnabled(false);
    return;
    }
  if(this->CurrentView == this->Internals->viewsContainer->count()-1)
    {
    this->Internals->nextView->setEnabled(true);
    }
  this->CurrentView--;
  this->Internals->viewsContainer->setCurrentIndex(this->CurrentView);
  if(this->CurrentView <= 0)
    {
    this->Internals->previousView->setEnabled(false);
    }
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

  QString export_rendering = "True";
  QString rendering_info; // a map from the render view name to render output params
  if (this->Internals->outputRendering->isChecked() == 0)
    {
    export_rendering = "False";
    // check to make sure that there is a writer hooked up since we aren't
    // exporting an image
    vtkSMSessionProxyManager* proxyManager =
        vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
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
  else // we are creating images so add information to the view proxies
    {
    vtkSMSessionProxyManager* proxyManager =
        vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    for(int i=0;i<this->Internals->viewsContainer->count();i++)
      {
      pqImageOutputInfo* viewInfo = dynamic_cast<pqImageOutputInfo*>(
        this->Internals->viewsContainer->widget(i));
      pqView* view = viewInfo->getView();
      vtkSMViewProxy* viewProxy = view->getViewProxy();
      QString info = QString(" '%1' : ['%2', '%3', '%4', '%5'],").
        arg(proxyManager->GetProxyName("views", viewProxy)).
        arg(viewInfo->getImageFileName()).arg(viewInfo->getWriteFrequency()).
        arg(static_cast<int>(viewInfo->fitToScreen())).
        arg(viewInfo->getMagnification());
      rendering_info+= info;
      }
    // remove the last comma -- assume that there's at least one view
    rendering_info.chop(1);
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

  // the map from the simulation inputs in the paraview gui
  // to the adaptor's named inputs (usually 'input')
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

  QString rescale_data_range = (this->Internals->rescaleDataRange->isChecked() == true ?
                                "True" : "False");

  QString command =
    QString(cp_export_py).arg(export_rendering).arg(sim_inputs_map).arg(rendering_info).arg(rescale_data_range).arg(filename);

  dialog->runString(command);

  return true;
}
