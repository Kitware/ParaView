/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWScale.cxx
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
#include "vtkKWScale.h"
#include "vtkKWEntry.h"
#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWScale );




int vtkKWScaleCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkKWScale::vtkKWScale()
{
  this->CommandFunction = vtkKWScaleCommand;
  this->Command = NULL;
  this->StartCommand = 0;
  this->EndCommand = NULL;
  this->Value = 0;
  this->Resolution = 1;
  this->Entry = NULL;
  this->ScaleLabel = NULL;
  this->ScaleWidget = vtkKWWidget::New();
  this->ScaleWidget->SetParent(this);
  this->Range[0] = 0;
  this->Range[1] = 1;  

  this->SetStartCommand(0, 0);
  this->SetEndCommand(0, 0);
  this->SetCommand(0,0);
}

vtkKWScale::~vtkKWScale()
{
  if (this->Command)
    {
    delete [] this->Command;
    }
  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    }
  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    }
  if (this->Entry)
    {
    this->Entry->Delete();
    }
  if (this->ScaleLabel)
    {
    this->ScaleLabel->Delete();
    }
  this->ScaleWidget->Delete();
}

void vtkKWScale::SetValue(float s)
{
  this->Script("%s set %f", this->ScaleWidget->GetWidgetName(),s);
  if (this->Entry)
    {
    this->Entry->SetValue(s,2);
    }
  this->Value = s;

  this->Script( "update idletasks");
}


void vtkKWScale::SetResolution( float r )
{
  this->Resolution = r;
  
  if ( this->Application )
    {
    this->Script("%s configure -resolution %f",
                 this->ScaleWidget->GetWidgetName(), r);
    }
  
  this->Modified();
}

void vtkKWScale::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Scale already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s",wname);
  this->ScaleWidget->Create(app,"scale","-orient horizontal -showvalue no"
			    " -borderwidth 2");
  this->Script("%s configure %s",this->ScaleWidget->GetWidgetName(),args);
  this->Script("%s configure -resolution %f -highlightthickness 0",
               this->ScaleWidget->GetWidgetName(),this->Resolution);
  this->ScaleWidget->SetCommand(this, "ScaleValueChanged");
  this->Script("pack %s -side bottom -fill x -expand yes -pady 0 -padx 0",
               this->ScaleWidget->GetWidgetName());

  this->Script("bind %s <ButtonPress> {%s InvokeStartCommand}",
               this->ScaleWidget->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <ButtonRelease> {%s InvokeEndCommand}",
               this->ScaleWidget->GetWidgetName(), this->GetTclName());
}

void vtkKWScale::SetRange(float min, float max)
{
  this->Range[0] = min;
  this->Range[1] = max;
  this->Script("%s configure -from %f -to %f",
               this->ScaleWidget->GetWidgetName(),min,max);
}

void vtkKWScale::GetRange(float &min, float &max)
{
  min = this->Range[0];
  max = this->Range[1];
}

void vtkKWScale::DisplayEntry()
{
  if (this->Entry)
    {
    return;
    }
  this->Entry = vtkKWEntry::New();
  this->Entry->SetParent(this);
  this->Entry->Create(this->Application,"-width 10");
  this->Script("bind %s <Return> {%s EntryValueChanged}",
               this->Entry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side right -padx 2", this->Entry->GetWidgetName());
}

void vtkKWScale::DisplayLabel(const char *name)
{
  if (this->ScaleLabel)
    {
    this->Script("%s configure -text {%s}",
		 this->ScaleLabel->GetWidgetName(), name );
    return;
    }

  this->ScaleLabel = vtkKWWidget::New();
  this->ScaleLabel->SetParent(this);
  char temp[1024];
  sprintf(temp,"-text {%s}",name);
  this->ScaleLabel->Create(this->Application,"label",temp);
  this->Script("pack %s -side left",
               this->ScaleLabel->GetWidgetName());
}

void vtkKWScale::EntryValueChanged()
{
  this->Value = this->Entry->GetValueAsFloat();
  this->Script("%s set %f", this->ScaleWidget->GetWidgetName(), this->Value);
  this->Script("eval %s",this->Command);
}

void vtkKWScale::InvokeStartCommand()
{
  if ( this->StartCommand )
    {
    this->Script("eval %s",this->StartCommand);
    }
}

void vtkKWScale::InvokeEndCommand()
{
  if ( this->EndCommand )
    {
    this->Script("eval %s",this->EndCommand);
    }
}

void vtkKWScale::ScaleValueChanged(float num)
{
  if ( this->GetValue() == num )
    {
    return;
    }
  this->Value = num;
  if (this->Entry)
    {
    this->Entry->SetValue(this->Value,2);
    }
  if (this->Command)
    {
    this->Script("eval %s",this->Command);
    }
}


void vtkKWScale::SetStartCommand(vtkKWObject* Object, const char * MethodAndArgString)
{
  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    }
  ostrstream command;
  if ( !Object )
    {
    command << "{}" << ends;
    }
  else
    {
    command << Object->GetTclName() << " " << MethodAndArgString << ends;
    }
  this->StartCommand = command.str();
}

void vtkKWScale::SetEndCommand(vtkKWObject* Object, const char * MethodAndArgString)
{
  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    }
  ostrstream command;
  if ( !Object )
    {
    command << "{}" << ends;
    }
  else
    {
    command << Object->GetTclName() << " " << MethodAndArgString << ends;
    }
  this->EndCommand = command.str();
}


void vtkKWScale::SetCommand(vtkKWObject* CalledObject, const char *CommandString)
{
  if (this->Command)
    {
    delete [] this->Command;
    }
  ostrstream command;
  if ( !CalledObject )
    {
    command << "{}" << ends;
    }
  else
    {
    command << CalledObject->GetTclName() << " " << CommandString << ends;
    }
  this->Command = command.str();
}

void vtkKWScale::SetBalloonHelpString( const char *string )
{
  if ( !this->Application )
    {
    vtkErrorMacro("Must set application before setting balloon help string");
    return;
    }
  
  this->ScaleWidget->SetBalloonHelpString( string );
  if ( this->Entry )
    {
    this->Entry->SetBalloonHelpString( string );
    }
  if ( this->ScaleLabel )
    {
    this->ScaleLabel->SetBalloonHelpString( string );
    }
  
}

void vtkKWScale::SetBalloonHelpJustification( int j )
{
  this->ScaleWidget->SetBalloonHelpJustification( j );
  if ( this->Entry )
    {
    this->Entry->SetBalloonHelpJustification( j );
    }
  if ( this->ScaleLabel )
    {
    this->ScaleLabel->SetBalloonHelpJustification( j );
    }
  
}
