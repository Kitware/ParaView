/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWChangeColorButton.cxx
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
}

void vtkKWChangeColorButton::SetColor(float c[3])
{
  this->Color[0] = c[0];
  this->Color[1] = c[1];
  this->Color[2] = c[2];

  if ( this->Application )
    {
    this->Script( "%s configure -bg {#%02x%02x%02x}", 
		  this->GetWidgetName(),
		  (int)(c[0]*255.5), 
		  (int)(c[1]*255.5), 
		  (int)(c[2]*255.5) );
    this->Script( "update idletasks");
    }
}


void vtkKWChangeColorButton::Create(vtkKWApplication *app, char *args)
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
  if ( this->Color[0] < 0.6 && this->Color[1] < 0.6 && this->Color[2] < 0.6 )
    {
    this->Script("button %s -bg %s -fg #ffffff -text {%s} %s", 
		 wname, color, this->Text, args);
    }
  else
    {
    this->Script("button %s -bg %s -fg #000000 -text {%s} %s", 
		 wname, color, this->Text, args);
    }
  this->Script("%s configure -command {%s ChangeColor}",
	       wname, this->GetTclName() );

}

void vtkKWChangeColorButton::ChangeColor()
{  
  unsigned char r, g, b;
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
    if ( r < 154 && g < 154 && b < 154 )
      {
      this->Script( "%s configure -bg %s -fg #ffffff", 
		    this->GetWidgetName(), result );
      }
    else
      {
      this->Script( "%s configure -bg %s -fg #000000", 
		    this->GetWidgetName(), result );
      }
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
