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
#include "vtkKWTree.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTree );
vtkCxxRevisionMacro(vtkKWTree, "1.8");

//----------------------------------------------------------------------------
void vtkKWTree::Create(vtkKWApplication *app)
{
  // Use BWidget's Tree class:
  // http://aspn.activestate.com/ASPN/docs/ActiveTcl/bwidget/Tree.html

  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::CreateSpecificTkWidget(app, "KWTree"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetReliefToFlat();
  this->SetBorderWidth(0);
  this->SetHighlightThickness(0);

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSelectionToNode(const char *node)
{
  if (this->IsCreated() && node)
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
                        const char *text,
                        const char *data,
                        int is_open,
                        int is_selectable)
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
      text, vtkKWWidget::ConvertStringEscapeInterpretable);
    cmd.append(" -text \"").append(val).append("\"");
    }
  if (data && *data)
    {
    const char *val = this->ConvertInternalStringToTclString(
      data, vtkKWWidget::ConvertStringEscapeInterpretable);
    cmd.append(" -data \"").append(val).append("\"");
    }
  cmd.append(" -open ").append(is_open ? "1" : "0");
  cmd.append(" -selectable ").append(is_selectable ? "1" : "0");

  vtkKWTkUtilities::EvaluateSimpleString(
    this->GetApplication(), cmd.c_str());
}

//----------------------------------------------------------------------------
void vtkKWTree::SeeNode(const char *node)
{
  if (this->IsCreated() && node)
    {
    this->Script("%s see %s", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::OpenNode(const char *node)
{
  if (this->IsCreated() && node)
    {
    this->Script("%s opentree %s 0", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::CloseNode(const char *node)
{
  if (this->IsCreated() && node)
    {
    this->Script("%s closetree %s 0", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
int vtkKWTree::IsNodeOpen(const char *node)
{
  if (this->IsCreated() && node)
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
  if (this->IsCreated() && node)
    {
    this->Script("%s opentree %s 1", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::CloseTree(const char *node)
{
  if (this->IsCreated() && node)
    {
    this->Script("%s closetree %s 1", this->GetWidgetName(), node);
    }
}

//----------------------------------------------------------------------------
int vtkKWTree::HasNode(const char *node)
{
  if (this->IsCreated() && node)
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
  if (this->IsCreated() && node)
    {
    return this->Script("%s nodes %s", this->GetWidgetName(), node);
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeParent(const char *node)
{
  if (this->IsCreated() && node)
    {
    return this->Script("%s parent %s", this->GetWidgetName(), node);
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeUserData(const char *node)
{
  if (this->IsCreated() && node)
    {
    return this->ConvertTclStringToInternalString(
      this->Script("%s itemcget %s -data", this->GetWidgetName(), node));
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeUserData(const char *node, const char *data)
{
  if (this->IsCreated() && node && data)
    {
    const char *val = this->ConvertInternalStringToTclString(
      data, vtkKWWidget::ConvertStringEscapeInterpretable);
    this->Script("%s itemconfigure %s -data \"%s\"", 
                 this->GetWidgetName(), node, val);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeText(const char *node)
{
  if (this->IsCreated() && node)
    {
    return this->ConvertTclStringToInternalString(
      this->Script("%s itemcget %s -text", this->GetWidgetName(), node));
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeText(const char *node, const char *text)
{
  if (this->IsCreated() && node && text)
    {
    const char *val = this->ConvertInternalStringToTclString(
      text, vtkKWWidget::ConvertStringEscapeInterpretable);
    this->Script("%s itemconfigure %s -text \"%s\"", 
                 this->GetWidgetName(), node, val);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWTree::GetNodeFont(const char *node)
{
  if (this->IsCreated() && node)
    {
    return this->ConvertTclStringToInternalString(
      this->Script("%s itemcget %s -font", this->GetWidgetName(), node));
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeFont(const char *node, const char *font)
{
  if (this->IsCreated() && node && font)
    {
    const char *val = this->ConvertInternalStringToTclString(
      font, vtkKWWidget::ConvertStringEscapeInterpretable);
    this->Script("%s itemconfigure %s -font \"%s\"", 
                 this->GetWidgetName(), node, val);
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeFontWeightToBold(const char *node)
{
  if (this->IsCreated() && node)
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
  if (this->IsCreated() && node)
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
  if (this->IsCreated() && node)
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
  if (this->IsCreated() && node)
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
  if (this->IsCreated() && node)
    {
    return atoi(this->Script("%s itemcget %s -selectable", 
                             this->GetWidgetName(), node));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetNodeSelectableFlag(const char *node, int flag)
{
  if (this->IsCreated() && node)
    {
    this->Script("%s itemconfigure %s -selectable %d", 
                 this->GetWidgetName(), node, flag);
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
void vtkKWTree::GetSelectionBackgroundColor(double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-selectbackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWTree::GetSelectionBackgroundColor()
{
  static double rgb[3];
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
  static double rgb[3];
  this->GetSelectionForegroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSelectionForegroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-selectforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTree::SetOpenCommand(vtkObject *obj, const char *method)
{
  if (!this->IsCreated())
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, obj, method);
  this->SetConfigurationOption("-opencmd", command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetCloseCommand(vtkObject *obj, const char *method)
{
  if (!this->IsCreated())
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, obj, method);
  this->SetConfigurationOption("-closecmd", command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetBindText(const char *event, 
                            vtkObject *obj, 
                            const char *method)
{
  if (!this->IsCreated() || !event)
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, obj, method);
  this->Script("%s bindText %s {%s}", this->GetWidgetName(), event, command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWTree::SetDoubleClickOnNodeCommand(vtkObject *obj, 
                                            const char *method)
{
  this->SetBindText("<Double-ButtonPress-1>", obj, method);
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSingleClickOnNodeCommand(vtkObject *obj, 
                                            const char *method)
{
  this->SetBindText("<ButtonPress-1>", obj, method);
}

//----------------------------------------------------------------------------
void vtkKWTree::SetSelectionChangedCommand(vtkObject *obj, 
                                           const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, obj, method);
    this->SetBind("<<TreeSelect>>", command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWTree::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetStateOption(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
