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
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCheckButton );
vtkCxxRevisionMacro(vtkKWCheckButton, "1.24");

//----------------------------------------------------------------------------
vtkKWCheckButton::vtkKWCheckButton() 
{
  this->IndicatorOn = 1;
  this->MyText = 0;
  this->VariableName = NULL;
}

//----------------------------------------------------------------------------
vtkKWCheckButton::~vtkKWCheckButton() 
{
  this->SetMyText(0);
  this->SetVariableName(0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetVariableName(const char* _arg)
{
  if (this->VariableName == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->VariableName && _arg && (!strcmp(this->VariableName, _arg))) 
    { 
    return;
    }

  int has_old_state = 0, old_state = 0;
  if (this->VariableName) 
    { 
    if (_arg)
      {
      has_old_state = 1;
      old_state = this->GetState();
      }
    delete [] this->VariableName; 
    }

  if (_arg)
    {
    this->VariableName = new char[strlen(_arg)+1];
    strcpy(this->VariableName,_arg);
    }
   else
    {
    this->VariableName = NULL;
    }

  this->Modified();
  
  if (this->IsCreated() && this->VariableName)
    {
    this->Script("%s configure -variable {%s}", 
                 this->GetWidgetName(), this->VariableName);
    if (has_old_state)
      {
      this->SetState(old_state);
      }
    }
} 

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetIndicator(int ind)
{
  if (ind != this->IndicatorOn)
    {
    this->IndicatorOn = ind;
    this->Modified();
    if (this->IsCreated())
      {
      this->Script("%s configure -indicatoron %d", 
                   this->GetWidgetName(), (ind ? 1 : 0));
      }
    }
  this->SetMyText(0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetText(const char* txt)
{
  this->SetMyText(txt);
  this->SetTextOption(txt);
}

//----------------------------------------------------------------------------
const char* vtkKWCheckButton::GetText()
{
  return this->MyText;
}

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetState()
{
  if (this->IsCreated())
    {
    this->Script("expr {${%s}} == {[%s cget -onvalue]}",
                 this->VariableName, this->GetWidgetName());
    return vtkKWObject::GetIntegerResult(this->Application);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetState(int s)
{
  if (this->IsCreated())
    {
    if (s)
      {
      this->Script("%s select",this->GetWidgetName());
      }
    else
      {
      this->Script("%s deselect",this->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("CheckButton already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  const char *wname = this->GetWidgetName();

  this->Script("checkbutton %s", wname);
  this->Configure();
  this->Script("%s configure %s", wname, (args?args:""));

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::Configure()
{
  const char *wname = this->GetWidgetName();

  this->Script("%s configure -indicatoron %d",
               wname, (this->IndicatorOn ? 1 : 0));

  this->SetTextOption(this->MyText);

  // Set the variable name if not set already
  if (!this->VariableName)
    {
    char *vname = new char [strlen(wname) + 5 + 1];
    sprintf(vname, "%sValue", wname);
    this->SetVariableName(vname);
    delete [] vname;
    }
  else
    {
    this->Script("%s configure -variable {%s}", wname, this->VariableName);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageOption(int icon_index,
                                      const char *blend_color_option,
                                      const char *image_option)
{
  this->Superclass::SetImageOption(
    icon_index, blend_color_option, image_option);
  if (!blend_color_option && !image_option)
    {
    this->Superclass::SetImageOption(
      icon_index, "-selectcolor", "-selectimage");
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageOption(vtkKWIcon* icon,
                                      const char *blend_color_option,
                                      const char *image_option)
{
  this->Superclass::SetImageOption(icon, blend_color_option, image_option);
  if (!blend_color_option && !image_option)
    {
    this->Superclass::SetImageOption(icon, "-selectcolor", "-selectimage");
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageOption(const unsigned char* data, 
                                      int width, 
                                      int height,
                                      int pixel_size,
                                      unsigned long buffer_length,
                                      const char *blend_color_option,
                                      const char *image_option)
{
  this->Superclass::SetImageOption(
    data, width, height, pixel_size, buffer_length, 
    blend_color_option, image_option);
  if (!blend_color_option && !image_option)
    {
    this->Superclass::SetImageOption(
      data, width, height, pixel_size, buffer_length, 
      "-selectcolor", "-selectimage");
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VariableName: " 
     << (this->VariableName ? this->VariableName : "None" );
}

