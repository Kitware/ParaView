/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDWriter.cxx
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
#include "vtkPVDWriter.h"

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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDWriter);
vtkCxxRevisionMacro(vtkPVDWriter, "1.3.2.1");

//----------------------------------------------------------------------------
vtkPVDWriter::vtkPVDWriter()
{
}

//----------------------------------------------------------------------------
vtkPVDWriter::~vtkPVDWriter()
{
}

//----------------------------------------------------------------------------
void vtkPVDWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVDWriter::CanWriteData(vtkDataSet* data, int, int)
{
  // We support all data types in both parallel and serial mode, and
  // with any number of parts.
  return data?1:0;
}

//----------------------------------------------------------------------------
void vtkPVDWriter::Write(const char* fileName, vtkPVSource* pvs,
                         int numProcs, int ghostLevel, int timeSeries)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkPVReaderModule* rm = vtkPVReaderModule::SafeDownCast(pvs);
  if(!rm)
    {
    timeSeries = 0;
    }
  
  // Create the writer.
  pm->ServerScript(
    "%s writer\n"
    "writer SetNumberOfPieces %d\n"
    "writer SetFileName {%s}\n"
    "writer SetGhostLevel %d\n"
    "writer SetPiece [[$Application GetProcessModule] GetPartitionId]\n",
    (timeSeries?"vtkXMLPVAnimationWriter":"vtkXMLPVDWriter"),
    numProcs, fileName, ghostLevel);
  
  // Tell each process's writer whether it should write the summary
  // file.
  if(numProcs > 1)
    {
    pm->ServerScript(
      "vtkPVSummaryHelper helper\n"
      "helper SetWriter writer\n"
      "helper SetController [$Application GetController]\n"
      "helper SynchronizeSummaryFiles\n"
      "helper Delete\n"
      );
    }
  
  if(timeSeries)
    {
    // Plug the inputs into the writer.
    int i;
    for(i=0; i < pvs->GetNumberOfParts(); ++i)
      {
// ********
//       pm->ServerScript("writer AddInput \"%s\" {%s}",
//                        pvs->GetPart(i)->GetVTKDataTclName(),
//                        pvs->GetName());
      }
    
    // Start the animation.
    pm->ServerScript("writer Start");
    
    // Loop through all of the time steps.
    for(int t = 0; t < rm->GetNumberOfTimeSteps(); ++t)
      {
      // Update the data to the next time step.
      rm->SetRequestedTimeStep(t);
      
      // Write this time step.
      pm->ServerScript("writer WriteTime {%d}", t);
      }
    
    // Finish the animation.
    pm->ServerScript("writer Finish");
    }
  else
    {
    // Plug the inputs into the writer.
    int i;
    // *********************
 //    for(i=0; i < pvs->GetNumberOfParts(); ++i)
//       {
//       pm->ServerScript("writer AddInput \"%s\"",
//                        pvs->GetPart(i)->GetVTKDataTclName());
//       }
    
    // Just write the current data.
    pm->ServerScript("writer Write");
    pm->ServerScript("writer GetErrorCode");
    int retVal = vtkKWObject::GetIntegerResult(pvApp);
    if (retVal == vtkErrorCode::OutOfDiskSpaceError)
      {
      vtkKWMessageDialog::PopupMessage(
        pvApp, pvApp->GetMainWindow(),
        "Write Error", "There is insufficient disk space to save this data. "
        "The file(s) already written will be deleted.");
      }
    }
  
  // Delete the writer.
  pm->ServerScript("writer Delete");
}
