/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourceList.cxx
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
#include "vtkPVSourceList.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkObjectFactory.h"

int vtkPVSourceListCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVSourceList::vtkPVSourceList()
{
  this->CommandFunction = vtkPVSourceListCommand;

  this->ScrollFrame = vtkKWWidget::New();
  this->ScrollFrame->SetParent(this);
  this->Canvas = vtkKWWidget::New();
  this->Canvas->SetParent(this->ScrollFrame);
  this->ScrollBar = vtkKWWidget::New();
  this->ScrollBar->SetParent(this->ScrollFrame);

  this->Sources = NULL;

  this->NameEntrySource = NULL;
  this->NameEntryTag = NULL;
  this->NameEntry = vtkKWEntry::New();
  this->NameEntry->SetParent(this->Canvas);
}

//----------------------------------------------------------------------------
vtkPVSourceList::~vtkPVSourceList()
{
  this->ScrollFrame->Delete();
  this->ScrollFrame = NULL;
  this->Canvas->Delete();
  this->Canvas = NULL;
  this->ScrollBar->Delete();
  this->ScrollBar = NULL;

  this->Sources->Delete();
  this->Sources = NULL;

  this->SetNameEntrySource(NULL);
  this->SetNameEntryTag(NULL);
  this->NameEntry->Delete();
  this->NameEntry = NULL;
}


//------------------------------------------------------------------------------
vtkPVSourceList* vtkPVSourceList::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSourceList");
  if(ret)
    {
    return (vtkPVSourceList*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSourceList;
}




//----------------------------------------------------------------------------
void vtkPVSourceList::Create(vtkKWApplication *app, char *args)
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

  this->ScrollFrame->Create(app,"frame"," -bd 1");
  this->Script( "pack %s -side top -fill both -expand yes",
                this->ScrollFrame->GetWidgetName());
  sprintf(str, "-command {%s yview} -bd 0", this->Canvas->GetWidgetName());
  this->ScrollBar->Create(app,"scrollbar",str);
  this->Script("pack %s -side right -fill y -expand no",
   	       this->ScrollBar->GetWidgetName());
  sprintf(str,
	  "-bg white -width 100 -height 150 -yscrollcommand {%s set} -bd 0",
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
void vtkPVSourceList::EditName(int compIdx)
{
  vtkPVSource *comp;
  int x1, y1, x2, y2;
  char *result;

  // If we are already editing a name, then break its loop.
  if (this->NameEntrySource)
    {
    this->NameEntryClose();
    this->Script("after idle {%s EditName %d}",this->GetTclName(), compIdx);
    return;
    }

  comp = vtkPVSource::SafeDownCast(
    this->Sources->GetItemAsObject(compIdx));
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
  this->SetNameEntrySource(comp);
  this->NameEntry->SetValue(comp->GetName());
  // should we bind the tag, or the window
  this->Script( "bind %s <KeyPress-Return> {%s NameEntryClose}",
                this->NameEntry->GetWidgetName(), this->GetTclName());

  // Wait for the
  while (this->NameEntrySource)
    {
    this->Script("update");
    }
}

//----------------------------------------------------------------------------
void vtkPVSourceList::NameEntryClose()
{
  int num = this->Sources->GetNumberOfItems();
  int i;
  vtkPVSource *comp;
  
  if (this->NameEntrySource == NULL)
    {
    return;
    }
  this->NameEntrySource->SetName(this->NameEntry->GetValue());
  
  for (i = 0; i < num; i++)
    {
    comp = vtkPVSource::SafeDownCast(
      this->Sources->GetItemAsObject(i));
    vtkErrorMacro("Source "<<i<<": "<<comp->GetName());
    }
  
  this->SetNameEntrySource(NULL);
  this->Script("%s delete %s", this->Canvas->GetWidgetName(), 
               this->NameEntryTag);
  this->SetNameEntryTag(NULL);
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVSourceList::Pick(int compIdx)
{
  vtkPVSource *comp;

  comp = vtkPVSource::SafeDownCast(
    this->Sources->GetItemAsObject(compIdx));
  comp->GetWindow()->SetCurrentSource(comp);
  comp->ShowProperties();
  comp->GetNotebook()->Raise(0);
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVSourceList::ToggleVisibility(int compIdx, int button)
{
  vtkPVSource *comp;
  int i;
  
  comp = vtkPVSource::SafeDownCast(
    this->Sources->GetItemAsObject(compIdx));
  if (comp)
    {
    // Toggle visibility
    for (i = 0; i < comp->GetNumberOfPVOutputs(); i++)
      {
      if (comp->GetPVOutput(i)->GetActorComposite()->GetVisibility())
	{
	comp->GetPVOutput(i)->GetActorComposite()->VisibilityOff();
	}
      else
	{
	comp->GetPVOutput(i)->GetActorComposite()->VisibilityOn();
	}
      }
    comp->GetView()->Render();
    }

  this->Update();
}


//----------------------------------------------------------------------------
void vtkPVSourceList::EditColor(int compIdx)
{
}

//----------------------------------------------------------------------------
void vtkPVSourceList::Update()
{
  vtkPVSource *comp;
  int num, idx;
  int y, in;
  
  // Get us out of the name entry state if we are in it.
  this->NameEntryClose();
  
  this->Script("%s delete all",
               this->Canvas->GetWidgetName());

  if (this->Sources == NULL)
    {
    vtkErrorMacro("Sources is NULL");
    return;
    }

  y = 30;
  in = 10;
  num = this->Sources->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    comp = vtkPVSource::SafeDownCast(
      this->Sources->GetItemAsObject(idx));
    y = this->Update(comp, y, in);
    }

  this->Script("%s config -scrollregion [%s bbox all]",
               this->Canvas->GetWidgetName(),this->Canvas->GetWidgetName());
}

//----------------------------------------------------------------------------
// We do not want the user to edit the clip board.
int vtkPVSourceList::Update(vtkPVSource *comp, int y, int in)
{
  int compIdx, x, yNext; 
  static char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";
  char *result;
  int bbox[4];
  char *tmp;

  compIdx = this->Sources->IsItemPresent(comp) - 1;

  // Draw the small horizontal indent line.
  x = in + 8;
  this->Script("%s create line %d %d %d %d -fill gray50",
               this->Canvas->GetWidgetName(), in, y, x, y);
  yNext = y + 17;

  // Draw the icon indicating visibility.
  result = NULL;
  if (comp->GetPVOutput(0) == NULL)
    {
    this->Script("%s create image %d %d -image visonbm",
                 this->Canvas->GetWidgetName(), x, y);
    result = this->Application->GetMainInterp()->result;
    x += 9;
    }
  else
    {
    switch (comp->GetPVOutput(0)->GetActorComposite()->GetVisibility())
      {
      case 0:
        this->Script("%s create image %d %d -image visoffbm",
                     this->Canvas->GetWidgetName(), x, y);
        result = this->Application->GetMainInterp()->result;
        x += 9;
        break;
      case 1:
        this->Script("%s create image %d %d -image visonbm",
                     this->Canvas->GetWidgetName(), x, y);
        result = this->Application->GetMainInterp()->result;
        x += 9;
        break;
      }
    }
  if (result)
    {
    tmp = new char[strlen(result)+1];
    strcpy(tmp,result);
    this->Script("%s bind %s <ButtonPress-1> {%s ToggleVisibility %d 1}",
                 this->Canvas->GetWidgetName(), tmp,
                 this->GetTclName(), compIdx);
    this->Script("%s bind %s <ButtonPress-3> {%s ToggleVisibility %d 3}",
                 this->Canvas->GetWidgetName(), tmp,
                 this->GetTclName(), compIdx);
    delete [] tmp;
    tmp = NULL;
    }

  // Draw the name of the assembly.
  this->Script(
    "%s create text %d %d -text {%s} -font %s -anchor w -tags x",
    this->Canvas->GetWidgetName(), x, y, comp->GetName(), font);

  // Make the name hot for picking.
  result = this->Application->GetMainInterp()->result;
  tmp = new char[strlen(result)+1];
  strcpy(tmp,result);
  this->Script("%s bind %s <ButtonPress-1> {%s Pick %d}",
	       this->Canvas->GetWidgetName(), tmp,
	       this->GetTclName(), compIdx);
  this->Script("%s bind %s <ButtonPress-3> {%s EditName %d}",
	       this->Canvas->GetWidgetName(), tmp,
	       this->GetTclName(), compIdx);
  
// Get the bounding box for the name. We may need to highlight it.
  this->Script( "%s bbox %s",this->Canvas->GetWidgetName(), tmp);
  delete [] tmp;
  tmp = NULL;
  result = this->Application->GetMainInterp()->result;
  sscanf(result, "%d %d %d %d %s %d", bbox, bbox+1, bbox+2, bbox+3);
  
  // Highlight the name based on the picked status. 
  if (comp->GetWindow()->GetCurrentSource() == comp)
    {
    tmp = "yellow";

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

  return yNext;
}

//----------------------------------------------------------------------------
void vtkPVSourceList::DeletePicked()
{
}

//----------------------------------------------------------------------------
void vtkPVSourceList::DeletePickedVerify()
{
}
