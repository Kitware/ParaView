
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
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqServer.h"
#include "pqTestUtility.h"

pqFileDialogTestUtility::pqFileDialogTestUtility()
{
}

pqFileDialogTestUtility::~pqFileDialogTestUtility()
{
  this->cleanupFiles();
}

void pqFileDialogTestUtility::playTheTests(const QStringList& files)
{
  this->playTests(files);
}
bool pqFileDialogTestUtility::playTests(const QStringList& filenames)
{
  this->setupFiles();
  bool val = this->pqTestUtility::playTests(filenames);

  pqOptions* const options =
    pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
  if (options && options->GetExitAppWhenTestsDone())
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
  file.write(str.toLocal8Bit().data(), str.size());
  file.close();
}

void pqFileDialogTestUtility::setupFiles()
{
  pqOptions* const options =
    pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
  QString testDirName = options ? options->GetTestDirectory() : QString();
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
  pqOptions* const options =
    pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
  QString testDirName = options ? options->GetTestDirectory() : QString();
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
  rec->setText("record...");
  QObject::connect(rec, SIGNAL(clicked(bool)), this, SLOT(record()));
  l->addWidget(rec);

  this->ConnectionMode = new QComboBox(this);
  this->ConnectionMode->setObjectName("ConnectionMode");
  l->addWidget(this->ConnectionMode);
  this->ConnectionMode->addItem("Local");
  this->ConnectionMode->addItem("Remote");

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
  QObject::connect(this->OpenButton, SIGNAL(clicked(bool)), this, SLOT(openFileDialog()));
}

void pqFileDialogTestWidget::openFileDialog()
{
  pqOptions* const options =
    pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
  QString testDirName = options ? options->GetTestDirectory() : QString();
  QDir testDir(testDirName);
  if (testDir.exists())
  {
    testDirName += QDir::separator();
    testDirName += "FileDialogTest";
  }

  pqServer* server = this->Server;
  if (this->ConnectionMode->currentText() == "Local")
  {
    server = NULL;
  }

  pqFileDialog diag(
    server, this, this->FileMode->currentText(), testDirName, this->FileFilter->text());
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
    this->ReturnLabel->setText("cancelled");
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
  QApplication app(argc, argv);
  pqOptions* options = pqOptions::New();
  pqApplicationCore appCore(argc, argv, options);
  options->Delete();

  pqFileDialogTestWidget mainWidget;
  mainWidget.show();

  QMetaObject::invokeMethod(mainWidget.Tester(), "playTheTests", Qt::QueuedConnection,
    Q_ARG(QStringList, options->GetTestScripts()));
  return app.exec();
}
