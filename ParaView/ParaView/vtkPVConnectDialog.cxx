/*=========================================================================

  Program:   ParaView
  Module:    vtkPVConnectDialog.cxx
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
#include "vtkPVConnectDialog.h"

#include "vtkPVApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledCheckButton.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVWindow.h"

#include "vtkstd/string"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVConnectDialog);
vtkCxxRevisionMacro(vtkPVConnectDialog, "1.11");

//----------------------------------------------------------------------------
void vtkPVConnectDialog::Create(vtkKWApplication* app, const char* vtkNotUsed(opts))
{
  char buffer[1024];
  sprintf(buffer, "Cannot connect to the server %s:%d.\nPlease specify server to connect:",
    this->HostnameString, this->PortInt);
  this->SetOptions(
    vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault |
    vtkKWMessageDialog::WarningIcon );
  this->SetStyleToOkCancel();
  this->Superclass::Create(app, 0);
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  this->SetMasterWindow(pvApp->GetMainWindow());
  this->SetText(buffer);
  this->SetTitle("ParaView Connection Warning");
  this->Label->SetParent(this->BottomFrame);
  this->Label->Create(app, "-text {Hostname}");
  vtkKWFrame* frame = vtkKWFrame::New();
  frame->SetParent(this->BottomFrame);
  frame->Create(app, 0);
  this->Username->SetParent(frame);
  this->Username->Create(app, 0);
  this->Username->SetValue(this->SSHUser);
  this->Hostname->SetParent(frame);
  this->Hostname->GetEntry()->PullDownOn();
  this->Hostname->Create(app, 0);
  this->Hostname->SetLabel("@");
  this->Port->SetParent(frame);
  this->Port->Create(app,0);
  this->Port->SetLabel(":");
  this->Port->GetEntry()->SetWidth(4);
  this->Username->SetWidth(7);
  this->Script("pack %s -side left -expand 0", 
    this->Username->GetWidgetName());
  this->Script("pack %s -side left -expand 1 -fill x", 
    this->Hostname->GetWidgetName());
  this->Script("pack %s -side left -expand 0", 
    this->Port->GetWidgetName());
  this->Script("pack %s -side top -expand 1 -fill both", 
    frame->GetWidgetName());
  frame->Delete();
  frame = vtkKWFrame::New();
  frame->SetParent(this->BottomFrame);
  frame->Create(app, 0);

  this->MPIMode->SetParent(frame);
  this->MPIMode->Create(app, 0);
  this->MPIMode->SetLabel("Use MPI");
  if ( this->MultiProcessMode == 1 )
    {
    this->MPIMode->GetCheckButton()->SetState(1);
    }
  else
    {
    this->MPIMode->GetCheckButton()->SetState(0);
    }
  this->MPIMode->GetCheckButton()->SetCommand(this, "MPICheckBoxCallback");
  this->MPINumberOfServers->SetParent(frame);
  this->MPINumberOfServers->PopupScaleOn();
  this->MPINumberOfServers->Create(app, 0);
  this->MPINumberOfServers->DisplayEntry();
  this->MPINumberOfServers->DisplayLabel("Number of processes");
  this->MPINumberOfServers->SetRange(2, 10);
  this->MPINumberOfServers->SetValue(this->NumberOfProcesses);
  this->Script("pack %s -side left -expand 1 -fill x", 
    this->MPIMode->GetWidgetName());
  this->Script("pack %s -side left -expand 1 -fill x", 
    this->MPINumberOfServers->GetWidgetName());

  this->Script("pack %s -side top -expand 1 -fill both", 
    frame->GetWidgetName());
  frame->Delete();
  this->SetHostname(this->HostnameString);
  this->SetPort(this->PortInt);
  this->MPINumberOfServers->EnabledOff();

  char servers[1024];
  if ( app->GetRegisteryValue(2, "RunTime", "ConnectionServers", servers) )
    {
    const char* server = servers;
    size_t cc;
    size_t len = strlen(servers);
    for ( cc = 0; cc < len; cc ++ )
      {
      if ( servers[cc] == ',' )
        {
        servers[cc] = 0;
        cout << "Found server: [" << server << "]" << endl;
        this->Hostname->GetEntry()->AddValue(server);
        server = servers + cc + 1;
        }
      }
    if ( strlen(server) )
      {
      cout << "Last server: " << server << endl;
      this->Hostname->GetEntry()->AddValue(server);
      }
    }
  this->GrabDialog = 0;
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::OK()
{
  this->SetHostnameString(this->Hostname->GetEntry()->GetValue());
  this->PortInt = this->Port->GetEntry()->GetValueAsInt();
  this->NumberOfProcesses = static_cast<int>(this->MPINumberOfServers->GetValue());
  if ( this->MPIMode->GetCheckButton()->GetState() )
    {
    this->MultiProcessMode = 1;
    }
  else
    {
    this->MultiProcessMode = 0;
    }
  this->SetSSHUser(this->Username->GetValue());

  int cc;
  vtkstd::string servers;
  servers = this->Hostname->GetEntry()->GetValue();
  for ( cc = 0; cc < this->Hostname->GetEntry()->GetNumberOfValues(); cc ++ )
    {
    servers += ",";
    cout << "Store server: " << this->Hostname->GetEntry()->GetValueFromIndex(cc) << endl;
    servers += this->Hostname->GetEntry()->GetValueFromIndex(cc);
    }
  cout << "Servers: " << servers.c_str() << endl;
  this->Application->SetRegisteryValue(2, "RunTime", "ConnectionServers", servers.c_str());

  this->Superclass::OK();
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::SetHostname(const char* hn)
{
  if ( this->Hostname->IsCreated() )
    {
    this->Hostname->GetEntry()->SetValue(hn);
    }
  this->SetHostnameString(hn);
}

//----------------------------------------------------------------------------
const char* vtkPVConnectDialog::GetHostName()
{
  return this->HostnameString;
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::SetPort(int pt)
{
  if ( this->Port->IsCreated() )
    {
    char buffer[100];
    sprintf(buffer, "%d", pt);
    this->Port->GetEntry()->SetValue(buffer);
    }
  this->PortInt = pt;
}

//----------------------------------------------------------------------------
int vtkPVConnectDialog::GetPort()
{
  return this->PortInt;
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::MPICheckBoxCallback()
{
  if ( this->MPIMode->GetCheckButton()->GetState() )
    {
    this->MPINumberOfServers->EnabledOn();
    }
  else
    {
    this->MPINumberOfServers->EnabledOff();
    }
}

//----------------------------------------------------------------------------
vtkPVConnectDialog::vtkPVConnectDialog()
{
  this->Username = vtkKWEntry::New();
  this->Hostname = vtkKWLabeledEntry::New();
  this->Port = vtkKWLabeledEntry::New();
  this->Label = vtkKWLabel::New();
  this->MPINumberOfServers = vtkKWScale::New();
  this->MPIMode = vtkKWLabeledCheckButton::New();

  this->HostnameString = 0;
  this->PortInt = 0;
  this->MultiProcessMode = 0;
  this->NumberOfProcesses = 2;
  this->SSHUser = 0;
}

//----------------------------------------------------------------------------
vtkPVConnectDialog::~vtkPVConnectDialog()
{
  this->Username->Delete();
  this->Hostname->Delete();
  this->Port->Delete();
  this->Label->Delete();
  this->MPINumberOfServers->Delete();
  this->MPIMode->Delete();
  this->SetSSHUser(0);
}


//----------------------------------------------------------------------------
void vtkPVConnectDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SSHUser: " << (this->SSHUser?this->SSHUser:"(none)") << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "MultiProcessMode: " << this->MultiProcessMode << endl;
}
