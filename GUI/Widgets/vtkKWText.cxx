/*=========================================================================

  Module:    vtkKWText.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWText.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include <vtkstd/string>

#define VTK_KWTEXT_BOLD_MARKER "**"
#define VTK_KWTEXT_BOLD_TAG "_boldtag_"

#define VTK_KWTEXT_ITALIC_MARKER "~~"
#define VTK_KWTEXT_ITALIC_TAG "_italictag_"

#define VTK_KWTEXT_UNDERLINE_MARKER "__"
#define VTK_KWTEXT_UNDERLINE_TAG "_underlinetag_"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWText);
vtkCxxRevisionMacro(vtkKWText, "1.27");

//----------------------------------------------------------------------------
vtkKWText::vtkKWText()
{
  this->TextWidget           = NULL;
  this->ValueString          = NULL;
  this->VerticalScrollBar    = NULL;
  this->EditableText         = 1;
  this->QuickFormatting      = 0;
  this->UseVerticalScrollbar = 0;
}

//----------------------------------------------------------------------------
vtkKWText::~vtkKWText()
{
  if (this->TextWidget)
    {
    this->TextWidget->Delete();
    this->TextWidget = NULL;
    }

  if (this->VerticalScrollBar)
    {
    this->VerticalScrollBar->Delete();
    this->VerticalScrollBar = NULL;
    }

  this->SetValueString(NULL);
}

//----------------------------------------------------------------------------
char *vtkKWText::GetValue()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  const char *val = this->Script("%s get 1.0 {end -1 chars}", 
                                 this->TextWidget->GetWidgetName());
  this->SetValueString(this->ConvertTclStringToInternalString(val));
  return this->GetValueString();
}

//----------------------------------------------------------------------------
void vtkKWText::SetValue(const char *s, const char *tag)
{
  if (!this->IsCreated() || !s)
    {
    return;
    }

  // Delete everything

  int state = this->TextWidget->GetStateOption();
  this->TextWidget->SetStateOption(1);

  this->Script("%s delete 1.0 end", this->TextWidget->GetWidgetName());

  this->TextWidget->SetStateOption(state);

  // Append to the end

  this->AppendValue(s, tag);
}

//----------------------------------------------------------------------------
void vtkKWText::AppendValue(const char *s, const char *tag)
{
  if (!this->IsCreated() || !s)
    {
    return;
    }

  int state = this->TextWidget->GetStateOption();
  this->TextWidget->SetStateOption(1);

  vtkstd::string str(this->ConvertInternalStringToTclString(s));

  // In QuickFormatting mode, look for markers, and use tags accordingly

  if (this->QuickFormatting)
    {
    const int nb_markers = 3;
    const char* markertag[nb_markers * 2] = 
      {
        VTK_KWTEXT_BOLD_MARKER, VTK_KWTEXT_BOLD_TAG,
        VTK_KWTEXT_ITALIC_MARKER, VTK_KWTEXT_ITALIC_TAG,
        VTK_KWTEXT_UNDERLINE_MARKER, VTK_KWTEXT_UNDERLINE_TAG
      };

    // First find the closest known marker

    char *closest_marker = NULL;
    int i, closest_marker_id = -1;
    for (i = 0; i < nb_markers; i++)
      {
      char *find_marker = strstr(str.c_str(), markertag[i * 2]);
      if (find_marker && (!closest_marker || find_marker < closest_marker))
        {
        closest_marker = find_marker;
        closest_marker_id = i;
        }
      }

    // Then find its counterpart end marker, if any

    if (closest_marker)
      {
      int len_marker = strlen(markertag[closest_marker_id * 2]);
      char *end_marker = 
        strstr(closest_marker + len_marker, markertag[closest_marker_id * 2]);
      if (end_marker)
        {
        // Text before the marker, using the current tag

        vtkstd::string before;
        before.append(str.c_str(), closest_marker - str.c_str());
        this->AppendValue(before.c_str(), tag);

        // Zone inside the marker, using the current tag + the marker's tag

        vtkstd::string new_tag;
        if (tag)
          {
          new_tag.append(tag);
          }
        new_tag.append(" ").append(markertag[closest_marker_id * 2 + 1]);
        vtkstd::string zone;
        zone.append(closest_marker + len_marker, 
                    end_marker - closest_marker - len_marker);
        this->AppendValue(zone.c_str(), new_tag.c_str());

        // Text after the marker, using the current tag

        vtkstd::string after;
        after.append(end_marker + len_marker);
        this->AppendValue(after.c_str(), tag);

        return;
        }
      }
    }

  this->Script("catch {%s insert end {%s} %s}", 
               this->TextWidget->GetWidgetName(),
               str.c_str(),
               tag ? tag : "");
  
  this->TextWidget->SetStateOption(state);
}

//----------------------------------------------------------------------------
void vtkKWText::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Create the text

  if (!this->TextWidget)
    {
    this->TextWidget = vtkKWWidget::New();
    }

  vtkstd::string all_args("-width 20 -wrap word ");
  if (args)
    {
    all_args += args;
    }
  this->TextWidget->SetParent(this);
  this->TextWidget->Create(app, "text", all_args.c_str());

  // Create the default tags

  const char *wname = this->TextWidget->GetWidgetName();
  vtkstd::string font(this->Script("%s cget -font", wname));

  char bold_font[512], italic_font[512];
  vtkKWTkUtilities::ChangeFontWeightToBold(
    app->GetMainInterp(), font.c_str(), bold_font);
  vtkKWTkUtilities::ChangeFontSlantToItalic(
    app->GetMainInterp(), font.c_str(), italic_font);

  this->Script("%s tag config %s -font \"%s\"", 
               wname, VTK_KWTEXT_BOLD_TAG, bold_font);
  this->Script("%s tag config %s -font \"%s\"", 
               wname, VTK_KWTEXT_ITALIC_TAG, italic_font);
  this->Script("%s tag config %s -underline 1", 
               wname, VTK_KWTEXT_UNDERLINE_TAG);
  
  // Create the scrollbars

  if (this->UseVerticalScrollbar)
    {
    this->CreateVerticalScrollbar(app);
    }

  // Pack

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWText::CreateVerticalScrollbar(vtkKWApplication *app)
{
  if (!this->VerticalScrollBar)
    {
    this->VerticalScrollBar = vtkKWWidget::New();
    }

  if (!this->VerticalScrollBar->IsCreated())
    {
    this->VerticalScrollBar->SetParent(this);
    this->VerticalScrollBar->Create(app, "scrollbar", "-orient vertical");
    if (this->TextWidget && this->TextWidget->IsCreated())
      {
      vtkstd::string command("-command {");
      command += this->TextWidget->GetWidgetName();
      command += " yview}";
      this->VerticalScrollBar->ConfigureOptions(command.c_str());
      command = "-yscrollcommand {";
      command += this->VerticalScrollBar->GetWidgetName();
      command += " set}";
      this->TextWidget->ConfigureOptions(command.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWText::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->UnpackChildren();

  ostrstream tk_cmd;

  if (this->TextWidget && this->TextWidget->IsCreated())
    {
    tk_cmd << "pack " << this->TextWidget->GetWidgetName() 
           << " -side left -expand t -fill both" << endl;
    }

  if (this->UseVerticalScrollbar && 
      this->VerticalScrollBar && this->VerticalScrollBar->IsCreated())
    {
    tk_cmd << "pack " << this->VerticalScrollBar->GetWidgetName() 
           << " -side right -expand no -fill y -padx 2" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWText::SetUseVerticalScrollbar(int arg)
{
  if (this->UseVerticalScrollbar == arg)
    {
    return;
    }

  this->UseVerticalScrollbar = arg;
  if (this->UseVerticalScrollbar)
    {
    this->CreateVerticalScrollbar(this->GetApplication());
    }
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWText::SetEditableText(int arg)
{
  if (this->EditableText == arg)
    {
    return;
    }

  this->EditableText = arg;
  this->UpdateEnableState();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWText::SetWidth(int width)
{
  if (this->TextWidget && this->TextWidget->IsCreated())
    {
    this->Script("%s configure -width %d", 
                 this->TextWidget->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWText::GetWidth()
{
  if (this->TextWidget)
    {
    return this->TextWidget->GetConfigurationOptionAsInt("-width");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWText::SetHeight(int height)
{
  if (this->TextWidget && this->TextWidget->IsCreated())
    {
    this->Script("%s configure -height %d", 
                 this->TextWidget->GetWidgetName(), height);
    }
}

//----------------------------------------------------------------------------
int vtkKWText::GetHeight()
{
  if (this->TextWidget)
    {
    return this->TextWidget->GetConfigurationOptionAsInt("-height");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWText::SetWrapToNone()
{
  if (this->TextWidget && this->TextWidget->IsCreated())
    {
    this->Script("%s configure -wrap none", this->TextWidget->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWText::SetWrapToChar()
{
  if (this->TextWidget && this->TextWidget->IsCreated())
    {
    this->Script("%s configure -wrap char", this->TextWidget->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWText::SetWrapToWord()
{
  if (this->TextWidget && this->TextWidget->IsCreated())
    {
    this->Script("%s configure -wrap word", this->TextWidget->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWText::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->TextWidget)
    {
    this->TextWidget->SetStateOption(this->EditableText ? this->Enabled : 0);
    }
}

//----------------------------------------------------------------------------
void vtkKWText::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "EditableText: " 
     << (this->EditableText ? "On" : "Off") << endl;
  os << indent << "QuickFormatting: " 
     << (this->QuickFormatting ? "On" : "Off") << endl;
  os << indent << "UseVerticalScrollbar: " 
     << (this->UseVerticalScrollbar ? "On" : "Off") << endl;
  os << indent << "TextWidget: ";
  if (this->TextWidget)
    {
    os << this->TextWidget << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
  if (this->VerticalScrollBar)
    {
    os << this->VerticalScrollBar << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
}
