/*=========================================================================

  Program:   ParaView
  Module:    vtkPVConnectDialog.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVConnectDialog
// .SECTION Description
// A class to encapaulate all of the process initialization,
// distributed data model and duplication  of the pipeline.
// Filters and compositers will still need a controller, 
// but every thing else should be handled here.  This class 
// sets up the default MPI processes with the user interface
// running on process 0.  I plan to make an alternative module
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

private:
  vtkPVConnectDialog(const vtkPVConnectDialog&); // Not implemented
  void operator=(const vtkPVConnectDialog&); // Not implemented
};

#endif


