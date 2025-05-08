// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqExampleVisualizationsDialog.h"
#include "ui_pqExampleVisualizationsDialog.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEventDispatcher.h"
#include "pqLoadStateReaction.h"
#include "vtkPVFileInformation.h"

#include <QFile>
#include <QFileInfo>
#include <QMainWindow>
#include <QMessageBox>
#include <QStatusBar>

#include <cassert>

//-----------------------------------------------------------------------------
pqExampleVisualizationsDialog::pqExampleVisualizationsDialog(QWidget* parentObject)
  : Superclass(parentObject)
  , ui(new Ui::pqExampleVisualizationsDialog)
{
  ui->setupUi(this);
  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  QObject::connect(
    this->ui->CanExampleButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
  QObject::connect(
    this->ui->DiskOutRefExampleButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
  QObject::connect(
    this->ui->HeadSQExampleButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
  QObject::connect(
    this->ui->HotGasAnalysisExampleButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
  QObject::connect(
    this->ui->TosExampleButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
  QObject::connect(
    this->ui->WaveletExampleButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
  QObject::connect(
    this->ui->RectilinearGridTimestepsButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
  QObject::connect(
    this->ui->UnstructuredGridParallelButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
  QObject::connect(
    this->ui->MandelbrotButton, SIGNAL(clicked(bool)), this, SLOT(onButtonPressed()));
}

//-----------------------------------------------------------------------------
pqExampleVisualizationsDialog::~pqExampleVisualizationsDialog()
{
  delete ui;
}

//-----------------------------------------------------------------------------
void pqExampleVisualizationsDialog::onButtonPressed()
{
  QString dataPath(vtkPVFileInformation::GetParaViewExampleFilesDirectory().c_str());
  QPushButton* button = qobject_cast<QPushButton*>(sender());
  if (button)
  {
    const char* stateFile = nullptr;
    bool needsData = false;
    if (button == this->ui->CanExampleButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/CanExample.pvsm";
      needsData = true;
    }
    else if (button == this->ui->DiskOutRefExampleButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/DiskOutRefExample.pvsm";
      needsData = true;
    }
    else if (button == this->ui->WaveletExampleButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/WaveletExample.pvsm";
      needsData = false;
    }
    else if (button == this->ui->HotGasAnalysisExampleButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/HotGasAnalysisExample.pvsm";
      needsData = true;
    }
    else if (button == this->ui->HeadSQExampleButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/HeadSQExample.pvsm";
      needsData = true;
    }
    else if (button == this->ui->TosExampleButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/TosExample.pvsm";
      needsData = true;
    }
    else if (button == this->ui->RectilinearGridTimestepsButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/RectilinearGrid.pvsm";
      needsData = true;
    }
    else if (button == this->ui->UnstructuredGridParallelButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/UnstructuredGridParallel.pvsm";
      needsData = true;
    }
    else if (button == this->ui->MandelbrotButton)
    {
      stateFile = ":/pqApplicationComponents/ExampleVisualizations/Mandelbrot.pvsm";
      needsData = false;
    }
    else
    {
      qCritical("No example file for button");
      return;
    }
    if (needsData)
    {
      QFileInfo fdataPath(dataPath);
      if (!fdataPath.isDir())
      {
        QString msg =
          tr("Your installation doesn't have datasets to load the example visualizations. "
             "You can manually download the datasets from paraview.org and then "
             "place them under the following path for examples to work:\n\n'%1'")
            .arg(fdataPath.absoluteFilePath());
        // dump to cout for easy copy/paste.
        cout << msg.toUtf8().data() << endl;
        QMessageBox::warning(this, tr("Missing data"), msg, QMessageBox::Ok);
        return;
      }
      dataPath = fdataPath.absoluteFilePath();
    }

    this->hide();
    assert(stateFile != nullptr);

    QFile qfile(stateFile);
    if (qfile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
      if (mainWindow)
      {
        mainWindow->statusBar()->showMessage(tr("Loading example visualization."), 2000);
      }

      QString xmldata(qfile.readAll());
      xmldata.replace("$PARAVIEW_EXAMPLES_DATA", dataPath);
      pqApplicationCore::instance()->loadStateFromString(
        xmldata.toUtf8().data(), pqActiveObjects::instance().activeServer());

      // This is needed since XML state currently does not save active view.
      pqLoadStateReaction::activateView();
    }
    else
    {
      qCritical("Failed to open resource");
    }
  }
}
