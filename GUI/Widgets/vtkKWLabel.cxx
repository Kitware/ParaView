/*=========================================================================

  Module:    vtkKWLabel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"


int vtkKWLabelCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLabel );
vtkCxxRevisionMacro(vtkKWLabel, "1.33");

//----------------------------------------------------------------------------
vtkKWLabel::vtkKWLabel()
{
  this->CommandFunction         = vtkKWLabelCommand;

  this->Text                    = NULL;
  this->LineType                = vtkKWLabel::SingleLine;
  this->Width                   = 0;
  this->AdjustWrapLengthToWidth = 0;
}

//----------------------------------------------------------------------------
vtkKWLabel::~vtkKWLabel()
{
  if (this->Text)
    {
    delete [] this->Text;
    this->Text = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetText(const char* _arg)
{
  if (this->Text == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->Text && _arg && (!strcmp(this->Text, _arg))) 
    {
    return;
    }

  if (this->Text) 
    { 
    delete [] this->Text; 
    }

  if (_arg)
    {
    this->Text = new char[strlen(_arg) + 1];
    strcpy(this->Text, _arg);
    }
  else
    {
    this->Text = NULL;
    }

  this->Modified();

  this->UpdateText();
} 

//----------------------------------------------------------------------------
void vtkKWLabel::UpdateText()
{
  if (this->IsCreated())
    {
    this->SetTextOption(this->Text); // NULL is handled correctly as ""

    // Whatever the label, -image always takes precedence, unless it's empty
    // so change it accordingly
    
    if (this->LineType != vtkKWLabel::MultiLine && 
        this->Text && *this->Text)
      {
      this->Script("%s configure -image {}", this->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

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
  this->ConfigureOptions(args);

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
      this->SetText(this->GetTextOption());
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
   sscanf(this->GetApplication()->GetMainInterp()->result, 
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

// ---------------------------------------------------------------------------
void vtkKWLabel::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->LineType != vtkKWLabel::MultiLine)
    {
    this->SetStateOption(this->Enabled);
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
  if (this->Text)
    {
    os << this->Text << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
}

