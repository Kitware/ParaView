/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoxWidget.cxx
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
#include "vtkPVBoxWidget.h"

#include "vtkCamera.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkBoxWidget.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVBoxWidget);
vtkCxxRevisionMacro(vtkPVBoxWidget, "1.1");

int vtkPVBoxWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVBoxWidget::vtkPVBoxWidget()
{
  this->BoxTclName = 0;
}

//----------------------------------------------------------------------------
vtkPVBoxWidget::~vtkPVBoxWidget()
{
  if (this->BoxTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", 
                                              this->BoxTclName);
    this->SetBoxTclName(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ResetInternal(const char* sourceTclName)
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  if ( this->BoxTclName )
  {
  //this->Script("eval %s SetState [ %s GetState ]", 
  //this->GetTclName(), this->BoxTclName);
  }
  this->Superclass::ResetInternal(sourceTclName);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ActualPlaceWidget()
{
  float center[3];
  float radius;
  this->Superclass::ActualPlaceWidget();
  vtkPVApplication *pvApp = static_cast<vtkPVApplication*>(
    this->Application);
  pvApp->BroadcastScript("%s GetPlanes %s", this->Widget3DTclName, this->BoxTclName);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::AcceptInternal(const char* sourceTclName)  
{
  this->PlaceWidget();
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  if ( this->BoxTclName )
    {
    vtkPVApplication *pvApp = static_cast<vtkPVApplication*>(
      this->Application);
    pvApp->BroadcastScript("%s GetPlanes %s", this->Widget3DTclName, this->BoxTclName);
    }
  this->Superclass::AcceptInternal(sourceTclName);
}


//---------------------------------------------------------------------------
void vtkPVBoxWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  /*
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof( this->CenterEntry[cc]->GetValue() );
    }
  *file << "$kw(" << this->GetTclName() << ") SetCenter "
        << val[0] << " " << val[1] << " " << val[2] << endl;

  rad = atof(this->RadiusEntry->GetValue());
  this->AddTraceEntry("$kw(%s) SetRadius %f", 
                      this->GetTclName(), rad);
  *file << "$kw(" << this->GetTclName() << ") SetRadius "
        << rad << endl;
        */
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::UpdateVTKObject(const char*)
{
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SaveInBatchScript(ofstream *file)
{
  *file << "vtkPlanes " << this->BoxTclName << endl;
  /*
  *file << "\t" << this->BoxTclName << " SetCenter ";
  this->Script("%s GetCenter", this->BoxTclName);
  *file << this->Application->GetMainInterp()->result << endl;
  *file << "\t" << this->BoxTclName << " SetRadius ";
  this->Script("%s GetRadius", this->BoxTclName);
  *file << this->Application->GetMainInterp()->result << endl;
  */
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BoxTclName: " 
     << (this->BoxTclName?this->BoxTclName:"none") << endl;
}

//----------------------------------------------------------------------------
vtkPVBoxWidget* vtkPVBoxWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVBoxWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ChildCreate(vtkPVApplication* pvApp)
{
  static int instanceCount = 0;
  char tclName[256];

  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Box");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  ++instanceCount;
  sprintf(tclName, "pvBoxWidget%d", instanceCount);
  this->SetWidget3DTclName(tclName);
  pvApp->BroadcastScript("vtkBoxWidget %s", tclName);
  pvApp->BroadcastScript("%s PlaceWidget 0 1 0 1 0 1", tclName);

  sprintf(tclName, "pvBox%d", instanceCount);
  pvApp->BroadcastScript("vtkPlanes %s", tclName);
  this->SetBoxTclName(tclName);
  
  this->SetFrameLabel("Box Widget");

  // Initialize the center of the sphere based on the input bounds.
  if (this->PVSource)
    {
    vtkPVSource *input = this->PVSource->GetPVInput(0);
    if (input)
      {
      this->Reset();
      pvApp->BroadcastScript("%s GetPlanes %s", this->Widget3DTclName, this->BoxTclName);
      }
    }

  this->SetBalloonHelpString(this->BalloonHelpString);

}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVBoxWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}
