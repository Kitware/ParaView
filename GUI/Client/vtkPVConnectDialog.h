/*=========================================================================

  Program:   ParaView
  Module:    vtkPVConnectDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVConnectDialog - Class to encapsulate all of the process initialization
//
// .SECTION Description
// A class to encapsulate all of the process initialization,
// distributed data model and duplication  of the pipeline.
// Filters and compositers will still need a controller, 
// but every thing else should be handled here. This class 
// sets up the default MPI processes with the user interface
// running on process 0. I plan to make an alternative module
// for client server mode, where the client running the UI 
// is not in the MPI group but links to the MPI group through 
// a socket connection.

#ifndef __vtkPVConnectDialog_h
#define __vtkPVConnectDialog_h

#include "vtkKWMessageDialog.h"

class vtkKWLabeledCheckButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWScale;
class vtkKWLabeledEntry;
class vtkKWApplication;
class vtkStringList;

class VTK_EXPORT vtkPVConnectDialog : public vtkKWMessageDialog
{
public:
  static vtkPVConnectDialog* New();
  vtkTypeRevisionMacro(vtkPVConnectDialog, vtkKWMessageDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Create(vtkKWApplication* app, const char* opts);
  void OK();
  void SetHostname(const char* hn);
  const char* GetHostName();
  void SetPort(int pt);
  int GetPort();
  void MPICheckBoxCallback();

  void SetListOfServers(const char* list);
  const char* GetListOfServers();

  vtkSetMacro(MultiProcessMode, int);
  vtkGetMacro(MultiProcessMode, int);
  vtkSetMacro(NumberOfProcesses, int);
  vtkGetMacro(NumberOfProcesses, int);

  vtkSetStringMacro(SSHUser);
  vtkGetStringMacro(SSHUser);

protected:
  vtkPVConnectDialog();
  ~vtkPVConnectDialog();

  vtkKWEntry* Username;
  vtkKWLabeledEntry* Hostname;
  vtkKWLabeledEntry* Port;
  vtkKWLabel* Label;
  vtkKWLabeledCheckButton* MPIMode;
  vtkKWScale* MPINumberOfServers;

  vtkSetStringMacro(HostnameString);
  char* HostnameString;
  char* SSHUser;
  int PortInt;
  int MultiProcessMode;
  int NumberOfProcesses;

  vtkStringList *Servers;

  vtkSetStringMacro(ListOfServersString);
  char* ListOfServersString;


private:
  vtkPVConnectDialog(const vtkPVConnectDialog&); // Not implemented
  void operator=(const vtkPVConnectDialog&); // Not implemented
};

#endif


