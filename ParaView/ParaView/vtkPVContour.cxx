/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContour.cxx
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
#include "vtkPVContour.h"
#include "vtkPVContourEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVSourceInterface.h"
#include "vtkContourFilter.h"
#include "vtkPVData.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkKWCompositeCollection.h"
#include "vtkObjectFactory.h"

int vtkPVContourCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContour::vtkPVContour()
{
  this->CommandFunction = vtkPVContourCommand;
  
  
  this->ScalarRangeLabel = vtkKWLabel::New();

  this->ReplaceInputOff();

}

//----------------------------------------------------------------------------
vtkPVContour::~vtkPVContour()
{
  this->ScalarRangeLabel->Delete();
  this->ScalarRangeLabel = NULL;
}

//----------------------------------------------------------------------------
vtkPVContour* vtkPVContour::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVContour");
  if(ret)
    {
    return (vtkPVContour*)ret;
    }
  return new vtkPVContour;
}

//----------------------------------------------------------------------------
void vtkPVContour::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVContourEntry *entry;
  vtkPVLabeledToggle *computeScalarsCheck;
  vtkPVLabeledToggle *computeNormalsCheck;
  vtkPVLabeledToggle *computeGradientsCheck;

  this->vtkPVSource::CreateProperties();

  this->AddInputMenu("Input", "NthPVInput 0", "vtkDataSet",
                     "Set the input to this filter.",
                     this->GetPVWindow()->GetSources());
  
  this->ScalarRangeLabel->SetParent(this->GetParameterFrame()->GetFrame());
  this->ScalarRangeLabel->Create(pvApp, "");
  
  entry = vtkPVContourEntry::New();
  entry->SetPVSource(this);
  entry->SetParent(this->GetParameterFrame()->GetFrame());
  entry->SetLabel("Contour Values");
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(pvApp);
  this->Widgets->AddItem(entry);

  this->Script("pack %s %s", this->ScalarRangeLabel->GetWidgetName(),
               entry->GetWidgetName());
  entry->Delete();
  entry = NULL;

  
  computeNormalsCheck = vtkPVLabeledToggle::New();
  computeNormalsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeNormalsCheck->SetLabel("Compute Normals");
  computeNormalsCheck->SetObjectVariable(this->GetVTKSourceTclName(), 
                                         "ComputeNormals");
  computeNormalsCheck->SetModifiedCommand(this->GetTclName(), 
                                          "ChangeAcceptButtonColor");
  computeNormalsCheck->Create(pvApp, "Select whether to compute normals");
  computeNormalsCheck->SetState(1);
  this->Widgets->AddItem(computeNormalsCheck);

  computeGradientsCheck = vtkPVLabeledToggle::New();
  computeGradientsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeGradientsCheck->SetLabel("Compute Gradients");
  computeGradientsCheck->SetObjectVariable(this->GetVTKSourceTclName(), 
                                           "ComputeGradients");
  computeGradientsCheck->SetModifiedCommand(this->GetTclName(), 
                                            "ChangeAcceptButtonColor");
  computeGradientsCheck->Create(pvApp, "Select whether to compute gradients");
  computeGradientsCheck->SetState(0);
  this->Widgets->AddItem(computeGradientsCheck);
  
  computeScalarsCheck = vtkPVLabeledToggle::New();
  computeScalarsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeScalarsCheck->SetLabel("Compute Scalars");
  computeScalarsCheck->SetObjectVariable(this->GetVTKSourceTclName(), 
                                         "ComputeScalars");
  computeScalarsCheck->SetModifiedCommand(this->GetTclName(), 
                                          "ChangeAcceptButtonColor");
  computeScalarsCheck->Create(pvApp, "Select whether to compute scalars");
  computeScalarsCheck->SetState(1);
  this->Widgets->AddItem(computeScalarsCheck);

  this->Script("pack %s %s %s -anchor w -padx 10",
               computeNormalsCheck->GetWidgetName(),
               computeGradientsCheck->GetWidgetName(),
               computeScalarsCheck->GetWidgetName());

  computeNormalsCheck->Delete();
  computeNormalsCheck= NULL;
  computeGradientsCheck->Delete();
  computeGradientsCheck= NULL;
  computeScalarsCheck->Delete();
  computeScalarsCheck= NULL;

  this->UpdateParameterWidgets();
}

//----------------------------------------------------------------------------
void vtkPVContour::UpdateScalars()
{
  float range[2];
  char label[100];
  // call the superclass method
  this->vtkPVSource::UpdateScalars();
  
  this->GetDataArrayRange(range);
  if (range[0] == 1.0 && range[1] == 0.0)
    {
    sprintf(label, "Invalid Data Range");
    }
  else
    {
    sprintf(label, "Data Range: %f to %f", range[0], range[1]);
    }
  this->ScalarRangeLabel->SetLabel(label);
}

//----------------------------------------------------------------------------
void vtkPVContour::GetDataArrayRange(float range[2])
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int id, num;
  vtkDataArray *array;
  float temp[2];
  
  range[0] = 1.0;
  range[1] = 0.0;
  if (! this->DefaultScalarsName)
    {
    return;
    }
  
  pvApp->BroadcastScript("Application SendDataArrayRange %s %s",
                         this->GetNthPVInput(0)->GetVTKDataTclName(),
                         this->DefaultScalarsName);
  array = this->GetNthPVInput(0)->GetVTKData()->GetPointData()->
                  GetArray(this->DefaultScalarsName);

  if (array != NULL)
    {
    array->GetRange(range, 0);
    }
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; id++)
    {
    controller->Receive(temp, 2, id, 1976);
    // try to protect against invalid ranges.
    if (range[0] > range[1])
      {
      range[0] = temp[0];
      range[1] = temp[1];
      }
    else if (temp[0] < temp[1])
      {
      if (temp[0] < range[0])
        {
        range[0] = temp[0];
        }
      if (temp[1] > range[1])
        {
        range[1] = temp[1];
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVContour::SaveInTclScript(ofstream* file)
{
  char* tempName;
  int i;
  vtkContourFilter *source =
    (vtkContourFilter*)this->GetVTKSource();
  vtkPVSourceInterface *pvsInterface =
    this->GetNthPVInput(0)->GetPVSource()->GetInterface();
  
  if (this->DefaultScalarsName)
    {
    *file << "vtkFieldDataToAttributeDataFilter "
          << this->ChangeScalarsFilterTclName << "\n\t"
          << this->ChangeScalarsFilterTclName << " SetInput [";
    if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                               "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      delete [] dataName;
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
      {
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput]\n\t";
      delete [] dataName;
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    *file << this->ChangeScalarsFilterTclName
          << " SetInputFieldToPointDataField\n\t";
    *file << this->ChangeScalarsFilterTclName
          << " SetOutputAttributeDataToPointData\n\t";
    *file << this->ChangeScalarsFilterTclName << " SetScalarComponent 0 "
          << this->DefaultScalarsName << " 0\n\n";
    }

  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";

  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  if (!this->DefaultScalarsName)
    {
    if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                               "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      delete [] dataName;
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
      {
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput]\n\t";
      delete [] dataName;
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    }
  else
    {
    *file << this->ChangeScalarsFilterTclName << " GetOutput]\n\t";
    }
  
  *file << this->VTKSourceTclName << " SetNumberOfContours "
        << source->GetNumberOfContours() << "\n\t";
  
  for (i = 0; i < source->GetNumberOfContours(); i++)
    {
    *file << this->VTKSourceTclName << " SetValue " << i << " "
          << source->GetValue(i) << "\n\t";
    }

  this->GetPVOutput(0)->SaveInTclScript(file, this->VTKSourceTclName);
}
