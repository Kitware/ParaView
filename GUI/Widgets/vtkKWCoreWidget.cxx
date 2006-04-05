/*=========================================================================

  Module:    vtkKWCoreWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWCoreWidget.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWIcon.h"
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCoreWidget );
vtkCxxRevisionMacro(vtkKWCoreWidget, "1.17");

//----------------------------------------------------------------------------
void vtkKWCoreWidget::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();
}

//----------------------------------------------------------------------------
int vtkKWCoreWidget::SetConfigurationOption(
  const char *option, const char *value)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  if (!option)
    {
    vtkWarningMacro("Missing option !");
    return 0;
    }

  const char *res = 
    this->Script("%s configure %s {%s}", 
                 this->GetWidgetName(), option, value ? value : "");

  // 'configure' is not supposed to return anything, so let's assume
  // any output is an error

  if (res && *res)
    {
    vtksys_stl::string err_msg(res);
    vtksys_stl::string tcl_name(this->GetTclName());
    vtksys_stl::string widget_name(this->GetWidgetName());
    vtksys_stl::string type(this->GetType());
    vtkErrorMacro(
      "Error configuring " << tcl_name.c_str() << " (" << type.c_str() << ": " 
      << widget_name.c_str() << ") with option: [" << option 
      << "] and value [" << value << "] => " << err_msg.c_str());
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWCoreWidget::HasConfigurationOption(const char *option)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  return (this->GetApplication() && 
          !this->GetApplication()->EvaluateBooleanExpression(
            "catch {%s cget %s}",
            this->GetWidgetName(), option));
}

//----------------------------------------------------------------------------
const char* vtkKWCoreWidget::GetConfigurationOption(const char *option)
{
  if (!this->HasConfigurationOption(option))
    {
    return NULL;
    }

  return this->Script("%s cget %s", this->GetWidgetName(), option);
}

//----------------------------------------------------------------------------
int vtkKWCoreWidget::SetConfigurationOptionAsInt(
  const char *option, int value)
{
  char buffer[20];
  sprintf(buffer, "%d", value);
  return this->SetConfigurationOption(option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWCoreWidget::GetConfigurationOptionAsInt(const char *option)
{
  if (!this->HasConfigurationOption(option))
    {
    return 0;
    }

  return atoi(this->Script("%s cget %s", this->GetWidgetName(), option));
}

//----------------------------------------------------------------------------
int vtkKWCoreWidget::SetConfigurationOptionAsDouble(
  const char *option, double value)
{
  char buffer[2048];
  sprintf(buffer, "%f", value);
  return this->SetConfigurationOption(option, buffer);
}

//----------------------------------------------------------------------------
double vtkKWCoreWidget::GetConfigurationOptionAsDouble(const char *option)
{
  if (!this->HasConfigurationOption(option))
    {
    return 0.0;
    }

  return atof(this->Script("%s cget %s", this->GetWidgetName(), option));
}

//----------------------------------------------------------------------------
void vtkKWCoreWidget::GetConfigurationOptionAsColor(
  const char *option, double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, option, r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWCoreWidget::GetConfigurationOptionAsColor(const char *option)
{
  static double rgb[3];
  this->GetConfigurationOptionAsColor(option, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWCoreWidget::SetConfigurationOptionAsColor(
  const char *option, double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, option, r, g, b);
}

//----------------------------------------------------------------------------
const char* vtkKWCoreWidget::ConvertInternalStringToTclString(
  const char *source, int options)
{
  if (!source || !this->IsCreated())
    {
    return NULL;
    }

  static vtksys_stl::string dest;
  const char *res = source;

  // Handle the encoding

  int app_encoding = this->GetApplication()->GetCharacterEncoding();
  if (app_encoding != VTK_ENCODING_NONE &&
      app_encoding != VTK_ENCODING_UNKNOWN)
    {
    // Get the Tcl encoding name

    const char *tcl_encoding_name = 
      vtkKWTkOptions::GetCharacterEncodingAsTclOptionValue(app_encoding);

    // Check if we have that encoding
    
    Tcl_Encoding tcl_encoding = 
      Tcl_GetEncoding(
        this->GetApplication()->GetMainInterp(), tcl_encoding_name);
    if (tcl_encoding != NULL)
      {
      Tcl_FreeEncoding(tcl_encoding);
      
      // Convert from that encoding
      // We need to escape interpretable chars to perform that conversion

      dest = vtksys::SystemTools::EscapeChars(source, "[]$\"\\");
      res = source = this->Script(
        "encoding convertfrom %s \"%s\"", tcl_encoding_name, dest.c_str());
      }
    }

  // Escape
  
  vtksys_stl::string escape_chars;
  if (options)
    {
    if (options & vtkKWCoreWidget::ConvertStringEscapeCurlyBraces)
      {
      escape_chars += "{}";
      }
    if (options & vtkKWCoreWidget::ConvertStringEscapeInterpretable)
      {
      escape_chars += "[]$\"\\";
      }
    dest = 
      vtksys::SystemTools::EscapeChars(source, escape_chars.c_str());
    res = source = dest.c_str();
    }

  return res;
}

//----------------------------------------------------------------------------
const char* vtkKWCoreWidget::ConvertTclStringToInternalString(
  const char *source, int options)
{
  if (!source || !this->IsCreated())
    {
    return NULL;
    }

  static vtksys_stl::string dest;
  const char *res = source;

  // Handle the encoding

  int app_encoding = this->GetApplication()->GetCharacterEncoding();
  if (app_encoding != VTK_ENCODING_NONE &&
      app_encoding != VTK_ENCODING_UNKNOWN)
    {
    // Convert from that encoding
    // We need to escape interpretable chars to perform that conversion

    dest = vtksys::SystemTools::EscapeChars(source, "[]$\"\\");
    res = source = this->Script(
      "encoding convertfrom identity \"%s\"", dest.c_str());
    }
  
  // Escape
  
  vtksys_stl::string escape_chars;
  if (options)
    {
    if (options & vtkKWCoreWidget::ConvertStringEscapeCurlyBraces)
      {
      escape_chars += "{}";
      }
    if (options & vtkKWCoreWidget::ConvertStringEscapeInterpretable)
      {
      escape_chars += "[]$\"\\";
      }
    dest = 
      vtksys::SystemTools::EscapeChars(source, escape_chars.c_str());
    res = source = dest.c_str();
    }

  return res;
}

//----------------------------------------------------------------------------
void vtkKWCoreWidget::SetTextOption(const char *option, const char *value)
{
  if (!option || !this->IsCreated())
    {
    return;
    }

  const char *val = this->ConvertInternalStringToTclString(
    value, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
  this->Script("%s configure %s \"%s\"", 
               this->GetWidgetName(), option, val ? val : "");
}

//----------------------------------------------------------------------------
const char* vtkKWCoreWidget::GetTextOption(const char *option)
{
  if (!option || !this->IsCreated())
    {
    return "";
    }

  return this->ConvertTclStringToInternalString(
    this->GetConfigurationOption(option));
}

//----------------------------------------------------------------------------
void vtkKWCoreWidget::SetState(int state)
{
  if (this->IsAlive())
    {
    this->SetConfigurationOption(
      "-state", vtkKWTkOptions::GetStateAsTkOptionValue(state));
    }
}

//----------------------------------------------------------------------------
int vtkKWCoreWidget::GetState()
{
  if (this->IsAlive())
    {
    return vtkKWTkOptions::GetStateFromTkOptionValue(
      this->GetConfigurationOption("-state"));
    }
  return vtkKWTkOptions::StateUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWCoreWidget::GetType()
{
  const char *res = vtkKWTkUtilities::GetWidgetClass(this);
  if (!res)
    {
    return "None";
    }
  return res;
}

//----------------------------------------------------------------------------
void vtkKWCoreWidget::Raise()
{
  if (this->IsCreated())
    {
    this->Script("raise %s", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWCoreWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

