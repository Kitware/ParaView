/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataList.cxx
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
#include "vtkKWEntry.h"
#include "vtkPVDataList.h"

int vtkPVDataListCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVDataList::vtkPVDataList()
{
  this->CommandFunction = vtkPVDataListCommand;

  this->ScrollFrame = vtkKWWidget::New();
  this->ScrollFrame->SetParent(this);
  this->Canvas = vtkKWWidget::New();
  this->Canvas->SetParent(this->ScrollFrame);
  this->ScrollBar = vtkKWWidget::New();
  this->ScrollBar->SetParent(this->ScrollFrame);

  this->CompositeCollection = vtkCollection::New();

  this->NameEntryCollection = NULL;
  this->NameEntryTag = NULL;
  this->NameEntry = vtkKWEntry::New();
  this->NameEntry->SetParent(this->Canvas);
}

//----------------------------------------------------------------------------
vtkPVDataList::~vtkPVDataList()
{
  this->ScrollFrame->Delete();
  this->ScrollFrame = NULL;
  this->Canvas->Delete();
  this->Canvas = NULL;
  this->ScrollBar->Delete();
  this->ScrollBar = NULL;

  this->CompositeCollection->Delete();
  this->CompositeCollection = NULL;

  this->SetNameEntryComposite(NULL);
  this->SetNameEntryTag(NULL);
  this->NameEntry->Delete();
  this->NameEntry = NULL;
}


//----------------------------------------------------------------------------
void vtkPVDataList::Create(vtkKWApplication *app, char *args)
{
  char str[1024], str2[1024];

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);
  
  // create the main frame for this widget
  this->Script( "frame %s", this->GetWidgetName());


  this->ScrollFrame->Create(app,"frame", 
                "-relief raised -bd 1");
  this->Script( "pack %s -side top -fill both -expand yes",
                this->ScrollFrame->GetWidgetName());
  sprintf(str, "-command {%s yview} -bd 0", this->Canvas->GetWidgetName());
  this->ScrollBar->Create(app,"scrollbar",str);
  this->Script("pack %s -side right -fill y -expand no",
   	       this->ScrollBar->GetWidgetName());
  sprintf(str, "-bg white -width 100 -yscrollcommand {%s set} -bd 0",
          this->ScrollBar->GetWidgetName());
  this->Canvas->Create(app,"canvas ", str);
  this->Script( "pack %s -side right -fill both -expand yes",
                this->Canvas->GetWidgetName());
 
  // Set up bindings for the canvas (cut and paste).
  this->Script("bind %s <Enter> {focus %s}", this->Canvas->GetWidgetName(), 
               this->Canvas->GetWidgetName());
  this->Script("bind %s <Delete> {%s DeletePickedVerify}", 
               this->Canvas->GetWidgetName(), this->GetTclName());   

  // Bitmaps used to show which parts of the tree can be opened.

  // Unique names ?
  sprintf(str, "{%s \n%s \n%s \n%s}",
          "#define open_width 9\n#define open_height 9",
          "static unsigned char closed_bits[] = {",
          "0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x7d, 0x01, 0x01, 0x01,",
          "0x01, 0x01, 0x01, 0x01, 0xff, 0x01};");
  sprintf(str2, "{%s \n%s \n%s \n%s}",
          "#define solid_width 9\n#define solid_height 9",
          "static unsigned char closed_bits[] = {",
          "0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01,",
          "0xff, 0x01, 0xff, 0x01, 0xff, 0x01};");
  this->Script("image create bitmap openbm -data %s -maskdata %s -foreground black -background white", str, str2);
 
  sprintf(str, "{%s \n%s \n%s \n%s}",
          "#define closed_width 9\n#define closed_height 9",
          "static unsigned char closed_bits[] = {",
          "0xff, 0x01, 0x01, 0x01, 0x11, 0x01, 0x11, 0x01, 0x7d, 0x01, 0x11, 0x01,",
          "0x11, 0x01, 0x01, 0x01, 0xff, 0x01};");
  this->Script("image create bitmap closedbm -data %s -maskdata %s -foreground black -background white", str, str2);
  
  // lets try eyes
  sprintf(str, "{%s \n%s \n%s \n%s}",
          "#define open_eye_width 13\n#define open_eye_height 9",
          "static unsigned char open_eye_bits[] = {",
          "0x48, 0x02, 0xf2, 0x09, 0xec, 0x06, 0x12, 0x09, 0x51, 0x11, 0x12, 0x09,",
          "0xec, 0x06, 0xf0, 0x01, 0x00, 0x00};");
  this->Script("image create bitmap visonbm -data %s -foreground black -background white", str, str2);
  this->Script("image create bitmap vispartbm -data %s -foreground gray80 -background white", str, str2);
  sprintf(str, "{%s \n%s \n%s \n%s}",
          "#define closed_eye_width 13\n#define closed_eye_height 9",
          "static unsigned char open_eye_bits[] = {",
          "0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x02, 0x08",
          "0x0c, 0x06, 0xf2, 0x09, 0x48, 0x02};");
  this->Script("image create bitmap visoffbm -data %s -foreground black -background white", str, str2);
  
  this->NameEntry->Create(app, "");

  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVDataList::EditName(int compIdx)
{
  vtkPVComposite *comp;
  int x1, y1, x2, y2;
  char *result;

  // If we are already editing a name, then break its loop.
  if (this->NameEntryComposite)
    {
    this->NameEntryClose();
    this->Script("after idle {%s EditName %d}",this->GetTclName(), compIdx);
    return;
    }

  comp =(vtkPVComposite*)(this->CompositeCollection->GetItemAsObject(compIdx));
  if (comp == NULL)
    {
    return;
    }

  this->Script("%s bbox current", this->Canvas->GetWidgetName());
  result = this->Application->GetMainInterp()->result;
  sscanf(result, "%d %d %d %d", &x1, &y1, &x2, &y2);

  this->Script("%s create window %d %d -width %d -height %d -window %s -anchor nw", 
               this->Canvas->GetWidgetName(), x1, y1, 100, y2-y1,
               this->NameEntry->GetWidgetName());
  result = this->Application->GetMainInterp()->result;
  this->SetNameEntryTag(result);
  this->SetNameEntryComposite(comp);
  this->NameEntry->SetValue(comp->GetName());
  // should we bind the tag, or the window
  this->Script( "bind %s <KeyPress-Return> {%s NameEntryClose}",
                this->NameEntry->GetWidgetName(), this->GetTclName());

  // Should we give the entry the focus?

  // Wait for the
  while (this->NameEntryComposite)
    {
    this->Script("update");
    } 
}

//----------------------------------------------------------------------------
void vtkPVDataList::NameEntryClose()
{
  if (this->NameEntryComposite == NULL)
    {
    return;
    }
  this->NameEntryComposite->SetName(this->NameEntry->GetValue());
  this->SetNameEntryComposite(NULL); 
  this->Script("%s delete %s", this->Canvas->GetWidgetName(), 
               this->NameEntryTag);
  this->SetNameEntryTag(NULL);
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVDataList::Pick(int compIdx)
{
  vtkPVComposite *comp;

  comp = (vtkPVComposite *)(this->CompositeCollection->GetItemAsObject(compIdx));
  comp->Select();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVDataList::ToggleVisibility(int compIdx, int button)
{
  vtkPVComposite *comp;
  int status;

  comp = (vtkPVComposite *)(this->CompositeCollection->GetItemAsObject(compIdx));
  if (comp)
    {
    // Toggle visibility
    
    if (comp->GetProp()->GetVisibility())
      {
      comp->GetProp()->VisibilityOff();
      }
    else
      {
      comp->GetProp()->VisibilityOff();
      }
    comp->GetView()->Render();
    }

  this->Update();
}


//----------------------------------------------------------------------------
void vtkPVDataList::EditColor(int compIdx)
{
  vtkPVComponent *comp;
  float rgb[3];
  unsigned char r, g, b;
  char *result, tmp[3];

  comp=(vtkPVComposite *)(this->CompositeCollection->GetItemAsObject(compIdx));
  if (comp)
    {
    comp->GetProp()->GetProperty()->GetColor(rgb);
    r = (unsigned char)(rgb[0] * 255.5);
    g = (unsigned char)(rgb[1] * 255.5);
    b = (unsigned char)(rgb[2] * 255.5);
    this->Script(
      "tk_chooseColor -initialcolor {#%02x%02x%02x} -title {Choose Color}",
      r, g, b);
    result = this->Application->GetMainInterp()->result;
    if (strlen(result) > 6)
      {
      tmp[2] = '\0';
      tmp[0] = result[1];
      tmp[1] = result[2];
      sscanf(tmp, "%x", &r);
      tmp[0] = result[3];
      tmp[1] = result[4];
      sscanf(tmp, "%x", &g);
      tmp[0] = result[5];
      tmp[1] = result[6];
      sscanf(tmp, "%x", &b);
      comp->GetProp()->GetProperty()->SetColor((float)(r)/255.0, (float)(g)/255.0, (float)(b)/255.0);
      this->Update();
      this->RenderView->Render();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDataList::PartsPickedNotify()
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVDataList::Update()
{
  vtkGoAssembly *child, *clipBoard;
  int num, idx;
  int y, in;

  // Get us out of the name entry state if we are in it.
  this->NameEntryClose();

  this->CompositeCollection->RemoveAllItems();
  this->Script("%s delete all",
               this->Canvas->GetWidgetName());

  if (this->RootAssembly == NULL)
    {
    return;
    }

  y = 30;
  in = 10;
  num = this->RootAssembly->GetNumberOfChildren();
  for (idx = 0; idx < num; ++idx)
    {
    child = this->RootAssembly->GetChild(idx);
    y = this->Update(child, y, in, 1);
    }

  // Display the clip board if their is anything in it.
  clipBoard = this->ClipBoard;
  if (clipBoard && clipBoard->GetNumberOfChildren() > 0)
    {
    y = this->Update(clipBoard, y, in, 0);
    }

  this->Script("%s config -scrollregion [%s bbox all]",
               this->Canvas->GetWidgetName(),this->Canvas->GetWidgetName());

}

//----------------------------------------------------------------------------
// We do not want the user to edit the clip board.
int vtkPVDataList::Update(vtkGoAssembly *assy, int y, int in,
                                 int pickable)
{
  vtkGoAssembly *child;
  int num, idx, assyIdx, status; 
  int x, yNext, yLastChild;
  //switch $tcl_platform(platform) {
  // unix windows
  //static char *font = "-adobe-helvetica-medium-r-normal-*-11-80-100-100-p-56-iso8859-1";
  static char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";
  char *result;
  int bbox[4];
  float color[3];
  char *tmp;

  assyIdx = this->CompositeCollection->GetNumberOfItems();
  this->CompositeCollection->AddItem(assy);

  // Draw the small horizontal indent line.
  x = in + 8;
  this->Script("%s create line %d %d %d %d -fill gray50",
               this->Canvas->GetWidgetName(), in, y, x, y);
  yNext = y + 17;
  if (assy->GetPart() == NULL)
    { // The item is not a leaf, so give the option to open or close.
    if (assy->GetEditorOpen())
      {  // Open item: make a button to close it. 
      this->Script("%s create image %d %d -image openbm",
                   this->Canvas->GetWidgetName(), x, y);
      result = this->Application->GetMainInterp()->result;
      tmp = new char[strlen(result)+1];
      strcpy(tmp,result);
      this->Script("%s bind %s <ButtonPress-1> {%s Close %d}",
                   this->Canvas->GetWidgetName(), tmp,
                   this->GetTclName(), assyIdx);
      delete [] tmp;
      tmp = NULL;
      // Draw all of the children.
      num = assy->GetNumberOfChildren();
      for (idx = 0; idx < num; ++idx)
        {
        child = assy->GetChild(idx);
        yLastChild = yNext;
        yNext = this->Update(child, yNext, x, pickable);
        }
      // Draw the vertical line connecting the children.
      if (num > 0)
        {
        this->Script("%s create line %d %d %d %d -fill gray50",
                     this->Canvas->GetWidgetName(),x, y, x, yLastChild);
        // Put this line below the buttons.
        result = this->Application->GetMainInterp()->result;
        this->Script( "%s lower %s",this->Canvas->GetWidgetName(), result);
        }
      } 
    else 
      { // Closed Item: make a button to open it.
      this->Script("%s create image %d %d -image closedbm",
                   this->Canvas->GetWidgetName(), x, y);
      result = this->Application->GetMainInterp()->result;
      tmp = new char[strlen(result)+1];
      strcpy(tmp,result);
      this->Script("%s bind %s <ButtonPress-1> {%s Open %d}",
                   this->Canvas->GetWidgetName(), tmp,
                   this->GetTclName(), assyIdx);
      delete [] tmp;
      tmp = NULL;
      }
    // Leave space for the open closed item.
    x += 4;
    }
  x += 8;

  // Draw the icon indicating visibility.
  result = NULL;
  switch (assy->GetVisibilityStatus())
    {
    case VTK_GO_STATUS_OFF:
      this->Script("%s create image %d %d -image visoffbm",
                   this->Canvas->GetWidgetName(), x, y);
      result = this->Application->GetMainInterp()->result;
      x += 9;
      break;
    case VTK_GO_STATUS_ON:
      this->Script("%s create image %d %d -image visonbm",
                   this->Canvas->GetWidgetName(), x, y);
      result = this->Application->GetMainInterp()->result;
      x += 9;
      break;
    case VTK_GO_STATUS_PARTIAL:
      this->Script("%s create image %d %d -image vispartbm",
                   this->Canvas->GetWidgetName(), x, y);
      result = this->Application->GetMainInterp()->result;
      x += 9;
      break;
    }
  if (result && pickable)
    {
    tmp = new char[strlen(result)+1];
    strcpy(tmp,result);
    this->Script("%s bind %s <ButtonPress-1> {%s ToggleVisibility %d 1}",
                 this->Canvas->GetWidgetName(), tmp,
                 this->GetTclName(), assyIdx);
    this->Script("%s bind %s <ButtonPress-3> {%s ToggleVisibility %d 3}",
                 this->Canvas->GetWidgetName(), tmp,
                 this->GetTclName(), assyIdx);
    delete [] tmp;
    tmp = NULL;
    }

  // Draw the button indicating the color of the assembly.
  status = assy->GetColor(color);
  if (status)
    {
    unsigned char r, g, b;
    r = (unsigned char)(color[0] * 255.0);
    g = (unsigned char)(color[1] * 255.0);
    b = (unsigned char)(color[2] * 255.0);
    this->Script( 
      "%s create rectangle %d %d %d %d -fill {#%02x%02x%02x} -outline {black}",
      this->Canvas->GetWidgetName(), 
      x, y-4, x+8, y+4, r, g, b);
    }
  else
    {
    this->Script( 
      "%s create rectangle %d %d %d %d -fill {gray80} -outline {grey90}",
      this->Canvas->GetWidgetName(), 
      x, y-4, x+8, y+4);
    }
  x += 12;
  // Make it a button that changes the assemblies color.
  if (pickable)
    {
    result = this->Application->GetMainInterp()->result;
    tmp = new char[strlen(result)+1];
    strcpy(tmp,result);
    this->Script("%s bind %s <ButtonPress-1> {%s EditColor %d}",
                 this->Canvas->GetWidgetName(), tmp,
                 this->GetTclName(), assyIdx);
    delete [] tmp;
    tmp = NULL;
    }

  // Draw the name of the assembly. (P) for path indicator.
  if (assy->GetPath())
    {
    this->Script(
      "%s create text %d %d -text {%s (p)} -font %s -anchor w -tags x",
      this->Canvas->GetWidgetName(), x, y, assy->GetName(), font);
    }
  else if (assy->GetKinematicElement() 
            && !assy->GetKinematicElement()->GetSlaveFlag())
    {
    this->Script(
      "%s create text %d %d -text {%s (k)} -font %s -anchor w -tags x",
      this->Canvas->GetWidgetName(), x, y, assy->GetName(), font);
    }
  else
    {
    this->Script(
      "%s create text %d %d -text {%s} -font %s -anchor w -tags x",
      this->Canvas->GetWidgetName(), x, y, assy->GetName(), font);
    }
  // Make the name hot for picking.
  if (pickable)
    {
    result = this->Application->GetMainInterp()->result;
    tmp = new char[strlen(result)+1];
    strcpy(tmp,result);
    this->Script("%s bind %s <ButtonPress-1> {%s Pick %d}",
                 this->Canvas->GetWidgetName(), tmp,
                 this->GetTclName(), assyIdx);
    this->Script("%s bind %s <Shift-1> {%s ShiftPick %d}",
                 this->Canvas->GetWidgetName(), tmp,
                 this->GetTclName(), assyIdx);
    this->Script("%s bind %s <ButtonPress-3> {%s EditName %d}",
                 this->Canvas->GetWidgetName(), tmp,
                 this->GetTclName(), assyIdx);
    // Get the bounding box for the name. We may need to highlight it.
    this->Script( "%s bbox %s",this->Canvas->GetWidgetName(), tmp);
    delete [] tmp;
    tmp = NULL;
    result = this->Application->GetMainInterp()->result;
    sscanf(result, "%d %d %d %d %s %d", bbox, bbox+1, bbox+2, bbox+3);
  
    // Highlight the name based on the picked status. 
    if (assy->GetPickedStatus() != VTK_GO_STATUS_OFF)
      {
      if (assy->GetPickedStatus() == VTK_GO_STATUS_ON)
        {
        tmp = "yellow";
        }
      else
        {
        tmp = "grey90";
        }
      this->Script("%s create rectangle %d %d %d %d -fill %s -outline {}",
                   this->Canvas->GetWidgetName(), 
                   bbox[0], bbox[1], bbox[2], bbox[3], tmp);
      result = this->Application->GetMainInterp()->result;
      tmp = new char[strlen(result)+1];
      strcpy(tmp,result);
      this->Script( "%s lower %s",this->Canvas->GetWidgetName(), tmp);
      delete [] tmp;
      tmp = NULL;
      }
    }

  return yNext;
}

//----------------------------------------------------------------------------
void vtkPVDataList::SetSequenceMode(int val)
{
  this->SequenceCheck->SetState(val);
}

//----------------------------------------------------------------------------
int vtkPVDataList::GetSequenceMode()
{
  return this->SequenceCheck->GetState();
}

//----------------------------------------------------------------------------
void vtkPVDataList::SequenceCheckCallback()
{
  int val = this->GetSequenceMode();

  if (val)
    {
    this->Script("pack %s -side top -expand no",
                 this->SequenceEditor->GetWidgetName());
    this->Update();
    this->SequenceEditor->Update();
    this->RenderView->EventuallyRender();
    }
  else
    {
    this->Script( "pack forget %s",this->SequenceEditor->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
// Popup file dialog, create a part, and ad it to the renderer.
void vtkPVDataList::LoadPart()
{
  char *result, *path = NULL;
  char *tmp, *dirEnd, *name;

  this->Script(
    "tk_getOpenFile -initialdir {%s} -title {Load Part} -filetypes {{{VTK} {.vtk}} {{STL} {.stl}} {{BYU} {.g}}}",
    this->LoadPartDirectory);
  result = this->Application->GetMainInterp()->result;

  if (strlen(result) == 0)
    {
    return;
    }

  path = new char[strlen(result)+1];
  strcpy(path,result);

  // here we should just let the import assembly dialog create the assembly.
  vtkGoPart *part = vtkGoPart::New();
  part->SetFileName(path);
  vtkGoAssembly *assy = vtkGoAssembly::New();
  assy->SetPart(part);
  assy->SetVisibilityStatus(VTK_GO_STATUS_ON);

  // Extract the file name from the directory name.
  tmp = name = path;
  dirEnd = NULL;
  while (*tmp != '.' && *tmp != '\0')
    {
    if (*tmp == '/')
      {
      dirEnd = tmp;
      name = tmp+1;
      }
    ++tmp;
    }
  *tmp = '\0';
  assy->SetName(name);
  if (dirEnd)
    {
    *dirEnd = '\0';
    this->SetLoadPartDirectory(path);
    }

  this->RenderView->AddAssembly(assy);
    
  part->Delete();
  assy->Delete();
  delete [] path;
}


//----------------------------------------------------------------------------
vtkGoPart *vtkPVDataList::CheckFileNames(vtkGoAssembly *assy)
{
  vtkGoPart *part;
  int num, idx;
  vtkGoAssembly *child;

  if (assy == NULL)
    {
    return NULL;
    }
  
  part = assy->GetPart();
  if (part && part->GetFileName() == NULL)
    {
    return part;
    }
  num = assy->GetNumberOfChildren();
  for (idx = 0; idx < num; ++idx)
    {
    child = assy->GetChild(idx);
    part = this->CheckFileNames(child);
    if (part)
      {
      // Should we open the tree to show the part?
      assy->SetEditorOpen(1);
      return part;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVDataList::ExportPart()
{
  char *result, *path, *ext;
  vtkGoAssembly *assy = this->RenderView->GetPickedAssembly();
  vtkGoPart *part;

  if (assy->GetNumberOfChildren() == 0)
    {
    this->RenderView->Warning("No picked part to export");
    return;
    }
  if (assy->GetNumberOfChildren() > 1)
    {
    this->RenderView->Warning("Export only handles a single part.");
    return;
    }
  assy = assy->GetChild(0);
  part = assy->GetPart();
  if (part == NULL)
    {
    this->RenderView->Warning("Export does not handles assemblies.");
    return;
    }


  this->Script(
    "tk_getSaveFile -title {Load Part} -filetypes {{{VTK} {.vtk}} {{STL} {.stl}} {{BYU} {.g}}}");
  result = this->Application->GetMainInterp()->result;

  if (strlen(result) == 0)
    {
    return;
    }

  path = new char[strlen(result)+1];
  strcpy(path,result);

  // extract the extension to determine what kind of file it is.
  ext = path;
  while (*ext != '.')
    {
    if (*ext == '\0')
      {
      this->RenderView->Warning("Could not find extension.");
      return;
      }
    ++ext;
    }
  if (ext != NULL && strcmp(ext, ".vtk") == 0)
    {
    vtkPolyDataWriter *writer;
    writer = vtkPolyDataWriter::New();
    writer->SetFileTypeToBinary();
    writer->SetFileName(path);
    writer->SetInput(part->GetPolyData());
    writer->Write();
    if (part->GetFileName() == NULL)
      {
      part->SetFileName(path);
      }
    delete [] path;
    return;
    }
  if (ext != NULL && strcmp(ext, ".stl") == 0)
    {
    vtkSTLWriter *writer;
    writer = vtkSTLWriter::New();
    writer->SetFileName(path);
    writer->SetInput(part->GetPolyData());
    writer->Write();
    if (part->GetFileName() == NULL)
      {
      part->SetFileName(path);
      }
    delete [] path;
    return;
    }
  if (ext != NULL && strcmp(ext, ".g") == 0)
    {
    vtkBYUWriter *writer;
    writer = vtkBYUWriter::New();
    writer->SetGeometryFileName(path);
    writer->SetInput(part->GetPolyData());
    writer->Write();
    if (part->GetFileName() == NULL)
      {
      part->SetFileName(path);
      }
    delete [] path;
    return;
    }

  this->RenderView->Warning("Unknown extention.");
}



//----------------------------------------------------------------------------
void vtkPVDataList::ImportAssembly()
{
  vtkGoAssembly *assy;

  if (this->ImportAssemblyDialog->Invoke())
    {
    assy = this->ImportAssemblyDialog->GetAssembly();
    this->RenderView->AddAssembly(assy);
    }
}

//----------------------------------------------------------------------------
// Popup file dialog, load assembly and add it to the renderer.
void vtkPVDataList::LoadAssembly()
{
  char *result;
  char *path = NULL;
  vtkGoAssembly *assy, *child;
  int idx;

  this->Script(
    "tk_getOpenFile -title \"Load Assembly\" -filetypes {{{Assembly} {.asy}}}");
  result = this->Application->GetMainInterp()->result;
  path = new char[strlen(result)+1];
  strcpy(path,result);

  if (strlen(path) == 0)
    {
    delete [] path;
    return;
    }
  ifstream inFile(path, ios::in);
  char c1, c2;
  char str[20];

  if ( !inFile)
    {
    this->RenderView->Warning("Could not open file %s", path);
    delete [] path;
    return;
    }

  // Read the magic number 
  inFile >> c1;
  inFile >> c2;
  if (c1 != 'a' || c2 != '1')
    {
    this->RenderView->Warning("%s is not an assembly file.", path);      
    inFile.close();
    delete [] path;
    return;
    }
  delete [] path;
  path = NULL;
  inFile >> str;
  if (strcmp(str, "Assembly") != 0)
    {
    this->RenderView->Warning("Expecting token: Assembly");
    inFile.close();
    return;
    }
  assy = vtkGoAssembly::New();
  assy->LoadFromFile(inFile);
  inFile.close();

  // Add the assemblies children to the root assembly.
  int num;
  num = assy->GetNumberOfChildren();
  for (idx = 0; idx < num; ++idx)
    {
    child = assy->GetChild(idx);
    this->RenderView->AddAssembly(child);
    }
  this->RenderView->PartsPickedNotify();
  assy->Delete();
}

//----------------------------------------------------------------------------
// Popup file dialog, load assembly and add it to the renderer.
void vtkPVDataList::SaveAssembly()
{
  vtkIndent indent;
  char *result, *path = NULL;
  vtkGoAssembly *assy;
  vtkGoPart *partWithoutFilename;

  assy = this->RenderView->GetRootAssembly();
  partWithoutFilename = this->CheckFileNames(assy);
  if (partWithoutFilename)
    {
    assy->SetPickedStatus(VTK_GO_STATUS_OFF);
    partWithoutFilename->SetPickedStatus(VTK_GO_STATUS_ON);
    this->RenderView->PartsPickedNotify();
    this->RenderView->Warning("Please export of delete this part (picked) before saving the assembly.");
    return;
    }

  this->Script(
    "tk_getSaveFile -title \"Save Assembly\" -filetypes {{{Assembly} {.asy}}}");
  result = this->Application->GetMainInterp()->result;
  if (strlen(result) == 0)
    {
    return;
    }
  path = new char[strlen(result)+1];
  strcpy(path,result);

  // Make sure we have the correct extension.
  this->Script("file extension {%s}", path);
  result = this->Application->GetMainInterp()->result;
  if (strcmp(result, ".asy") != 0)
    {
    char *tmp = new char[strlen(path)+5];
    sprintf(tmp, "%s.asy", path);
    delete [] path;
    path = tmp;
    }

  if (strlen(path) == 0)
    {
    delete [] path;
    return;
    }

  ofstream outFile(path, ios::out);

  if ( !outFile)
    {
    this->RenderView->Warning("Could not open file %s", path);
    delete [] path;
    return;
    }
  outFile << "a1\n";
  assy->SaveInFile(outFile, indent);
  outFile.close();
}

//----------------------------------------------------------------------------
// Remove all assembies from the editor.
void vtkPVDataList::DeleteAssembly()
{
  vtkGoAssembly *root;

  root = this->RenderView->GetRootAssembly();
  if (root->GetNumberOfChildren() > 0)
    {
    // This should give user the oportunity to abort.
    if ( ! this->RenderView->Verify("This will remove all current assemblies."))
      {
      return;
      }
    root->Unload(this->RenderView->GetRenderer());
    root->RemoveAllChildren();
    this->RenderView->PartsPickedNotify();
    this->RenderView->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDataList::RandomizeColors()
{
  this->RecursiveRandomizeColors(this->GetRootAssembly());
  this->Update();
  this->RenderView->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVDataList::RecursiveRandomizeColors(vtkGoAssembly *assy)
{
  vtkGoAssembly *child;
  int idx, num;

  if ( ! assy->GetEditorOpen() || assy->GetNumberOfChildren() == 0)
    {
    float r, g, b;
    r = 0.5 + 0.5 * vtkMath::Random();
    g = 0.5 + 0.5 * vtkMath::Random();
    b = 0.5 + 0.5 * vtkMath::Random();
    assy->SetColor(r, g, b);
    return;
    }
  num = assy->GetNumberOfChildren();
  for (idx = 0; idx < num; ++idx)
    {
    child = assy->GetChild(idx);
    this->RecursiveRandomizeColors(child);
    }
}

//----------------------------------------------------------------------------
void vtkPVDataList::DeletePicked()
{
  this->RenderView->GetRootAssembly()->DeletePicked(
                                        this->RenderView->GetRenderer());
  this->Update();
  this->RenderView->PartsPickedNotify();
  this->RenderView->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVDataList::DeletePickedVerify()
{
  vtkGoAssembly *pickedAssy;

  // Before we ask, see if we have any parts to delete.
  pickedAssy = this->RenderView->GetPickedAssembly();
  if (pickedAssy->GetNumberOfChildren() == 0)
    {
    this->RenderView->SetStatusText("No picked assemblies to delete.");
    return;
    }

  if ( ! this->RenderView->Verify("Do you really want to remove the picked assemblies?"))
    {
    return;
    }
  
  this->DeletePicked();
}

//----------------------------------------------------------------------------
void vtkPVDataList::CutPicked()
{
  this->RenderView->GetRootAssembly()->CutPicked(this->ClipBoard);
  this->ClipBoard->SetPickedStatus(VTK_GO_STATUS_OFF);
  this->ClipBoard->SetRepresentationToWireframe();
  this->Update();
  this->RenderView->PartsPickedNotify();
  this->RenderView->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVDataList::CopyPicked()
{
  this->CopyPicked(this->RenderView->GetRootAssembly());
  this->ClipBoard->SetPickedStatus(VTK_GO_STATUS_OFF);
  this->ClipBoard->SetRepresentationToWireframe();
  this->Update();
  this->RenderView->EventuallyRender();
}



//----------------------------------------------------------------------------
void vtkPVDataList::CopyPicked(vtkGoAssembly *assy)
{
  if (assy->GetPickedStatus() == VTK_GO_STATUS_ON)
    {
    vtkMatrix4x4 *m = vtkMatrix4x4::New();
    m->SetElement(0, 3, 2.0);
    vtkGoAssembly *copy = vtkGoAssembly::New();
    copy->DeepCopy(assy);
    copy->Load(this->RenderView->GetRenderer());
    this->ClipBoard->AddChild(copy);
    copy->SetMatrix(*m);
    m->Delete();
    copy->Delete();
    }
  if (assy->GetPickedStatus() == VTK_GO_STATUS_PARTIAL)
    {
    int num, idx;
    vtkGoAssembly *child;
    num = assy->GetNumberOfChildren();
    for (idx = 0; idx < num; ++idx)
      {
      child = assy->GetChild(idx);
      this->CopyPicked(child);
      }
    }
}



//----------------------------------------------------------------------------
void vtkPVDataList::PasteBeforePicked()
{
  this->ClipBoard->SetRepresentationToSurface();
  this->ClipBoard->SetPickedStatus(VTK_GO_STATUS_ON);
  this->PasteBeforePicked(this->GetRootAssembly());
  this->RenderView->PartsPickedNotify();
}

//----------------------------------------------------------------------------
void vtkPVDataList::PasteBeforePicked(vtkGoAssembly *parent)
{
  vtkGoAssembly *child;
  int idx, num, insertIdx;

  if (parent == NULL)
    {
    return;
    }

  // Find the first "picked" child.
  num = parent->GetNumberOfChildren();
  if (num == 0 || parent->GetPickedStatus() == VTK_GO_STATUS_OFF)
    {
    return;
    }

  for (idx = 0; idx < num; ++idx)
    {
    child = parent->GetChild(idx);
    if (child->GetPickedStatus() == VTK_GO_STATUS_ON)
      { // Insert here
      insertIdx = idx;
      num = this->ClipBoard->GetNumberOfChildren();
      for (idx = num-1; idx >= 0; --idx)
        {
        child = this->ClipBoard->GetChild(idx);
        parent->InsertChild(insertIdx, child); 
        }
      this->ClipBoard->RemoveAllChildren();
      this->RenderView->GetRootAssembly()->UpdateStatus();
      this->Update();
      this->RenderView->EventuallyRender();
      return;
      }
    if (child->GetPickedStatus() == VTK_GO_STATUS_PARTIAL)
      {
      this->PasteBeforePicked(child);
      return;
      }
    }
  vtkErrorMacro("Inconsistent PickedStatus");
}

//----------------------------------------------------------------------------
void vtkPVDataList::PasteAfterPicked()
{
  this->ClipBoard->SetRepresentationToSurface();
  //this->ClipBoard->SetPickedStatus(VTK_GO_STATUS_ON);
  this->PasteAfterPicked(this->GetRootAssembly());
  this->RenderView->PartsPickedNotify();
}

//----------------------------------------------------------------------------
void vtkPVDataList::PasteAfterPicked(vtkGoAssembly *parent)
{
  vtkGoAssembly *child;
  int idx, num, insertIdx;

  if (parent == NULL)
    {
    return;
    }

  // Find the last "picked" child.
  num = parent->GetNumberOfChildren();
  if (num == 0 || parent->GetPickedStatus() == VTK_GO_STATUS_OFF)
    {
    return;
    }

  for (idx = num-1; idx >= 0; --idx)
    {
    child = parent->GetChild(idx);
    if (child->GetPickedStatus() == VTK_GO_STATUS_ON)
      { // Insert here
      insertIdx = idx+1;
      num = this->ClipBoard->GetNumberOfChildren();
      for (idx = num-1; idx >= 0; --idx)
        {
        child = this->ClipBoard->GetChild(idx);
        parent->InsertChild(insertIdx, child); 
        }
      this->ClipBoard->RemoveAllChildren();
      this->RenderView->GetRootAssembly()->UpdateStatus();
      this->Update();
      this->RenderView->EventuallyRender();
      return;
      }
    if (child->GetPickedStatus() == VTK_GO_STATUS_PARTIAL)
      {
      this->PasteAfterPicked(child);
      return;
      }
    }
  vtkErrorMacro("Inconsistent PickedStatus");
}

//----------------------------------------------------------------------------
void vtkPVDataList::ResetPositionPicked()
{ 
  vtkGoAssembly *assy, *child;
  int idx, num;

  assy = this->RenderView->GetPickedAssembly();
  num = assy->GetNumberOfChildren();
  for (idx = 0; idx < num; ++idx)
    {
    child = assy->GetChild(idx);
    child->SetMatrixToIdentity();
    }
  this->RenderView->PartsMovedNotify();
  this->RenderView->EventuallyRender();
}

//----------------------------------------------------------------------------
// How should we doo this exactly: What if the picked assemblies do not share 
// the same parent.
void vtkPVDataList::GroupPicked()
{
  int idx, num;
  vtkGoAssembly *assy, *child, *group;

  // First find the first picked part.  This is where we will create
  // the new assembly.
  assy = this->RenderView->GetRootAssembly();
  if (assy->GetPickedStatus() == VTK_GO_STATUS_OFF || 
      assy->GetPickedStatus() == VTK_GO_STATUS_EMPTY)
    {
    return;
    }
  // If picked status is set properly, we do not need the terminating condition.
  // As a sanity check, we will test anyway.
  num = assy->GetNumberOfChildren();
  idx = 0;
  if (num == 0)
    {
    vtkErrorMacro("Sanity check failed.");
    }
  child = assy->GetChild(idx);
  while (child->GetPickedStatus() != VTK_GO_STATUS_ON)
    {
    if (child->GetPickedStatus() == VTK_GO_STATUS_OFF ||
        child->GetPickedStatus() == VTK_GO_STATUS_EMPTY)
      {
      ++idx;
      if (idx >= num)
        {
        vtkErrorMacro("Sanity check failed.");
        }
      child = assy->GetChild(idx);
      }
    if (child->GetPickedStatus() == VTK_GO_STATUS_PARTIAL)
      {
      assy = child;
      num = assy->GetNumberOfChildren();
      if (num == 0)
        {
        vtkErrorMacro("Sanity check failed.");
        }
      idx = 0;
      child = assy->GetChild(idx);
      }
    }

  // Now clip picked.
  group = vtkGoAssembly::New();
  group->SetName("Group");
  group->SetEditorOpen(1);
  this->RenderView->GetRootAssembly()->CutPicked(group);
  group->SetCenterToDefault();
  assy->InsertChild(idx, group);

  // Now set the installed position of the new groups children.
  // Maintain the relative positions of the children.
  num = group->GetNumberOfChildren();
  for (idx = 0; idx < num; ++idx)
    {
    child = group->GetChild(idx);
    vtkMatrix4x4 *m = child->GetInstalled();
    if (m == NULL)
      {
      m = vtkMatrix4x4::New();
      child->SetInstalled(m);
      // Modify to let the collision sphere tree know it should regenerate.
      child->Modified();
      m->Delete();
      m = child->GetInstalled();
      }
    child->GetAssemblyToWorldMatrix(m);
    // Force the path icon to regenerate
    if (child->GetPath())
      {
      child->GetPath()->Modified();
      }
    }

  group->Delete();

  this->RenderView->PartsPickedNotify();
}

//----------------------------------------------------------------------------
void vtkPVDataList::UngroupPicked()
{
  this->RecursiveUngroupPicked(this->RenderView->GetRootAssembly());
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVDataList::RecursiveUngroupPicked(vtkGoAssembly *assy)
{
  int idx, num, add;
  vtkGoAssembly *child;

  num = assy->GetNumberOfChildren();
  for (idx = 0; idx < num; ++idx)
    {
    child = assy->GetChild(idx);
    if (child->GetPickedStatus() == VTK_GO_STATUS_PARTIAL)
      {
      this->RecursiveUngroupPicked(child);
      }
    else if (child->GetPickedStatus() == VTK_GO_STATUS_ON)
      {
      add = this->UngroupChild(child, assy, idx);
      idx += add;
      num += add;
      }
    }    
}

//----------------------------------------------------------------------------
// Returns the number of additional children parent will adopt.
int vtkPVDataList::UngroupChild(vtkGoAssembly *child, 
                                        vtkGoAssembly *parent, int start)
{
  int num, idx;
  vtkGoAssembly *grandChild;

  num = child->GetNumberOfChildren();
  if (num == 0)
    {
    return 0;
    }
  child->Register(this);
  parent->RemoveChild(start);
  for (idx = 0; idx < num; ++idx)
    {
    grandChild = child->GetChild(idx);
    parent->InsertChild(idx + start, grandChild);
    }
  child->UnRegister(this);

  return num - 1;
}



//----------------------------------------------------------------------------
void vtkPVDataList::SetInstalledPicked()
{
  int idx, num;
  vtkGoAssembly *assy, *child;
  vtkMatrix4x4 *m;

  assy = this->RenderView->GetPickedAssembly();
  num = assy->GetNumberOfChildren();
  for (idx = 0; idx < num ; ++idx)
    {
    child = assy->GetChild(idx);

    // Set the installed position form it's current position.
    m = child->GetInstalled();
    if (m == NULL)
      {
      m = vtkMatrix4x4::New();
      child->SetInstalled(m);
      m->Delete();
      // Simply because it's bad karma to use an object after it has been deleted
      m = NULL;
      m = child->GetInstalled();
      }
    // We need to eliminate the influence of the previous installed 
    // before we get the matrix (new installed) of the child.
    m->Identity();
    child->GetAssemblyToWorldMatrix(m);
    // Force the path icon to regenerate
    if (child->GetPath())
      {
      child->GetPath()->Modified();
      }
    }

  // This may not be necessary. (Maybe to update the center of rotation UIs).
  this->RenderView->PartsMovedNotify();
}

//----------------------------------------------------------------------------
void vtkPVDataList::ResetInstalledPicked()
{
  int idx, num;
  vtkGoAssembly *assy, *child;

  assy = this->RenderView->GetPickedAssembly();
  num = assy->GetNumberOfChildren();
  for (idx = 0; idx < num ; ++idx)
    {
    child = assy->GetChild(idx);
    child->SetInstalled(NULL);
    // Force the path icon to regenerate
    if (child->GetPath())
      {
      child->GetPath()->Modified();
      }
    }

  // Go ahead and reset the positions.
  // I would expect the part to move if we are reseting the installed.
  // This eliminates installed and temporary (part) transformations.
  // Maybe we should only undo the installed portion of the transormation.
  this->ResetPositionPicked();
}

//----------------------------------------------------------------------------
void vtkPVDataList::DeleteKinematicPicked()
{
  int idx, num;
  vtkGoAssembly *assy, *child;

  assy = this->RenderView->GetPickedAssembly();
  num = assy->GetNumberOfChildren();
  for (idx = 0; idx < num ; ++idx)
    {
    child = assy->GetChild(idx);
    child->SetKinematicElement(NULL);
    }
  
  this->Update();
}


//----------------------------------------------------------------------------
void vtkPVDataList::EditKinematicPicked()
{
  int num;
  vtkGoAssembly *assy;
  vtkGoKinematicElement *k;

  assy = this->RenderView->GetPickedAssembly();
  num = assy->GetNumberOfChildren();

  if (num > 1)
    {
    this->RenderView->Warning("Too many assemblies picked.");
    return;
    }
  if (num == 0)
    {
    this->RenderView->Warning("No assemblies picked.");
    return;
    }

  assy = assy->GetChild(0);
  k = assy->GetKinematicElement();
  if (k)
    {
    this->KinematicDialog->SetParameterMinimum(k->GetMinimum());
    this->KinematicDialog->SetParameterMaximum(k->GetMaximum());
    this->KinematicDialog->SetSlaveFlag(k->GetSlaveFlag());
    this->KinematicDialog->SetDelegate(k->GetDelegate());
    if (k->IsA("vtkGoKinematicRotation"))
      {
      vtkGoKinematicRotation *kr = (vtkGoKinematicRotation *)k;
      this->KinematicDialog->SetKinematicTypeToRotation();
      this->KinematicDialog->SetVector(kr->GetAxis());
      }
    else if (k->IsA("vtkGoKinematicTranslation"))
      {
      vtkGoKinematicTranslation *kt = (vtkGoKinematicTranslation *)k;
      this->KinematicDialog->SetKinematicTypeToTranslation();
      this->KinematicDialog->SetVector(kt->GetDirection());
      }
    else 
      {
      this->RenderView->Warning("Unknown kinematic type.");
      }
    }
  
  this->Script("pack %s -side top -expand yes -fill x",
               this->KinematicDialog->GetWidgetName());
  if (this->KinematicDialog->Verify("Kinematic Selection") == 0)
    { // User pressed cancel.
    this->Script( "pack forget %s",this->KinematicDialog->GetWidgetName());
    return;
    }   
  this->Script( "pack forget %s",this->KinematicDialog->GetWidgetName());

  // Create a new kinimatic.
  if (this->KinematicDialog->GetKinematicType() == VTK_GO_KINEMATIC_ROTATION)
    {
    vtkGoKinematicRotation *kr = vtkGoKinematicRotation::New();
    kr->SetAxis(this->KinematicDialog->GetVector());
    k = kr;
    }
  else if (this->KinematicDialog->GetKinematicType() == VTK_GO_KINEMATIC_TRANSLATION)
    {
    vtkGoKinematicTranslation *kt = vtkGoKinematicTranslation::New();
    kt->SetDirection(this->KinematicDialog->GetVector());
    k = kt;
    }
  else
    {
    vtkErrorMacro("Dialog generated unkown kinematic type.");
    return;
    }
  
  k->SetSlaveFlag(this->KinematicDialog->GetSlaveFlag());
  k->SetDelegate(this->KinematicDialog->GetDelegate());
  k->SetMinimum(this->KinematicDialog->GetParameterMinimum());
  k->SetMaximum(this->KinematicDialog->GetParameterMaximum());
  k->SetAssembly(assy);
  assy->SetKinematicElement(k);
  k->Delete();

  this->Update();
  this->RenderView->PartsPickedNotify();
}
