/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.cxx
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

#include "vtkToolkits.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPVTreeComposite.h"
#include "vtkPVRenderView.h"
#include "vtkKWInteractor.h"
#include "vtkPVApplication.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include "vtkKWEventNotifier.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#if VTK_USE_MESA
#include "vtkMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#endif
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#endif

#include "vtkTimerLog.h"
#include "vtkPVActorComposite.h"
#include "vtkKWCornerAnnotation.h"


//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVRenderView::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVRenderView");
  if(ret)
    {
    return (vtkPVRenderView*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVRenderView;
}


int vtkPVRenderViewCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  if (getenv("PV_SEPARATE_RENDER_WINDOW") != NULL)
    {
    this->TopLevelRenderWindow = vtkKWWidget::New();
    this->TopLevelRenderWindow->SetParent(this->Frame);
    this->VTKWidget->SetParent(NULL);
    this->VTKWidget->SetParent(this->TopLevelRenderWindow);
    }
  else
    {
    this->TopLevelRenderWindow = NULL;
    }
    

  this->CommandFunction = vtkPVRenderViewCommand;
  
  this->Interactive = 0;

  this->UseReductionFactor = 1;
  
  this->RenderWindow->SetDesiredUpdateRate(1.0);  

  this->NavigationFrame = vtkKWLabeledFrame::New();
  this->NavigationCanvas = vtkKWWidget::New();
  
  this->CurrentInteractor = NULL;
  this->EventuallyRenderFlag = 0;
  this->RenderPending = NULL;
}

//----------------------------------------------------------------------------
void PVRenderViewAbortCheck(void *arg)
{
  vtkPVRenderView *me = (vtkPVRenderView*)arg;
  int abort;

  // if we are printing then do not abort
  if (me->GetPrinting())
    {
    return;
    }
  
  abort = me->ShouldIAbort();
  if (abort == 1)
    {
    me->GetRenderWindow()->SetAbortRender(1);
    me->EventuallyRender();
    }
  if (abort == 2)
    {
    //("Abort 2");
    me->GetRenderWindow()->SetAbortRender(2);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CreateRenderObjects(vtkPVApplication *pvApp)
{
  char *str;

  // Get rid of renderer created by the superclass
  this->Renderer->Delete();
  this->Renderer = (vtkRenderer*)pvApp->MakeTclObject("vtkRenderer", "Ren1");
  this->RendererTclName = NULL;
  this->SetRendererTclName("Ren1");
  
  // Get rid of render window created by the superclass
  this->RenderWindow->Delete();
  this->RenderWindow = (vtkRenderWindow*)pvApp->MakeTclObject("vtkRenderWindow", "RenWin1");
  this->RenderWindowTclName = NULL;
  this->SetRenderWindowTclName("RenWin1");
  
  // Create the compositer.
  str = getenv("PV_DISABLE_COMPOSITE_INTERRUPTS");
  if ( str != NULL)
    {
    this->Composite = (vtkTreeComposite*)pvApp->MakeTclObject("vtkTreeComposite", "TreeComp1");
    }
  else
    {
    this->Composite = (vtkTreeComposite*)pvApp->MakeTclObject("vtkPVTreeComposite", "TreeComp1");
    }

  this->CompositeTclName = NULL;
  this->SetCompositeTclName("TreeComp1");

  pvApp->BroadcastScript("%s AddRenderer %s", this->RenderWindowTclName,
			 this->RendererTclName);
  pvApp->BroadcastScript("%s SetRenderWindow %s", this->CompositeTclName,
			 this->RenderWindowTclName);
  pvApp->BroadcastScript("%s InitializeRMIs", this->CompositeTclName);
  pvApp->BroadcastScript("%s InitializeOffScreen", this->CompositeTclName);

  // Tree compositer handles aborts now.
#ifndef VTK_USE_MPI
  this->RenderWindow->SetAbortCheckMethod(PVRenderViewAbortCheck, (void*)this);
#endif
  
  // The only call that should not be a broadcast is render.
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->NavigationFrame->Delete();
  this->NavigationFrame = NULL;
  this->NavigationCanvas->Delete();
  this->NavigationCanvas = NULL;

  if (this->CurrentInteractor != NULL)
    {
    this->CurrentInteractor->UnRegister(this);
    this->CurrentInteractor = NULL;
    }

  // Tree Composite
  if (this->Composite)
    {
    pvApp->BroadcastScript("%s Delete", this->CompositeTclName);
    this->SetCompositeTclName(NULL);
    this->Composite = NULL;
    }

  if (this->Renderer)
    {
    pvApp->BroadcastScript("%s Delete", this->RendererTclName);
    this->SetRendererTclName(NULL);
    this->Renderer = NULL;
    }

  if (this->RenderWindow)
    {
    pvApp->BroadcastScript("%s Delete", this->RenderWindowTclName);
    this->SetRenderWindowTclName(NULL);
    this->RenderWindow = NULL;
    }

  // undo the binding we set up
  this->Script("bind %s <Motion> {}", this->VTKWidget->GetWidgetName());
  if (this->RenderPending)
    {
    this->Script("after cancel %s", this->RenderPending);
    }
  this->SetRenderPending(NULL);

  if (this->TopLevelRenderWindow)
    {
    this->TopLevelRenderWindow->Delete();
    this->TopLevelRenderWindow = NULL;
    }
}

//----------------------------------------------------------------------------
// Here we are going to change only the satellite procs.
void vtkPVRenderView::PrepareForDelete()
{
  vtkPVTreeComposite *c;
  
  // Circular reference.
  c = vtkPVTreeComposite::SafeDownCast(this->Composite); 
  if (c)
    {
    c->SetRenderView(NULL);
    }

  //if (this->CornerAnnotation)
  //  {
  //  this->CornerAnnotation->SetView(NULL);
  //  }


  //if (this->Frame)
  //  {
  //  this->Frame->SetParent(NULL);
  //  this->Frame->Delete();
  //  this->Frame = NULL;
  //  }

  //this->SetParent(NULL);
}


//----------------------------------------------------------------------------
void vtkPVRenderView::Close()
{
  this->PrepareForDelete();
  vtkKWView::Close();
}


//----------------------------------------------------------------------------
// Here we are going to change only the satellite procs.
void vtkPVRenderView::OffScreenRenderingOn()
{
#ifdef VTK_USE_MESA  
  int i, num;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller;
  
  controller = pvApp->GetController();
  num = 1;
  if (controller)
    {  
    num = controller->GetNumberOfProcesses();
    }
  
  for (i = 1; i < num; ++i)
    {
    pvApp->RemoteScript(i, "%s Delete", this->RendererTclName);
    pvApp->RemoteScript(i, "%s Delete", this->RenderWindowTclName);
    pvApp->RemoteScript(i, "vtkMesaRenderer %s", this->RendererTclName);
    pvApp->RemoteScript(i, "vtkMesaRenderWindow %s", this->RenderWindowTclName);
    pvApp->RemoteScript(i, "%s AddRenderer %s", 
			this->RenderWindowTclName, this->RendererTclName);
    pvApp->RemoteScript(i, "%s SetRenderWindow %s", 
			this->CompositeTclName, this->RendererTclName);
    }
  
#endif
}
  


//----------------------------------------------------------------------------
vtkRenderer *vtkPVRenderView::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkPVRenderView::GetRenderWindow()
{
  return this->RenderWindow;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Create(vtkKWApplication *app, const char *args)
{
  char *local;
  const char *wname;
  
  local = new char [strlen(args)+100];

  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }
  this->SetApplication(app);

  // Application has to be set before we can get a tcl name.
  // Otherwise I would have done this in "CreateRenderObjects".
  // Create the compositer.
  if ( getenv("PV_DISABLE_COMPOSITE_INTERRUPTS") == NULL)
    {
    // Since the render view is only on process 0, do not broadcast.
    this->GetPVApplication()->Script("%s SetRenderView %s", 
			                          this->CompositeTclName, this->GetTclName());
    }

  // create the frame
  wname = this->GetWidgetName();
  this->Script("frame %s -bd 0 %s",wname,args);
  //this->Script("pack %s -expand yes -fill both",wname);
  
  // create the label
  this->Frame->Create(app,"frame","-bd 3 -relief ridge");
  this->Script("pack %s -expand yes -fill both -side top -anchor nw",
               this->Frame->GetWidgetName());
  this->Frame2->Create(app,"frame","-bd 0 -bg #888");
  this->Script("pack %s -fill x -side top -anchor nw",
               this->Frame2->GetWidgetName());
  this->Label->Create(app,"label","-fg #fff -text {3D View} -bd 0");
  this->Script("pack %s  -side left -anchor w",this->Label->GetWidgetName());
  this->Script("bind %s <Any-ButtonPress> {%s MakeSelected}",
               this->Label->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonPress> {%s MakeSelected}",
               this->Frame2->GetWidgetName(), this->GetTclName());

  // Create the control frame - only pack it if support option enabled
  this->ControlFrame->Create(app,"frame","-bd 0");
  if (this->SupportControlFrame)
    {
    this->Script("pack %s -expand t -fill both -side top -anchor nw",
                 this->ControlFrame->GetWidgetName());
    }
  
  // Separate window for the renderer.
  if (getenv("PV_SEPARATE_RENDER_WINDOW") != NULL)
    {
    this->TopLevelRenderWindow->Create(app, "toplevel", "");
    this->Script("wm title %s ParaView", this->TopLevelRenderWindow->GetWidgetName());
    }

  // add the -rw argument
  sprintf(local,"%s -rw Addr=%p",args,this->RenderWindow);
  this->Script("vtkTkRenderWidget %s %s",
               this->VTKWidget->GetWidgetName(),local);
  //this->Script("vtkTkRenderWidget %s %s",
  //             this->VTKWidget->GetWidgetName(),args);
  this->Script("pack %s -expand yes -fill both -side top -anchor nw",
               this->VTKWidget->GetWidgetName());
  
  // Expose.
  this->Script("bind %s <Expose> {%s Exposed}", this->GetTclName(),
	       this->GetTclName());
  
  this->NavigationFrame->SetParent(this->GetPropertiesParent());
  this->NavigationFrame->Create(this->Application);
  this->NavigationFrame->SetLabel("Navigation");
  this->Script("pack %s -fill x -expand t -side top", this->NavigationFrame->GetWidgetName());
  this->NavigationCanvas->SetParent(this->NavigationFrame->GetFrame());

  // 360
  this->NavigationCanvas->Create(this->Application, "canvas", "-height 45 -width 356 -bg white"); 
  this->Script("pack %s -fill x -expand t -side top", this->NavigationCanvas->GetWidgetName());

  this->EventuallyRender();
  delete [] local;
}

void vtkPVRenderView::UpdateNavigationWindow(vtkPVSource *currentSource)
{
  vtkPVSource *source;
  vtkPVData **inputs = currentSource->GetPVInputs();
  vtkPVData **outputs;
  int numInputs, xMid, yMid, y, i;
  char *result, *tmp;
  int bbox[4], bboxOut[4];
  vtkPVData *moreOut;
  static char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";  
  
  // Clear the canvas
  this->Script("%s delete all",
               this->NavigationCanvas->GetWidgetName());

  
  // Put the inputs in the canvas.
  if (inputs)
    {
    y = 10;
    numInputs = currentSource->GetNumberOfPVInputs();
    for (i = 0; i < numInputs; i++)
      {
      source = inputs[i]->GetPVSource();
      if (source)
        {
        // Draw the name of the assembly.
        this->Script(
          "%s create text %d %d -text {%s} -font %s -anchor w -tags x -fill blue",
          this->NavigationCanvas->GetWidgetName(), 20, y,
          source->GetName(), font);
        
        result = this->Application->GetMainInterp()->result;
        tmp = new char[strlen(result)+1];
        strcpy(tmp,result);
        this->Script("%s bind %s <ButtonPress-1> {%s SelectSource %s}",
                     this->NavigationCanvas->GetWidgetName(), tmp,
                     currentSource->GetTclName(), source->GetTclName());
        
        // Get the bounding box for the name. We may need to highlight it.
        this->Script("%s bbox %s", this->NavigationCanvas->GetWidgetName(),
                     tmp);
        delete [] tmp;
        tmp = NULL;
        result = this->Application->GetMainInterp()->result;
        sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
        if (i == 0)
          {
          // only want to set xMid and yMid once
          yMid = (int)(0.5 * (bbox[1]+bbox[3]));
          xMid = (int)(0.5 * (bbox[2]+120));
          }
        
        // Draw a line from input to source.
        if (y == 10)
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->NavigationCanvas->GetWidgetName(), bbox[2], yMid,
                       125, yMid);
          }
        else
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->NavigationCanvas->GetWidgetName(), xMid, yMid,
                       xMid, yMid+15);
          yMid += 15;
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->NavigationCanvas->GetWidgetName(), bbox[2],
                       yMid, xMid, yMid);
          }
        
        if (source->GetPVInputs())
          {
          if (source->GetNthPVInput(0)->GetPVSource())
            {
            // Draw ellipsis indicating that this source has a source.
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 6, yMid, 8,
                         yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 10, yMid, 12,
                         yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 14, yMid, 16,
                         yMid);
            }
          }
        }
      y += 15;
      }
    }

  // Draw the name of the assembly.
  this->Script(
    "%s create text %d %d -text {%s} -font %s -anchor w -tags x",
    this->NavigationCanvas->GetWidgetName(), 130, 10, currentSource->GetName(),
    font);
  result = this->Application->GetMainInterp()->result;
  tmp = new char[strlen(result)+1];
  strcpy(tmp,result);
  // Get the bounding box for the name. We may need to highlight it.
  this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(), tmp);
  delete [] tmp;
  tmp = NULL;
  result = this->Application->GetMainInterp()->result;
  sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
  yMid = (int)(0.5 * (bbox[1]+bbox[3]));
  xMid = (int)(0.5 * (bbox[2] + 245));

  // Put the outputs in the canvas.
  outputs = currentSource->GetPVOutputs();
  if (outputs)
    {
    y = 10;
    if (outputs[0])
      {
      for (i = 0; i < outputs[0]->GetNumberOfPVConsumers(); i++)
	{
        source = outputs[0]->GetPVConsumer(i);
        
        // Draw the name of the assembly.
        this->Script(
          "%s create text %d %d -text {%s} -font %s -anchor w -tags x -fill blue",
          this->NavigationCanvas->GetWidgetName(), 250, y,
          source->GetName(), font);
        
        result = this->Application->GetMainInterp()->result;
        tmp = new char[strlen(result)+1];
        strcpy(tmp, result);
        this->Script("%s bind %s <ButtonPress-1> {%s SelectSource %s}",
                     this->NavigationCanvas->GetWidgetName(), tmp,
                     currentSource->GetTclName(), source->GetTclName());
        // Get the bounding box for the name. We may need to highlight it.
        this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(),
                      tmp);
        delete [] tmp;
        tmp = NULL;
        result = this->Application->GetMainInterp()->result;
        sscanf(result, "%d %d %d %d", bboxOut, bboxOut+1, bboxOut+2,
               bboxOut+3);
        
        // Draw to output.
        if (y == 10)
          { // first is a special case (single line).
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->NavigationCanvas->GetWidgetName(), bbox[2],
                       yMid, 245, yMid);
          }
        else
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->NavigationCanvas->GetWidgetName(), xMid, yMid,
                       xMid, yMid+15);
          yMid += 15;
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->NavigationCanvas->GetWidgetName(), xMid, yMid,
                       245, yMid);
          }
        if ((moreOut = source->GetPVOutput(0)))
          {
          if (moreOut->GetNumberOfPVConsumers() > 0)
            {
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(),
                         bboxOut[2]+10, yMid, bboxOut[2]+12, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(),
                         bboxOut[2]+14, yMid, bboxOut[2]+16, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(),
                         bboxOut[2]+18, yMid, bboxOut[2]+20, yMid);
            }
          }
        y += 15;
	}
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackgroundColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  pvApp->BroadcastScript("%s SetBackground %f %f %f",
                         this->RendererTclName, r, g, b);
  this->EventuallyRender();
}

//----------------------------------------------------------------------------
// a litle more complex than just "bind $widget <Expose> {%W Render}"
// we have to handle all pending expose events otherwise they que up.
void vtkPVRenderView::Exposed()
{
  if (this->InExpose) return;
  this->InExpose = 1;
  this->Script("update");
  this->EventuallyRender();
  this->InExpose = 0;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Update()
{
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ComputeVisiblePropBounds(float bounds[6])
{  
  this->GetRenderer()->ComputeVisiblePropBounds(bounds);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCamera()
{
  vtkCamera *cam;
  double *n;
  double mag2;

  // Lets see if we can correct the situation when camera ivars go arwy.
  // Unfortunately, I cannot reproduce the problem.
  cam = this->GetRenderer()->GetActiveCamera();
  n = cam->GetViewPlaneNormal();
  mag2 = n[0]*n[0] + n[1]*n[1] + n[2]*n[2];
  if (mag2 > 99999.0)
    {
    // Must be a problem.
    cam->SetPosition(0.0, 0.0, -1.0);
    cam->SetFocalPoint(0.0, 0.0, -1.0);
    cam->SetViewUp(0.0, 1.0, 0.0);
    }

  this->GetRenderer()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCameraClippingRange()
{
  this->GetRenderer()->ResetCameraClippingRange();
}

void vtkPVRenderView::AddBindings()
{
  this->Script("bind %s <Motion> {%s MotionCallback %%x %%y}",
               this->VTKWidget->GetWidgetName(), this->GetTclName());
}
    
//----------------------------------------------------------------------------
void vtkPVRenderView::AButtonPress(int num, int x, int y)
{
  if (this->CurrentInteractor)
    {
    this->CurrentInteractor->AButtonPress(num, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::AButtonRelease(int num, int x, int y)
{
  if (this->CurrentInteractor)
    {
    this->CurrentInteractor->AButtonRelease(num, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Button1Motion(int x, int y)
{
  if (this->CurrentInteractor)
    {
    this->CurrentInteractor->Button1Motion(x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Button2Motion(int x, int y)
{
  if (this->CurrentInteractor)
    {
    this->CurrentInteractor->Button2Motion(x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Button3Motion(int x, int y)
{
  if (this->CurrentInteractor)
    {
    this->CurrentInteractor->Button3Motion(x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::MotionCallback(int x, int y)
{
  if (this->CurrentInteractor)
    {
    this->CurrentInteractor->MotionCallback(x, y);
    }
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVRenderView::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


//----------------------------------------------------------------------------
void vtkPVRenderView::AddComposite(vtkKWComposite *c)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVActorComposite *pvc = vtkPVActorComposite::SafeDownCast(c);
  
  if (pvc == NULL)
    {
    // Default
    this->vtkKWView::AddComposite(c);
    return;
    }
  
  c->SetView(this);
  // never allow a composite to be added twice
  if (this->Composites->IsItemPresent(c))
    {
    return;
    }
  this->Composites->AddItem(c);
  if (pvc->GetPropTclName() != NULL)
    {
    pvApp->BroadcastScript("%s AddProp %s", this->RendererTclName,
			   pvc->GetPropTclName());
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemoveComposite(vtkKWComposite *c)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVActorComposite *pvc = vtkPVActorComposite::SafeDownCast(c);

  if (pvc == NULL)
    {
    // Default
    this->vtkKWView::RemoveComposite(c);
    return;
    }
  
  c->SetView(NULL);
  if (pvc->GetPropTclName() != NULL)
    {
    pvApp->BroadcastScript("%s RemoveProp %s", this->RendererTclName,
			   pvc->GetPropTclName());
    }
  this->Composites->RemoveItem(c);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StartRender()
{
  float renderTime = 1.0 / this->RenderWindow->GetDesiredUpdateRate();
  int *windowSize = this->RenderWindow->GetSize();
  int area, reducedArea, reductionFactor;
  float timePerPixel;
  float getBuffersTime, setBuffersTime, transmitTime;
  float newReductionFactor;
  float maxReductionFactor;
  
  if (!this->UseReductionFactor)
    {
    this->GetComposite()->SetReductionFactor(1);
    return;
    }
  
  // Do not let the width go below 150.
  maxReductionFactor = windowSize[0] / 150.0;

  renderTime *= 0.5;
  area = windowSize[0] * windowSize[1];
  reductionFactor = this->GetComposite()->GetReductionFactor();
  reducedArea = area / (reductionFactor * reductionFactor);
  getBuffersTime = this->GetComposite()->GetGetBuffersTime();
  setBuffersTime = this->GetComposite()->GetSetBuffersTime();
  transmitTime = this->GetComposite()->GetCompositeTime();

  // Do not consider SetBufferTime because 
  //it is not dependent on reduction factor.,
  timePerPixel = (getBuffersTime + transmitTime) / reducedArea;
  newReductionFactor = sqrt(area * timePerPixel / renderTime);
  
  if (newReductionFactor > maxReductionFactor)
    {
    newReductionFactor = maxReductionFactor;
    }
  if (newReductionFactor < 1.0)
    {
    newReductionFactor = 1.0;
    }

  //cerr << "---------------------------------------------------------\n";
  //cerr << "New ReductionFactor: " << newReductionFactor << ", oldFact: " 
  //     << reductionFactor << endl;
  //cerr << "Alloc.Comp.Time: " << renderTime << ", area: " << area 
  //     << ", pixelTime: " << timePerPixel << endl;
  //cerr << "GetBufTime: " << getBuffersTime << ", SetBufTime: " << setBuffersTime
  //     << ", transTime: " << transmitTime << endl;
  
  this->GetComposite()->SetReductionFactor(newReductionFactor);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Render()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int abort;

  this->Update();

  this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
  //this->RenderWindow->SetDesiredUpdateRate(20.0);

  // Some aborts require us to que another render.
  abort = this->ShouldIAbort();
  if (abort)
    {
    if (abort == 1)
      {
      this->EventuallyRender();
      }
    return;
    }

  this->StartRender();
  this->RenderWindow->Render();

  this->InteractiveRenderTime = this->Composite->GetMaxRenderTime();
  this->InteractiveCompositeTime = this->Composite->GetCompositeTime()
                                     + this->Composite->GetGetBuffersTime()
                                     + this->Composite->GetSetBuffersTime();
  pvApp->AddLogEntry("InteractiveRender", this->InteractiveRenderTime);
  pvApp->AddLogEntry("InteractiveCcomposite", this->InteractiveCompositeTime);

}

//----------------------------------------------------------------------------
void vtkPVRenderView::EventuallyRender()
{
  if (this->EventuallyRenderFlag)
    {
    return;
    }
  this->EventuallyRenderFlag = 1;

  // Keep track of whether there is a render pending so that if a render is
  // pending when this object is deleted, we can cancel the "after" command.
  // We don't want to have this object register itself because this can
  // cause leaks if we exit before EventuallyRenderCallBack is called.
  this->Script("after idle {%s EventuallyRenderCallBack}",this->GetTclName());
  this->SetRenderPending(this->Application->GetMainInterp()->result);
}
                      
//----------------------------------------------------------------------------
void vtkPVRenderView::EventuallyRenderCallBack()
{
  int abort;
  vtkPVApplication *pvApp = this->GetPVApplication();

  // sanity check
  if (this->EventuallyRenderFlag == 0 || !this->RenderPending)
    {
    vtkErrorMacro("Inconsistent EventuallyRenderFlag");
    return;
    }
  this->EventuallyRenderFlag = 0;
  this->RenderWindow->SetDesiredUpdateRate(0.000001);
  //this->SetRenderModeToStill();

  // I do not know if these are necessary here.
  abort = this->ShouldIAbort();
  if (abort)
    {
    if (abort == 1)
      {
      this->EventuallyRender();
      }
    return;
    }

  this->ResetCameraClippingRange();
  this->StartRender();
  this->RenderWindow->Render();

  this->StillRenderTime = this->Composite->GetMaxRenderTime();
  this->StillCompositeTime = this->Composite->GetCompositeTime()
                               + this->Composite->GetGetBuffersTime()
                               + this->Composite->GetSetBuffersTime();
  pvApp->AddLogEntry("StillRender", this->StillRenderTime);
  pvApp->AddLogEntry("StillCcomposite", this->StillCompositeTime);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetInteractor(vtkKWInteractor *interactor)
{
  vtkKWInteractor *old = this->CurrentInteractor;

  if (old == interactor)
    {
    return;
    }
  if (interactor)
    {
    interactor->Register(this);
    }
  this->CurrentInteractor = interactor;


  // Now let the interactors do their thing.
  if (old)
    {
    old->Deselect();
    old->UnRegister(this);
    }
  if (interactor)
    {
    interactor->Select();
    }

}

void vtkPVRenderView::SaveInTclScript(ofstream *file)
{
  vtkCamera *camera;
  float position[3];
  float focalPoint[3];
  float viewUp[3];
  float viewAngle;
  float clippingRange[2];

  *file << "vtkRenderer " << this->RendererTclName << "\n"
        << "vtkRenderWindow " << this->RenderWindowTclName << "\n\t"
        << this->RenderWindowTclName << " AddRenderer "
        << this->RendererTclName << "\n"
        << "vtkRenderWindowInteractor iren\n\t"
        << "iren SetRenderWindow " << this->RenderWindowTclName << "\n\n";
  
  camera = this->GetRenderer()->GetActiveCamera();
  camera->GetPosition(position);
  camera->GetFocalPoint(focalPoint);
  camera->GetViewUp(viewUp);
  viewAngle = camera->GetViewAngle();
  camera->GetClippingRange(clippingRange);
  
  *file << "# camera parameters\n"
        << "vtkCamera camera\n\t"
        << "camera SetPosition " << position[0] << " " << position[1] << " "
        << position[2] << "\n\t"
        << "camera SetFocalPoint " << focalPoint[0] << " " << focalPoint[1]
        << " " << focalPoint[2] << "\n\t"
        << "camera SetViewUp " << viewUp[0] << " " << viewUp[1] << " "
        << viewUp[2] << "\n\t"
        << "camera SetViewAngle " << viewAngle << "\n\t"
        << "camera SetClippingRange " << clippingRange[0] << " "
        << clippingRange[1] << "\n"
        << this->RendererTclName << " SetActiveCamera camera\n\n";
}

void vtkPVRenderView::AddActorsToTclScript(ofstream *file)
{
  int i;
  char *result;
  //char tclName[100];
  
  *file << "# assign actors to the renderer\n";
  
  for (i = 0; i < this->GetRenderer()->GetProps()->GetNumberOfItems(); i++)
    {
    this->Script("set tempValue [[%s GetProps] GetItemAsObject %d]",
                 this->RendererTclName, i);
    result = this->Application->GetMainInterp()->result;
    // Notice we are using actors and not LODProp3Ds in the script/
    if (strncmp(result, "vtkTemp", 7) != 0)
      {
      *file << this->RendererTclName << " AddActor ";
      *file << result << "\n";
      }
    }

  int *windowSize = this->GetVTKWindow()->GetSize();
  *file << this->RenderWindowTclName << " SetSize "
        << windowSize[0] << " " << windowSize[1] << "\n";

  // No longer necessary because the above loop does all props. 
  //for (i = 0; i < this->GetRenderer()->GetActors2D()->GetNumberOfItems(); i++)
  //  {
  //  this->Script("set tempValue [[%s GetActors2D] GetItemAsObject %d]",
  //               this->RendererTclName, i);
  //  result = this->Application->GetMainInterp()->result;
  //  sprintf(tclName, result);
  //  if (strncmp(result, "vtkTemp", 7) != 0)
  //    {
  //    this->Script("set vis [%s GetVisibility]", result);
  //    result = this->Application->GetMainInterp()->result;
  //    if (strncmp(result, "1", 1) == 0)
  //      {
  //      *file << this->RendererTclName << " AddActor ";
  //      *file << tclName << "\n";
  //      }
  //    }
  //  }
  
  *file << "\n";
}
