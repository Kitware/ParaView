/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSummaryHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
vtkCxxRevisionMacro(vtkPVSummaryHelper, "1.1");

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
