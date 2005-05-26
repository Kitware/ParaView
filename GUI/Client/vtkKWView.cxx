/*=========================================================================

  Module:    vtkKWView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// need all of windows
#define VTK_WINDOWS_FULL

#include "vtkKWView.h"

#include "vtkBMPWriter.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkKWApplication.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkPVCornerAnnotationEditor.h"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWSaveImageDialog.h"
#include "vtkKWSegmentedProgressGauge.h"
#include "vtkKWUserInterfaceManager.h"
#include "vtkKWWindow.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkCallbackCommand.h"
#include "vtkPostScriptWriter.h"
#include "vtkProperty2D.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkTIFFWriter.h"
#include "vtkTextActor.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPVTraceHelper.h"

#if defined(PARAVIEW_USE_WIN32_RW)
#include "vtkWin32OpenGLRenderWindow.h"
#elif defined(PARAVIEW_USE_CARBON_RW)
#include "vtkCarbonRenderWindow.h"
#include "vtkKWMessageDialog.h"
#elif defined(PARAVIEW_USE_COCOA_RW)
#include "vtkCocoaRenderWindow.h"
#include "vtkKWMessageDialog.h"
#else //PARAVIEW_USE_X_RW
#include "vtkXOpenGLRenderWindow.h"
#include "vtkKWMessageDialog.h"
int vtkKWViewFoundMatch;
extern "C"
Bool vtkKWRenderViewPredProc(Display *vtkNotUsed(disp), XEvent *event, 
                             XPointer vtkNotUsed(arg))
{  
  if (event->type == Expose)
    {
    vtkKWViewFoundMatch = 1;
    }
  if (event->type == ConfigureNotify)
    {
    vtkKWViewFoundMatch = 2;
    }
  if (event->type == ButtonPress)
    {
    vtkKWViewFoundMatch = 2;    
    }

  return 0;
}
#endif

vtkStandardNewMacro( vtkKWView );
vtkCxxRevisionMacro(vtkKWView, "1.13");

//----------------------------------------------------------------------------
int vtkKWViewCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
void KWViewAbortCheckMethod( vtkObject*, unsigned long, void* arg, void* )
{
  vtkKWView *me = (vtkKWView *)arg;

  // if we are printing then do not abort
  if (me->GetPrinting())
    {
    return;
    }

  if ( me->ShouldIAbort() == 2 )
    {
    me->GetRenderWindow()->SetAbortRender(1);    
    }
}

//----------------------------------------------------------------------------
vtkKWView::vtkKWView()
{
  this->SupportPrint        = 1;
  this->SupportSaveAsImage  = 1;
  this->SupportCopy         = 1;
  this->SupportControlFrame = 0;
  
  this->Frame = vtkKWWidget::New();
  this->Frame->SetParent(this);
  this->Frame2 = vtkKWWidget::New();
  this->Frame2->SetParent(this->Frame);
  this->ControlFrame = vtkKWWidget::New();
  this->ControlFrame->SetParent(this->Frame);
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this->Frame2);
  this->VTKWidget = vtkKWWidget::New();
  this->VTKWidget->SetParent(this->Frame);
  this->InExpose = 0;
  this->ParentWindow = NULL;
  this->PropertiesParent = NULL;
  this->CommandFunction = vtkKWViewCommand;
  this->SharedPropertiesParent = 0;
  this->Notebook = vtkKWNotebook::New();

  this->UseProgressGauge = 0;
  this->ProgressGauge = vtkKWSegmentedProgressGauge::New();
  this->ProgressGauge->SetParent(this->Frame2);

  this->AnnotationPropertiesFrame = vtkKWFrameWithScrollbar::New();
  this->CornerAnnotation = vtkPVCornerAnnotationEditor::New();
  this->CornerAnnotation->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->CornerAnnotation->GetTraceHelper()->SetReferenceCommand("GetCornerAnnotation");
  
  this->PropertiesCreated = 0;
  this->InteractiveUpdateRate = 5.0;
  this->NumberOfStillUpdates  = 1;
  this->StillUpdateRates = new float[1];
  this->StillUpdateRates[0] = 1.0;
  this->RenderMode = VTK_KW_STILL_RENDER;
  this->RenderState = 1;

  this->GeneralPropertiesFrame = vtkKWFrameWithScrollbar::New();

  this->ColorsFrame = vtkKWFrameLabeled::New();
  this->RendererBackgroundColor = vtkKWChangeColorButton::New();

  this->Printing = 0;
  
  this->MenuEntryName = NULL;
  this->MenuEntryHelp = NULL;
  this->MenuEntryUnderline = -1;
}

//----------------------------------------------------------------------------
vtkKWView::~vtkKWView()
{
  // Remove all binding
  const char *wname = this->VTKWidget->GetWidgetName();
  if (this->IsCreated())
    {
    this->Script("bind %s <Expose> {}",wname);
    this->Script("bind %s <Any-ButtonPress> {}",wname);
    this->Script("bind %s <Any-ButtonRelease> {}",wname);
    this->Script("bind %s <Shift-Any-ButtonPress> {}",wname);
    this->Script("bind %s <Shift-Any-ButtonRelease> {}",wname);
    this->Script("bind %s <Control-Any-ButtonPress> {}", wname);
    this->Script("bind %s <Control-Any-ButtonRelease> {}",wname);
    this->Script("bind %s <B1-Motion> {}",wname);
    this->Script("bind %s <B2-Motion> {}",wname);
    this->Script("bind %s <B3-Motion> {}",wname);
    this->Script("bind %s <Shift-B1-Motion> {}",wname);
    this->Script("bind %s <Shift-B2-Motion> {}",wname);
    this->Script("bind %s <Shift-B3-Motion> {}",wname);
    this->Script("bind %s <Control-B1-Motion> {}",wname);
    this->Script("bind %s <Control-B2-Motion> {}",wname);
    this->Script("bind %s <Control-B3-Motion> {}",wname);
    this->Script("bind %s <KeyPress> {}",wname);
    this->Script("bind %s <Enter> {}",wname);
    }
  this->GeneralPropertiesFrame->Delete();
  this->ColorsFrame->Delete();
  this->RendererBackgroundColor->Delete();

  this->AnnotationPropertiesFrame->Delete();

  if (this->CornerAnnotation)
    {
    this->CornerAnnotation->Delete();
    this->CornerAnnotation = NULL;
    }
  
  this->Notebook->SetParent(NULL);
  this->Notebook->Delete();
  this->VTKWidget->Delete();
  this->Label->Delete();
  this->Frame->Delete();
  this->Frame2->Delete();
  this->ControlFrame->Delete();
  this->SetPropertiesParent(0);

  this->ProgressGauge->Delete();
  
  delete [] this->StillUpdateRates;
  
  this->SetMenuEntryName(NULL);
  this->SetMenuEntryHelp(NULL);  
}

//----------------------------------------------------------------------------
// Return 1 to mean abort but keep trying, 2 to mean hard abort
int vtkKWView::ShouldIAbort()
{
  int flag = 0;
  
#ifdef PARAVIEW_USE_WIN32_RW
  MSG msg;

  // Check all four - can't get the range right in one call without
  // including events we don't want

  if (PeekMessage(&msg,NULL,WM_LBUTTONDOWN,WM_LBUTTONDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_NCLBUTTONDOWN,WM_NCLBUTTONDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_MBUTTONDOWN,WM_MBUTTONDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_RBUTTONDOWN,WM_RBUTTONDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_WINDOWPOSCHANGING,WM_WINDOWPOSCHANGING,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_WINDOWPOSCHANGED,WM_WINDOWPOSCHANGED,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_SIZE,WM_SIZE,PM_NOREMOVE))
    {
    flag = 2;
    }

  if ( !flag )
    {
    // Check some other events to make sure UI isn't being updated
    if (PeekMessage(&msg,NULL,WM_SYNCPAINT,WM_SYNCPAINT,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_NCPAINT,WM_NCPAINT,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_PAINT,WM_PAINT,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_ERASEBKGND,WM_ERASEBKGND,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_ACTIVATE,WM_ACTIVATE,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_NCACTIVATE,WM_NCACTIVATE,PM_NOREMOVE))
      {
      flag = 1;
      }
    }
  
#elif defined(PARAVIEW_USE_X_RW)
  XEvent report;
  
  vtkKWViewFoundMatch = 0;
  Display *dpy = ((vtkXOpenGLRenderWindow*)this->GetRenderWindow())->GetDisplayId();
  XSync(dpy,0);
  XCheckIfEvent(dpy, &report, vtkKWRenderViewPredProc, NULL);
  XSync(dpy,0);
  flag = vtkKWViewFoundMatch;
#endif
//What to do for CARBON or COCOA?


  int flag2 = this->CheckForOtherAbort();
  if ( flag2 > flag )
    {
    flag = flag2;
    }
  
  return flag;
  
}

//----------------------------------------------------------------------------
void vtkKWView::SetStillUpdateRates( int count, float *rates )
{
  if ( count < 1 || count > 5 )
    {
    vtkErrorMacro( << "Number of still updates should be between 1 and 5" );
    return;
    }

  if ( count != this->NumberOfStillUpdates )
    {
    delete [] this->StillUpdateRates;
    this->StillUpdateRates = new float[count];
    this->NumberOfStillUpdates = count;
    }
  
  memcpy( this->StillUpdateRates, rates, count*sizeof(float) );
}

//----------------------------------------------------------------------------
void vtkKWView::Close()
{
  if (this->PropertiesCreated && this->CornerAnnotation)
    {
    this->CornerAnnotation->Close();
    }
}

//----------------------------------------------------------------------------
void vtkKWView::CreateViewProperties()
{
  vtkKWApplication *app = this->GetApplication();

  this->Notebook->SetParent(this->GetPropertiesParent());
  this->Notebook->Create(app,"");

  vtkKWIcon *ico = vtkKWIcon::New();
  ico->SetImage(vtkKWIcon::ICON_GENERAL);
  this->Notebook->AddPage(
    "General", "Set the general properties of the image view", ico);
  ico->SetImage(vtkKWIcon::ICON_ANNOTATE);
  this->Notebook->AddPage(
    "Annotate", "Set the corner annotation", ico);
  ico->Delete();
  
  this->AnnotationPropertiesFrame->SetParent(this->Notebook->GetFrame("Annotate"));
  this->AnnotationPropertiesFrame->Create(app,0);
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->Script("pack %s -pady 2 -fill both -expand yes -anchor n",
               this->AnnotationPropertiesFrame->GetWidgetName());
  this->Notebook->Raise("Annotate");
  
  // create the anno widgets
  this->CornerAnnotation->SetParent(
    this->AnnotationPropertiesFrame->GetFrame());
  this->CornerAnnotation->SetView(this);
  this->CornerAnnotation->GetFrame()->ShowHideFrameOn();
  this->CornerAnnotation->Create(app, "");
  this->CornerAnnotation->GetFrame()->SetLabelText("Corner Annotation");
  this->Script("pack %s -padx 2 -pady 4 -fill x -expand yes -anchor w",
               this->CornerAnnotation->GetWidgetName());

  this->GeneralPropertiesFrame->SetParent(this->Notebook->GetFrame("General"));
  this->GeneralPropertiesFrame->Create(app,0);
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->Script("pack %s -pady 2 -fill both -expand yes -anchor n",
               this->GeneralPropertiesFrame->GetWidgetName());  

  this->ColorsFrame->SetParent(
    this->GeneralPropertiesFrame->GetFrame());
  this->ColorsFrame->ShowHideFrameOn();
  this->ColorsFrame->Create( app,0 );
  this->ColorsFrame->SetLabelText("Colors");
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->ColorsFrame->GetWidgetName());

  double c[3];  c[0] = 0.0;  c[1] = 0.0;  c[2] = 0.0;
  this->RendererBackgroundColor->SetParent( this->ColorsFrame->GetFrame() );
  this->RendererBackgroundColor->SetColor( c );
  this->RendererBackgroundColor->GetLabel()->SetText("Set Background Color");
  this->RendererBackgroundColor->Create( app, "" );
  this->RendererBackgroundColor->SetCommand(
    this, "SetRendererBackgroundColor");
  this->RendererBackgroundColor->SetBalloonHelpString(
    "Set the background color");
  this->RendererBackgroundColor->SetDialogText("Background Color");
  this->Script("pack %s -side top -padx 15 -pady 4 -expand 1 -fill x",
               this->RendererBackgroundColor->GetWidgetName());

  this->PropertiesCreated = 1;
}

//----------------------------------------------------------------------------
void vtkKWView::SetPropertiesParent(vtkKWWidget *args)
{
  if (this->PropertiesParent != args)
    {
    if (this->PropertiesParent != NULL)
      { 
      this->PropertiesParent->UnRegister(this);
      }
    this->PropertiesParent = args;
    if (this->PropertiesParent != NULL)
      { 
      this->PropertiesParent->Register(this); 
      }
    this->Modified();
    this->SharedPropertiesParent = 0;
    }                                                           
}

//----------------------------------------------------------------------------
vtkKWWidget *vtkKWView::GetPropertiesParent()
{
  // if already set then return
  if (this->PropertiesParent)
    {
    return this->PropertiesParent;
    }
  
  // if the window has defined one then use it

  this->PropertiesParent = vtkKWWidget::New();
  this->PropertiesParent->SetParent
    (this->ParentWindow->GetMainPanelFrame());
  this->PropertiesParent->Create(this->GetApplication(),"frame","-bd 0");
  this->SharedPropertiesParent = 1;

  return this->PropertiesParent;
}

//----------------------------------------------------------------------------
// if you are not using window based properties then you are probably 
// using view based properties
void vtkKWView::CreateDefaultPropertiesParent()
{
  if (!this->PropertiesParent)
    {
    this->PropertiesParent = vtkKWWidget::New();
    this->PropertiesParent->SetParent(this);
    this->PropertiesParent->Create(this->GetApplication(),"frame","-bd 0");
    this->Script("pack %s -before %s -fill y -side left -anchor nw",
                 this->PropertiesParent->GetWidgetName(),
                 this->Frame->GetWidgetName());
    }
  else
    {
    vtkDebugMacro("Properties Parent Already Set for view");
    }
}

//----------------------------------------------------------------------------
void vtkKWView::ShowViewProperties()
{
  this->ParentWindow->SetMainPanelVisibility(1);
  
  // make sure we have an applicaiton
  if (!this->GetApplication())
    {
    vtkErrorMacro("attempt to update properties without an application set");
    }

  if (this->MenuEntryName)
    {
    // make sure the variable is set, otherwise set it
    this->ParentWindow->GetViewMenu()->CheckRadioButton(
      this->ParentWindow->GetViewMenu(), "Radio", VTK_KW_VIEW_MENU_INDEX);
    }

  // unpack any current children
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->GetPropertiesParent()->GetWidgetName());
  
  // do we need to create the props ?
  if (!this->PropertiesCreated)
    {
    this->CreateViewProperties();
    }

  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->PackProperties();
}

//----------------------------------------------------------------------------
void vtkKWView::PackProperties()
{
  // make sure the view is packed if necc
  if (this->SharedPropertiesParent)
    {
    // if the windows prop is not currently this views prop
    this->Script("pack slaves %s",
                 this->PropertiesParent->GetParent()->GetWidgetName());
    if (strcmp(this->GetApplication()->GetMainInterp()->result,
               this->PropertiesParent->GetWidgetName()))
      {
      // forget current props
      this->Script("pack forget [pack slaves %s]",
                   this->PropertiesParent->GetParent()->GetWidgetName());  
      this->Script("pack %s -side left -anchor nw -fill y",
                   this->PropertiesParent->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWView::Enter(int /*x*/, int /*y*/)
{
//  this->Script("focus %s",this->VTKWidget->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWView::PrintView()
{
  this->Printing = 1;
  vtkWindow *vtkWin = this->GetVTKWindow();

#ifdef _WIN32  
  int oldrm = this->GetRenderMode();
  this->SetRenderModeToSingle();

  int size[2];
  memcpy(size,vtkWin->GetSize(),sizeof(int)*2);

  PRINTDLG  pd;
  DOCINFO di;
  
  // Initialize a PRINTDLG struct and call PrintDlg to allow user to
  //   specify various printing options...
  //
  memset ((void *) &pd, 0, sizeof(PRINTDLG));
  
  pd.lStructSize = sizeof(PRINTDLG);
  pd.hwndOwner   = (HWND)vtkWin->GetGenericWindowId();
  pd.Flags       = PD_RETURNDC;
  pd.hInstance   = NULL;
  
  PrintDlg(&pd);
  HDC ghdc = pd.hDC;
  
  if (!ghdc)
    {
    return;
    }

  if (pd.hDevMode)
    {
    GlobalFree(pd.hDevMode);
    }
  if (pd.hDevNames)
    {
    GlobalFree(pd.hDevNames);
    }
  
  this->Script("%s configure -cursor watch; update",
               this->ParentWindow->GetWidgetName());  

  di.cbSize      = sizeof(DOCINFO);
  di.lpszDocName = "Kitware Test";
  di.lpszOutput  = NULL;
  
  StartDoc  (ghdc, &di);
  StartPage (ghdc);

  this->Print(ghdc, ghdc);
  
  EndPage   (ghdc);
  EndDoc    (ghdc);
  DeleteDC  (ghdc);

  this->SetRenderMode(oldrm);
  this->Script("%s configure -cursor top_left_arrow",
               this->ParentWindow->GetWidgetName());
#else

  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  double DPI=0;
  if (this->GetParentWindow())
    {
    // Is this right? Should DPI be int or float?
    DPI = this->GetApplication()->GetPrintTargetDPI();
    }
  if (DPI >= 150.0)
    {
    w2i->SetMagnification(2);
    }
  if (DPI >= 300.0)
    {
    w2i->SetMagnification(3);
    }
  w2i->SetInput(vtkWin);
  w2i->Update();
  
  this->Script("tk_getSaveFile -title \"Save Postscript\" -filetypes {{{Postscript} {.ps}}}");
  char* path = 
    strcpy(new char[strlen(this->GetApplication()->GetMainInterp()->result)+1], 
           this->GetApplication()->GetMainInterp()->result);
  if (strlen(path) != 0)
    {
    vtkPostScriptWriter *psw = vtkPostScriptWriter::New();
    psw->SetInput(w2i->GetOutput());
    psw->SetFileName(path);
    psw->Write();
    psw->Delete();

    vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
    dlg->SetMasterWindow(this->ParentWindow);
    dlg->Create(this->GetApplication(),"");
    dlg->SetText(
      "A postscript file has been generated. You will need to\n"
      "print this file using a print command appropriate for\n"
      "your system. Typically this command is lp or lpr. For\n"
      "additional information on printing a postscript file\n"
      "please contact your system administrator.");
    dlg->Invoke();
    }
  w2i->Delete();
#endif
  
  this->Printing = 0;
}

#ifdef _WIN32
void vtkKWView::Print(HDC ghdc, HDC adc)
{
  if (!ghdc || !adc)
    {
    return;
    }
  
  // get size of printer page (in pixels)
  int cxPage = GetDeviceCaps(ghdc,HORZRES);
  int cyPage = GetDeviceCaps(ghdc,VERTRES);
  // get printer pixels per inch
  int cxInch = GetDeviceCaps(ghdc,LOGPIXELSX);
  int cyInch = GetDeviceCaps(ghdc,LOGPIXELSY);

  this->Print(ghdc, adc, cxPage, cyPage, cxInch, cyInch,
              0.0, 0.0, 1.0, 1.0);
}

void vtkKWView::Print(HDC ghdc, HDC,
                      int printerPageSizeX, int printerPageSizeY,
                      int printerDPIX, int printerDPIY,
                      float minX, float minY, float scaleX, float scaleY)
{
  RECT rcDest;  
  vtkWindow *vtkWin = this->GetVTKWindow();  
  int size[2];
  memcpy(size,vtkWin->GetSize(),sizeof(int)*2);

  this->SetupPrint(rcDest, ghdc, printerPageSizeX, printerPageSizeY,
                   printerDPIX, printerDPIY,
                   scaleX, scaleY, size[0], size[1]);
  double scale;
  // target DPI specified here
  if (this->GetParentWindow())
    {
    scale = printerDPIX/this->GetApplication()->GetPrintTargetDPI();
    }
  else
    {
    scale = printerDPIX/100.0;
    }

  this->Render();
  
  SetStretchBltMode(ghdc,HALFTONE);
  StretchBlt(ghdc, rcDest.right*minX, rcDest.top*minY,
             rcDest.right*scaleX, rcDest.top*scaleY,
             (HDC)this->GetMemoryDC(), 0, 0,
             rcDest.right/scale*scaleX,
             rcDest.top/scale*scaleY,
             SRCCOPY);
  
  this->ResumeScreenRendering();
}

void vtkKWView::SetupPrint(RECT &rcDest, HDC ghdc,
                           int printerPageSizeX, int printerPageSizeY,
                           int printerDPIX, int printerDPIY,
                           float scaleX, float scaleY,
                           int screenSizeX, int screenSizeY)
{
  double scale;
  int cxDIB = screenSizeX;         // Size of DIB - x
  int cyDIB = screenSizeY;         // Size of DIB - y
  
  // target DPI specified here
  if (this->GetParentWindow())
    {
    scale = printerDPIX/this->GetApplication()->GetPrintTargetDPI();
    }
  else
    {
    scale = printerDPIX/100.0;
    }
  

  // Best Fit case -- create a rectangle which preserves
  // the DIB's aspect ratio, and fills the page horizontally.
  //
  // The formula in the "->bottom" field below calculates the Y
  // position of the printed bitmap, based on the size of the
  // bitmap, the width of the page, and the relative size of
  // a printed pixel (printerDPIY / printerDPIX).
  //
  rcDest.bottom = rcDest.left = 0;
  if (((float)cyDIB*(float)printerPageSizeX/(float)printerDPIX) > 
      ((float)cxDIB*(float)printerPageSizeY/(float)printerDPIY))
    {
    rcDest.top = printerPageSizeY;
    rcDest.right = (static_cast<float>(printerPageSizeY)*printerDPIX*cxDIB) /
      (static_cast<float>(printerDPIY)*cyDIB);
    }
  else
    {
    rcDest.right = printerPageSizeX;
    rcDest.top = (static_cast<float>(printerPageSizeX)*printerDPIY*cyDIB) /
      (static_cast<float>(printerDPIX)*cxDIB);
    } 
  
  this->SetupMemoryRendering(rcDest.right/scale*scaleX,
                             rcDest.top/scale*scaleY, ghdc);
}
#endif

//----------------------------------------------------------------------------
void vtkKWView::SaveAsImage() 
{
  char *path = 0;
  
  vtkKWWindowBase* window = this->GetWindow();

  // first get the file name
  vtkKWSaveImageDialog *dlg = vtkKWSaveImageDialog::New();
  dlg->SetParent(window);
  dlg->Create(this->GetApplication(),"");  
  int enabled = 0;
  if (window)
    {
    enabled = window->GetEnabled();
    window->SetEnabled(0);
    }
  dlg->Invoke();
  if (window)
    {
    window->SetEnabled(enabled);
    }
  path = dlg->GetFileName();

  // make sure we have a file name
  if (path && strlen(path) > 1)
    {
    this->SaveAsImage(path);
    }
  dlg->Delete();
}

//----------------------------------------------------------------------------
void vtkKWView::SaveAsImage(const char* filename) 
{
  if ( !filename || !*filename )
    {
    vtkErrorMacro("Filename not specified");
    return;
    }
  
  // first get the file name
  vtkWindow *vtkWin = this->GetVTKWindow();
  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(vtkWin);
  w2i->Update();
  
  int success = 1;
  
  if (!strcmp(filename + strlen(filename) - 4,".bmp"))
    {
    vtkBMPWriter *bmp = vtkBMPWriter::New();
    bmp->SetInput(w2i->GetOutput());
    bmp->SetFileName((char *)filename);
    bmp->Write();
    if (bmp->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    bmp->Delete();
    }
  else if (!strcmp(filename + strlen(filename) - 4,".tif"))
    {
    vtkTIFFWriter *tif = vtkTIFFWriter::New();
    tif->SetInput(w2i->GetOutput());
    tif->SetFileName((char *)filename);
    tif->Write();
    if (tif->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    tif->Delete();
    }
  else if (!strcmp(filename + strlen(filename) - 4,".ppm"))
    {
    vtkPNMWriter *pnm = vtkPNMWriter::New();
    pnm->SetInput(w2i->GetOutput());
    pnm->SetFileName((char *)filename);
    pnm->Write();
    if (pnm->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    pnm->Delete();
    }
  else if (!strcmp(filename + strlen(filename) - 4,".png"))
    {
    vtkPNGWriter *png = vtkPNGWriter::New();
    png->SetInput(w2i->GetOutput());
    png->SetFileName((char *)filename);
    png->Write();
    if (png->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    png->Delete();
    }
  else if (!strcmp(filename + strlen(filename) - 4,".jpg"))
    {
    vtkJPEGWriter *jpg = vtkJPEGWriter::New();
    jpg->SetInput(w2i->GetOutput());
    jpg->SetFileName((char *)filename);
    jpg->Write();
    if (jpg->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    jpg->Delete();
    }

  w2i->Delete();
  
  if (!success)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this->ParentWindow, "Write Error",
      "There is insufficient disk space to save this image. The file will be "
      "deleted.");
    }
}

//----------------------------------------------------------------------------
void vtkKWView::EditCopy()
{
  vtkWindow *vtkWin = this->GetVTKWindow();
  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(vtkWin);
  w2i->Update();

#ifdef _WIN32
  // get the pointer to the data
  unsigned char *ptr = 
    (unsigned char *)(w2i->GetOutput()->GetScalarPointer());
  
  LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
  DWORD               dwLen;      // size of memory block
  HANDLE              hDIB = NULL;  // handle to DIB, temp handle
  int *size = this->GetVTKWindow()->GetSize();
  int dataWidth = ((size[0]*3+3)/4)*4;
  int srcWidth = size[0]*3;
  
  if (::OpenClipboard((HWND)this->GetVTKWindow()->GetGenericWindowId()))
    {
    EmptyClipboard();
    
    dwLen = sizeof(BITMAPINFOHEADER) + dataWidth*size[1];
    hDIB = ::GlobalAlloc(GHND, dwLen);
    lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(hDIB);
    
    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth = size[0];
    lpbi->biHeight = size[1];
    lpbi->biPlanes = 1;
    lpbi->biBitCount = 24;
    lpbi->biCompression = BI_RGB;
    lpbi->biClrUsed = 0;
    lpbi->biClrImportant = 0;
    lpbi->biSizeImage = dataWidth*size[1];
    
    // copy the data to the clipboard
    unsigned char *dest = (unsigned char *)lpbi + lpbi->biSize;
    int i,j;
    for (i = 0; i < size[1]; i++)
      {
      for (j = 0; j < size[0]; j++)
        {
        *dest++ = ptr[2];
        *dest++ = ptr[1];
        *dest++ = *ptr;
        ptr += 3;
        }
      dest = dest + (dataWidth - srcWidth);
      }
    
    SetClipboardData (CF_DIB, hDIB);
    ::GlobalUnlock(hDIB);
    CloseClipboard();
    }           
#endif
  w2i->Delete();
}

//----------------------------------------------------------------------------
void vtkKWView::Select(vtkKWWindow *pw)
{
  if (this->MenuEntryName)
    {
    // now add property options
    char *rbv = 
      pw->GetViewMenu()->CreateRadioButtonVariable(
        pw->GetViewMenu(),"Radio");

    pw->GetViewMenu()->AddRadioButton(VTK_KW_VIEW_MENU_INDEX, 
                                      this->MenuEntryName, 
                                      rbv, 
                                      this, 
                                      "ShowViewProperties", 
                                      this->GetMenuEntryUnderline(),
                                      this->MenuEntryHelp ? 
                                      this->MenuEntryHelp :
                                      this->MenuEntryName
      );
    delete [] rbv;
    }

  if ( this->SupportSaveAsImage )
    {
    // add the save as image option
    pw->GetFileMenu()->InsertCommand(this->ParentWindow->GetFileMenuInsertPosition(),
                                     "Save View Image",
                                     this, 
                                     "SaveAsImage", 8,
                                     "Save an image of the current view contents");
    pw->GetFileMenu()->InsertSeparator(this->ParentWindow->GetFileMenuInsertPosition());
    }
  
  if ( this->SupportPrint )
    {
    // add the Print option
    // If there is a "Page Setup" menu, insert below
    int clidx;
    if (pw->GetFileMenu()->HasItem(
          vtkKWWindowBase::GetPrintOptionsMenuLabel()))
      {
      clidx = pw->GetFileMenu()->GetIndex(
        vtkKWWindowBase::GetPrintOptionsMenuLabel()) + 1;  
      }
    else
      {
      clidx = this->ParentWindow->GetFileMenuInsertPosition();  
      }
    pw->GetFileMenu()->InsertCommand(clidx, "Print", this, "PrintView", 0);
    }
  
  if ( this->SupportCopy )
    {
#ifdef _WIN32
  // add the edit copy option
  pw->GetEditMenu()->AddCommand("Copy View Image",this,"EditCopy", "Copy an image of current view contents to the clipboard");
#endif
    }
  
  // change the color of the frame
  this->Script("%s configure -bg #008", this->Label->GetWidgetName());
  this->Script("%s configure -bg #008", this->Frame2->GetWidgetName());
  
  // map the property sheet as needed
  if (this->SharedPropertiesParent && this->MenuEntryName)
    {
    // if the window prop is empty then pack this one
    if (this->ParentWindow->GetViewMenu()->GetRadioButtonValue(
      this->ParentWindow->GetViewMenu(), "Radio") >= VTK_KW_VIEW_MENU_INDEX)
      {
      this->Script("pack %s -side left -anchor nw -fill y",
                   this->PropertiesParent->GetWidgetName());
      }
    }
  this->InvokeEvent(vtkKWEvent::ViewSelectedEvent, 0);
}



//----------------------------------------------------------------------------
void vtkKWView::Deselect(vtkKWWindow *pw)
{
  if (this->MenuEntryName)
    {
    pw->GetViewMenu()->DeleteMenuItem(this->MenuEntryName);
    }
      
  if ( this->SupportPrint )
    {
    pw->GetFileMenu()->DeleteMenuItem("Print");
    }
  
  if ( this->SupportSaveAsImage )
    {
    pw->GetFileMenu()->DeleteMenuItem("Save Image");
    }
  
  if ( this->SupportCopy )
    {
#ifdef _WIN32
  // add the edit copy option
  pw->GetEditMenu()->DeleteMenuItem("Copy");
#endif
    }
  
  // change the color of the frame
  this->Script("%s configure -bg #888", this->Label->GetWidgetName());
  this->Script("%s configure -bg #888", this->Frame2->GetWidgetName());
  
  // forget the properties parent as necc
  if (this->SharedPropertiesParent)
    {
    this->Script("pack forget %s", this->PropertiesParent->GetWidgetName());
    }
}


//----------------------------------------------------------------------------
void vtkKWView::MakeSelected()
{
  if (this->ParentWindow)
    {
    if (this->ParentWindow->GetUserInterfaceManager())
      {
      this->ParentWindow->GetUserInterfaceManager()->Update();
      }
    }
  this->Script("focus %s", this->VTKWidget->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWView::SetupBindings()
{
  const char *wname = this->VTKWidget->GetWidgetName();
  const char *tname = this->GetTclName();

  // setup some default bindings
  this->Script("bind %s <Expose> {%s Exposed}",wname,tname);
  
  this->Script("bind %s <Any-ButtonPress> {%s AButtonPress %%b %%x %%y}",
               wname, tname);

  this->Script("bind %s <Any-ButtonRelease> {%s AButtonRelease %%b %%x %%y}",
               wname, tname);

  this->Script(
    "bind %s <Shift-Any-ButtonPress> {%s AShiftButtonPress %%b %%x %%y}",
    wname, tname);

  this->Script(
    "bind %s <Shift-Any-ButtonRelease> {%s AShiftButtonRelease %%b %%x %%y}",
    wname, tname);

  this->Script(
    "bind %s <Control-Any-ButtonPress> {%s AControlButtonPress %%b %%x %%y}",
    wname, tname);

  this->Script(
    "bind %s <Control-Any-ButtonRelease> {%s AControlButtonRelease %%b %%x %%y}",
    wname, tname);

  this->Script("bind %s <B1-Motion> {%s Button1Motion %%x %%y}",
               wname, tname);

  this->Script("bind %s <B2-Motion> {%s Button2Motion %%x %%y}", 
               wname, tname);
  
  this->Script("bind %s <B3-Motion> {%s Button3Motion %%x %%y}", 
               wname, tname);

  this->Script("bind %s <Shift-B1-Motion> {%s ShiftButton1Motion %%x %%y}", 
               wname, tname);

  this->Script("bind %s <Shift-B2-Motion> {%s ShiftButton2Motion %%x %%y}", 
               wname, tname);
  
  this->Script("bind %s <Shift-B3-Motion> {%s ShiftButton3Motion %%x %%y}", 
               wname, tname);

  this->Script("bind %s <Control-B1-Motion> {%s ControlButton1Motion %%x %%y}",
               wname, tname);

  this->Script("bind %s <Control-B2-Motion> {%s ControlButton2Motion %%x %%y}",
               wname, tname);
  
  this->Script("bind %s <Control-B3-Motion> {%s ControlButton3Motion %%x %%y}",
               wname, tname);

  this->Script("bind %s <KeyPress> {%s AKeyPress %%A %%x %%y}", 
               wname, tname);
  
  this->Script("bind %s <Enter> {%s Enter %%x %%y}", wname, tname);
}


//----------------------------------------------------------------------------
void vtkKWView::UnRegister(vtkObjectBase *o)
{
  // Have reference to this object
  // this->Frame2 (Parent of)
  // vtkPVWindow::LowerFrame->GetFrame1() (Child of)
  // this->CornerAnnotation (View of)
  // (vtkPVRenderView)this->CameraIcons[] * 6 (RenderView of)

  // Delete the children if we are about to be deleted
  // the last extra '1' is for the CornerAnnotation ref

  int nb_children = this->GetNumberOfChildren();
  if (nb_children && 
      this->ReferenceCount == nb_children + 1 + 1 &&
      !this->HasChild((vtkKWWidget*)(o)))
    {
    this->RemoveAllChildren();
    this->CornerAnnotation->SetView(NULL);
    }
  
  this->Superclass::UnRegister(o);
}

//----------------------------------------------------------------------------
void vtkKWView::SetParentWindow(vtkKWWindow *_arg)
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting ParentWindow to " << _arg ); 
  if (this->ParentWindow != _arg) 
    { 
    if (this->ParentWindow != NULL) { this->ParentWindow->UnRegister(this); }
    this->ParentWindow = _arg; 
    if (this->ParentWindow != NULL) { this->ParentWindow->Register(this); } 
    this->Modified(); 
    } 
} 

//----------------------------------------------------------------------------
void vtkKWView::SetTitle(const char *title)
{
  this->Label->SetTextOption(title);
  //  this->Script("update idletasks");
}

//----------------------------------------------------------------------------
void vtkKWView::InteractOn()
{
  this->SetRenderModeToInteractive();
}

//----------------------------------------------------------------------------
void vtkKWView::InteractOff()
{
  this->SetRenderModeToStill();
  this->Render();
}

//----------------------------------------------------------------------------
void vtkKWView::SetupMemoryRendering(
#ifdef PARAVIEW_USE_WIN32_RW
  int x, int y, void *cd
#else
  int, int, void*
#endif
  ) 
{
#ifdef PARAVIEW_USE_WIN32_RW
  if (!cd)
    {
    cd = this->GetRenderWindow()->GetGenericContext();
    }
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->GetRenderWindow())->SetupMemoryRendering(x,y,(HDC)cd);
#endif
}

//----------------------------------------------------------------------------
void vtkKWView::ResumeScreenRendering() 
{
#ifdef PARAVIEW_USE_WIN32_RW
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->GetRenderWindow())->ResumeScreenRendering();
#endif
}

//----------------------------------------------------------------------------
void *vtkKWView::GetMemoryDC()
{
#ifdef PARAVIEW_USE_WIN32_RW
  return (void *)vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->GetRenderWindow())->GetMemoryDC();
#else
  return NULL;
#endif
}

//----------------------------------------------------------------------------
unsigned char *vtkKWView::GetMemoryData()
{
#ifdef PARAVIEW_USE_WIN32_RW
  return vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->GetRenderWindow())->GetMemoryData();
#else
  return NULL;
#endif
}

//----------------------------------------------------------------------------
void vtkKWView::SetRendererBackgroundColor( double r, double g, double b )
{
  if ( r < 0 || g < 0 || b < 0 )
    {
    return;
    }
  double *ff = this->GetRenderer()->GetBackground( );
  if ( ff[0] == r && ff[1] == g && ff[2] == b )
    {
    return;
    }

  this->RendererBackgroundColor->SetColor( r, g, b );
  this->GetRenderer()->SetBackground( r, g, b );
  this->Render();
  float color[3];
  color[0] = r;
  color[1] = g;
  color[2] = b;
  this->InvokeEvent( vtkKWEvent::BackgroundColorChangedEvent, color );
}

//----------------------------------------------------------------------------
void vtkKWView::GetRendererBackgroundColor(double *r, double *g, double *b)
{
  this->GetRenderer()->GetBackground(*r, *g, *b);
}


//----------------------------------------------------------------------------
void vtkKWView::SetCornerTextColor( double rgb[3] )
{
  if ( rgb[0] < 0 || rgb[1] < 0 || rgb[2] < 0 )
    {
    return;
    }
  this->CornerAnnotation->SetTextColor( rgb );
  this->InvokeEvent(vtkKWEvent::ViewAnnotationChangedEvent, 0);
}

//----------------------------------------------------------------------------
double *vtkKWView::GetCornerTextColor()
{
  return this->CornerAnnotation->GetTextColor();
}

//----------------------------------------------------------------------------
vtkWindow *vtkKWView::GetVTKWindow() 
{ 
  return this->GetRenderWindow(); 
}

//----------------------------------------------------------------------------
vtkViewport *vtkKWView::GetViewport() 
{ 
  return this->GetRenderer(); 
}

//----------------------------------------------------------------------------
void vtkKWView::Render() 
{
  this->GetVTKWindow()->Render();
}

//----------------------------------------------------------------------------
void vtkKWView::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->CornerAnnotation);
  this->PropagateEnableState(this->Notebook);
  this->PropagateEnableState(this->PropertiesParent);
  this->PropagateEnableState(this->VTKWidget);
  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->ProgressGauge);
  this->PropagateEnableState(this->Frame);
  this->PropagateEnableState(this->Frame2);
  this->PropagateEnableState(this->ControlFrame);
  this->PropagateEnableState(this->AnnotationPropertiesFrame);
  this->PropagateEnableState(this->GeneralPropertiesFrame);
  this->PropagateEnableState(this->ColorsFrame);
  this->PropagateEnableState(this->RendererBackgroundColor);
}

//----------------------------------------------------------------------------
void vtkKWView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ControlFrame: " << this->GetControlFrame() << endl;
  os << indent << "CornerAnnotation: " << this->GetCornerAnnotation() << endl;
  os << indent << "InExpose: " << this->GetInExpose() << endl;
  os << indent << "InteractiveUpdateRate: " 
     << this->GetInteractiveUpdateRate() << endl;
  os << indent << "MenuEntryHelp: " << this->GetMenuEntryHelp() 
     << endl;
  os << indent << "MenuEntryName: " << this->GetMenuEntryName() 
     << endl;
  os << indent << "MenuEntryUnderline: " 
     << this->GetMenuEntryUnderline() << endl;
  os << indent << "Notebook: " << this->GetNotebook() << endl;
  os << indent << "NumberOfStillUpdates: " << this->GetNumberOfStillUpdates()
     << endl;
  os << indent << "ParentWindow: " << this->GetParentWindow() << endl;
  os << indent << "Printing: " << this->GetPrinting() << endl;
  os << indent << "ProgressGauge: " << this->ProgressGauge << endl;
  os << indent << "RenderMode: " << this->GetRenderMode() << endl;
  os << indent << "RenderState: " << this->GetRenderState() << endl;
  //os << indent << "RenderWindow: " << this->GetRenderWindow() << endl;
  //os << indent << "Renderer: " << this->GetRenderer() << endl;
  os << indent << "SupportControlFrame: " << this->GetSupportControlFrame() 
     << endl;
  os << indent << "SupportPrint: " << this->GetSupportPrint() << endl;
  os << indent << "SupportSaveAsImage: " << this->GetSupportSaveAsImage() 
     << endl;
  os << indent << "SupportCopy: " << this->GetSupportCopy() << endl;
  os << indent << "UseProgressGauge: " << this->UseProgressGauge << endl;
}

