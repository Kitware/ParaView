/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitPlaneWidget.cxx
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
#include "vtkPVImplicitPlaneWidget.h"

#include "vtkArrayMap.txx"
#include "vtkCamera.h"
#include "vtkImplicitPlaneWidget.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInputMenu.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVImplicitPlaneWidget);
vtkCxxRevisionMacro(vtkPVImplicitPlaneWidget, "1.22");

vtkCxxSetObjectMacro(vtkPVImplicitPlaneWidget, InputMenu, vtkPVInputMenu);

int vtkPVImplicitPlaneWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImplicitPlaneWidget::vtkPVImplicitPlaneWidget()
{
  int cc;

  this->InputMenu = 0;

  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();  
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->CenterEntry[cc] = vtkKWEntry::New();
    this->NormalEntry[cc] = vtkKWEntry::New();
    this->CoordinateLabel[cc] = vtkKWLabel::New();
   }
  this->CenterResetButton = vtkKWPushButton::New();
  this->NormalButtonFrame = vtkKWWidget::New();
  this->NormalCameraButton = vtkKWPushButton::New();
  this->NormalXButton = vtkKWPushButton::New();
  this->NormalYButton = vtkKWPushButton::New();
  this->NormalZButton = vtkKWPushButton::New();
  this->PlaneID.ID = 0;
  
  this->LastAcceptedCenter[0] = this->LastAcceptedCenter[1] =
    this->LastAcceptedCenter[2] = 0;
  this->LastAcceptedNormal[0] = this->LastAcceptedNormal[1] = 0;
  this->LastAcceptedNormal[2] = 1;
}

//----------------------------------------------------------------------------
vtkPVImplicitPlaneWidget::~vtkPVImplicitPlaneWidget()
{
  vtkPVProcessModule* pm = 0;
  this->SetInputMenu(NULL);
  if(this->GetPVApplication())
    {
    pm = this->GetPVApplication()->GetProcessModule();
    if (pm && this->PlaneID.ID != 0)
      {
      pm->DeleteStreamObject(this->PlaneID);
      pm->SendStreamToServer();
      this->PlaneID.ID = 0;
      }
    }
  int i;
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->Delete();
    this->NormalEntry[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->CenterResetButton->Delete();
  this->NormalButtonFrame->Delete();
  this->NormalCameraButton->Delete();
  this->NormalXButton->Delete();
  this->NormalYButton->Delete();
  this->NormalZButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::CenterResetCallback()
{
  vtkPVSource *input;
  double bds[6];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  input = this->PVSource->GetPVInput(0);
  if (input == NULL)
    {
    return;
    }
  input->GetDataInformation()->GetBounds(bds);
  this->CenterEntry[0]->SetValue(0.5*(bds[0]+bds[1]));
  this->CenterEntry[1]->SetValue(0.5*(bds[2]+bds[3]));
  this->CenterEntry[2]->SetValue(0.5*(bds[4]+bds[5]));

  this->SetCenter();
}


//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalCameraCallback()
{
  vtkKWView *view;
  vtkRenderer *ren;
  vtkCamera *cam;
  double normal[3];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  view = this->PVSource->GetView();
  if (view == NULL)
    {
    vtkErrorMacro("Could not get the view/camera to set the normal.");
    return;
    }
  ren = vtkRenderer::SafeDownCast(view->GetViewport());
  if (ren == NULL)
    {
    vtkErrorMacro("Could not get the renderer/camera to set the normal.");
    return;
    }
  cam = ren->GetActiveCamera();
  if (cam == NULL)
    {
    vtkErrorMacro("Could not get the camera to set the normal.");
    return;
    }
  cam->GetViewPlaneNormal(normal);

  this->SetNormal(-normal[0], -normal[1], -normal[2]);
  this->SetNormal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalXCallback()
{
  this->SetNormal(1,0,0);
  this->SetNormal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalYCallback()
{
  this->SetNormal(0,1,0);
  this->SetNormal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalZCallback()
{
  this->SetNormal(0,0,1);
  this->SetNormal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ResetInternal()
{
  vtkPVApplication *pvApp;

  if ( ! this->ModifiedFlag)
    {
    return;
    }

  pvApp = this->GetPVApplication(); 
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID << "SetDrawPlane" << 0
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  this->SetCenter(this->LastAcceptedCenter);
  this->SetNormal(this->LastAcceptedNormal);

  this->Superclass::ResetInternal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ActualPlaceWidget()
{
  float center[3];
  float normal[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    center[cc] = atof(this->CenterEntry[cc]->GetValue());
    normal[cc] = atof(this->NormalEntry[cc]->GetValue());
    }
 
  this->Superclass::ActualPlaceWidget();
  this->SetCenter(center[0], center[1], center[2]);
  this->SetNormal(normal[0], normal[1], normal[2]);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::AcceptInternal(vtkClientServerID sourceID)
{ 
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  this->PlaceWidget();
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID << "SetDrawPlane" << 0
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  // This should be done in the initialization.
  // There must be a more general way of hooking up the plane object.
  // ExtractCTH uses this varible, General Clipping uses the select widget.
  if (this->VariableName && sourceID.ID != 0)
    {
    ostrstream str;
    str << "Set" << this->VariableName << ends;
    pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                    << str.str() << this->PlaneID << vtkClientServerStream::End;
    pm->SendStreamToServer();
    delete [] str.str();
    }
  if ( this->PlaneID.ID != 0 )
    {
    float val[3];
    int cc;
    for ( cc = 0; cc < 3; cc ++ )
      {
      val[cc] = atof( this->CenterEntry[cc]->GetValue() );
      }
    this->SetCenterInternal(val[0], val[1], val[2]);
    pm->GetStream() << vtkClientServerStream::Invoke << this->PlaneID << "SetOrigin"
                    << val[0] << val[1] <<  val[2] << vtkClientServerStream::End;
    this->SetLastAcceptedCenter(val);
    for ( cc = 0; cc < 3; cc ++ )
      {
      val[cc] = atof( this->NormalEntry[cc]->GetValue() );
      }
    this->SetNormalInternal(val[0], val[1], val[2]);
    pm->GetStream() << vtkClientServerStream::Invoke << this->PlaneID << "SetNormal"
                    << val[0] << val[1] <<  val[2] << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    this->SetLastAcceptedNormal(val);
    }

  this->Superclass::AcceptInternal(sourceID);
}

//---------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Trace(ofstream *file)
{
  float val[3];
  int cc;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof( this->CenterEntry[cc]->GetValue() );
    }
  *file << "$kw(" << this->GetTclName() << ") SetCenter "
        << val[0] << " " << val[1] << " " << val[2] << endl;
   for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof( this->NormalEntry[cc]->GetValue() );
    }
  *file << "$kw(" << this->GetTclName() << ") SetNormal "
        << val[0] << " " << val[1] << " " << val[2] << endl;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SaveInBatchScript(ofstream *file)
{
  *file << "vtkPlane " << "pvTemp" << this->PlaneID.ID << endl;
  *file << "\t" << this->PlaneID.ID << " SetOrigin ";
  this->Script("%s GetOrigin", this->PlaneID.ID);
  *file << this->Application->GetMainInterp()->result << endl;
  *file << "\t" << this->PlaneID.ID << " SetNormal ";
  this->Script("%s GetNormal", this->PlaneID.ID);
  *file << this->Application->GetMainInterp()->result << endl;


  // There must be a more general way of hooking up the plane object.
  // ExtractCTH uses this varible, General Clipping uses the select widget.
  if (this->VariableName && this->PVSource)
    {
    int num, idx;
    num = this->PVSource->GetNumberOfVTKSources();
    for (idx = 0; idx < num; ++idx)
      {
      *file << this->PVSource->GetVTKSourceID().ID
            << " Set" << this->VariableName << " " 
            << this->PlaneID.ID << endl;                  
      }
    }

}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PlaneID: " << this->PlaneID;
  os << indent << "InputMenu: " << this->GetInputMenu();
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVImplicitPlaneWidget::ClonePrototypeInternal(
                                 vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* pvWidget = 0;

  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);

    vtkPVImplicitPlaneWidget* ipw = vtkPVImplicitPlaneWidget::SafeDownCast(pvWidget);
    if (!ipw)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
    
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      ipw->SetInputMenu(im);
      im->Delete();
      }
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }

  return pvWidget;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetBalloonHelpString(const char *str)
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
    this->Labels[0]->SetBalloonHelpString(this->BalloonHelpString);
    this->Labels[1]->SetBalloonHelpString(this->BalloonHelpString);

    this->CenterResetButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalCameraButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalXButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalYButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalZButton->SetBalloonHelpString(this->BalloonHelpString);

    for (int i=0; i<3; i++)
      {
      this->CoordinateLabel[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->CenterEntry[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->NormalEntry[i]->SetBalloonHelpString(this->BalloonHelpString);
      }

    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ChildCreate(vtkPVApplication* pvApp)
{
  static int instanceCount = 0;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();

  ++instanceCount;

  // Now that the 3D widget is on each process,
  // we do not need to create our own plane (but it does not hurt).
  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Plane");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
  this->PlaneID = pm->NewStreamObject("vtkPlane");

  // Create the 3D widget on each process.
  // This is for tiled display and client server.
  // This may decrease compresion durring compositing.
  // We should have a special call instead of broadcast script.
  // Better yet, control visibility based on mode (client-server ...). 
  this->Widget3DID = pm->NewStreamObject("vtkImplicitPlaneWidget");
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "SetPlaceFactor" << 1.0 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "OutlineTranslationOff"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "PlaceWidget"
                  << 0 << 1 << 0 << 1 << 0 << 1
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();

  this->SetFrameLabel("Plane Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel("Center");
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetLabel("Normal");

  int i;
  for (i=0; i<3; i++)
    {
    this->CoordinateLabel[i]->SetParent(this->Frame->GetFrame());
    this->CoordinateLabel[i]->Create(pvApp, "");
    char buffer[3];
    sprintf(buffer, "%c", "xyz"[i]);
    this->CoordinateLabel[i]->SetLabel(buffer);
    }
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->SetParent(this->Frame->GetFrame());
    this->CenterEntry[i]->Create(pvApp, "");
    }

  for (i=0; i<3; i++)    
    {
    this->NormalEntry[i]->SetParent(this->Frame->GetFrame());
    this->NormalEntry[i]->Create(pvApp, "");
    }

  this->Script("grid propagate %s 1",
               this->Frame->GetFrame()->GetWidgetName());

  this->Script("grid x %s %s %s -sticky ew",
               this->CoordinateLabel[0]->GetWidgetName(),
               this->CoordinateLabel[1]->GetWidgetName(),
               this->CoordinateLabel[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
               this->Labels[0]->GetWidgetName(),
               this->CenterEntry[0]->GetWidgetName(),
               this->CenterEntry[1]->GetWidgetName(),
               this->CenterEntry[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
               this->Labels[1]->GetWidgetName(),
               this->NormalEntry[0]->GetWidgetName(),
               this->NormalEntry[1]->GetWidgetName(),
               this->NormalEntry[2]->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0", 
               this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 2", 
               this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 2", 
               this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 3 -weight 2", 
               this->Frame->GetFrame()->GetWidgetName());

  for (i=0; i<3; i++)
    {
    this->Script("bind %s <Key> {%s SetValueChanged}",
                 this->CenterEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <Key> {%s SetValueChanged}",
                 this->NormalEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetCenter}",
                 this->CenterEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetNormal}",
                 this->NormalEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetCenter}",
                 this->CenterEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetNormal}",
                 this->NormalEntry[i]->GetWidgetName(),
                 this->GetTclName());
    }
  this->CenterResetButton->SetParent(this->Frame->GetFrame());
  this->CenterResetButton->Create(pvApp, "");
  this->CenterResetButton->SetLabel("Set Plane Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("grid %s - - - - -sticky ew", 
               this->CenterResetButton->GetWidgetName());

  this->NormalButtonFrame->SetParent(this->Frame->GetFrame());
  this->NormalButtonFrame->Create(pvApp, "frame", "");
  this->Script("grid %s - - - - -sticky ew", 
               this->NormalButtonFrame->GetWidgetName());

  this->NormalCameraButton->SetParent(this->NormalButtonFrame);
  this->NormalCameraButton->Create(pvApp, "");
  this->NormalCameraButton->SetLabel("Use Camera Normal");
  this->NormalCameraButton->SetCommand(this, "NormalCameraCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalCameraButton->GetWidgetName());
  this->NormalXButton->SetParent(this->NormalButtonFrame);
  this->NormalXButton->Create(pvApp, "");
  this->NormalXButton->SetLabel("X Normal");
  this->NormalXButton->SetCommand(this, "NormalXCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalXButton->GetWidgetName());
  this->NormalYButton->SetParent(this->NormalButtonFrame);
  this->NormalYButton->Create(pvApp, "");
  this->NormalYButton->SetLabel("Y Normal");
  this->NormalYButton->SetCommand(this, "NormalYCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalYButton->GetWidgetName());
  this->NormalZButton->SetParent(this->NormalButtonFrame);
  this->NormalZButton->Create(pvApp, "");
  this->NormalZButton->SetLabel("Z Normal");
  this->NormalZButton->SetCommand(this, "NormalZCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalZButton->GetWidgetName());

  // Initialize the center of the plane based on the input bounds.
  if (this->PVSource)
    {
    vtkPVSource *input = this->PVSource->GetPVInput(0);
    if (input)
      {
      double bds[6];
      input->GetDataInformation()->GetBounds(bds);
      pm->GetStream() << vtkClientServerStream::Invoke << this->PlaneID << "SetOrigin"
                      << 0.5*(bds[0]+bds[1]) << 0.5*(bds[2]+bds[3]) << 0.5*(bds[4]+bds[5])
                      << vtkClientServerStream::End;
      this->SetLastAcceptedCenter(0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                                  0.5*(bds[4]+bds[5]));
      this->SetCenter(0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                      0.5*(bds[4]+bds[5]));
      pm->GetStream() << vtkClientServerStream::Invoke << this->PlaneID << "SetNormal"
                      << 0 << 0 << 1
                      << vtkClientServerStream::End;
      this->SetLastAcceptedNormal(0, 0, 1);
      this->SetNormal(0, 0, 1);
      }
    }
  float opacity = 1.0;
  if (pvApp->GetProcessModule()->GetNumberOfPartitions() == 1)
    { 
    opacity = .25;
    }
  
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID << "GetPlaneProperty"
                  << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetOpacity" 
                  << opacity 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID
                  << "GetSelectedPlaneProperty" 
                  << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetOpacity" 
                  << opacity 
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  this->SetBalloonHelpString(this->BalloonHelpString);

}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  vtkImplicitPlaneWidget *widget = vtkImplicitPlaneWidget::SafeDownCast(wdg);
  if ( widget )
    {
    double val[3];
    widget->GetOrigin(val); 
    this->SetCenterInternal(val[0], val[1], val[2]);
    widget->GetNormal(val);
    this->SetNormalInternal(val[0], val[1], val[2]);
    if (!widget->GetDrawPlane())
      { 
      vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->Widget3DID << "SetDrawPlane" << 1
                      << vtkClientServerStream::End;
      pm->SendStreamToClientAndServer();
      }
    }
  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVImplicitPlaneWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }
  
  vtkPVXMLElement* ame = element->LookupElement(input_menu);
  if (!ame)
    {
    vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
    return 0;
    }
  vtkPVWidget* w = this->GetPVWidgetFromParser(ame, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();  
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetCenterInternal(float x, float y, float z)
{
  this->CenterEntry[0]->SetValue(x);
  this->CenterEntry[1]->SetValue(y);
  this->CenterEntry[2]->SetValue(z); 
  if ( this->Widget3DID.ID )
    { 
    vtkPVApplication *pvApp = this->GetPVApplication();
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->Widget3DID << "SetOrigin" << x << y << z
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetCenter(float x, float y, float z)
{
  this->SetCenterInternal(x,y,z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetNormalInternal(float x, float y, float z)
{
  this->NormalEntry[0]->SetValue(x);
  this->NormalEntry[1]->SetValue(y);
  this->NormalEntry[2]->SetValue(z); 
  if ( this->Widget3DID.ID)
    {
    vtkPVApplication *pvApp = this->GetPVApplication(); 
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->Widget3DID << "SetNormal" << x << y << z
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetNormal(float x, float y, float z)
{
  this->SetNormalInternal(x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetCenter()
{
  float val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->CenterEntry[cc]->GetValue());
    }
  this->SetCenter(val[0], val[1], val[2]);
  this->Render();
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetNormal()
{
  float val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->NormalEntry[cc]->GetValue());
    }
  this->SetNormal(val[0], val[1], val[2]);
  this->Render();
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Update()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVSource *input;
  double bds[6];

  this->Superclass::Update();

  if (this->InputMenu == NULL)
    {
    return;
    }

  input = this->InputMenu->GetCurrentValue();
  if (input)
    {
    input->GetDataInformation()->GetBounds(bds);
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "PlaceWidget"
                    << bds[0] << bds[1] << bds[2] << bds[3] << bds[4] << bds[5]
                    << vtkClientServerStream::End;

    // Should I also move the center of the plane?  Keep the old plane?
    // Keep the old normal?

    // There has to be a better way to que a render.
    this->GetPVApplication()->GetMainWindow()->GetMainView()->EventuallyRender();
    }
}
