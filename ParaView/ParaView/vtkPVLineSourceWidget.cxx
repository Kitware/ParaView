/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVLineSourceWidget.cxx
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
#include "vtkPVLineSourceWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVLineWidget.h"
#include "vtkPVSource.h"
#include "vtkPVProcessModule.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLineSourceWidget);
vtkCxxRevisionMacro(vtkPVLineSourceWidget, "1.7.4.4");

int vtkPVLineSourceWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::vtkPVLineSourceWidget()
{
  this->CommandFunction = vtkPVLineSourceWidgetCommand;
  this->LineWidget = vtkPVLineWidget::New();
  this->LineWidget->SetParent(this);
  this->LineWidget->SetTraceReferenceObject(this);
  this->LineWidget->SetTraceReferenceCommand("GetLineWidget");
  this->LineWidget->SetUseLabel(0);
}

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::~vtkPVLineSourceWidget()
{
  this->LineWidget->Delete();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  if (pvApp)
    {
    this->SourceID = pm->NewStreamObject("vtkLineSource");
    this->OutputID = pm->NewStreamObject("vtkPolyData");
    pm->GetStream() << vtkClientServerStream::Invoke << this->SourceID 
                    << "SetOutput" << this->OutputID << vtkClientServerStream::End;
    pm->SendStreamToServer();
    }



  this->LineWidget->SetPoint1VariableName("Point1");
  this->LineWidget->SetPoint2VariableName("Point2");
  this->LineWidget->SetResolutionVariableName("Resolution");
  this->LineWidget->SetPVSource(this->GetPVSource());
  this->LineWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToRed");
  
  this->LineWidget->Create(this->Application);
  this->Script("pack %s -side top -fill both -expand true",
               this->LineWidget->GetWidgetName());
}


//----------------------------------------------------------------------------
int vtkPVLineSourceWidget::GetModifiedFlag()
{
  if (this->ModifiedFlag || this->LineWidget->GetModifiedFlag())
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::ResetInternal()
{
  this->ModifiedFlag = 0;
  // Ignore the source passed in.  Modify our one source.
  this->LineWidget->ResetInternal();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::AcceptInternal(vtkClientServerID)
{
  // Ignore the source passed in.  Modify our one source.
  this->LineWidget->AcceptInternal(this->SourceID);
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVLineSourceWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  this->LineWidget->Trace(file);
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Select()
{
  this->LineWidget->Select();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Deselect()
{
  this->LineWidget->Deselect();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::SaveInBatchScript(ofstream *file)
{
  float pt[3];
  
  if (this->SourceID.ID == 0 || this->LineWidget == NULL)
    {
    vtkErrorMacro(<< this->GetClassName() << " must not have SaveInBatchScript method.");
    return;
    } 

  *file << "vtkLineSource " << "pvTemp" << this->SourceID.ID << "\n";
  this->LineWidget->GetPoint1(pt);
  *file << "\t" << this->SourceID << " SetPoint1 " 
        << pt[0] << " " << pt[1] << " " << pt[2] << endl; 
  this->LineWidget->GetPoint2(pt);
  *file << "\t" << this->SourceID << " SetPoint2 " 
        << pt[0] << " " << pt[1] << " " << pt[2] << endl; 
  *file << "\t" << this->SourceID << " SetResolution " 
        << this->LineWidget->GetResolution() << endl; 
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Line widget: " << this->LineWidget << endl;
}
