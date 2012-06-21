#ifndef __pqSQRemoteCommandDialog_h
#define __pqSQRemoteCommandDialog_h

// .NAME pqSQRemoteCommandDialog - Dialog for configuring remote commands
//
// .SECTION Description
//
// .SECTION See Also
//
// .SECTION Thanks

#include <QDialog>
#include <QLineEdit>

#include <vector>
using std::vector;
#include <string>
using std::string;

class pqSQRemoteCommandDialogUI;

class pqSQRemoteCommandDialog : public QDialog
{
Q_OBJECT

public:
  pqSQRemoteCommandDialog(QWidget *parent,Qt::WindowFlags f);
  ~pqSQRemoteCommandDialog();

  // Description:
  // before running the dialog call these to set the
  //  active host and process id.
  void SetActiveHost(string host);
  void SetActivePid(string pid);

  // Description:
  // Returns the command that the user has selected
  string GetCommand();

private slots:
  void AddCommandTemplate();
  void EditCommandTemplate();
  void DeleteCommandTemplate();

  void UpdateCommandPreview();
  void UpdateTokenValues();
  void UpdateForm();

  void FindXTermExecutable();
  void FindSshExecutable();

private:
  void Save();
  void Restore();
  string LocateFile();

private:
  pqSQRemoteCommandDialogUI *Ui;

  vector<string> Tokens;
  vector<string> TokenValues;
  vector<QLineEdit*> TokenWidgets;
};

#endif
