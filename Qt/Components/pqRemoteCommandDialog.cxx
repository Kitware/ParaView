#include "pqRemoteCommandDialog.h"
#include "pqRemoteCommandTemplateDialog.h"
#include "ui_pqRemoteCommandDialogForm.h"

#include <QDebug>
#include <QInputDialog>
#include <QList>
#include <QMessageBox>
#include <QSettings>
#include <QString>
#include <QStringList>

#include "vtkIndent.h"

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqSettings.h"

#include <string>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#define QT_ENDL endl
#else
#define QT_ENDL Qt::endl
#endif

#define pqErrorMacro(estr)                                                                         \
  qDebug() << "Error in:" << QT_ENDL << __FILE__ << ", line " << __LINE__ << QT_ENDL << "" estr    \
           << QT_ENDL;

// User interface
//=============================================================================
class pqRemoteCommandDialogUI : public Ui::pqRemoteCommandDialogForm
{
};

// command token map, the following tokens may be provided by
// the user to be substituted into the selected command template
// at run time.
enum
{
  TERM_EXEC = 0,
  TERM_OPTS,
  SSH_EXEC,
  FE_URL,
  PV_HOST,
  PV_PID,
  N_TOKENS
};

const char* tokens[] = { "$TERM_EXEC$", "$TERM_OPTS$", "$SSH_EXEC$", "$FE_URL$", "$PV_HOST$",
  "$PV_PID$" };

// a command set is selected based on host system types.
// client | server | descr
//--------+--------+----------
//   0    |  -1    | unix cli
//   0    |   0    | unix unix
//   0    |   1    | unix win
//   1    |  -1    | win cli
//   1    |   0    | win unix
//   1    |   1    | win win
// the command set identifier is computed as follows:
// csid=10*server+client
enum
{
  SET_UNIX_ONLY = -10,
  SET_UNIX_UNIX = 0,
  SET_UNIX_WIN = 10,
  SET_WIN_ONLY = -9,
  SET_WIN_UNIX = 1,
  SET_WIN_WIN = 11
};

namespace
{

//*****************************************************************************
int commandSetId(int clientType, int serverType)
{
  return 10 * serverType + clientType;
}

//*****************************************************************************
int searchAndReplace(const string& searchFor, const string& replaceWith, string& inText)
{
  int nFound = 0;
  const size_t n = searchFor.size();
  size_t at = string::npos;
  while ((at = inText.find(searchFor)) != string::npos)
  {
    inText.replace(at, n, replaceWith);
    ++nFound;
  }
  return nFound;
}

//*****************************************************************************
int searchAndReplace(const char* searchFor, const char* replaceWith, string& inText)
{
  string searchForStr(searchFor);
  string replaceWithStr(replaceWith);
  return searchAndReplace(searchForStr, replaceWithStr, inText);
}

//*****************************************************************************
void addWinDefaults(QStringList& cmds)
{
  // TERM_EXEC, TERM_OPTS,SSH_EXEC,FE_URL,last used template
  cmds << "cmd.exe"
       << ""
       << "plink.exe"
       << "USER$CLUSTER.COM"
       << "0";
}

//*****************************************************************************
void addUnixDefaults(QStringList& cmds)
{
  // TERM_EXEC, TERM_OPTS,SSH_EXEC,FE_URL,last used template
  cmds << "xterm"
       << "-geometry 200x40 -fg white -bg black -T $PV_HOST$:$PV_PID$"
       << "ssh"
       << "USER@CLUSTER.COM"
       << "0";
}

//*****************************************************************************
void addUnixOnlyCommands(QStringList& cmds)
{
  cmds << "local gdb"
       << "$TERM_EXEC$ $TERM_OPTS$ -e gdb --pid=$PV_PID$"
       << "local stack trace"
       << "$TERM_EXEC$ -e kill -INT $PV_PID$"
       << "local kill process"
       << "$TERM_EXEC$ -e kill -KILL $PV_PID$";
}

//*****************************************************************************
void addUnixUnixCommands(QStringList& cmds)
{
  cmds << "remote top"
       << "$TERM_EXEC$ $TERM_OPTS$ -e $SSH_EXEC$ -t $PV_HOST$ top"
       << "remote gdb"
       << "$TERM_EXEC$ $TERM_OPTS$ -e $SSH_EXEC$ -t $PV_HOST$ gdb --pid=$PV_PID$"
       << "remote stack trace"
       << "$TERM_EXEC$ -e $SSH_EXEC$ -t $PV_HOST$ kill -INT --pid=$PV_PID$"
       << "remote kill process"
       << "$TERM_EXEC$ -e $SSH_EXEC$ -t $PV_HOST$ kill -KILL --pid=$PV_PID$"
       << "remote w/login top"
       << "$TERM_EXEC$ $TERM_OPTS$ -e $SSH_EXEC$ -t $FE_URL$ ssh -t $PV_HOST$ top"
       << "remote w/login gdb"
       << "$TERM_EXEC$ $TERM_OPTS$ -e $SSH_EXEC$ -t $FE_URL$ ssh -t $PV_HOST$ gdb --pid=$PV_PID$"
       << "remote w/login stack trace"
       << "$TERM_EXEC$ -e $SSH_EXEC$ -t $FE_URL$ ssh -t $PV_HOST$ kill -INT --pid=$PV_PID$"
       << "remote w/login kill process"
       << "$TERM_EXEC$ -e $SSH_EXEC$ -t $FE_URL$ ssh -t $PV_HOST$ kill -KILL --pid=$PV_PID$";
}

//*****************************************************************************
void addUnixWinCommands(QStringList& cmds)
{
  (void)cmds;
  // TODO
  // base these off from WinOnly commands
}

//*****************************************************************************
void addWinOnlyCommands(QStringList& cmds)
{
  (void)cmds;
  // TODO
  // scripts to attach a debugger etc...
  cmds << "local debug"
       << "$TERM_EXEC$ /K start \"C:\\Program Files\\Debugging Tools for Windows "
          "(x86)\\windbg.exe\" -p $PV_PID$ /g";
}

//*****************************************************************************
void addWinUnixCommands(QStringList& cmds)
{
  cmds << "remote top"
       << "$TERM_EXEC$ /K start $SSH_EXEC$ -t $PV_HOST$ top"
       << "remote gdb"
       << "$TERM_EXEC$ /K start $SSH_EXEC$ -t $PV_HOST$ gdb --pid=$PV_PID$"
       << "remote stack trace"
       << "$TERM_EXEC$ /K start $SSH_EXEC$ -t $PV_HOST$ kill -INT --pid=$PV_PID$"
       << "remote kill process"
       << "$TERM_EXEC$ /K start $SSH_EXEC$ -t $PV_HOST$ kill -KILL --pid=$PV_PID$"
       << "remote w/login top"
       << "$TERM_EXEC$ /K start $SSH_EXEC$ -t $FE_URL$ ssh -t $PV_HOST$ top"
       << "remote w/login gdb"
       << "$TERM_EXEC$ /K start $SSH_EXEC$ -t $FE_URL$ ssh -t $PV_HOST$ gdb --pid=$PV_PID$"
       << "remote w/login stack trace"
       << "$TERM_EXEC$ /K start $SSH_EXEC$ -t $FE_URL$ ssh -t $PV_HOST$ kill -INT --pid=$PV_PID$"
       << "remote w/login kill process"
       << "$TERM_EXEC$ /K start $SSH_EXEC$ -t $FE_URL$ ssh -t $PV_HOST$ kill -KILL --pid=$PV_PID$";
}

//*****************************************************************************
void addWinWinCommands(QStringList& cmds)
{
  (void)cmds;
  // TODO
  // base these off from WinOnly commands
}
};

//------------------------------------------------------------------------------
pqRemoteCommandDialog::pqRemoteCommandDialog(
  QWidget* Parent, Qt::WindowFlags flags, int clientSystemType, int serverSystemType)
  : QDialog(Parent, flags)
  , Ui(0)
{
  this->Ui = new pqRemoteCommandDialogUI;
  this->Ui->setupUi(this);

  int csid = commandSetId(clientSystemType, serverSystemType);
  switch (csid)
  {
    case SET_UNIX_ONLY:
      this->CommandSetName = "UnixOnly";
      ::addUnixDefaults(this->CommandSet);
      ::addUnixOnlyCommands(this->CommandSet);
      break;
    case SET_UNIX_UNIX:
      this->CommandSetName = "UnixUnix";
      ::addUnixDefaults(this->CommandSet);
      ::addUnixUnixCommands(this->CommandSet);
      break;
    case SET_UNIX_WIN:
      this->CommandSetName = "UnixWin";
      ::addUnixDefaults(this->CommandSet);
      ::addUnixWinCommands(this->CommandSet);
      break;
    case SET_WIN_ONLY:
      this->CommandSetName = "WinOnly";
      ::addWinDefaults(this->CommandSet);
      ::addWinOnlyCommands(this->CommandSet);
      break;
    case SET_WIN_UNIX:
      this->CommandSetName = "WinUnix";
      ::addWinDefaults(this->CommandSet);
      ::addWinUnixCommands(this->CommandSet);
      break;
    case SET_WIN_WIN:
      this->CommandSetName = "WinWin";
      ::addWinDefaults(this->CommandSet);
      ::addWinWinCommands(this->CommandSet);
      break;
    default:
      pqErrorMacro("invalid commandSetType " << csid);
      return;
  }

  // load users last used values
  this->Restore();

  // initialize token processing related arrays
  this->Tokens.resize(N_TOKENS);
  for (int i = 0; i < N_TOKENS; ++i)
  {
    this->Tokens[i] = tokens[i];
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
  QObject::connect(this->Ui->addCommand, SIGNAL(clicked()), this, SLOT(AddCommandTemplate()));

  QObject::connect(this->Ui->editCommand, SIGNAL(clicked()), this, SLOT(EditCommandTemplate()));

  QObject::connect(this->Ui->deleteCommand, SIGNAL(clicked()), this, SLOT(DeleteCommandTemplate()));

  QObject::connect(this->Ui->commandTemplates, SIGNAL(currentIndexChanged(int)), this,
    SLOT(UpdateCommandPreview()));

  QObject::connect(
    this->Ui->commandTemplates, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateForm()));

  // line edits
  QObject::connect(
    this->Ui->sshExec, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->sshExec, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateCommandPreview()));
  QObject::connect(this->Ui->sshExecBrowse, SIGNAL(released()), this, SLOT(FindSshExecutable()));

  QObject::connect(
    this->Ui->xtExec, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->xtExec, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateCommandPreview()));
  QObject::connect(this->Ui->xtExecBrowse, SIGNAL(released()), this, SLOT(FindXTermExecutable()));

  QObject::connect(
    this->Ui->pvHost, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->pvHost, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateCommandPreview()));

  QObject::connect(
    this->Ui->xtOpts, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->xtOpts, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateCommandPreview()));

  QObject::connect(
    this->Ui->feUrl, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateTokenValues()));
  QObject::connect(
    this->Ui->feUrl, SIGNAL(textChanged(const QString&)), this, SLOT(UpdateCommandPreview()));
}

//------------------------------------------------------------------------------
pqRemoteCommandDialog::~pqRemoteCommandDialog()
{
  this->Save();

  delete this->Ui;
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::Save()
{
  string group = "RemoteCommandDialog";
  group += this->CommandSetName;

  QStringList defaults;

  defaults << this->Ui->xtExec->text() << this->Ui->xtOpts->text() << this->Ui->sshExec->text()
           << this->Ui->feUrl->text()
           << QString("%1").arg(this->Ui->commandTemplates->currentIndex());

  int nCmds = this->Ui->commandTemplates->count();
  for (int i = 0; i < nCmds; ++i)
  {
    defaults << this->Ui->commandTemplates->itemText(i)
             << this->Ui->commandTemplates->itemData(i).toString();
  }

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(group.c_str());
  settings->setValue("defaults", defaults);
  settings->setValue("test", "test");
  settings->endGroup();
  settings->sync();
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::Restore()
{
  string group = "RemoteCommandDialog";
  group += this->CommandSetName;

  QStringList defaults;

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->sync();
  settings->beginGroup(group.c_str());
  defaults = settings->value("defaults", this->CommandSet).toStringList();
  settings->endGroup();

  this->Ui->xtExec->setText(defaults.at(0));
  this->Ui->xtOpts->setText(defaults.at(1));
  this->Ui->sshExec->setText(defaults.at(2));
  this->Ui->feUrl->setText(defaults.at(3));
  int lastTemplateId = defaults.at(4).toInt();
  int n = defaults.size();
  for (int i = 5; i < n; i += 2)
  {
    this->Ui->commandTemplates->addItem(defaults.at(i), defaults.at(i + 1));
  }
  this->Ui->commandTemplates->setCurrentIndex(lastTemplateId);
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::SetActiveHost(string host)
{
  this->TokenValues[PV_HOST] = host;
  this->Ui->pvHost->setText(host.c_str());
  this->UpdateCommandPreview();
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::SetActivePid(string pid)
{
  this->TokenValues[PV_PID] = pid;
  this->UpdateCommandPreview();
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::AddCommandTemplate()
{
  // remind the user of the available tokens.
  QString toks(tokens[0]);
  for (int i = 1; i < N_TOKENS; ++i)
  {
    toks += " ";
    toks += tokens[i];
  }

  pqRemoteCommandTemplateDialog dialog(this, Qt::WindowFlags{});
  dialog.SetCommandName(QString("new command"));
  dialog.SetCommandTemplate(toks);

  if ((dialog.exec() == QDialog::Accepted) && dialog.GetModified())
  {
    QString commandName = dialog.GetCommandName();
    QString commandTemplate = dialog.GetCommandTemplate();

    this->Ui->commandTemplates->addItem(commandName, commandTemplate);

    int itemId = this->Ui->commandTemplates->count() - 1;
    this->Ui->commandTemplates->setCurrentIndex(itemId);

    this->UpdateCommandPreview();
  }
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::DeleteCommandTemplate()
{
  int idx = this->Ui->commandTemplates->currentIndex();
  this->Ui->commandTemplates->removeItem(idx);
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::EditCommandTemplate()
{
  int itemId = this->Ui->commandTemplates->currentIndex();
  QString commandName = this->Ui->commandTemplates->itemText(itemId);
  QString commandTemplate = this->Ui->commandTemplates->itemData(itemId).toString();

  pqRemoteCommandTemplateDialog dialog(this, Qt::WindowFlags{});
  dialog.SetCommandName(commandName);
  dialog.SetCommandTemplate(commandTemplate);

  if ((dialog.exec() == QDialog::Accepted) && dialog.GetModified())
  {
    commandName = dialog.GetCommandName();
    commandTemplate = dialog.GetCommandTemplate();

    this->Ui->commandTemplates->setItemText(itemId, commandName);
    this->Ui->commandTemplates->setItemData(itemId, commandTemplate);

    this->UpdateCommandPreview();
  }
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::UpdateTokenValues()
{
  this->TokenValues[TERM_EXEC] = (const char*)this->Ui->xtExec->text().toLocal8Bit();

  this->TokenValues[TERM_OPTS] = (const char*)this->Ui->xtOpts->text().toLocal8Bit();

  this->TokenValues[SSH_EXEC] = (const char*)this->Ui->sshExec->text().toLocal8Bit();

  this->TokenValues[PV_HOST] = (const char*)this->Ui->pvHost->text().toLocal8Bit();

  this->TokenValues[FE_URL] = (const char*)this->Ui->feUrl->text().toLocal8Bit();
}

//-----------------------------------------------------------------------------
void pqRemoteCommandDialog::UpdateForm()
{
  // this would enable or disable the various field depending
  // on whether or not the token is used in the current command
  // template. however, it may be confusing to users as to when
  // they should change field values, so leaving them active
  // all the time is probably the best thing to do.
  /*
  string command
    = (const char*)this->Ui->commandTemplates->currentText().toLocal8Bit();

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
string pqRemoteCommandDialog::GetCommand()
{
  string command =
    (const char*)this->Ui->commandTemplates->itemData(this->Ui->commandTemplates->currentIndex())
      .toString()
      .toLocal8Bit();

  size_t nTokens = this->TokenValues.size();
  for (size_t i = 0; i < nTokens; ++i)
  {
    ::searchAndReplace(this->Tokens[i], this->TokenValues[i], command);
  }

  return command;
}

//------------------------------------------------------------------------------
void pqRemoteCommandDialog::UpdateCommandPreview()
{
  string command = this->GetCommand();
  this->Ui->previewCommand->setText(command.c_str());
}

//------------------------------------------------------------------------------
void pqRemoteCommandDialog::FindSshExecutable()
{
  string exe = this->LocateFile();
  this->Ui->sshExec->setText(exe.c_str());
}

//------------------------------------------------------------------------------
void pqRemoteCommandDialog::FindXTermExecutable()
{
  string exe = this->LocateFile();
  this->Ui->xtExec->setText(exe.c_str());
}

//------------------------------------------------------------------------------
string pqRemoteCommandDialog::LocateFile()
{
  QString filters = QString("All Files (*)");

  pqFileDialog dialog(0, this, "Find file", "", filters);
  dialog.setFileMode(pqFileDialog::ExistingFile);

  if (dialog.exec() == QDialog::Accepted)
  {
    string filename((const char*)dialog.getSelectedFiles()[0].toLocal8Bit());
    // escape whitespace
    ::searchAndReplace(" ", "^", filename);
    return filename;
  }

  return "";
}
