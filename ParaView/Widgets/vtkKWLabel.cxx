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
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"


int vtkKWLabelCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLabel );
vtkCxxRevisionMacro(vtkKWLabel, "1.27");

//----------------------------------------------------------------------------
vtkKWLabel::vtkKWLabel()
{
  this->Label    = new char[1];
  this->Label[0] = 0;
  this->LineType = vtkKWLabel::SingleLine;
  this->Width    = 0;
  this->AdjustWrapLengthToWidth = 0;
  this->CommandFunction = vtkKWLabelCommand;
}

//----------------------------------------------------------------------------
vtkKWLabel::~vtkKWLabel()
{
  delete [] this->Label;
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetLabel(const char* _arg)
{
  if (!_arg)
    {
    _arg = "";
    }

  if (this->Label == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->Label && _arg && (!strcmp(this->Label, _arg))) 
    {
    return;
    }

  if (this->Label) 
    { 
    delete [] this->Label; 
    }

  if (_arg)
    {
    this->Label = new char[strlen(_arg)+1];
    strcpy(this->Label, _arg);
    }
  else
    {
    this->Label = NULL;
    }

  this->Modified();

  this->UpdateText();
} 

//----------------------------------------------------------------------------
void vtkKWLabel::UpdateText()
{
  if (this->Label && this->IsCreated())
    {
    this->SetTextOption(this->Label);

    // Whatever the label, -image always takes precedence, unless it's empty
    // so change it accordingly
    
    if (this->LineType != vtkKWLabel::MultiLine && *this->Label)
      {
      this->Script("%s configure -image {}", this->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Label already created");
    return;
    }

  this->SetApplication(app);

  // Create the top level

  const char *wname = this->GetWidgetName();

  if (this->LineType == vtkKWLabel::MultiLine)
    {
    this->Script("message %s -width %d", wname, this->Width);
    }
  else
    {
    this->Script("label %s -justify left -width %d", wname, this->Width);
    }

  this->UpdateText();

  if (args && *args)
    {
    this->Script("%s config %s", wname, args);
    }

  // Set bindings (if any)
  
  this->UpdateBindings();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetLineType( int type )
{
  if (this->IsCreated())
    {
    if (this->LineType != type)
      {
      const char *wname = this->GetWidgetName();
      this->SetLabel(this->GetTextOption());
      this->Script("destroy %s", wname);
      if (this->LineType == vtkKWLabel::MultiLine)
        {
        this->Script("message %s -width %d", wname, this->Width);
        }
      else
        {
        this->Script("label %s -justify left -width %d", wname, this->Width);
        }
      this->UpdateText();
      }
    }
  this->LineType = type;
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Width = width;
  this->Modified();

  if (this->IsCreated())
    {
    this->Script("%s configure -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetAdjustWrapLengthToWidth(int v)
{
  if (this->AdjustWrapLengthToWidth == v)
    {
    return;
    }

  this->AdjustWrapLengthToWidth = v;
  this->Modified();

  this->UpdateBindings();
}

//----------------------------------------------------------------------------
void vtkKWLabel::AdjustWrapLengthToWidthCallback()
{
  if (!this->IsCreated() || !this->AdjustWrapLengthToWidth)
    {
    return;
    }

  // Get the widget width and the current wraplength

  this->Script("concat [winfo width %s] [%s cget -wraplength]", 
               this->GetWidgetName(), this->GetWidgetName());

  int width, wraplength;
  sscanf(this->Application->GetMainInterp()->result, 
         "%d %d", 
         &width, &wraplength);

  // Adjust the wraplength to width (within a tolerance so that it does
  // not put too much stress on the GUI).

  if (width < (wraplength - 5) || width > (wraplength + 5))
    {
    this->Script("%s config -wraplength %d", this->GetWidgetName(), width - 5);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::UpdateBindings()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->AdjustWrapLengthToWidth)
    {
    this->Script("bind %s <Configure> {%s AdjustWrapLengthToWidthCallback}",
                 this->GetWidgetName(), this->GetTclName());
    }
  else
    {
    this->Script("bind %s <Configure>", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Width: " << this->GetWidth() << endl;
  os << indent << "AdjustWrapLengthToWidth: " 
     << (this->AdjustWrapLengthToWidth ? "On" : "Off") << endl;
  os << indent << "Label: ";
  if (this->Label)
    {
    os << this->Label << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
}

