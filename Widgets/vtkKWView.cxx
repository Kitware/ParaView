/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWView.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWWindow.h"
#include "vtkWindow.h"
#include "vtkKWSaveImageDialog.h"
#include "vtkBMPWriter.h"
#include "vtkPNMWriter.h"
#include "vtkTIFFWriter.h"
#include "vtkWindowToImageFilter.h"
#include "vtkViewport.h"
#include "vtkKWGenericComposite.h"
#include "vtkActor2D.h"
#include "vtkPostScriptWriter.h"
#include "vtkKWMessageDialog.h"

int vtkKWViewCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

vtkKWView::vtkKWView()
{
  this->Frame = vtkKWWidget::New();
  this->Frame->SetParent(this);
  this->Frame2 = vtkKWWidget::New();
  this->Frame2->SetParent(this->Frame);
  this->Label = vtkKWWidget::New();
  this->Label->SetParent(this->Frame2);
  this->VTKWidget = vtkKWWidget::New();
  this->VTKWidget->SetParent(this->Frame);
  this->InExpose = 0;
  this->ParentWindow = NULL;
  this->PropertiesParent = NULL;
  this->CommandFunction = vtkKWViewCommand;
  this->Composites = vtkKWCompositeCollection::New();
  this->SelectedComposite = NULL;
  this->SharedPropertiesParent = 0;
  this->Notebook = vtkKWNotebook::New();

  this->AnnotationProperties = vtkKWWidget::New();
  this->HeaderLabel = vtkKWWidget::New();
  this->HeaderLabel->SetParent(this->AnnotationProperties);
  this->HeaderEntry = vtkKWEntry::New();
  this->HeaderEntry->SetParent(this->AnnotationProperties);
  this->HeaderMapper = vtkTextMapper::New();
  this->HeaderMapper->SetJustificationToCentered();
  this->HeaderMapper->SetVerticalJustificationToTop();
  this->HeaderMapper->SetFontSize(15);  
  this->HeaderMapper->ShadowOn();  
  this->HeaderProp = vtkScaledTextActor::New();
  this->HeaderProp->GetPositionCoordinate()->SetValue(0.2,0.88);
  this->HeaderProp->SetMapper(this->HeaderMapper);
  this->HeaderComposite = vtkKWGenericComposite::New();
  this->HeaderComposite->SetProp(this->HeaderProp);
  this->HeaderButton = vtkKWCheckButton::New();
  this->HeaderButton->SetParent(this->AnnotationProperties);

  this->CornerLabel = vtkKWWidget::New();
  this->CornerLabel->SetParent(this->AnnotationProperties);
  this->CornerText = vtkKWText::New();
  this->CornerText->SetParent(this->AnnotationProperties);
  this->CornerMapper = vtkTextMapper::New();
  this->CornerMapper->SetFontSize(15);  
  this->CornerMapper->ShadowOn();  
  this->CornerProp = vtkScaledTextActor::New();
  this->CornerProp->SetMaximumLineHeight(0.25);
  this->CornerProp->SetWidth(0.4);
  this->CornerProp->SetHeight(0.3);
  this->CornerProp->SetMapper(this->CornerMapper);
  this->CornerProp->GetPositionCoordinate()->SetValue(0.02,0.02);
  this->CornerMapper->SetJustificationToLeft();
  this->CornerComposite = vtkKWGenericComposite::New();
  this->CornerComposite->SetProp(this->CornerProp);
  this->CornerButton = vtkKWCheckButton::New();
  this->CornerButton->SetParent(this->AnnotationProperties);
  this->CornerOptions = vtkKWOptionMenu::New();
  this->CornerOptions->SetParent(this->AnnotationProperties);
  this->PropertiesCreated = 0;
  this->InteractiveUpdateRate = 5.0;
  this->NumberOfStillUpdates  = 1;
  this->StillUpdateRates = new float[1];
  this->StillUpdateRates[0] = 1.0;
  this->RenderMode = VTK_KW_STILL_RENDER;
  this->MultiPassStillAbortCheckMethod = NULL;
  this->MultiPassStillAbortCheckMethodArg = NULL;

  this->Printing = 0;
  this->PrintTargetDPI = 100;
}

vtkKWView::~vtkKWView()
{
  this->AnnotationProperties->Delete();
  this->HeaderComposite->Delete();
  this->HeaderProp->Delete();
  this->HeaderMapper->Delete();
  this->HeaderButton->Delete();
  this->HeaderLabel->Delete();
  this->HeaderEntry->Delete();

  this->CornerComposite->Delete();
  this->CornerProp->Delete();
  this->CornerMapper->Delete();
  this->CornerButton->Delete();
  this->CornerLabel->Delete();
  this->CornerText->Delete();
  this->CornerOptions->Delete();
  
  this->Notebook->SetParent(NULL);
  this->Notebook->Delete();
  this->VTKWidget->Delete();
  this->Composites->Delete();
  this->Label->Delete();
  this->Frame->Delete();
  this->Frame2->Delete();
  if (this->PropertiesParent)
    {
    this->PropertiesParent->Delete();
    }

  delete [] this->StillUpdateRates;
}

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

// Specify a function to be called to check and see if an abort
// of the multi-pass still rendering in progress is desired
void vtkKWView::SetMultiPassStillAbortCheckMethod(int (*f)(void *), void *arg)
{
  if ( f != this->MultiPassStillAbortCheckMethod || 
       arg != this->MultiPassStillAbortCheckMethodArg )
    {
    this->MultiPassStillAbortCheckMethod = f;
    this->MultiPassStillAbortCheckMethodArg = arg;
    }
}


void vtkKWView::Close()
{
  vtkKWComposite *c;
       
  if (this->PropertiesCreated)
    {
    if (this->HeaderButton->GetState())
      {
      this->RemoveComposite(this->HeaderComposite);
      }
    if (this->CornerButton->GetState())
      {
      this->RemoveComposite(this->CornerComposite);
      }
    }
  
  // first unselect any composites
  this->SetSelectedComposite(NULL);
  
  this->Composites->InitTraversal();
  while(c = this->Composites->GetNextKWComposite())
    {
    c->Close();
    c->SetView(NULL);
    this->GetViewport()->RemoveProp(c->GetProp());
    }
  this->Composites->RemoveAllItems();
}

vtkKWWidget *vtkKWView::GetPropertiesParent()
{
  // if already set then return
  if (this->PropertiesParent)
    {
    return this->PropertiesParent;
    }
  
  // if the window has defined one then use it
  if (this->ParentWindow && 
      this->ParentWindow->GetPropertiesParent())
    {
    // if the views props have not been defined the define them now
    this->PropertiesParent = vtkKWWidget::New();
    this->PropertiesParent->SetParent
      (this->ParentWindow->GetPropertiesParent());
    this->PropertiesParent->Create(this->Application,"frame","-bd 0");
    this->SharedPropertiesParent = 1;
    }
  return this->PropertiesParent;
}

// if you are not using window based properties then you are probably 
// using view based properties
void vtkKWView::CreateDefaultPropertiesParent()
{
  if (!this->PropertiesParent)
    {
    this->PropertiesParent = vtkKWWidget::New();
    this->PropertiesParent->SetParent(this);
    this->PropertiesParent->Create(this->Application,"frame","-bd 0");
    vtkKWObject::Script(this->Application,
			"pack %s -before %s -fill y -side left -anchor nw",
			this->PropertiesParent->GetWidgetName(),
			this->Frame->GetWidgetName());
    }
  else
    {
    vtkDebugMacro("Properties Parent Already Set for view");
    }
}


void vtkKWView::CreateViewProperties()
{
  vtkKWApplication *app = this->Application;

  this->Notebook->SetParent(this->GetPropertiesParent());
  this->Notebook->Create(this->Application,"");
  this->Notebook->AddPage("Anno");
  
  this->AnnotationProperties->SetParent
    (this->Notebook->GetFrame("Anno"));
  this->AnnotationProperties->Create(app,"frame","");
  vtkKWObject::Script(this->Application,"pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
    this->Notebook->GetWidgetName());
  vtkKWObject::Script(this->Application,"pack %s -pady 2 -fill both -expand yes -anchor n",
    this->AnnotationProperties->GetWidgetName());
  this->Notebook->Raise("Anno");

  // create the anno widgets
  this->HeaderButton->Create(this->Application,
                           "-text {Display Header Annotation}");
  this->HeaderButton->SetCommand("{%s OnDisplayHeader}",
                                 this->GetTclName());
  vtkKWObject::Script(this->Application,"pack %s -padx 2 -pady 4 -anchor nw",
                      this->HeaderButton->GetWidgetName());
  this->HeaderLabel->Create(app,"label","-text Header");
  this->HeaderEntry->Create(app,"-width 28");
  vtkKWObject::Script(app,
		      "bind %s <Return> {%s HeaderChanged}",
		      this->HeaderEntry->GetWidgetName(), 
                      this->GetTclName());
  vtkKWObject::Script(app,"pack %s -side top -anchor w -padx 4 -expand yes",
    this->HeaderLabel->GetWidgetName());
  vtkKWObject::Script(app,"pack %s -side top -anchor w -padx 4 -expand yes -fill x",
    this->HeaderEntry->GetWidgetName());

  this->CornerButton->Create(this->Application,
                           "-text {Display Corner Annotation}");
  this->CornerButton->SetCommand("{%s OnDisplayCorner}",
                                 this->GetTclName());
  vtkKWObject::Script(this->Application,"pack %s -padx 2 -pady 4 -anchor nw",
                      this->CornerButton->GetWidgetName());
  this->CornerLabel->Create(app,"label","-text {Corner Text}");
  this->CornerText->Create(app,"-height 4 -width 28");
  vtkKWObject::Script(app,"bind %s <Return> {%s CornerChanged}",
                      this->CornerText->GetWidgetName(), 
                      this->GetTclName());
  vtkKWObject::Script(app,"pack %s -side top -anchor w -padx 4 -expand yes",
    this->CornerLabel->GetWidgetName());
  vtkKWObject::Script(app,"pack %s -side top -anchor w -padx 4 -expand yes -fill x",
    this->CornerText->GetWidgetName());

  this->CornerOptions->Create(this->Application,"");
  this->CornerOptions->AddEntryWithCommand("Lower Left",this->GetTclName(),
					  "CornerSelected 0");
  this->CornerOptions->AddEntryWithCommand("Upper Left",this->GetTclName(),
					  "CornerSelected 1");
  this->CornerOptions->AddEntryWithCommand("Lower Right",this->GetTclName(),
					  "CornerSelected 2");
  this->CornerOptions->AddEntryWithCommand("Upper Right",this->GetTclName(),
					  "CornerSelected 3");
  this->CornerOptions->SetCurrentEntry("Lower Left");
  vtkKWObject::Script(app,"pack %s -side top -anchor w -padx 4 -expand yes",
    this->CornerOptions->GetWidgetName());
}

void vtkKWView::ShowViewProperties()
{
  // make sure we have an applicaiton
  if (!this->Application)
    {
    vtkErrorMacro("attempt to update properties without an application set");
    }
  
  // unpack any current children
  vtkKWObject::Script(this->Application,
    "catch {eval pack forget [pack slaves %s]}",
    this->GetPropertiesParent()->GetWidgetName());

  // do we need to create the props ?
  if (!this->PropertiesCreated)
    {
    this->CreateViewProperties();
    this->PropertiesCreated = 1;
    }
  vtkKWObject::Script(this->Application,"pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
    this->Notebook->GetWidgetName());
}

void vtkKWView::SetSelectedComposite(vtkKWComposite *_arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting SelectedComposite to " << _arg ); 
  if (this->SelectedComposite != _arg) 
    { 
    if (this->SelectedComposite != NULL) 
      { 
      if (this->ParentWindow->GetSelectedView() == this)
        {
        this->SelectedComposite->Deselect(this);
        }
      this->SelectedComposite->UnRegister(this); 
      }
    this->SelectedComposite = _arg; 
    if (this->SelectedComposite != NULL) 
      { 
      this->SelectedComposite->Register(this); 
      if (this->ParentWindow->GetSelectedView() == this)
        {
        this->SelectedComposite->Select(this);
        } 
      } 
    this->Modified(); 
    } 
}

void vtkKWView::AddComposite(vtkKWComposite *c)
{
  c->SetView(this);
  this->Composites->AddItem(c);
  this->GetViewport()->AddProp(c->GetProp());
}
void vtkKWView::RemoveComposite(vtkKWComposite *c)
{
  c->SetView(NULL);
  this->GetViewport()->RemoveProp(c->GetProp());
  this->Composites->RemoveItem(c);
}

void vtkKWView::Enter(int x, int y)
{
  vtkKWObject::Script(this->Application,
		      "focus %s",this->VTKWidget->GetWidgetName());
}

void vtkKWView::Print()
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
  
  vtkKWObject::Script(this->Application,
		      "%s configure -cursor watch; update",
                      this->ParentWindow->GetWidgetName());  

  di.cbSize      = sizeof(DOCINFO);
  di.lpszDocName = "Kitware Test";
  di.lpszOutput  = NULL;
  
  StartDoc  (ghdc, &di);
  StartPage (ghdc);

  float scale;
  int cxDIB = size[0];         // Size of DIB - x
  int cyDIB = size[1];         // Size of DIB - y
  RECT rcDest;
  
  // get size of printer page (in pixels)
  int cxPage = GetDeviceCaps(ghdc,HORZRES);
  int cyPage = GetDeviceCaps(ghdc,VERTRES);
  // get printer pixels per inch
  int cxInch = GetDeviceCaps(ghdc,LOGPIXELSX);
  int cyInch = GetDeviceCaps(ghdc,LOGPIXELSY);
  // target DPI specified here
  scale = cxInch/this->PrintTargetDPI;


  // Best Fit case -- create a rectangle which preserves
  // the DIB's aspect ratio, and fills the page horizontally.
  //
  // The formula in the "->bottom" field below calculates the Y
  // position of the printed bitmap, based on the size of the
  // bitmap, the width of the page, and the relative size of
  // a printed pixel (cyInch / cxInch).
  //
  rcDest.bottom = rcDest.left = 0;
  if (((float)cyDIB*(float)cxPage/(float)cxInch) > 
      ((float)cxDIB*(float)cyPage/(float)cyInch))
    {
    rcDest.top = cyPage;
    rcDest.right = ((float)(cyPage*cxInch*cxDIB)) /
      ((float)(cyInch*cyDIB));
    }
  else
    {
    rcDest.right = cxPage;
    rcDest.top = ((float)(cxPage*cyInch*cyDIB)) /
      ((float)(cxInch*cxDIB));
    } 
  
  int DPI = vtkWin->GetDPI();
  
  // check to see if we want a scale of one
  if (this->RequireUnityScale())
    {
    this->SetupMemoryRendering(size[0],size[1], ghdc);
    this->Render();
    SetStretchBltMode(ghdc,HALFTONE);
    StretchBlt(ghdc,0,0,
	       rcDest.right, rcDest.top, (HDC)this->GetMemoryDC(),
	       0, 0, size[0], size[1], SRCCOPY);
    }
  else
    {
    this->SetupMemoryRendering(rcDest.right/scale,
			       rcDest.top/scale, ghdc);
    this->Render();
    SetStretchBltMode(ghdc,HALFTONE);
    StretchBlt(ghdc,0,0,
	       rcDest.right, rcDest.top, (HDC)this->GetMemoryDC(),
	       0, 0, rcDest.right/scale, rcDest.top/scale, SRCCOPY);
    }

  
  this->ResumeScreenRendering();
  EndPage   (ghdc);
  EndDoc    (ghdc);
  DeleteDC  (ghdc);

  this->SetRenderMode(oldrm);
  vtkKWObject::Script(this->Application,
                      "%s configure -cursor top_left_arrow",
                      this->ParentWindow->GetWidgetName());
#else

  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(vtkWin);
  w2i->Update();
  
  vtkKWObject::Script(this->Application,
		      "tk_getSaveFile -title \"Save Postscript\" -filetypes {{{Postscript} {.ps}}}");
  char *path = strdup(this->Application->GetMainInterp()->result);
  if (strlen(path) != 0)
    {
    vtkPostScriptWriter *psw = vtkPostScriptWriter::New();
    psw->SetInput(w2i->GetOutput());
    psw->SetFileName(path);
    psw->Write();
    psw->Delete();

    vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
    dlg->Create(this->Application,"");
    dlg->SetText(
      "A postscript file has been generated. You will need to\n"
      "print this file using a print command appropriate for\n"
      "your system. Typically this command is lp or lpr. For\n"
      "additional informaiton on printing a postscript file\n"
      "please contact your system administrator.");
    dlg->Invoke();
    }
  w2i->Delete();
#endif
  
  this->Printing = 0;
}

void vtkKWView::SaveAsImage() 
{
  char *path;
  
  // first get the file name
  vtkKWSaveImageDialog *dlg = vtkKWSaveImageDialog::New();
  dlg->Create(this->Application,"");
  
  vtkWindow *vtkWin = this->GetVTKWindow();
  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(vtkWin);
  w2i->Update();
  
  dlg->Invoke();
  path = dlg->GetFileName();
  
  // make sure we have a file name
  if (!path || strlen(path) < 1)
    {
    dlg->Delete();
    w2i->Delete();
    return;
    }
    
  if (!strcmp(path + strlen(path) - 4,".bmp"))
    {
    vtkBMPWriter *bmp = vtkBMPWriter::New();
    bmp->SetInput(w2i->GetOutput());
    bmp->SetFileName((char *)path);
    bmp->Write();
    bmp->Delete();
    }
  else if (!strcmp(path + strlen(path) - 4,".tif"))
    {
    vtkTIFFWriter *tif = vtkTIFFWriter::New();
    tif->SetInput(w2i->GetOutput());
    tif->SetFileName((char *)path);
    tif->Write();
    tif->Delete();
    }
  else if (!strcmp(path + strlen(path) - 4,".pnm"))
    {
    vtkPNMWriter *pnm = vtkPNMWriter::New();
    pnm->SetInput(w2i->GetOutput());
    pnm->SetFileName((char *)path);
    pnm->Write();
    pnm->Delete();
    }

  w2i->Delete();
  dlg->Delete();
}

void vtkKWView::EditCopy()
{
  vtkWindow *vtkWin = this->GetVTKWindow();
  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(vtkWin);
  w2i->Update();

  // get the pointer to the data
  unsigned char *ptr = 
    (unsigned char *)(w2i->GetOutput()->GetScalarPointer());
  
#ifdef _WIN32
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

void vtkKWView::Select(vtkKWWindow *pw)
{
  // now add property options
  vtkKWObject::Script(this->Application,
		      "%s insert 0 command -label \"View\" -command {%s ShowViewProperties}",
		      this->GetParentWindow()->GetMenuProperties()->GetWidgetName(), 
		      this->GetTclName());

  // add the Print option
  vtkKWObject::Script(this->Application,
		      "%s insert %d command -label {Print} -command {%s Print}",
		      this->ParentWindow->GetMenuFile()->GetWidgetName(), 
                      this->ParentWindow->GetFileMenuIndex(),
		      this->GetTclName());

  // add the save as image option
  vtkKWObject::Script(this->Application,
		      "%s insert %d command -label {Save As Image} -command {%s SaveAsImage}",
		      this->ParentWindow->GetMenuFile()->GetWidgetName(), 
                      this->ParentWindow->GetFileMenuIndex(),
		      this->GetTclName());
  
#ifdef _WIN32
  // add the edit copy option
  vtkKWObject::Script(this->Application,
		      "%s add command -label \"Copy\" -command {%s EditCopy}",
		      this->ParentWindow->GetMenuEdit()->GetWidgetName(), 
		      this->GetTclName());
#endif
  // change the color of the frame
  vtkKWObject::Script(this->Application,"%s configure -bg #008",
		      this->Label->GetWidgetName());
  vtkKWObject::Script(this->Application,"%s configure -bg #008",
		      this->Frame2->GetWidgetName());
  
  // forward to selected composite
  if (this->SelectedComposite)
    {
    this->SelectedComposite->Select(this);
    }
  
  // map the property sheet as needed
  if (this->SharedPropertiesParent)
    {
    vtkKWObject::Script(this->Application,
			"pack %s -side left -anchor nw -fill y",
			this->PropertiesParent->GetWidgetName());
    }
}



void vtkKWView::Deselect(vtkKWWindow *pw)
{
  vtkKWObject::Script(this->Application, "%s delete View",
                      pw->GetMenuProperties()->GetWidgetName());

  vtkKWObject::Script(this->Application, "%s delete Print",
		                  pw->GetMenuFile()->GetWidgetName());
  vtkKWObject::Script(this->Application, "%s delete {Save As Image}",
		                  pw->GetMenuFile()->GetWidgetName());
#ifdef _WIN32
  // add the edit copy option
  vtkKWObject::Script(this->Application, "%s delete Copy",
		                  pw->GetMenuEdit()->GetWidgetName());
#endif
  // change the color of the frame
  vtkKWObject::Script(this->Application,"%s configure -bg #888",
		      this->Label->GetWidgetName());
  vtkKWObject::Script(this->Application,"%s configure -bg #888",
		      this->Frame2->GetWidgetName());
  
  // forward to selected composite
  if (this->SelectedComposite)
    {
    this->SelectedComposite->Deselect(this);
    }

  // forget the properties parent as necc
  if (this->SharedPropertiesParent)
    {
    vtkKWObject::Script(this->Application,
			"pack forget %s",
			this->PropertiesParent->GetWidgetName());
    }
}


void vtkKWView::MakeSelected()
{
  if (this->ParentWindow)
    {
    this->ParentWindow->SetSelectedView(this);
    }
}

void vtkKWView::SetupBindings()
{
  vtkKWApplication *app = this->Application;
  const char *wname = this->VTKWidget->GetWidgetName();
  const char *tname = this->GetTclName();

  // setup some default bindings
  vtkKWObject::Script(app,
    "bind %s <Expose> {%s Exposed}",wname,tname);
  
  vtkKWObject::Script(app,
    "bind %s <Any-ButtonPress> {%s AButtonPress %%b %%x %%y}",
    wname, tname);

  vtkKWObject::Script(app,
    "bind %s <Any-ButtonRelease> {%s AButtonRelease %%b %%x %%y}",
    wname, tname);

  vtkKWObject::Script(app,
    "bind %s <B1-Motion> {%s Button1Motion %%x %%y}",
    wname, tname);

  vtkKWObject::Script(app,
    "bind %s <B2-Motion> {%s Button2Motion %%x %%y}", wname, tname);

  vtkKWObject::Script(app,
    "bind %s <B3-Motion> {%s Button3Motion %%x %%y}", wname, tname);

  vtkKWObject::Script(app,
    "bind %s <Shift-B1-Motion> {%s Button2Motion %%x %%y}", 
		      wname, tname);

  vtkKWObject::Script(app,
    "bind %s <KeyPress> {%s AKeyPress %%A %%x %%y}", wname, tname);

  vtkKWObject::Script(app,
    "bind %s <Enter> {%s Enter %%x %%y}", wname, tname);

  //vtkKWObject::Script(app,
  //  "bind %s <Leave> {%s Leave %%x %%y}", wname, tname);
}


void vtkKWView::UnRegister(vtkObject *o)
{
  if (!this->DeletingChildren)
    {
    // delete the children if we are about to be deleted
    if (this->ReferenceCount == this->Composites->GetNumberOfItems() + 
        this->Children->GetNumberOfItems() + 1)
      {
      if (!(this->Composites->IsItemPresent((vtkKWComposite *)o) ||
            this->Children->IsItemPresent((vtkKWWidget *)o)))
        {
        vtkKWWidget *child;
        vtkKWComposite *c;
        
        this->DeletingChildren = 1;
        this->Children->InitTraversal();
        while(child = this->Children->GetNextKWWidget())
          {
          child->SetParent(NULL);
          }
        this->Composites->InitTraversal();
        while(c = this->Composites->GetNextKWComposite())
          {
          c->SetView(NULL);
          }
        this->DeletingChildren = 0;
        }
      }
    }
  
  this->vtkObject::UnRegister(o);
}

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

void vtkKWView::SetTitle(char *title)
{
  vtkKWObject::Script(this->Application,"%s configure -text {%s}",
		      this->Label->GetWidgetName(), title);
  vtkKWObject::Script(this->Application,"update idletasks");
}

void vtkKWView::OnDisplayHeader() 
{
  if (this->HeaderButton->GetState())
    {
    this->AddComposite(this->HeaderComposite);
    this->HeaderMapper->SetInput(this->HeaderEntry->GetValue());
    this->Render();
    }
  else
    {
    this->RemoveComposite(this->HeaderComposite);
    this->Render();
    }
}

void vtkKWView::HeaderChanged() 
{
  this->HeaderMapper->SetInput(this->HeaderEntry->GetValue());
  if (this->HeaderButton->GetState())
    {
    this->Render();
    }
}


void vtkKWView::OnDisplayCorner() 
{
  if (this->CornerButton->GetState())
    {
    this->AddComposite(this->CornerComposite);
    this->CornerMapper->SetInput(this->CornerText->GetValue());
    this->Render();
    }
  else
    {
    this->RemoveComposite(this->CornerComposite);
    this->Render();
    }
}

void vtkKWView::CornerChanged() 
{
  this->CornerMapper->SetInput(this->CornerText->GetValue());
  if (this->CornerButton->GetState())
    {
    this->Render();
    }
}

void vtkKWView::CornerSelected(int i)
{
  this->CornerMapper->SetInput(this->CornerText->GetValue());
  switch (i)
    {
    case 0: 
      this->CornerProp->GetPositionCoordinate()->SetValue(0.02,0.02);
      this->CornerMapper->SetJustificationToLeft();
      this->CornerMapper->SetVerticalJustificationToBottom();
      break;
    case 1: 
      this->CornerProp->GetPositionCoordinate()->SetValue(0.02,0.68);
      this->CornerMapper->SetJustificationToLeft();
      this->CornerMapper->SetVerticalJustificationToTop();
      break;
    case 2: 
      this->CornerProp->GetPositionCoordinate()->SetValue(0.58,0.02);
      this->CornerMapper->SetJustificationToRight();
      this->CornerMapper->SetVerticalJustificationToBottom();
      break;
    case 3: 
      this->CornerProp->GetPositionCoordinate()->SetValue(0.58,0.68);
      this->CornerMapper->SetJustificationToRight();
      this->CornerMapper->SetVerticalJustificationToTop();
      break;
    }

  this->Render();	  
}

// Description:
// Chaining method to serialize an object and its superclasses.
void vtkKWView::SerializeSelf(ostream& os, vtkIndent indent)
{
  // invoke superclass
  this->vtkKWWidget::SerializeSelf(os,indent);

  // write out the composite
  if (this->PropertiesCreated)
	{
	os << indent << "CornerText ";
	vtkKWSerializer::WriteSafeString(os, this->CornerText->GetValue());
	os << endl;
	os << indent << "CornerButton " << this->CornerButton->GetState() << endl;
	os << indent << "CornerOptions ";
	vtkKWSerializer::WriteSafeString(os, this->CornerOptions->GetValue());
	os << endl;
  
	os << indent << "HeaderEntry ";
	vtkKWSerializer::WriteSafeString(os, this->HeaderEntry->GetValue());
	os << endl;
	os << indent << "HeaderButton " << this->HeaderButton->GetState() << endl;
	}
}

void vtkKWView::SerializeToken(istream& is, const char token[1024])
{
  int i;
  char tmp[1024];
  
  if (!strcmp(token,"HeaderButton"))
    {
    is >> i;
    this->HeaderButton->SetState(i);
    this->OnDisplayHeader();
    return;
    }
  if (!strcmp(token,"HeaderEntry"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->HeaderEntry->SetValue(tmp);
    this->OnDisplayHeader();
    return;
    }
  if (!strcmp(token,"CornerOptions"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->CornerOptions->SetCurrentEntry(tmp);
    if (!strcmp(tmp,"Lower Left"))
      {
      this->CornerSelected(0);
      }
    if (!strcmp(tmp,"Upper Left"))
      {
      this->CornerSelected(1);
      }
    if (!strcmp(tmp,"Lower Right"))
      {
      this->CornerSelected(2);
      }
    if (!strcmp(tmp,"Upper Right"))
      {
      this->CornerSelected(3);
      }
    return;
    }
  if (!strcmp(token,"CornerButton"))
    {
    is >> i;
    this->CornerButton->SetState(i);
    this->OnDisplayCorner();
    return;
    }
  if (!strcmp(token,"CornerText"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->CornerText->SetValue(tmp);
    this->OnDisplayCorner();
    return;
    }
  
  vtkKWWidget::SerializeToken(is,token);
}

