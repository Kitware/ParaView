/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWriter.cxx
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
#include "vtkPVWriter.h"

#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVWriter);
vtkCxxRevisionMacro(vtkPVWriter, "1.10.2.2");

//----------------------------------------------------------------------------
vtkPVWriter::vtkPVWriter()
{
  this->InputClassName = 0;
  this->WriterClassName = 0;
  this->Description = 0;
  this->Extension = 0;
  this->Parallel = 0;
  this->DataModeMethod = 0;
}

//----------------------------------------------------------------------------
vtkPVWriter::~vtkPVWriter()
{
  this->SetInputClassName(0);
  this->SetWriterClassName(0);
  this->SetDescription(0);
  this->SetExtension(0);
  this->SetDataModeMethod(0);
}

//----------------------------------------------------------------------------
void vtkPVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputClassName: " 
     << (this->InputClassName?this->InputClassName:"(none)") << endl;
  os << indent << "WriterClassName: " 
     << (this->WriterClassName?this->WriterClassName:"(none)") << endl;
  os << indent << "Description: " 
     << (this->Description?this->Description:"(none)") << endl;
  os << indent << "Extension: " 
     << (this->Extension?this->Extension:"(none)") << endl;
  os << indent << "Parallel: " << this->Parallel << endl;
  os << indent << "DataModeMethod: " 
     << (this->DataModeMethod?this->DataModeMethod:"(none)") << endl;
}

//----------------------------------------------------------------------------
int vtkPVWriter::CanWriteData(vtkDataSet* data, int parallel, int numParts)
{
  if (data == NULL)
    {
    return 0;
    }
  return ((numParts == 1) &&
          (parallel == this->Parallel) &&
          data->IsA(this->InputClassName));
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVWriter::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->Application);
}

//----------------------------------------------------------------------------
void vtkPVWriter::Write(const char* fileName, vtkPVSource* pvs,
                        int numProcs, int ghostLevel, int timeSeries)
{
  vtkPVReaderModule* rm = vtkPVReaderModule::SafeDownCast(pvs);
  if(rm && timeSeries)
    {
    vtkstd::string name = fileName;
    vtkstd::string::size_type pos = name.find_last_of(".");
    vtkstd::string base = name.substr(0, pos);
    vtkstd::string ext = name.substr(pos);
    int n = rm->GetNumberOfTimeSteps();
    char buf[100];
    for(int i=0; i < n; ++i)
      {
      sprintf(buf, "T%03d", i);
      name = base;
      name += buf;
      name += ext;
      rm->SetRequestedTimeStep(i);
      if (!this->WriteOneFile(name.c_str(), pvs, numProcs, ghostLevel))
        {
        return;
        }
      }
    }
  else
    {
    this->WriteOneFile(fileName, pvs, numProcs, ghostLevel);
    }
}

//----------------------------------------------------------------------------
int vtkPVWriter::WriteOneFile(const char* fileName, vtkPVSource* pvs,
                              int numProcs, int ghostLevel)
{
  return 0;
#if 0  
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  const char* dataTclName = pvs->GetPart()->GetVTKDataTclName();
  int success = 1;
  
  if(!this->Parallel)
    {
    // Create the writer and configure it.
    pm->ServerScript("%s writer", this->WriterClassName);
    pm->ServerScript("writer SetFileName {%s}", fileName);
    pm->ServerScript("writer SetInput %s", dataTclName);
    if (this->DataModeMethod)
      {
      pm->ServerScript("writer %s", this->DataModeMethod);
      }
    
    // Write the data.
    pm->ServerScript("writer Write");
    pm->ServerScript("writer GetErrorCode");
    int retVal = vtkKWObject::GetIntegerResult(pvApp);
    if (retVal == vtkErrorCode::OutOfDiskSpaceError)
      {
      vtkKWMessageDialog::PopupMessage(
        pvApp, pvApp->GetMainWindow(),
        "Write Error", "There is insufficient disk space to save this data. "
        "The file(s) already written will be deleted.");
      success = 0;
      }

    // Cleanup.
    pm->ServerScript("writer Delete");
    }
  else
    {    
    // Create the writer and configure it.
    pm->ServerScript("%s writer", this->WriterClassName);
    pm->ServerScript("writer SetFileName %s", fileName);
    pm->ServerScript("writer SetInput %s", dataTclName);
    if(this->DataModeMethod)
      {
      pm->ServerScript("writer %s", this->DataModeMethod);
      }
    pm->ServerScript("writer SetNumberOfPieces %d", numProcs);
    pm->ServerScript("writer SetGhostLevel %d", ghostLevel);    
    pm->ServerScript(
      "writer SetStartPiece [[$Application GetProcessModule] GetPartitionId]\n"
      "writer SetEndPiece [[$Application GetProcessModule] GetPartitionId]");
    
    // Tell each process's writer whether it should write the summary
    // file.  This assumes that the writer is a vtkXMLWriter.  When we
    // add more writers, we will need a separate writer module.
    pm->ServerScript(
      "vtkPVSummaryHelper helper\n"
      "helper SetWriter writer\n"
      "helper SetController [$Application GetController]\n"
      "helper SynchronizeSummaryFiles\n"
      "helper Delete\n"
      );
    
    // Write the data.
    pm->ServerScript("writer Write");
    pm->ServerScript("writer GetErrorCode");
    int retVal = vtkKWObject::GetIntegerResult(pvApp);
    if (retVal == vtkErrorCode::OutOfDiskSpaceError)
      {
      vtkKWMessageDialog::PopupMessage(
        pvApp, pvApp->GetMainWindow(),
        "Write Error", "There is insufficient disk space to save this data. "
        "The file(s) already written will be deleted.");
      success = 0;
      }
    
    // Cleanup.
    pm->ServerScript("writer Delete");
    }
  return success;
#endif
}
