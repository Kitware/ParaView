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

#include <vtksys/stl/string>
#include <vtksys/stl/list>
#include <vtksys/RegularExpression.hxx>

const char *vtkKWText::MarkerBold = "**";
const char *vtkKWText::MarkerItalic = "~~";
const char *vtkKWText::MarkerUnderline = "__";

const char *vtkKWText::TagBold = "_bold_tag_";
const char *vtkKWText::TagItalic = "_italic_tag_";
const char *vtkKWText::TagUnderline = "_underline_tag_";
const char *vtkKWText::TagFgNavy = "_fg_navy_tag_";
const char *vtkKWText::TagFgRed = "_fg_red_tag_";
const char *vtkKWText::TagFgBlue = "_fg_blue_tag_";
const char *vtkKWText::TagFgDarkGreen = "_fg_dark_green_tag_";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWText);
vtkCxxRevisionMacro(vtkKWText, "1.42");

//----------------------------------------------------------------------------
class vtkKWTextInternals
{
public:

  class TagMatcher
  {
  public:
    vtksys_stl::string Regexp;
    vtksys_stl::string Tag;
  };

  typedef vtksys_stl::list<TagMatcher> TagMatchersContainer;
  typedef vtksys_stl::list<TagMatcher>::iterator TagMatchersContainerIterator;

  TagMatchersContainer TagMatchers;
};

//----------------------------------------------------------------------------
vtkKWText::vtkKWText()
{
  this->InternalValueString = NULL;
  this->ReadOnly            = 0;
  this->QuickFormatting     = 0;

  this->Internals = new vtkKWTextInternals;
}

//----------------------------------------------------------------------------
vtkKWText::~vtkKWText()
{
  this->SetInternalValueString(NULL);

  // Delete all presets

  if (this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
char *vtkKWText::GetValue()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  const char *val = this->Script("%s get 1.0 {end -1 chars}", 
                                 this->GetWidgetName());
  this->SetInternalValueString(this->ConvertTclStringToInternalString(val));
  return this->GetInternalValueString();
}

//----------------------------------------------------------------------------
void vtkKWText::SetValue(const char *s)
{
  this->SetValue(s, NULL);
}

//----------------------------------------------------------------------------
void vtkKWText::SetValue(const char *s, const char *tag)
{
  if (!this->IsCreated() || !s)
    {
    return;
    }

  // Delete everything

  int state = this->GetState();
  this->SetStateToNormal();

  this->Script("%s delete 1.0 end", this->GetWidgetName());

  this->SetState(state);

  // Append to the end

  this->AppendValue(s, tag);
}

//----------------------------------------------------------------------------
void vtkKWText::AppendValue(const char *s)
{
  this->AppendValue(s, NULL);
}

//----------------------------------------------------------------------------
void vtkKWText::AppendValue(const char *s, const char *tag)
{
  if (!this->IsCreated() || !s)
    {
    return;
    }

  int state = this->GetState();
  this->SetStateToNormal();

  this->AppendValueInternalTagging(s, tag);
  
  this->SetState(state);
}

//----------------------------------------------------------------------------
void vtkKWText::AppendValueInternalTagging(const char *str, const char *tag)
{
  // Don't check for this->Created() for speed, since it is called
  // by AppendValue which does the check already

  // In QuickFormatting mode, look for markers, and use tags accordingly

  if (this->QuickFormatting)
    {
    const int nb_markers = 3;
    const char* markertag[nb_markers * 2] = 
      {
        vtkKWText::MarkerBold, vtkKWText::TagBold,
        vtkKWText::MarkerItalic, vtkKWText::TagItalic,
        vtkKWText::MarkerUnderline, vtkKWText::TagUnderline
      };

    // First find the closest known marker

    const char *closest_marker = NULL;
    int i, closest_marker_id = -1;
    for (i = 0; i < nb_markers; i++)
      {
      const char *find_marker = strstr(str, markertag[i * 2]);
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
      const char *end_marker = 
        strstr(closest_marker + len_marker, markertag[closest_marker_id * 2]);
      if (end_marker)
        {
        // Text before the marker, using the current tag

        vtksys_stl::string before;
        before.append(str, closest_marker - str);
        this->AppendValueInternalTagging(before.c_str(), tag);

        // Zone inside the marker, using the current tag + the marker's tag

        vtksys_stl::string new_tag;
        if (tag)
          {
          new_tag.append(tag);
          }
        new_tag.append(" ").append(markertag[closest_marker_id * 2 + 1]);
        vtksys_stl::string zone;
        zone.append(closest_marker + len_marker, 
                    end_marker - closest_marker - len_marker);
        this->AppendValueInternalTagging(zone.c_str(), new_tag.c_str());

        // Text after the marker, using the current tag

        vtksys_stl::string after;
        after.append(end_marker + len_marker);
        this->AppendValueInternalTagging(after.c_str(), tag);

        return;
        }
      }
    }

  // The tag matchers

  vtkKWTextInternals::TagMatchersContainerIterator it = 
    this->Internals->TagMatchers.begin();
  vtkKWTextInternals::TagMatchersContainerIterator end = 
    this->Internals->TagMatchers.end();
  int found_regexp = 0;
  for (; it != end; ++it)
    {
    vtksys::RegularExpression re((*it).Regexp.c_str());
    if (re.find(str))
      {
      // Text before the regexp, using the current tag

      vtksys_stl::string before;
      before.append(str, re.start());

      // Zone inside the regexp, using the current tag + the marker's tag

      vtksys_stl::string new_tag;
      if (tag)
        {
        new_tag.append(tag);
        }
      new_tag.append(" ").append((*it).Tag);
      vtksys_stl::string zone;
      zone.append(str + re.start(), re.end() - re.start());

      // Text after the regexp, using the current tag

      vtksys_stl::string after;
      after.append(str + re.end());

      this->AppendValueInternalTagging(before.c_str(), tag);
      this->AppendValueInternal(zone.c_str(), new_tag.c_str());
      this->AppendValueInternalTagging(after.c_str(), tag);
      found_regexp = 1;
      break;
      }
    }

  if (!found_regexp)
    {
    this->AppendValueInternal(str, tag);
    }
}

//----------------------------------------------------------------------------
void vtkKWText::AppendValueInternal(const char *s, const char *tag)
{
  // Don't check for this->Created() for speed, since it is called
  // by AppendValue which does the check already

  const char *val = this->ConvertInternalStringToTclString(
    s, vtkKWCoreWidget::ConvertStringEscapeInterpretable);

  this->Script("%s insert end \"%s\" %s", 
               this->GetWidgetName(),
               val ? val : "", tag ? tag : "");
}

//----------------------------------------------------------------------------
void vtkKWText::Create(vtkKWApplication *app)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(app, "text"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetWidth(20);
  this->SetWrapToWord();

  // Create the default tags

  const char *wname = this->GetWidgetName();
  vtksys_stl::string font(this->GetConfigurationOption("-font"));

  char bold_font[512], italic_font[512];
  vtkKWTkUtilities::ChangeFontWeightToBold(
    app->GetMainInterp(), font.c_str(), bold_font);
  vtkKWTkUtilities::ChangeFontSlantToItalic(
    app->GetMainInterp(), font.c_str(), italic_font);

  this->Script("%s tag config %s -font \"%s\"", 
               wname, vtkKWText::TagBold, bold_font);

  this->Script("%s tag config %s -font \"%s\"", 
               wname, vtkKWText::TagItalic, italic_font);

  this->Script("%s tag config %s -underline 1", 
               wname, vtkKWText::TagUnderline);

  this->Script("%s tag config %s -foreground #000080", 
               wname, vtkKWText::TagFgNavy);

  this->Script("%s tag config %s -foreground #FF0000", 
               wname, vtkKWText::TagFgRed);

  this->Script("%s tag config %s -foreground #0000FF", 
               wname, vtkKWText::TagFgBlue);

  this->Script("%s tag config %s -foreground #006400", 
               wname, vtkKWText::TagFgDarkGreen);
  
  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWText::SetReadOnly(int arg)
{
  if (this->ReadOnly == arg)
    {
    return;
    }

  this->ReadOnly = arg;
  this->UpdateEnableState();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWText::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWText::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWText::SetHeight(int height)
{
  this->SetConfigurationOptionAsInt("-height", height);
}

//----------------------------------------------------------------------------
int vtkKWText::GetHeight()
{
  return this->GetConfigurationOptionAsInt("-height");
}

//----------------------------------------------------------------------------
void vtkKWText::SetWrapToNone()
{
  this->SetConfigurationOption("-wrap", "none");
}

//----------------------------------------------------------------------------
void vtkKWText::SetWrapToChar()
{
  this->SetConfigurationOption("-wrap", "char");
}

//----------------------------------------------------------------------------
void vtkKWText::SetWrapToWord()
{
  this->SetConfigurationOption("-wrap", "word");
}

//----------------------------------------------------------------------------
void vtkKWText::SetResizeToGrid(int arg)
{
  this->SetConfigurationOptionAsInt("-setgrid", arg);
}

//----------------------------------------------------------------------------
int vtkKWText::GetResizeToGrid()
{
  return this->GetConfigurationOptionAsInt("-setgrid");
}

//----------------------------------------------------------------------------
void vtkKWText::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->ReadOnly ? this->GetEnabled() : 0);
}

//----------------------------------------------------------------------------
void vtkKWText::AddTagMatcher(const char *regexp, const char *tag)
{
  if (!this->Internals || !regexp || !tag)
    {
    return;
    }

  vtkKWTextInternals::TagMatcher tagmatcher;
  tagmatcher.Regexp = regexp;
  tagmatcher.Tag = tag;
  this->Internals->TagMatchers.push_back(tagmatcher);
} 

//----------------------------------------------------------------------------
void vtkKWText::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ReadOnly: " 
     << (this->ReadOnly ? "On" : "Off") << endl;
  os << indent << "QuickFormatting: " 
     << (this->QuickFormatting ? "On" : "Off") << endl;
}
