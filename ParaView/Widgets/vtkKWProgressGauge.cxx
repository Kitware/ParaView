/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWProgressGauge.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWProgressGauge );
vtkCxxRevisionMacro(vtkKWProgressGauge, "1.15");

int vtkKWProgressGaugeCommand(ClientData cd, Tcl_Interp *interp,
                              int argc, char *argv[]);


vtkKWProgressGauge::vtkKWProgressGauge()
{ 
  this->CommandFunction = vtkKWProgressGaugeCommand;
  this->Length = 100;
  this->Height = 20;
  this->Value = 0;
  this->BarColor = vtkString::Duplicate("blue");
  this->BackgroundColor = 0;
}


vtkKWProgressGauge::~vtkKWProgressGauge()
{
  delete [] this->BarColor;
  this->BarColor = NULL;
}

void vtkKWProgressGauge::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // Set the application

  if (this->IsCreated())
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
    "%s.display create text [expr 0.5 * %d] [expr 0.5 * %d] "
    "-anchor c -text \"\" -tags value",
    wname, this->Length, this->Height);

  // Update enable state

  this->UpdateEnableState();
}

void vtkKWProgressGauge::SetValue(int value)
{
  if (!this->IsMapped())
    {
    return;
    }

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

  ostrstream tk_cmd;

  if(this->Value == 0)
    {
    // if the Value is 0, set the text to nothing and the color
    // of the bar to the background (0 0 0 0) rectangles show
    // up as a pixel...
    
    tk_cmd << wname << ".display itemconfigure value -text {}" << endl
           << wname << ".display coords bar 0 0 0 0" << endl
           << wname << ".display itemconfigure bar -fill {}" << endl;
    }
  else
    {
    // if the Value is not 0 then use the BarColor for the bar

    tk_cmd << wname << ".display itemconfigure bar -fill " 
           << this->BarColor << endl;

    // Set the text to the percent done

    const char* textcolor = "-fill black";
    if(this->Value > 50)
      {
      textcolor = "-fill white";
      }
    
    char buffer[5];
    sprintf(buffer, "%3.0d", this->Value);

    tk_cmd << wname << ".display itemconfigure value -text {" << buffer 
           << "%%} " << textcolor << endl;

    // Draw the correct rectangle

    tk_cmd << wname << ".display coords bar 0 0 [expr 0.01 * " << this->Value 
           << " * [winfo width " << wname << ".display]] [winfo height "
           << wname << ".display]" << endl;
    }

  // Do an update

  tk_cmd << "update idletasks" << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetHeight(int height)
{
  if (this->Height == height)
    {
    return;
    }

  this->Height = height;
  this->Modified();

  //  Change gauge height, move text

  if (this->Application)
    {
    this->Script("%s.display config -height %d", 
                 this->GetWidgetName(), this->Height);

    this->Script("%s.display coords value [expr 0.5 * %d] [expr 0.5 * %d]", 
                 this->GetWidgetName(), this->Length, this->Height);
    }
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BackgroundColor: " 
     << (this->BackgroundColor?this->BackgroundColor:"none") << endl;
  os << indent << "BarColor: " << (this->BarColor?this->BarColor:"none") 
     << endl;
  os << indent << "Height: " << this->GetHeight() << endl;
  os << indent << "Length: " << this->GetLength() << endl;
}

