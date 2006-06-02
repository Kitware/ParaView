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
#include "vtkKWTree.h"

#include "vtkKWApplication.h"
#include "vtkKWOptions.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWIcon.h"

#include <vtksys/stl/string>
#include <vtksys/stl/vector>
#include <vtksys/SystemTools.hxx>

#include "Utilities/BWidgets/vtkKWBWidgetsInit.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTree );
vtkCxxRevisionMacro(vtkKWTree, "1.22");

//----------------------------------------------------------------------------
class vtkKWTreeInternals
{
public:
  
  // Some temporary storage var that do not need to be exposed in the .h
  // This is used so that method that are wrapped can return a pointer
  // to a safer location than a static buffer.

  double SelectionBackgroundColorTemp[3];
  double SelectionForegroundColorTemp[3];
};

//----------------------------------------------------------------------------
vtkKWTree::vtkKWTree()
{
  this->SelectionMode = vtkKWOptions::SelectionModeSingle;
  this->SelectionChangedCommand = NULL;

  this->Internals = new vtkKWTreeInternals;
}

//----------------------------------------------------------------------------
vtkKWTree::~vtkKWTree()
{
  if (this->SelectionChangedCommand)
    {
    delete [] this->SelectionChangedCommand;
    this->SelectionChangedCommand = NULL;
    }

   delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkKWTree::CreateWidget()
{
  // Use BWidget's Tree class:
  // http://aspn.activestate.com/ASPN/docs/ActiveTcl/bwidget/contents.html

  vtkKWApplication *app = this->GetApplication();
  vtkKWBWidgetsInit::Initialize(app ? app->GetMainInterp() : NULL);

  // Call the superclass to create the widget and set the appropriate flags

  if (!vtkKWWidget::CreateSpecificTkWidget(this, 
        "Tree", "-relief flat -bd 0 -highlightthickness 0 -padx 2"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetBinding("<<TreeSelect>>", this, "SelectionCallback");
}

//----------------------------------------------------------------------------
void vtkKWTree::SelectionCallback()
{
  // This widget does not support multiple or single selection mode
  // natively. Let's take care of that.

  static int in_SelectionCallback = 0;
  if (in_SelectionCallback)
    {
    return;
    }
  in_SelectionCallback = 1;

  if (this->SelectionMode == vtkKWOptions::SelectionModeSingle)
    {
    vtksys_stl::vector<vtksys_stl::string> sel_nodes;
    vtksys::SystemTools::Split(this->GetSelection(), sel_nodes, ' ');
    if (sel_nodes.size() > 1)
      {
      this->SelectSingleNode(sel_nodes[sel_nodes.size() - 1].c_str());
      // We need to return now actually. A selection event will
      // still go through as we select that single node, and will
      // call that callback again, triggering InvokeSelectionChangedCommand.
      // It does not seem to be possible to avoid that event to be triggered
      in_SelectionCallback = 0;
      return;
      }
    }

  this->InvokeSelectionChangedCommand();

  in_SelectionCallback = 0;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSelectionMode(int arg)
{
  if ((arg != vtkKWOptions::SelectionModeSingle &&
       arg != vtkKWOptions::SelectionModeMultiple) ||
      arg == this->SelectionMode)
    {
    return;
    }

  this->SelectionMode = arg;

  // If we are switching to single mode, select the first node only

  if (this->SelectionMode == vtkKWOptions::SelectionModeSingle &&
      this->HasSelection())
    {
    vtksys_stl::vector<vtksys_stl::string> sel_nodes;
    vtksys::SystemTools::Split(this->GetSelection(), sel_nodes, ' ');
    this->SelectSingleNode(sel_nodes[0].c_str());
    }
}

void vtkKWTree::SetSelectionModeToSingle() 
{ 
  this->SetSelectionMode(vtkKWOptions::SelectionModeSingle); 
};
void vtkKWTree::SetSelectionModeToMultiple() 
{ 
  this->SetSelectionMode(vtkKWOptions::SelectionModeMultiple); 
};

//----------------------------------------------------------------------------
void vtkKWTree::SetSelectionChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->SelectionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWTree::InvokeSelectionChangedCommand()
{
  this->InvokeObjectMethodCommand(this->SelectionChangedCommand);
}

//----------------------------------------------------------------------------
void vtkKWTree::SelectNode(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s selection add %s", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::DeselectNode(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s selection remove %s", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SelectNodeChildren(const char *node)
{
  vtksys_stl::vector<vtksys_stl::string> children;
  vtksys::SystemTools::Split(this->GetNodeChildren(node), children, ' ');
  vtksys_stl::vector<vtksys_stl::string>::iterator it = children.begin();
  vtksys_stl::vector<vtksys_stl::string>::iterator end = children.end();
  for (; it != end; it++)
    {
    this->SelectNode((*it).c_str());
    this->SelectNodeChildren((*it).c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::DeselectNodeChildren(const char *node)
{
  vtksys_stl::vector<vtksys_stl::string> children;
  vtksys::SystemTools::Split(this->GetNodeChildren(node), children, ' ');
  vtksys_stl::vector<vtksys_stl::string>::iterator it = children.begin();
  vtksys_stl::vector<vtksys_stl::string>::iterator end = children.end();
  for (; it != end; it++)
    {
    this->DeselectNode((*it).c_str());
    this->DeselectNodeChildren((*it).c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SelectSingleNode(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s selection set %s", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::ClearSelection()
{
  if (this->IsCreated())
    {
    this->Script("%s selection clear", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetSelection()
{
  if (this->IsCreated())
    {
    return this->Script("%s selection get", this->GetWidgetName());
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWTree::HasSelection()
{
  const char *sel = this->GetSelection();
  return (sel && *sel ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWTree::AddNode(const char *parent,
                        const char *node,
                        const char *text)
{
  if (!this->IsCreated() || !node)
    {
    return;
    }

  vtksys_stl::string cmd;

  cmd.append(this->GetWidgetName()).append(" insert end ").append(parent && *parent ? parent : "root").append(" ").append(node);

  if (text && *text)
    {
    const char *val = this->ConvertInternalStringToTclString(
      text, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
    cmd.append(" -text \"").append(val).append("\"");
    }

  vtkKWTkUtilities::EvaluateSimpleString(
    this->GetApplication(), cmd.c_str());
}

//----------------------------------------------------------------------------
void vtkKWTree::SeeNode(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s see %s", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::OpenNode(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s opentree %s 0", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::CloseNode(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s closetree %s 0", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
int vtkKWTree::IsNodeOpen(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return atoi(
      this->Script("%s itemcget %s -open", this->GetWidgetName(), node));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWTree::OpenFirstNode()
{
  if (this->IsCreated())
    {
    this->Script("catch {%s opentree [lindex [%s nodes root] 0]}", 
                 this->GetWidgetName(), this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::CloseFirstNode()
{
  if (this->IsCreated())
    {
    this->Script("catch {%s closetree [lindex [%s nodes root] 0]}", 
                 this->GetWidgetName(), this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::OpenTree(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s opentree %s 1", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::CloseTree(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s closetree %s 1", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
int vtkKWTree::HasNode(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return atoi(this->Script("%s exists %s", this->GetWidgetName(), node));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWTree::DeleteAllNodes()
{
  if (this->IsCreated())
    {
    this->Script("%s delete [%s nodes root]", 
                 this->GetWidgetName(), this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeChildren(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return this->Script("%s nodes %s", this->GetWidgetName(), node);
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeParent(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return this->Script("%s parent %s", this->GetWidgetName(), node);
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeUserData(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return this->ConvertTclStringToInternalString(
      this->Script("%s itemcget %s -data", this->GetWidgetName(), node));
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeUserData(const char *node, const char *data)
{
  if (this->IsCreated() && node && *node && data)
    {
    const char *val = this->ConvertInternalStringToTclString(
      data, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
    this->Script("%s itemconfigure %s -data \"%s\"", 
                 this->GetWidgetName(), node, val);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeText(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return this->ConvertTclStringToInternalString(
      this->Script("%s itemcget %s -text", this->GetWidgetName(), node));
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeText(const char *node, const char *text)
{
  if (this->IsCreated() && node && *node && text)
    {
    const char *val = this->ConvertInternalStringToTclString(
      text, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
    this->Script("%s itemconfigure %s -text \"%s\"", 
                 this->GetWidgetName(), node, val);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeFont(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return this->ConvertTclStringToInternalString(
      this->Script("%s itemcget %s -font", this->GetWidgetName(), node));
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeFont(const char *node, const char *font)
{
  if (this->IsCreated() && node && *node && font)
    {
    const char *val = this->ConvertInternalStringToTclString(
      font, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
    this->Script("%s itemconfigure %s -font \"%s\"", 
                 this->GetWidgetName(), node, val);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeFontWeightToBold(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    char new_font[1024];
    vtksys_stl::string font(this->GetNodeFont(node));
    vtkKWTkUtilities::ChangeFontWeightToBold(
      this->GetApplication()->GetMainInterp(), font.c_str(), new_font);
    this->SetNodeFont(node, new_font);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeFontWeightToNormal(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    char new_font[1024];
    vtksys_stl::string font(this->GetNodeFont(node));
    vtkKWTkUtilities::ChangeFontWeightToNormal(
      this->GetApplication()->GetMainInterp(), font.c_str(), new_font);
    this->SetNodeFont(node, new_font);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeFontSlantToItalic(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    char new_font[1024];
    vtksys_stl::string font(this->GetNodeFont(node));
    vtkKWTkUtilities::ChangeFontSlantToItalic(
      this->GetApplication()->GetMainInterp(), font.c_str(), new_font);
    this->SetNodeFont(node, new_font);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeFontSlantToRoman(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    char new_font[1024];
    vtksys_stl::string font(this->GetNodeFont(node));
    vtkKWTkUtilities::ChangeFontSlantToRoman(
      this->GetApplication()->GetMainInterp(), font.c_str(), new_font);
    this->SetNodeFont(node, new_font);
    }
}

//----------------------------------------------------------------------------
int vtkKWTree::GetNodeSelectableFlag(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return atoi(this->Script("%s itemcget %s -selectable", 
                             this->GetWidgetName(), node));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeSelectableFlag(const char *node, int flag)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s itemconfigure %s -selectable %d", 
                 this->GetWidgetName(), node, flag);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeImageToPredefinedIcon(const char *node, int icon_index)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(icon_index);
  this->SetNodeImageToIcon(node, icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeImageToIcon(const char *node, vtkKWIcon* icon)
{
  if (icon)
    {
    this->SetNodeImageToPixels(
      node,
      icon->GetData(), 
      icon->GetWidth(), icon->GetHeight(), icon->GetPixelSize());
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeImageToPixels(const char *node,
                                     const unsigned char* pixels, 
                                     int width, 
                                     int height,
                                     int pixel_size,
                                     unsigned long buffer_length)
{
  if (!this->IsCreated() || !node || !*node || !pixels)
    {
    return;
    }

  // Use the prev pic, or create a new one

  int had_no_image = 0;
  vtksys_stl::string image_name(
    this->Script("%s itemcget %s -image", this->GetWidgetName(), node));
  if (!image_name.size())
    {
    image_name = this->GetWidgetName();
    image_name += "_";
    image_name += node;
    had_no_image = 1;
    }

  if (!vtkKWTkUtilities::UpdatePhoto(this->GetApplication(),
                                     image_name.c_str(),
                                     pixels, 
                                     width, height, pixel_size,
                                     buffer_length))
    {
    vtkWarningMacro(
      << "Error updating Tk photo " << image_name.c_str());
    }

  if (had_no_image)
    {
    this->Script("%s itemconfigure %s -image {%s}", 
                 this->GetWidgetName(), node, image_name.c_str());
    }
}

//----------------------------------------------------------------------------
int vtkKWTree::GetNodePadX(const char *node)
{
  if (this->IsCreated() && node && *node)
    {
    return atoi(this->Script("%s itemcget %s -padx", 
                             this->GetWidgetName(), node));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodePadX(const char *node, int arg)
{
  if (this->IsCreated() && node && *node)
    {
    this->Script("%s itemconfigure %s -padx %d", 
                 this->GetWidgetName(), node, arg);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWTree::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWTree::SetHeight(int height)
{
  this->SetConfigurationOptionAsInt("-height", height);
}

//----------------------------------------------------------------------------
int vtkKWTree::GetHeight()
{
  return this->GetConfigurationOptionAsInt("-height");
}

//----------------------------------------------------------------------------
void vtkKWTree::SetRedrawOnIdle(int redraw)
{
  this->SetConfigurationOptionAsInt("-redraw", redraw);
}

//----------------------------------------------------------------------------
int vtkKWTree::GetRedrawOnIdle()
{
  return this->GetConfigurationOptionAsInt("-redraw");
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSelectionFill(int arg)
{
  this->SetConfigurationOptionAsInt("-selectfill", arg);
}

//----------------------------------------------------------------------------
int vtkKWTree::GetSelectionFill()
{
  return this->GetConfigurationOptionAsInt("-selectfill");
}

//----------------------------------------------------------------------------
void vtkKWTree::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWTree::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWTree::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTree::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWTree::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWTree::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWTree::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWTree::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWTree::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWTree::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWTree::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWTree::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWTree::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWTree::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWTree::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWTree::SetPadX(int arg)
{
  this->SetConfigurationOptionAsInt("-padx", arg);
}

//----------------------------------------------------------------------------
int vtkKWTree::GetPadX()
{
  return this->GetConfigurationOptionAsInt("-padx");
}

//----------------------------------------------------------------------------
void vtkKWTree::GetSelectionBackgroundColor(double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-selectbackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWTree::GetSelectionBackgroundColor()
{
  double *rgb = &this->Internals->SelectionBackgroundColorTemp[0];
  this->GetSelectionBackgroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSelectionBackgroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-selectbackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTree::GetSelectionForegroundColor(double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-selectforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWTree::GetSelectionForegroundColor()
{
  double *rgb = &this->Internals->SelectionForegroundColorTemp[0];
  this->GetSelectionForegroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSelectionForegroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-selectforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTree::SetOpenCommand(vtkObject *object, const char *method)
{
  if (!this->IsCreated())
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);
  this->SetConfigurationOption("-opencmd", command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetCloseCommand(vtkObject *object, const char *method)
{
  if (!this->IsCreated())
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);
  this->SetConfigurationOption("-closecmd", command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetBindText(const char *event, 
                            vtkObject *object, 
                            const char *method)
{
  if (!this->IsCreated() || !event)
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);
  this->Script("%s bindText %s {%s}", this->GetWidgetName(), event, command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetDoubleClickOnNodeCommand(vtkObject *object, 
                                            const char *method)
{
  this->SetBindText("<Double-ButtonPress-1>", object, method);
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSingleClickOnNodeCommand(vtkObject *object, 
                                            const char *method)
{
  this->SetBindText("<ButtonPress-1>", object, method);
}

//----------------------------------------------------------------------------
void vtkKWTree::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
