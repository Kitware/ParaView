/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWProgressGauge.cxx
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
#include "vtkKWProgressGauge.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWProgressGauge* vtkKWProgressGauge::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWProgressGauge");
  if(ret)
    {
    return (vtkKWProgressGauge*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWProgressGauge;
}

int vtkKWProgressGaugeCommand(ClientData cd, Tcl_Interp *interp,
			      int argc, char *argv[]);


vtkKWProgressGauge::vtkKWProgressGauge()
{ 
  this->CommandFunction = vtkKWProgressGaugeCommand;
  this->Length = 100;
  this->Height = 20;
  this->Value = 0;
  this->BarColor = strcpy(new char[5], "blue");
}


vtkKWProgressGauge::~vtkKWProgressGauge()
{
  delete [] this->BarColor;
  this->BarColor = NULL;
}

void vtkKWProgressGauge::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("CheckButton already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s", wname);
  this->Script("canvas %s.display -borderwidth 0  -highlightthickness 0 -width %d -height %d %s",
	       wname, this->Length, this->Height, args);
  this->Script("pack %s.display -expand yes", wname);
  // initialize the bar color to the background so it does
  // not show up until used
  this->Script(
    "%s.display create rectangle 0 0 0 0 -outline \"\"  -tags bar", 
	       wname );
  this->Script(
    "%s.display create text [expr 0.5 * %d] "
    "%d "
    "-anchor c -text \"\" -tags value",
    wname, this->Length, int(this->Height/2));
}

void vtkKWProgressGauge::SetValue(int value)
{
  const char* wname = this->GetWidgetName();

  this->Value = value;
  if(this->Value < 0)
    {
    this->Value = 0;
    }
  if(this->Value > 100)
    {
    this->Value = 100;
    }
  if(this->Value == 0)
    {
    // if the Value is 0, set the text to nothing and the color
    // of the bar to the background (0 0 0 0) rectangles show
    // up as a pixel...
    this->Script("%s.display itemconfigure value -text {}", wname);
    this->Script("%s.display coords bar 0 0 0 0", wname);
    this->Script("%s.display itemconfigure bar -fill {}", 
		 wname);
    }
  else
    {
    // if the Value is not 0 then use the BarColor for the bar
    this->Script("%s.display itemconfigure bar -fill %s", 
		 wname, this->BarColor);
    // Set the text to the percent done
    const char* textcolor = "-fill black";
    if(this->Value > 50)
      {
      textcolor = "-fill white";
      }
    
    this->Script("%s.display itemconfigure value -text {%3.0d%%} %s", 
		 wname, this->Value, textcolor);
    // Draw the correct rectangle
    this->Script("%s.display coords bar 0 0 [expr 0.01 * %d * [winfo width %s.display]] [winfo height %s.display]", 
		 wname, this->Value, wname, wname);
    }
  // do an update
  this->Script("update");
}
