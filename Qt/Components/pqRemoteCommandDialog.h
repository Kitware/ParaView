/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

private:
  void Save();
  void Restore();
  string LocateFile();

private:
  pqRemoteCommandDialogUI* Ui;

  string CommandSetName;
  QStringList CommandSet;

  vector<string> Tokens;
  vector<string> TokenValues;
  vector<QLineEdit*> TokenWidgets;
};

#endif
