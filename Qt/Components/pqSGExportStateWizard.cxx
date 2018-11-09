/*=========================================================================

  Program: ParaView
  Module:    pqSGExportStateWizard.cxx

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
#include "pqSGExportStateWizard.h"
#include "pqSGExportStateWizardInternals.h"

#include "pqApplicationCore.h"
#include "pqContextView.h"
#include "pqImageOutputInfo.h"
#include "pqPipelineFilter.h"
#include "pqRenderViewBase.h"
#include "pqServerManagerModel.h"
#include "vtkPythonInterpreter.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include "pqCinemaTrack.h"
#include "pqPipelineFilter.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMProperty.h"

#include <vtksys/SystemTools.hxx>

#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPointer>
#include <QRegExp>
#include <QRegExpValidator>
#include <QSize>

#include <QDebug>

// HACK.
namespace
{
static QPointer<pqSGExportStateWizard> ActiveWizard;
} // end anonymous namespace

pqSGExportStateWizardPage2::pqSGExportStateWizardPage2(QWidget* _parent)
  : QWizardPage(_parent)
{
  this->Internals = ::ActiveWizard->Internals;
}

pqSGExportStateWizardPage3::pqSGExportStateWizardPage3(QWidget* _parent)
  : QWizardPage(_parent)
{
  this->Internals = ::ActiveWizard->Internals;
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizardPage2::initializePage()
{
  this->Internals->simulationInputs->clear();
  this->Internals->allInputs->clear();
  this->Internals->usedSources.clear();
  QList<pqPipelineSource*> sources =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqPipelineSource*>();
  foreach (pqPipelineSource* source, sources)
  {
    if (qobject_cast<pqPipelineFilter*>(source))
    {
      continue;
    }
    if (this->Internals->showAllSources->isChecked())
    {
      this->Internals->allInputs->addItem(source->getSMName());
      this->Internals->usedSources[source->getSMName()] = source;
    }
    else
    { // determine if the source is a reader or not, only include readers
      if (vtkSMCoreUtilities::GetFileNameProperty(source->getProxy()))
      {
        this->Internals->allInputs->addItem(source->getSMName());
        this->Internals->usedSources[source->getSMName()] = source;
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool pqSGExportStateWizardPage2::isComplete() const
{
  return this->Internals->simulationInputs->count() > 0;
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizardPage3::initializePage()
{
  this->Internals->nameWidget->clearContents();
  this->Internals->nameWidget->setRowCount(this->Internals->simulationInputs->count());
  for (int cc = 0; cc < this->Internals->simulationInputs->count(); cc++)
  {
    QListWidgetItem* item = this->Internals->simulationInputs->item(cc);
    QString text = item->text();
    this->Internals->nameWidget->setItem(cc, 0, new QTableWidgetItem(text));
    this->Internals->nameWidget->setItem(cc, 1, new QTableWidgetItem(text));
    QTableWidgetItem* tableItem = this->Internals->nameWidget->item(cc, 1);
    tableItem->setFlags(tableItem->flags() | Qt::ItemIsEditable);

    tableItem = this->Internals->nameWidget->item(cc, 0);
    tableItem->setFlags(tableItem->flags() & ~Qt::ItemIsEditable);
  }
}

//-----------------------------------------------------------------------------
pqSGExportStateWizard::pqSGExportStateWizard(QWidget* parentObject, Qt::WindowFlags parentFlags)
  : Superclass(parentObject, parentFlags)
{
  ::ActiveWizard = this;
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  ::ActiveWizard = NULL;
  // this->setWizardStyle(ModernStyle);
  this->setOption(QWizard::NoCancelButton, false);
  this->Internals->wViewSelection->hide();
  this->Internals->fileNamePaddingAmountLabel->hide();
  this->Internals->fileNamePaddingAmountSpinBox->hide();
  this->Internals->rescaleDataRange->hide();
  this->Internals->laRescaleDataRange->hide();

  QObject::connect(this->Internals->allInputs, SIGNAL(itemSelectionChanged()), this,
    SLOT(updateAddRemoveButton()));
  QObject::connect(this->Internals->simulationInputs, SIGNAL(itemSelectionChanged()), this,
    SLOT(updateAddRemoveButton()));
  QObject::connect(
    this->Internals->showAllSources, SIGNAL(toggled(bool)), this, SLOT(onShowAllSources(bool)));
  QObject::connect(this->Internals->addButton, SIGNAL(clicked()), this, SLOT(onAdd()));
  QObject::connect(this->Internals->removeButton, SIGNAL(clicked()), this, SLOT(onRemove()));

  QObject::connect(
    this->Internals->allInputs, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onAdd()));
  QObject::connect(this->Internals->simulationInputs, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
    this, SLOT(onRemove()));

  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
    this->Internals->wViewSelection, SLOT(setVisible(bool)));

  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
    this->Internals->fileNamePaddingAmountSpinBox, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
    this->Internals->fileNamePaddingAmountLabel, SLOT(setVisible(bool)));

  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
    this->Internals->rescaleDataRange, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->outputRendering, SIGNAL(toggled(bool)),
    this->Internals->laRescaleDataRange, SLOT(setVisible(bool)));

  this->Internals->wCinemaTrackSelection->hide();

  QObject::connect(this->Internals->outputCinema, SIGNAL(toggled(bool)),
    this->Internals->outputRendering, SLOT(setChecked(bool)));
  QObject::connect(this->Internals->outputCinema, SIGNAL(toggled(bool)),
    this->Internals->wCinemaTrackSelection, SLOT(setVisible(bool)));
  QObject::connect(
    this->Internals->outputCinema, SIGNAL(toggled(bool)), this, SLOT(toggleCinema(bool)));
  QObject::connect(this->Internals->wViewSelection, SIGNAL(arraySelectionEnabledChanged(bool)),
    this->Internals->wCinemaTrackSelection, SLOT(enableArraySelection(bool)));

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();

  // populate views in stacked widget
  QList<pqRenderViewBase*> renderViews = smModel->findItems<pqRenderViewBase*>();
  QList<pqContextView*> contextViews = smModel->findItems<pqContextView*>();
  this->Internals->wViewSelection->populateViews(renderViews, contextViews);

  // populate the pipeline browser
  this->Internals->wCinemaTrackSelection->initializePipelineBrowser();

  // a bit of a hack but we name the finish button here since for testing
  // it's having a hard time finding that button otherwise.
  QAbstractButton* finishButton = this->button(FinishButton);
  QString name("finishButton");
  finishButton->setObjectName(name);
}

//-----------------------------------------------------------------------------
pqSGExportStateWizard::~pqSGExportStateWizard()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::updateAddRemoveButton()
{
  this->Internals->addButton->setEnabled(this->Internals->allInputs->selectedItems().size() > 0);
  this->Internals->removeButton->setEnabled(
    this->Internals->simulationInputs->selectedItems().size() > 0);
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::onShowAllSources(bool isChecked)
{
  if (isChecked)
  { // add any sources that aren't readers and aren't in simulationInputs
    QList<pqPipelineSource*> sources =
      pqApplicationCore::instance()->getServerManagerModel()->findItems<pqPipelineSource*>();
    foreach (pqPipelineSource* source, sources)
    {
      if (qobject_cast<pqPipelineFilter*>(source))
      {
        continue;
      }
      if (vtkSMCoreUtilities::GetFileNameProperty(source->getProxy()) == NULL)
      {
        // make sure it's not in the list of simulationInputs
        QList<QListWidgetItem*> matchingNames =
          this->Internals->simulationInputs->findItems(source->getSMName(), 0);
        if (matchingNames.isEmpty())
        {
          this->Internals->allInputs->addItem(source->getSMName());
        }
      }
    }
  }
  else
  { // remove any source that aren't readers from allInputs
    for (int i = this->Internals->allInputs->count() - 1; i >= 0; i--)
    {
      QListWidgetItem* item = this->Internals->allInputs->item(i);
      QString text = item->text();
      pqPipelineSource* source =
        pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(text);
      if (vtkSMCoreUtilities::GetFileNameProperty(source->getProxy()) == NULL)
      {
        delete this->Internals->allInputs->takeItem(i);
      }
    }
  }
  dynamic_cast<pqSGExportStateWizardPage2*>(this->currentPage())->emitCompleteChanged();
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::onAdd()
{
  foreach (QListWidgetItem* item, this->Internals->allInputs->selectedItems())
  {
    QString text = item->text();
    this->Internals->simulationInputs->addItem(text);
    delete this->Internals->allInputs->takeItem(this->Internals->allInputs->row(item));
  }
  dynamic_cast<pqSGExportStateWizardPage2*>(this->currentPage())->emitCompleteChanged();
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::onRemove()
{
  foreach (QListWidgetItem* item, this->Internals->simulationInputs->selectedItems())
  {
    QString text = item->text();
    if (this->Internals->showAllSources->isChecked())
    { // we show all sources...
      this->Internals->allInputs->addItem(text);
    }
    else
    { // show only reader sources...
      pqPipelineSource* source =
        pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(text);
      if (vtkSMCoreUtilities::GetFileNameProperty(source->getProxy()))
      {
        this->Internals->allInputs->addItem(text);
      }
    }
    delete this->Internals->simulationInputs->takeItem(
      this->Internals->simulationInputs->row(item));
  }
  dynamic_cast<pqSGExportStateWizardPage2*>(this->currentPage())->emitCompleteChanged();
}

//-----------------------------------------------------------------------------
QList<pqImageOutputInfo*> pqSGExportStateWizard::getImageOutputInfos()
{
  return this->Internals->wViewSelection->getImageOutputInfos();
}

//-----------------------------------------------------------------------------
bool pqSGExportStateWizard::validateCurrentPage()
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

  QString command;
  if (this->getCommandString(command))
  {
    // ensure Python in initialized.
    vtkPythonInterpreter::Initialize();
    vtkPythonInterpreter::RunSimpleString(command.toLocal8Bit().data());
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqSGExportStateWizard::toggleCinema(bool state)
{
  if (state)
  {
    // cinema depends on rendering being on (unchecking it should not be possible)
    this->Internals->outputRendering->setEnabled(false);
    // add cinema controls to each view
    this->Internals->wViewSelection->setCinemaVisible(true);
  }
  else
  {
    this->Internals->outputRendering->setEnabled(true);
    this->Internals->wViewSelection->setCinemaVisible(false);
  }
}
