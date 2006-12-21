
// A Test of a very simple app based on pqCore

#include "pqFileDialogTest.h"

#include <QWidget>
#include <QApplication>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QTimer>
#include <QFileDialog>

#include "vtkProcessModule.h"

#include "pqMain.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqServer.h"
#include "pqFileDialog.h"
#include "pqApplicationCore.h"
#include "pqTestUtility.h"
#include "pqOptions.h"

#include "pqPythonDialog.h"

pqFileDialogTestUtility::pqFileDialogTestUtility()
{
}

pqFileDialogTestUtility::~pqFileDialogTestUtility()
{
  this->cleanupFiles();
}

void pqFileDialogTestUtility::playTests(const QStringList& filenames)
{
  if (filenames.size() > 0)
    {
    this->playTests(filenames[0]);
    }
}

void pqFileDialogTestUtility::playTests(const QString& filename)
{
  this->setupFiles();
  pqTestUtility::playTests(filename);
}

void pqFileDialogTestUtility::testSucceeded()
{
  pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  if(options && options->GetExitAppWhenTestsDone())
    {
    QApplication::exit(0);
    }
}

void pqFileDialogTestUtility::testFailed()
{
  pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  if(options && options->GetExitAppWhenTestsDone())
    {
    QApplication::exit(1);
    }
}

static void CreateEmptyFile(const QString& f)
{
  QFile file(f);
  file.open(QIODevice::WriteOnly);
  QString str = "can delete";
  file.write(str.toAscii().data(), str.size());
  file.close();
}

void pqFileDialogTestUtility::setupFiles()
{
  pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  QString testDirName = options ? options->GetTestDirectory() : QString();
  if(!testDirName.isEmpty())
    {
    QDir testDir(testDirName);
    if(!testDir.exists())
      {
      return;
      }
    testDir.mkdir("FileDialogTest");
    testDir.cd("FileDialogTest");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File1.png");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File2.png");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File3.png");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File1.bmp");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File2.bmp");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File3.bmp");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File1.jpg");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File2.jpg");
    CreateEmptyFile(testDir.path() + QDir::separator() + "File3.jpg");
    testDir.mkdir("SubDir1");
    testDir.mkdir("SubDir2");
    testDir.mkdir("SubDir3");
    }
}

void pqFileDialogTestUtility::cleanupFiles()
{
  pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  QString testDirName = options ? options->GetTestDirectory() : QString();
  if(!testDirName.isEmpty())
    {
    QDir testDir(testDirName);
    if(testDir.exists())
      {
      testDir.cd("FileDialogTest");
      testDir.rmdir("SubDir1");
      testDir.rmdir("SubDir2");
      testDir.rmdir("SubDir3");
      testDir.remove("File1.png");
      testDir.remove("File2.png");
      testDir.remove("File3.png");
      testDir.remove("File1.bmp");
      testDir.remove("File2.bmp");
      testDir.remove("File3.bmp");
      testDir.remove("File1.jpg");
      testDir.remove("File2.jpg");
      testDir.remove("File3.jpg");
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
  this->Server = core->createServer(pqServerResource("builtin:"));
  QVBoxLayout* l = new QVBoxLayout(this);

  QPushButton* python = new QPushButton(this);
  python->setObjectName("python");
  python->setText("Python Shell");
  QObject::connect(python, SIGNAL(clicked(bool)), this, SLOT(openPython()));
  l->addWidget(python);
  
  QPushButton* rec = new QPushButton(this);
  rec->setObjectName("record");
  rec->setText("record...");
  QObject::connect(rec, SIGNAL(clicked(bool)), this, SLOT(record()));
  l->addWidget(rec);


  this->FileMode = new QComboBox(this);
  this->FileMode->setObjectName("FileMode");
  l->addWidget(this->FileMode);
  this->FileMode->addItem("Any File", pqFileDialog::AnyFile);
  this->FileMode->addItem("Existing File", pqFileDialog::ExistingFile);
  this->FileMode->addItem("Existing Files", pqFileDialog::ExistingFiles);
  this->FileMode->addItem("Directory", pqFileDialog::Directory);

  this->FileFilter = new QLineEdit(this);
  this->FileFilter->setObjectName("FileFilter");
  l->addWidget(this->FileFilter);

  this->OpenButton = new QPushButton(this);
  this->OpenButton->setText("Open File Dialog...");
  this->OpenButton->setObjectName("OpenDialog");
  l->addWidget(this->OpenButton);
  this->EmitLabel = new QLabel(this);
  this->EmitLabel->setObjectName("EmitLabel");
  this->EmitLabel->setText("(nul)");
  l->addWidget(this->EmitLabel);
  this->ReturnLabel = new QLabel(this);
  this->ReturnLabel->setObjectName("ReturnLabel");
  this->ReturnLabel->setText("(nul)");
  l->addWidget(this->ReturnLabel);
  QObject::connect(this->OpenButton, SIGNAL(clicked(bool)),
                   this, SLOT(openFileDialog()));
}

void pqFileDialogTestWidget::openFileDialog()
{
  pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  QString testDirName = options ? options->GetTestDirectory() : QString();
  QDir testDir(testDirName);
  if(testDir.exists())
    {
    testDirName += QDir::separator();
    testDirName += "FileDialogTest";
    }

  pqFileDialog diag(this->Server, this, this->FileMode->currentText(),
                    testDirName, this->FileFilter->text());
  QVariant mode = this->FileMode->itemData(this->FileMode->currentIndex());
  diag.setFileMode(static_cast<pqFileDialog::FileMode>(mode.toInt()));
  QObject::connect(&diag, SIGNAL(filesSelected(const QStringList&)),
                   this, SLOT(emittedFiles(const QStringList&)));
  if(diag.exec() == QDialog::Accepted)
    {
    this->ReturnLabel->setText(diag.getSelectedFiles().join(";"));
    }
  else
    {
    this->ReturnLabel->setText("cancelled");
    }
}

void pqFileDialogTestWidget::openPython()
{
  QDialog* d = new pqPythonDialog(NULL, QApplication::argc(), QApplication::argv());
  d->show();
}

void pqFileDialogTestWidget::emittedFiles(const QStringList& files)
{
  this->EmitLabel->setText(files.join(";"));
}

void pqFileDialogTestWidget::record()
{
  QString file = QFileDialog::getSaveFileName();
  if(file != QString::null)
    {
    this->TestUtility.recordTests(file);
    }
}

// our gui helper makes our MainWindow
class GUIHelper : public pqProcessModuleGUIHelper
{
public:
  static GUIHelper* New()
  {
    return new GUIHelper;
  }

  pqTestUtility* TestUtility()
  {
    return this->TestWidget->Tester();
  }

  QWidget* CreateMainWindow()
  {
    this->TestWidget = new pqFileDialogTestWidget();
    return this->TestWidget;
  }
  pqFileDialogTestWidget* TestWidget;
};


int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  return pqMain::Run(app, GUIHelper::New());
}

