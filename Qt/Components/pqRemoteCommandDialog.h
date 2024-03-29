// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRemoteCommandDialog_h
#define pqRemoteCommandDialog_h

// .NAME pqRemoteCommandDialog - Dialog for configuring remote commands
// .SECTION Description
// .SECTION See Also
// .SECTION Thanks

#include <QDialog>
#include <QLineEdit>

#include <vector>
using std::vector;
#include <string>
using std::string;

class pqRemoteCommandDialogUI;

class pqRemoteCommandDialog : public QDialog
{
  Q_OBJECT

public:
  pqRemoteCommandDialog(QWidget* parent, Qt::WindowFlags f, int clientHostType, int serverHostType);

  ~pqRemoteCommandDialog() override;

  // Description:
  // before running the dialog call these to set the
  //  active host and process id.
  void SetActiveHost(string host);
  void SetActivePid(string pid);

  // Description:
  // Returns the command that the user has selected
  string GetCommand();

private Q_SLOTS:
  void AddCommandTemplate();
  void EditCommandTemplate();
  void DeleteCommandTemplate();

  void UpdateCommandPreview();
  void UpdateTokenValues();
  void UpdateForm();

  void FindXTermExecutable();
  void FindSshExecutable();

private: // NOLINT(readability-redundant-access-specifiers)
  void Save();
  void Restore();
  string LocateFile();

  pqRemoteCommandDialogUI* Ui;

  string CommandSetName;
  QStringList CommandSet;

  vector<string> Tokens;
  vector<string> TokenValues;
  vector<QLineEdit*> TokenWidgets;
};

#endif
