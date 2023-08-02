// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// A Test of a very simple app based on pqCore

#include "pqFileDialogTest.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include "pqApplicationCore.h"
#include "pqCoreConfiguration.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqServer.h"
#include "pqTestUtility.h"

pqFileDialogTestUtility::pqFileDialogTestUtility() = default;

pqFileDialogTestUtility::~pqFileDialogTestUtility()
{
  this->cleanupFiles();
}

void pqFileDialogTestUtility::playTheTests()
{
  auto config = pqCoreConfiguration::instance();
  QStringList files;
  for (int cc = 0, max = config->testScriptCount(); cc < max; ++cc)
  {
    files.push_back(QString::fromStdString(config->testScript(cc)));
  }
  this->playTests(files);
}

bool pqFileDialogTestUtility::playTests(const QStringList& filenames)
{
  this->setupFiles();
  bool val = this->pqTestUtility::playTests(filenames);

  if (pqCoreConfiguration::instance()->exitApplicationWhenTestsDone())
  {
    QApplication::exit(val ? 0 : 1);
  }
  return val;
}

static void CreateEmptyFile(const QString& f)
{
  QFile file(f);
  file.open(QIODevice::WriteOnly);
  QString str = "can delete";
  file.write(str.toUtf8().data(), str.size());
  file.close();
}

void pqFileDialogTestUtility::setupFiles()
{
  auto config = pqCoreConfiguration::instance();
  QString testDirName = QString::fromStdString(config->testDirectory());
  if (!testDirName.isEmpty())
  {
    QDir testDir(testDirName);
    if (!testDir.exists())
    {
      return;
    }
    testDir.mkdir("FileDialogTest");
    testDir.cd("FileDialogTest");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Filea.png");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Fileb.png");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Filec.png");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Filea.bmp");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Fileb.bmp");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Filec.bmp");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Filea.jpg");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Fileb.jpg");
    CreateEmptyFile(testDir.path() + QDir::separator() + "Filec.jpg");
    testDir.mkdir("SubDir1");
    testDir.mkdir("SubDir2");
    testDir.mkdir("SubDir3");
  }
}

void pqFileDialogTestUtility::cleanupFiles()
{
  auto config = pqCoreConfiguration::instance();
  QString testDirName = QString::fromStdString(config->testDirectory());
  if (!testDirName.isEmpty())
  {
    QDir testDir(testDirName);
    if (testDir.exists())
    {
      testDir.cd("FileDialogTest");
      testDir.rmdir("SubDir1");
      testDir.rmdir("SubDir2");
      testDir.rmdir("SubDir3");
      testDir.remove("Filea.png");
      testDir.remove("Fileb.png");
      testDir.remove("Filec.png");
      testDir.remove("Filea.bmp");
      testDir.remove("Fileb.bmp");
      testDir.remove("Filec.bmp");
      testDir.remove("Filea.jpg");
      testDir.remove("Fileb.jpg");
      testDir.remove("Filec.jpg");
      testDir.cdUp();
      testDir.rmdir("FileDialogTest");
    }
  }
}

pqFileDialogTestWidget::pqFileDialogTestWidget()
{
  this->setObjectName("main");
  // automatically make a server connection
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* ob = core->getObjectBuilder();
  this->Server = ob->createServer(pqServerResource("builtin:"));
  QVBoxLayout* l = new QVBoxLayout(this);

  QPushButton* rec = new QPushButton(this);
  rec->setObjectName("record");
  rec->setText(tr("record..."));
  QObject::connect(rec, SIGNAL(clicked(bool)), this, SLOT(record()));
  l->addWidget(rec);

  this->ConnectionMode = new QComboBox(this);
  this->ConnectionMode->setObjectName("ConnectionMode");
  l->addWidget(this->ConnectionMode);
  this->ConnectionMode->addItem(tr("Local"));
  this->ConnectionMode->addItem(tr("Remote"));

  this->FileMode = new QComboBox(this);
  this->FileMode->setObjectName("FileMode");
  l->addWidget(this->FileMode);
  this->FileMode->addItem(tr("Any File"), pqFileDialog::AnyFile);
  this->FileMode->addItem(tr("Existing File"), pqFileDialog::ExistingFile);
  this->FileMode->addItem(tr("Existing Files"), pqFileDialog::ExistingFiles);
  this->FileMode->addItem(tr("Directory"), pqFileDialog::Directory);

  this->FileFilter = new QLineEdit(this);
  this->FileFilter->setObjectName("FileFilter");
  l->addWidget(this->FileFilter);

  this->OpenButton = new QPushButton(this);
  this->OpenButton->setText(tr("Open File Dialog..."));
  this->OpenButton->setObjectName("OpenDialog");
  l->addWidget(this->OpenButton);
  this->EmitLabel = new QLabel(this);
  this->EmitLabel->setObjectName("EmitLabel");
  this->EmitLabel->setText(QString("(%1)").arg(tr("null")));
  l->addWidget(this->EmitLabel);
  this->ReturnLabel = new QLabel(this);
  this->ReturnLabel->setObjectName("ReturnLabel");
  this->ReturnLabel->setText(QString("(%1)").arg(tr("null")));
  l->addWidget(this->ReturnLabel);
  QObject::connect(this->OpenButton, SIGNAL(clicked(bool)), this, SLOT(openFileDialog()));
}

void pqFileDialogTestWidget::openFileDialog()
{
  auto config = pqCoreConfiguration::instance();
  QString testDirName = QString::fromStdString(config->testDirectory());
  QDir testDir(testDirName);
  if (testDir.exists())
  {
    testDirName += QDir::separator();
    testDirName += "FileDialogTest";
  }

  pqServer* server = this->Server;
  if (this->ConnectionMode->currentText() == tr("Local"))
  {
    server = nullptr;
  }

  pqFileDialog diag(
    server, this, this->FileMode->currentText(), testDirName, this->FileFilter->text(), false);
  QVariant mode = this->FileMode->itemData(this->FileMode->currentIndex());
  diag.setFileMode(static_cast<pqFileDialog::FileMode>(mode.toInt()));
  QObject::connect(&diag, SIGNAL(filesSelected(const QList<QStringList>&)), this,
    SLOT(emittedFiles(const QList<QStringList>&)));
  if (diag.exec() == QDialog::Accepted)
  {
    this->ReturnLabel->setText(diag.getSelectedFiles().join(";"));
  }
  else
  {
    this->ReturnLabel->setText(tr("cancelled"));
  }
}

void pqFileDialogTestWidget::emittedFiles(const QList<QStringList>& files)
{
  this->EmitLabel->setText(files[0].join(";"));
}

void pqFileDialogTestWidget::record()
{
  QString file = QFileDialog::getSaveFileName();
  if (!file.isNull())
  {
    this->TestUtility.recordTests(file);
  }
}

int main(int argc, char** argv)
{
  try
  {
    QApplication app(argc, argv);
    pqApplicationCore appCore(argc, argv);
    pqFileDialogTestWidget mainWidget;
    mainWidget.show();
    QMetaObject::invokeMethod(mainWidget.Tester(), "playTheTests", Qt::QueuedConnection);
    return app.exec();
  }
  catch (pqApplicationCoreExitCode& e)
  {
    return e.code();
  }
}
