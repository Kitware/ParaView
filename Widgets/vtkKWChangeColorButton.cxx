/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWChangeColorButton.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWChangeColorButton.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWChangeColorButton* vtkKWChangeColorButton::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWChangeColorButton");
  if(ret)
    {
    return (vtkKWChangeColorButton*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWChangeColorButton;
}


int vtkKWChangeColorButtonCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkKWChangeColorButton::vtkKWChangeColorButton()
{
  this->CommandFunction = vtkKWChangeColorButtonCommand;
  this->Command = NULL;
  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;
  this->Text = NULL;
  this->SetText("Set Color");
  this->Label1 = vtkKWWidget::New();
  this->Label1->SetParent(this);
  this->Label2 = vtkKWWidget::New();
  this->Label2->SetParent(this);
}

vtkKWChangeColorButton::~vtkKWChangeColorButton()
{
  if (this->Command)
    {
    delete [] this->Command;
    }
  if ( this->Text )
    {
    delete [] this->Text;
    }
  this->Label1->Delete();
  this->Label2->Delete();
}

void vtkKWChangeColorButton::SetColor(float r, float g, float b)
{
  if ( this->Color[0] == r && this->Color[1] == g && this->Color[2] == b )
    {
    return;
    }

  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;

  if ( this->Application )
    {
    this->Script( "%s configure -bg {#%02x%02x%02x}", 
                  this->Label2->GetWidgetName(),
		  (int)(r*255.5), 
		  (int)(g*255.5), 
		  (int)(b*255.5) );
    this->Script( "update idletasks");
    }
}


void vtkKWChangeColorButton::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;
  char color[256];

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Change color button already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  sprintf( color, "#%02x%02x%02x", 
	   (int)(this->Color[0]*255.5), 
	   (int)(this->Color[1]*255.5), 
	   (int)(this->Color[2]*255.5) );

  this->Script("frame %s -relief raised -bd 2 %s", wname, args);
  char textarg[1024];
  sprintf( textarg, "-text {%s}", this->Text );
  this->Label1->Create(this->Application,"label", textarg);
  this->Label2->Create(this->Application,
                       "label","-width 2 -height 1");
  this->Script("pack %s -padx 3 -pady 3 -ipadx 2 -side right",this->Label2->GetWidgetName());
  this->Script("pack %s -padx 3 -pady 3 -ipadx 2 -side left -fill x -expand yes", 
               this->Label1->GetWidgetName());
  
  this->Script("%s configure -bg %s",this->Label2->GetWidgetName(),color);
  
  // bind button presses
  this->Script("bind %s <Any-ButtonPress> {%s AButtonPress %%X %%Y}",
               wname, this->GetTclName());
  this->Script("bind %s <Any-ButtonRelease> {%s AButtonRelease %%X %%Y}",
               wname, this->GetTclName());
  this->Script("bind %s <Any-ButtonPress> {%s AButtonPress %%X %%Y}",
               this->Label1->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonRelease> {%s AButtonRelease %%X %%Y}",
               this->Label1->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonPress> {%s AButtonPress %%X %%Y}",
               this->Label2->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonRelease> {%s AButtonRelease %%X %%Y}",
               this->Label2->GetWidgetName(), this->GetTclName());

}

void vtkKWChangeColorButton::AButtonPress(int x, int y)
{  
  this->Script("%s configure -relief sunken", this->GetWidgetName());  
}

void vtkKWChangeColorButton::AButtonRelease(int x, int y)
{  
  this->Script("%s configure -relief raised", this->GetWidgetName());  

  // was it released over the button ?
  this->Script( "winfo rootx %s", this->GetWidgetName());
  int xw = vtkKWObject::GetIntegerResult(this->Application);
  this->Script( "winfo rooty %s", this->GetWidgetName());
  int yw = vtkKWObject::GetIntegerResult(this->Application);

  // get the size and of the window
  this->Script( "winfo width %s", this->GetWidgetName());
  int dxw = vtkKWObject::GetIntegerResult(this->Application);
  this->Script( "winfo height %s", this->GetWidgetName());
  int dyw = vtkKWObject::GetIntegerResult(this->Application);

  if ((x >= xw) && (x<= xw+dxw) && (y >= yw) && (y <= yw + dyw))
    {
    this->ChangeColor();
    }  
}

void vtkKWChangeColorButton::ChangeColor()
{  
  int r, g, b;
  char *result, tmp[3];

  this->Script(
     "tk_chooseColor -initialcolor {#%02x%02x%02x} -title {Choose Color}",
     (int)(this->Color[0]*255.5), 
     (int)(this->Color[1]*255.5), 
     (int)(this->Color[2]*255.5) );
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
    
    this->Script("%s configure -bg %s",this->Label2->GetWidgetName(),result);
    this->Script( "update idletasks");
    if ( this->Command )
      {
      this->Script("eval %s %f %f %f", this->Command, 
		   (float)r/255.0, (float)g/255.0, (float)b/255.0);
      }
    this->Color[0] = (float)r/255.0;
    this->Color[1] = (float)g/255.0;
    this->Color[2] = (float)b/255.0;
    }
}

void vtkKWChangeColorButton::SetCommand( vtkKWObject* CalledObject, 
					 const char *CommandString )
{
  if (this->Command)
    {
    delete [] this->Command;
    }
  ostrstream command;
  command << CalledObject->GetTclName() << " " << CommandString << ends;

  this->Command = command.str();
}

// Description:
// Chaining method to serialize an object and its superclasses.
void vtkKWChangeColorButton::SerializeSelf(ostream& os, vtkIndent indent)
{
  // invoke superclass
  this->vtkKWWidget::SerializeSelf(os,indent);
  os << indent << "Color " << this->Color[0] << " " << this->Color[1] <<
    " " << this->Color[2] << endl;
}

void vtkKWChangeColorButton::SerializeToken(istream& is, const char token[1024])
{
  float clr[3];
  if (!strcmp(token,"Color"))
    {
    is >> clr[0] >> clr[1] >> clr[2];
    this->SetColor(clr);
    if ( this->Command )
      {
      this->Script("eval %s %f %f %f", this->Command, 
		   clr[0], clr[1], clr[2]);
      }
    return;
    }
  vtkKWWidget::SerializeToken(is,token);
}

void vtkKWChangeColorButton::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWWidget::SerializeRevision(os,indent);
  os << indent << "vtkKWChangeColorButton ";
  this->ExtractRevision(os,"$Revision: 1.11 $");
}
