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

#include "vtkContourFilter.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWFrame.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVContourEntry.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVScalarRangeLabel.h"
#include "vtkPVWindow.h"

int vtkPVContourCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContour::vtkPVContour()
{
  this->CommandFunction = vtkPVContourCommand;
  
  this->ArrayMenu = NULL;

  this->ReplaceInputOff();
}

//----------------------------------------------------------------------------
vtkPVContour::~vtkPVContour()
{
  if (this->ArrayMenu)
    {
    this->ArrayMenu->UnRegister(this);
    this->ArrayMenu = NULL;
    }
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
  this->vtkPVSource::CreateProperties();

  vtkContourFilter* contour = vtkContourFilter::SafeDownCast(this->VTKSource);
  if (contour)
    {
    contour->SetNumberOfContours(0);
    }

  vtkPVApplication*      pvApp = this->GetPVApplication();
  vtkPVContourEntry*     entry;
  vtkPVLabeledToggle*    computeScalarsCheck;
  vtkPVLabeledToggle*    computeNormalsCheck;
  vtkPVLabeledToggle*    computeGradientsCheck;

  vtkPVInputMenu* im = this->AddInputMenu(
    "Input", "PVInput", "vtkDataSet","Set the input to this filter.",
    this->GetPVWindow()->GetSourceList("Sources")); 
  im->SetModifiedCommand(this->GetTclName(), 
			 "SetAcceptButtonColorToRed");
  this->Script("pack %s -side top -fill x -expand t", im->GetWidgetName());

  this->ArrayMenu = vtkPVArrayMenu::New();
  this->ArrayMenu->SetNumberOfComponents(1);
  this->ArrayMenu->SetInputName("Input");
  this->ArrayMenu->SetAttributeType(vtkDataSetAttributes::SCALARS);
  this->ArrayMenu->SetLabel("Scalars");
  this->ArrayMenu->SetObjectTclName(this->GetVTKSourceTclName());
  this->ArrayMenu->SetParent(this->ParameterFrame->GetFrame());
  this->ArrayMenu->Create(this->Application);
  this->ArrayMenu->SetBalloonHelpString("Choose which scalar array you want"
					" to contour.");
  this->ArrayMenu->SetInputMenu(im);
  im->AddDependent(this->ArrayMenu);
  this->AddPVWidget(this->ArrayMenu);
  this->ArrayMenu->SetModifiedCommand(this->GetTclName(), 
				      "SetAcceptButtonColorToRed");
  this->Script("pack %s -side top -fill x -expand t", this->ArrayMenu->GetWidgetName());

  vtkPVScalarRangeLabel* scalarRange = vtkPVScalarRangeLabel::New();
  scalarRange->SetArrayMenu(this->ArrayMenu);
  scalarRange->SetTraceName("ScalarRangeLabel");
  scalarRange->SetParent(this->ParameterFrame->GetFrame());
  scalarRange->Create(this->Application);
  this->ArrayMenu->AddDependent(scalarRange);  
  this->AddPVWidget(scalarRange);
  scalarRange->SetModifiedCommand(this->GetTclName(), 
				  "SetAcceptButtonColorToRed");
  this->Script("pack %s", scalarRange->GetWidgetName());
  scalarRange->Delete();

  entry = vtkPVContourEntry::New();
  entry->SetPVSource(this);
  entry->SetParent(this->GetParameterFrame()->GetFrame());
  entry->SetLabel("Contour Values");
  entry->Create(pvApp);
  this->AddPVWidget(entry);
  this->Script("pack %s", entry->GetWidgetName());
  entry->SetModifiedCommand(this->GetTclName(), 
			    "SetAcceptButtonColorToRed");
  entry->Delete();
  entry = NULL;
  
  computeNormalsCheck = vtkPVLabeledToggle::New();
  computeNormalsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeNormalsCheck->SetLabel("Compute Normals");
  computeNormalsCheck->SetObjectVariable(this->GetVTKSourceTclName(),
					 "ComputeNormals");
  computeNormalsCheck->SetBalloonHelpString(
    "Select whether to compute normals");
  computeNormalsCheck->Create(pvApp);
  computeNormalsCheck->SetState(1);
  this->AddPVWidget(computeNormalsCheck);
  computeNormalsCheck->SetModifiedCommand(this->GetTclName(), 
					  "SetAcceptButtonColorToRed");

  computeGradientsCheck = vtkPVLabeledToggle::New();
  computeGradientsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeGradientsCheck->SetLabel("Compute Gradients");
  computeGradientsCheck->SetObjectVariable(this->GetVTKSourceTclName(),
					   "ComputeGradients");
  computeGradientsCheck->SetBalloonHelpString(
    "Select whether to compute gradients");
  computeGradientsCheck->Create(pvApp);
  computeGradientsCheck->SetState(0);
  this->AddPVWidget(computeGradientsCheck);
  computeGradientsCheck->SetModifiedCommand(this->GetTclName(), 
					    "SetAcceptButtonColorToRed");
  
  computeScalarsCheck = vtkPVLabeledToggle::New();
  computeScalarsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeScalarsCheck->SetLabel("Compute Scalars");
  computeScalarsCheck->SetObjectVariable(this->GetVTKSourceTclName(),
					 "ComputeScalars");
  computeScalarsCheck->SetBalloonHelpString(
    "Select whether to compute scalars");
  computeScalarsCheck->Create(pvApp);
  computeScalarsCheck->SetState(1);
  this->AddPVWidget(computeScalarsCheck);
  computeScalarsCheck->SetModifiedCommand(this->GetTclName(), 
					  "SetAcceptButtonColorToRed");

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
}


//----------------------------------------------------------------------------
void vtkPVContour::SetPVInput(vtkPVData *input)
{
  if (input == this->GetPVInput())
    {
    return;
    }

  this->vtkPVSource::SetPVInput(input);

  if (this->ArrayMenu)
    {
    this->ArrayMenu->Reset();
    if (this->ArrayMenu->GetValue() == NULL)
      {
      vtkKWMessageDialog::PopupMessage(
	this->Application, this->GetPVApplication()->GetMainWindow(), 
	"Warning", 
	"Input does not have scalars to contour.",
	vtkKWMessageDialog::WarningIcon);
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
  vtkPVSource *pvs = this->GetPVInput()->GetPVSource();
  

  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";

  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  if (pvs && strcmp(pvs->GetSourceClassName(), "vtkGenericEnSightReader") == 0)
    {
    char *charFound;
    int pos;
    char *dataName = new char[strlen(this->GetPVInput()->GetVTKDataTclName()) + 1];
    strcpy(dataName, this->GetPVInput()->GetVTKDataTclName());
    
    charFound = strrchr(dataName, 't');
    tempName = strtok(dataName, "O");
    *file << tempName << " GetOutput ";
    pos = charFound - dataName + 1;
    *file << dataName+pos << "]\n\t";
    delete [] dataName;
    }
  else if (pvs && strcmp(pvs->GetSourceClassName(), "vtkPDataSetReader") == 0)
    {
    char *dataName = new char[strlen(this->GetPVInput()->GetVTKDataTclName()) + 1];
    strcpy(dataName, this->GetPVInput()->GetVTKDataTclName());
    
    tempName = strtok(dataName, "O");
    *file << tempName << " GetOutput]\n\t";
    delete [] dataName;
    }
  else
    {
    *file << this->GetPVInput()->GetPVSource()->GetVTKSourceTclName()
	  << " GetOutput]\n\t";
    }
  
  *file << this->VTKSourceTclName << " SetNumberOfContours "
        << source->GetNumberOfContours() << "\n\t";
  
  for (i = 0; i < source->GetNumberOfContours(); i++)
    {
    *file << this->VTKSourceTclName << " SetValue " << i << " "
          << source->GetValue(i) << "\n\t";
    }

  this->GetPVOutput(0)->SaveInTclScript(file);
}

void vtkPVContour::InitializePrototype()
{


}

int vtkPVContour::ClonePrototypeInternal(int makeCurrent, vtkPVSource*& clone)
{
  return this->Superclass::ClonePrototypeInternal(makeCurrent, clone);
}
