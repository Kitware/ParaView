#include "pqSQRemoteCommandDialog.h"

#include "ui_pqSQRemoteCommandDialogForm.h"

#include "FsUtils.h"

#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <QList>
#include <QStringList>
#include <QString>

#include <QDebug>


#include "vtkIndent.h"

#include "pqActiveObjects.h"
#include "pqFileDialog.h"

#include <string>
using std::string;

#define pqErrorMacro(estr)\
  qDebug()\
      << "Error in:" << endl\
      << __FILE__ << ", line " << __LINE__ << endl\
      << "" estr << endl;

// command token map, the following tokens may be provided by
// the user to be substituted into the selected command template
// at run time.
enum {
  TERM_EXEC=0,
  TERM_OPTS,
  SSH_EXEC,
  FE_URL,
  PV_HOST,
  PV_PID,
  N_TOKENS
  };

const char *tokens[] = {
  "@TERM_EXEC@",
  "@TERM_OPTS@",
  "@SSH_EXEC@",
  "@FE_URL@",
  "@PV_HOST@",
  "@PV_PID@"
  };

// User interface
//=============================================================================
class pqSQRemoteCommandDialogUI
    :
  public Ui::pqSQRemoteCommandDialogForm
    {};

//------------------------------------------------------------------------------
pqSQRemoteCommandDialog::pqSQRemoteCommandDialog(
    QWidget *Parent,
    Qt::WindowFlags flags)
            :
    QDialog(Parent,flags),
    Ui(0)
{
  this->Ui = new pqSQRemoteCommandDialogUI;
  this->Ui->setupUi(this);

  // load users last used values
  this->Restore();

  // initialize token processing related arrays
  this->Tokens.resize(N_TOKENS);
  for (int i=0; i<N_TOKENS; ++i)
    {
    this->Tokens[i]=tokens[i];
    }

  this->TokenValues.resize(N_TOKENS);

  this->TokenWidgets.push_back(this->Ui->xtExec);
  this->TokenWidgets.push_back(this->Ui->xtOpts);
  this->TokenWidgets.push_back(this->Ui->sshExec);
  this->TokenWidgets.push_back(this->Ui->feUrl);

  // sync the UI
  this->UpdateTokenValues();
  this->UpdateForm();
  this->UpdateCommandPreview();

  // plumbing that responds to user edits/changes

  // template drop down
  QObject::connect(
        this->Ui->addCommand,
        SIGNAL(clicked()),
        this,
        SLOT(AddCommandTemplate()));

  QObject::connect(
        this->Ui->editCommand,
        SIGNAL(clicked()),
        this,
        SLOT(EditCommandTemplate()));

  QObject::connect(
        this->Ui->deleteCommand,
        SIGNAL(clicked()),
        this,
        SLOT(DeleteCommandTemplate()));

  QObject::connect(
    this->Ui->commandTemplates, SIGNAL(currentIndexChanged(int)),
    this, SLOT(UpdateCommandPreview()));

  QObject::connect(
    this->Ui->commandTemplates, SIGNAL(currentIndexChanged(int)),
    this, SLOT(UpdateForm()));

  // line edits
  QObject::connect(
    this->Ui->sshExec, SIGNAL(textChanged(const QString &)),
    this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->sshExec, SIGNAL(textChanged(const QString &)),
    this, SLOT(UpdateCommandPreview()));
  QObject::connect(
    this->Ui->sshExecBrowse, SIGNAL(released()),
    this, SLOT(FindSshExecutable()));

  QObject::connect(
    this->Ui->xtExec, SIGNAL(textChanged(const QString &)),
    this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->xtExec, SIGNAL(textChanged(const QString &)),
    this, SLOT(UpdateCommandPreview()));
  QObject::connect(
    this->Ui->xtExecBrowse, SIGNAL(released()),
    this, SLOT(FindXTermExecutable()));

  QObject::connect(
    this->Ui->xtOpts, SIGNAL(textChanged(const QString &)),
    this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->xtOpts, SIGNAL(textChanged(const QString &)),
    this, SLOT(UpdateCommandPreview()));

  QObject::connect(
    this->Ui->feUrl, SIGNAL(textChanged(const QString &)),
    this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->feUrl, SIGNAL(textChanged(const QString &)),
    this, SLOT(UpdateCommandPreview()));
}

//------------------------------------------------------------------------------
pqSQRemoteCommandDialog::~pqSQRemoteCommandDialog()
{
  this->Save();

  delete this->Ui;
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::Save()
{
  QStringList defaults;

  defaults
    << this->Ui->xtExec->text()
    << this->Ui->xtOpts->text()
    << this->Ui->sshExec->text()
    << this->Ui->feUrl->text()
    << QString("%1").arg(this->Ui->commandTemplates->currentIndex());

  int nCmds=this->Ui->commandTemplates->count();
  for (int i=0; i<nCmds; ++i)
    {
    defaults << this->Ui->commandTemplates->itemText(i);
    }

  QSettings settings("SciberQuest","SciberQuestToolKit");

  settings.setValue("RemoteCommandDialog/defaults",defaults);
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::Restore()
{
  QStringList initialDefaults;
  initialDefaults
  #if 0
       << "cmd.exe"   // TERM_EXE
       << ""          // TERM_OPTS
       << "plink.exe" // SSH_EXEC
       << "USER@CLUSTER.COM"  // FE_URL
       << "0"         // last command template
                      // command templates
       << "@TERM_EXEC@ /K start @SSH_EXEC@ -t @FE_URL@ ssh -t @PV_HOST@ gdb --pid=@PV_PID@"
       << "@TERM_EXEC@ /K start @SSH_EXEC@ -t @FE_URL@ ssh -t @PV_HOST@ top"
       << "@TERM_EXEC@ /K start @SSH_EXEC@ -t @PV_HOST@ gdb --pid=@PV_PID@"
       << "@TERM_EXEC@ /K start @SSH_EXEC@ -t @PV_HOST@ top"
       << "@TERM_EXEC@ /K start @SSH_EXEC@ @PV_HOST@ kill -TERM @PV_PID@"
       << "@TERM_EXEC@ /K start @SSH_EXEC@ @PV_HOST@ kill -KILL @PV_PID@";
  #else
       << "xterm"             // TERM_EXE
       << "-geometry 200x40 -fg white -bg black -T @PV_HOST@:@PV_PID@"
       << "ssh"               // SSH_EXEC
       << "USER@CLUSTER.COM"  // FE_URL
       << "0"                 // last command template
                              // command templates
       << "@TERM_EXEC@ -e gdb --pid=@PV_PID@"
       << "@TERM_EXEC@ -e kill -KILL @PV_PID@"
       << "@TERM_EXEC@ -e @SSH_EXEC@ -t @FE_URL@ ssh -t @PV_HOST@ top"
       << "@TERM_EXEC@ -e @SSH_EXEC@ -t @FE_URL@ ssh -t @PV_HOST@ gdb --pid=@PV_PID@"
       << "@TERM_EXEC@ -e @SSH_EXEC@ -t @FE_URL@ ssh -t @PV_HOST@ kill -KILL --pid=@PV_PID@";
  #endif

  QSettings settings("SciberQuest", "SciberQuestToolKit");

  QStringList defaults
  =settings.value("RemoteCommandDialog/initialDefaults",initialDefaults).toStringList();

  this->Ui->xtExec->setText(defaults.at(0));
  this->Ui->xtOpts->setText(defaults.at(1));
  this->Ui->sshExec->setText(defaults.at(2));
  this->Ui->feUrl->setText(defaults.at(3));
  int lastTemplateId = defaults.at(4).toInt();
  int n=defaults.size();
  for (int i=5; i<n; ++i)
    {
    this->Ui->commandTemplates->addItem(defaults.at(i));
    }
  this->Ui->commandTemplates->setCurrentIndex(lastTemplateId);
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::SetActiveHost(string host)
{
  this->TokenValues[PV_HOST]=host;
  this->UpdateCommandPreview();
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::SetActivePid(string pid)
{
  this->TokenValues[PV_PID]=pid;
  this->UpdateCommandPreview();
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::AddCommandTemplate()
{
  // remind the user of the available tokens.
  QString toks(tokens[0]);
  for (int i=1; i<N_TOKENS; ++i)
    {
    toks+=" ";
    toks+=tokens[i];
    }

  bool ok;
  QString commandTemplate
    = QInputDialog::getText(
        this,
        tr("Add Command Template"),
        tr("Command Template:"),
        QLineEdit::Normal,
        toks,
        &ok);

  if (ok && !commandTemplate.isEmpty())
    {
    this->Ui->commandTemplates->addItem(commandTemplate);
    }
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::DeleteCommandTemplate()
{
  int idx=this->Ui->commandTemplates->currentIndex();
  this->Ui->commandTemplates->removeItem(idx);
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::EditCommandTemplate()
{
  QString commandTemplate = this->Ui->commandTemplates->currentText();
  if (!commandTemplate.isEmpty())
    {
    bool ok;
    commandTemplate
      = QInputDialog::getText(
          this,
          tr("QInputDialog::getText()"),
          tr("Command Template:"),
          QLineEdit::Normal,
          commandTemplate,
          &ok);
    if (ok)
      {
      if (commandTemplate.isEmpty())
        {
        this->DeleteCommandTemplate();
        }
      else
        {
        int idx=this->Ui->commandTemplates->currentIndex();
        this->Ui->commandTemplates->setItemText(idx,commandTemplate);
        }
      }
    }
  else
    {
    QMessageBox ebx(
        QMessageBox::Information,
        "Error",
        "An existing template must be selected from the drop down.");
    ebx.exec();
    return;
    }
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::UpdateTokenValues()
{
  string termExec;
  termExec += (const char *)this->Ui->xtExec->text().toAscii();
  termExec += " ";
  termExec += (const char *)this->Ui->xtOpts->text().toAscii();

  this->TokenValues[TERM_EXEC]=termExec;

  this->TokenValues[SSH_EXEC]
    = (const char *)this->Ui->sshExec->text().toAscii();

  this->TokenValues[FE_URL]
    = (const char *)this->Ui->feUrl->text().toAscii();
}

//-----------------------------------------------------------------------------
void pqSQRemoteCommandDialog::UpdateForm()
{
  /*
  string command
    = (const char*)this->Ui->commandTemplates->currentText().toAscii();

  int nWidgets = this->TokenWidgets.size();
  for (int i=0; i<nWidgets; ++i)
    {
    if (command.find(this->Tokens[i])!=string::npos)
      {
      this->TokenWidgets[i]->setEnabled(true);
      }
    else
      {
      this->TokenWidgets[i]->setEnabled(false);
      }
    }
  */
}

//------------------------------------------------------------------------------
string pqSQRemoteCommandDialog::GetCommand()
{
  string command
    = (const char*)this->Ui->commandTemplates->currentText().toAscii();

  size_t nTokens = this->TokenValues.size();
  for (size_t i=0; i<nTokens; ++i)
    {
    SearchAndReplace(
          this->Tokens[i],
          this->TokenValues[i],
          command);
    }

  return command;
}

//------------------------------------------------------------------------------
void pqSQRemoteCommandDialog::UpdateCommandPreview()
{
  string command=this->GetCommand();
  this->Ui->previewCommand->setText(command.c_str());
}

//------------------------------------------------------------------------------
void pqSQRemoteCommandDialog::FindSshExecutable()
{
  string exe=this->LocateFile();
  this->Ui->sshExec->setText(exe.c_str());
}

//------------------------------------------------------------------------------
void pqSQRemoteCommandDialog::FindXTermExecutable()
{
  string exe=this->LocateFile();
  this->Ui->xtExec->setText(exe.c_str());
}

//------------------------------------------------------------------------------
string pqSQRemoteCommandDialog::LocateFile()
{
  QString filters=QString("All Files (*)");

  pqFileDialog dialog(0,this,"Find file","",filters);
  dialog.setFileMode(pqFileDialog::ExistingFile);

  if (dialog.exec()==QDialog::Accepted)
    {
    QString filename(dialog.getSelectedFiles()[0]);
    return (const char *)filename.toAscii();
    }

  return "";
}
