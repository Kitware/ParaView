/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSummaryHelper.cxx
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
#include "vtkPVSummaryHelper.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkXMLPDataWriter.h"
#include "vtkXMLPVDWriter.h"

#include <vtkstd/string>

#include <sys/stat.h>

#ifdef _WIN32
# include <io.h>
#else
# include <unistd.h>
#endif

#define VTK_PV_SUMMARY_CHECK_TOKEN_TAG 923857
#define VTK_PV_SUMMARY_DELETE_TOKEN_TAG 923858

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSummaryHelper);
vtkCxxRevisionMacro(vtkPVSummaryHelper, "1.2.2.1");

vtkCxxSetObjectMacro(vtkPVSummaryHelper, Writer, vtkXMLWriter);
vtkCxxSetObjectMacro(vtkPVSummaryHelper, Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPVSummaryHelper::vtkPVSummaryHelper()
{
  this->Controller = 0;
  this->Writer = 0;
}

//----------------------------------------------------------------------------
vtkPVSummaryHelper::~vtkPVSummaryHelper()
{
  this->SetController(0);
  this->SetWriter(0);
}

//----------------------------------------------------------------------------
void vtkPVSummaryHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << "\n";
  os << indent << "Writer: " << this->Writer << "\n";
}

//----------------------------------------------------------------------------
int vtkPVSummaryHelper::SynchronizeSummaryFiles()
{
  // Only need test if a potentially parallel writer is used.
  if(!this->Writer || !this->Controller ||
     (!vtkXMLPDataWriter::SafeDownCast(this->Writer) &&
      !vtkXMLPVDWriter::SafeDownCast(this->Writer)))
    {
    return 0;
    }
  
  // Only need to test if more than one process is used.
  int numProcs = this->Controller->GetNumberOfProcesses();
  if(numProcs <= 1)
    {
    return 1;
    }
  
  // Delete any previously existing dummy file on all nodes.
  this->DeleteDummyFiles();
  
  // We must check the dummy file on one node at a time.  Use a token
  // passing approach starting at node 0.
  int myid = this->Controller->GetLocalProcessId();  
  int result = 0;
  if(myid == 0)
    {
    // Check node 0.
    result = this->CheckDummyFile();
    
    // Pass token to node 1.
    this->Controller->Send(&result, 1, 1, VTK_PV_SUMMARY_CHECK_TOKEN_TAG);
    
    // Wait for last node to finish.
    this->Controller->Receive(&result, 1, numProcs-1,
                              VTK_PV_SUMMARY_CHECK_TOKEN_TAG);
    }
  else
    {
    // Wait for previous node to give this node the token.
    this->Controller->Receive(&result, 1, myid-1,
                              VTK_PV_SUMMARY_CHECK_TOKEN_TAG);
    
    // Check this node.
    if(result)
      {
      result = this->CheckDummyFile();
      }
    
    // Pass token to next node.
    this->Controller->Send(&result, 1, (myid+1)%numProcs,
                           VTK_PV_SUMMARY_CHECK_TOKEN_TAG);
    }
  
  // Delete the dummy file on all nodes.
  this->DeleteDummyFiles();
  
  // Result is only meaningful on node 0.
  return result;
}

//----------------------------------------------------------------------------
int vtkPVSummaryHelper::CheckDummyFile()
{
  vtkstd::string name = this->Writer->GetFileName();
  name += ".dummy";
  struct stat fs;
  if(stat(name.c_str(), &fs) == 0)
    {
    // File exists.  We do not need to write a summary file.
    this->SetWriteSummaryFile(0);
    }
  else
    {
    // File does not exist.  We need to write a summary file.
    this->SetWriteSummaryFile(1);
    
    // Create the dummy file to tell any other node with which we
    // share a disk that it does not need to write a summary file.
#ifdef _WIN32
    ofstream fout(name.c_str(), ios::out | ios::binary);
#else
    ofstream fout(name.c_str(), ios::out);
#endif
    fout << "Dummy file to check disk sharing for parallel writes." << endl;
    if(!fout)
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSummaryHelper::DeleteDummyFiles()
{
  vtkstd::string name = this->Writer->GetFileName();
  name += ".dummy";

  // We must delete the dummy file on one node at a time.  Use a token
  // passing approach starting at node 0.
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myid = this->Controller->GetLocalProcessId();  
  int result = 0;
  if(myid == 0)
    {
    // Delete file on node 0.
    unlink(name.c_str());
    
    // Pass token to node 1.
    this->Controller->Send(&result, 1, 1, VTK_PV_SUMMARY_DELETE_TOKEN_TAG);
    
    // Wait for last node to finish.
    this->Controller->Receive(&result, 1, numProcs-1,
                              VTK_PV_SUMMARY_DELETE_TOKEN_TAG);
    }
  else
    {
    // Wait for previous node to give this node the token.
    this->Controller->Receive(&result, 1, myid-1,
                              VTK_PV_SUMMARY_DELETE_TOKEN_TAG);
    
    // Delete file on this node.
    unlink(name.c_str());
    
    // Pass token to next node.
    this->Controller->Send(&result, 1, (myid+1)%numProcs,
                           VTK_PV_SUMMARY_DELETE_TOKEN_TAG);
    }  
}

//----------------------------------------------------------------------------
void vtkPVSummaryHelper::SetWriteSummaryFile(int value)
{
  if(vtkXMLPDataWriter* wr1 = vtkXMLPDataWriter::SafeDownCast(this->Writer))
    {
    wr1->SetWriteSummaryFile(value);
    }
  else if(vtkXMLPVDWriter* wr2 = vtkXMLPVDWriter::SafeDownCast(this->Writer))
    {
    wr2->SetWriteCollectionFile(value);
    }
}
