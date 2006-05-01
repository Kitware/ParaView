/*=========================================================================

  Module:    vtkKWOptionDataBase.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWOptionDataBase.h"

#include "vtkKWApplication.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/vector>
#include <vtksys/stl/list>
#include <vtksys/stl/string>
#include <vtksys/stl/map>
#include <vtksys/RegularExpression.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWOptionDataBase);
vtkCxxRevisionMacro(vtkKWOptionDataBase, "1.5");

//----------------------------------------------------------------------------
class vtkKWOptionDataBaseInternals
{
public:

  // Context list
  // This is used to put a constraint on the context that should match
  // the object's parent.
  // Say, for example that the pattern:
  //   vtkKWWidget*vtkKWFrameWithLabel.vtkKWFrame
  // will match a vtkKWFrame that is an immediate child of a 
  // vtkKWFrameWithLabel, which is a child or sub-child of a vtkKWWidget.
  // The resulting list (ContextListType), for that example, will look like:
  //   {{"vtkKWFrameWithLabel", 1} {"vtkKWWidget", 0}}

  class ContextNodeType
  {
  public:
    vtksys_stl::string ClassName;
    int IsImmediateParent;
  };

  struct ContextContainerType: public vtksys_stl::list<ContextNodeType> {};

  // Option pool
  // Data structure used to store each option/
  // Say, if the following option entry is added:
  //     AddEntry("vtkKWFoo*vtkKWBar.vtkKWFrameWithLabel:CollapsibleFrame", 
  //              "SetBackgroundColor", "0.2 0.3 0.4");
  // The resulting entry in the EntryPoolType map  will be:
  //  "vtkKWFrameWithLabel" => 
  //    { 
  //      0,
  //      "vtkKWFoo*vtkKWBar.vtkKWFrameWithLabel:CollapsibleFrame",
  //      "SetBackgroundColor",
  //      "0.2 0.3 0.4",
  //      "CollapsibleFrame",
  //      {{"vtkKWFoo", 0} {"vtkKWBar", 1}}
  //    }
  // which means: each time a vtkKWFrameWithLabel is created, that is an
  // immediate child of a vtkKWBar, which itself is a child or sub-child
  // of a vtkKWFoo, then retrieve the CollapsibleFrame member of that
  // vtkKWFrameWithLabel (using GetCollapsibleFrame()) and call
  // "SetBackgroundColor 0.2 0.3 0.4" on that object.
  
  int EntryCounter;

  typedef vtksys_stl::string EntryKeyType;

  class EntryNodeType
  {
  public:
    int Id;
    vtksys_stl::string Pattern;
    vtksys_stl::string Command;
    vtksys_stl::string Value;
    vtksys_stl::string ClassName;
    vtksys_stl::string SlotName;
    ContextContainerType Context;
  };

  struct EntryContainerType: public vtksys_stl::vector<EntryNodeType> {};

  struct EntryPoolType: public vtksys_stl::map<EntryKeyType, EntryContainerType> {};

  EntryPoolType EntryPool;

  // Superclass cache
  // say, for vtkKWScale, the ClassHierarchyCacheType map will look like:
  //   "vtkKWScale" => { "vtkObject", "vtkKWObject", "vtkKWWidget", "vtkKWCoreWidget", "vtkKWScale"}
  
  struct ClassHierarchyContainerType: public vtksys_stl::vector<vtksys_stl::string> {};
  struct ClassHierarchyCacheType: public vtksys_stl::map<vtksys_stl::string, ClassHierarchyContainerType> {};

  ClassHierarchyCacheType ClassHierarchyCache;

  // Configure an object. Needs to be in the internals because I want to
  // pass STL containers around

  void ConfigureWidget(vtkKWWidget *obj, 
                       ClassHierarchyContainerType *obj_parents, 
                       const char *as_class_name);
};

//----------------------------------------------------------------------------
vtkKWOptionDataBase::vtkKWOptionDataBase()
{
  this->Internals = new vtkKWOptionDataBaseInternals;
  this->Internals->EntryCounter = 0;
}

//----------------------------------------------------------------------------
vtkKWOptionDataBase::~vtkKWOptionDataBase()
{
  // Delete our pool

  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntry(const char *pattern, 
                                  const char *command, 
                                  const char *value)
{
  if (!this->Internals || 
      !pattern || !*pattern || 
      !command || !*command)
    {
    return -1;
    }

  vtkKWOptionDataBaseInternals::EntryNodeType node;

  node.Pattern = pattern;

  // Get the class name and the context out of the pattern

  vtksys_stl::string::size_type pos = 0;
  while (1) 
    {
    vtksys_stl::string::size_type sep = node.Pattern.find_first_of(".*", pos);
    if (sep == vtksys_stl::string::npos)
      {
      node.ClassName = node.Pattern.substr(pos);
      break;
      }
    else 
      {
      vtkKWOptionDataBaseInternals::ContextNodeType context_node;
      context_node.ClassName = node.Pattern.substr(pos, sep - pos);
      context_node.IsImmediateParent = (node.Pattern[sep] == '.' ? 1 : 0);
      node.Context.push_front(context_node);
      pos = sep + 1;
      }
    };

  // Classname not found ? 

  if (!node.ClassName.size())
    {
    return -1;
    }

  // Get the slot name out of the classname 
  // (say, vtkKWFrameWithLabel:CollapsibleFrame)

  vtksys_stl::string::size_type sep_slot = node.ClassName.find_first_of(':');
  if (sep_slot != vtksys_stl::string::npos)
    {
    node.SlotName = node.ClassName.substr(sep_slot + 1);
    node.ClassName = node.ClassName.substr(0, sep_slot);
    if (!node.ClassName.size())
      {
      return -1;
      }
    }

  // New entry

  node.Id = this->Internals->EntryCounter++;
  node.Command = command;
  if (value)
    {
    node.Value = value;
    }

  this->Internals->EntryPool[node.ClassName].push_back(node);

  return node.Id;
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsInt(const char *pattern, 
                                       const char *command, 
                                       int value)
{
  char buffer[20];
  sprintf(buffer, "%d", value);
  return this->AddEntry(pattern, command, buffer);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsDouble(const char *pattern, 
                                          const char *command, 
                                          double value)
{
  char buffer[256];
  sprintf(buffer, "%lf", value);
  return this->AddEntry(pattern, command, buffer);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsInt3(const char *pattern, 
                                        const char *command, 
                                        int v0, int v1, int v2)
{
  char buffer[256];
  sprintf(buffer, "%d %d %d", v0, v1, v2);
  return this->AddEntry(pattern, command, buffer);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsInt3(const char *pattern, 
                                        const char *command, 
                                        int value3[3])
{
  return 
    this->AddEntryAsDouble3(pattern, command, value3[0], value3[1], value3[2]);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsDouble3(const char *pattern, 
                                           const char *command, 
                                           double v0, double v1, double v2)
{
  char buffer[256];
  sprintf(buffer, "%lf %lf %lf", v0, v1, v2);
  return this->AddEntry(pattern, command, buffer);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsDouble3(const char *pattern, 
                                           const char *command, 
                                           double value3[3])
{
  return 
    this->AddEntryAsDouble3(pattern, command, value3[0], value3[1], value3[2]);
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::DeepCopy(vtkKWOptionDataBase *p)
{
  if (!p)
    {
    return;
    }

  this->RemoveAllEntries();

  vtkKWOptionDataBaseInternals::EntryPoolType::iterator p_it = 
    p->Internals->EntryPool.begin();
  vtkKWOptionDataBaseInternals::EntryPoolType::iterator p_end = 
    p->Internals->EntryPool.end();
  for (; p_it != p_end; ++p_it)
    {
    vtkKWOptionDataBaseInternals::EntryContainerType::iterator l_it = 
      p_it->second.begin();
    vtkKWOptionDataBaseInternals::EntryContainerType::iterator l_end = 
      p_it->second.end();
    for (; l_it != l_end; ++l_it)
      {
      this->AddEntry(
        l_it->Pattern.c_str(), l_it->Command.c_str(), l_it->Value.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::RemoveAllEntries()
{
  this->Internals->EntryPool.clear();
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::GetNumberOfEntries()
{
  int total = 0;

  vtkKWOptionDataBaseInternals::EntryPoolType::iterator p_it = 
    this->Internals->EntryPool.begin();
  vtkKWOptionDataBaseInternals::EntryPoolType::iterator p_end = 
    this->Internals->EntryPool.end();
  for (; p_it != p_end; ++p_it)
    {
    total += p_it->second.size();
    }

  return total;
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::ConfigureWidget(vtkKWWidget *obj)
{
  if (!obj || !this->Internals->EntryPool.size())
    {
    return;
    }

  // Process the class hierarchy
  // say, for vtkKWScale: 
  // vtkObject, vtkKWObject, vtkKWWidget, vtkKWCoreWidget, vtkKWScale

  vtkKWOptionDataBaseInternals::ClassHierarchyCacheType::iterator p_it = 
    this->Internals->ClassHierarchyCache.find(obj->GetClassName());
  vtkKWOptionDataBaseInternals::ClassHierarchyContainerType &list = 
    this->Internals->ClassHierarchyCache[obj->GetClassName()];

  // Cache the class hierarchy

  if (p_it == this->Internals->ClassHierarchyCache.end())
    {
    ostrstream revisions;
    obj->PrintRevisions(revisions);
    revisions << ends;

    char buffer[512];
    const char *c = revisions.str();
    while (*c)
      {
      const char *begin_class = c;
      while (*c != ' ') ++c;
      memcpy(buffer, begin_class, c - begin_class);
      buffer[c - begin_class] = '\0';
      list.push_back(buffer);
      while (*c != '\n') ++c;
      ++c;
      }
    revisions.rdbuf()->freeze(0);
    }

  // Configure the object using the options defined for each class in
  // its class hierarchy, starting with the top-most classes.
  // Say, for vtkKWScale, try to find and apply the options defined for
  // vtkObject, then for vtkKWObject, vtkKWWidget, etc., down to vtkKWScale.

  vtkKWOptionDataBaseInternals::ClassHierarchyContainerType obj_parents;

  vtkKWOptionDataBaseInternals::ClassHierarchyContainerType::iterator l_it = 
    list.begin();
  vtkKWOptionDataBaseInternals::ClassHierarchyContainerType::iterator l_end = 
    list.end();
  for (; l_it != l_end; ++l_it)
    {
    this->Internals->ConfigureWidget(obj, &obj_parents, l_it->c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBaseInternals::ConfigureWidget(
  vtkKWWidget *obj, 
  vtkKWOptionDataBaseInternals::ClassHierarchyContainerType *obj_parents,
  const char *as_class_name)
{
  if (!obj || !this->EntryPool.size())
    {
    return;
    }

  // Do we have options for that class ?

  vtkKWOptionDataBaseInternals::EntryPoolType::iterator p_it = 
    this->EntryPool.find(as_class_name);
  if (p_it == this->EntryPool.end())
    {
    return;
    }

  vtksys_stl::string cmd;

  vtkKWOptionDataBaseInternals::EntryContainerType::iterator l_it = 
    p_it->second.begin();
  vtkKWOptionDataBaseInternals::EntryContainerType::iterator l_end = 
    p_it->second.end();
  for (; l_it != l_end; ++l_it)
    {
    // Does this option have constraints on the context this object should
    // be in, i.e. its parents. 

    if (l_it->Context.size())
      {
      // Do we have the parents of the object ? Find them.
      if (!obj_parents->size())
        {
        vtkKWWidget *ptr = obj;
        while (ptr->GetParent())
          {
          obj_parents->push_back(ptr->GetParent()->GetClassName());
          ptr = ptr->GetParent();
          }
        }

      // Check if the obj parents match the context. We are not being
      // very fancy here with the "*" separator (i.e. an element of the
      // context that can be a parent or grand-parent), we just look
      // for the closest parent or grand-parent and don't backtrack.

      vtkKWOptionDataBaseInternals::ClassHierarchyContainerType::iterator 
        obj_p_it = obj_parents->begin();
      vtkKWOptionDataBaseInternals::ClassHierarchyContainerType::iterator 
        obj_p_end = obj_parents->end();

      vtkKWOptionDataBaseInternals::ContextContainerType::iterator 
        context_it = l_it->Context.begin();
      vtkKWOptionDataBaseInternals::ContextContainerType::iterator 
        context_end = l_it->Context.end();

      int context_ok = 1;
      for (; context_it != context_end && obj_p_it != obj_p_end; ++context_it)
        {
        // OK, we match right away
        if (!context_it->ClassName.compare(*obj_p_it))
          {
          ++obj_p_it;
          }
        else
          {
          // We did not match an immediate parent, ouch
          if (context_it->IsImmediateParent)
            {
            context_ok = 0;
            break;
            }
          // We did not match the parent but can match a grand-parent
          else
            {
            do
              {
              ++obj_p_it;
              } while (obj_p_it != obj_p_end &&
                       context_it->ClassName.compare(*obj_p_it));
            // Nope, we did not match any grand-parent either, ouch
            if (obj_p_it == obj_p_end)
              {
              context_ok = 0;
              break;
              }
            }
          }
        }

      // No context match. Too bad.

      if (context_it != context_end || !context_ok)
        {
        continue;
        }
      }

    cmd += "catch {";
    if (l_it->SlotName.size())
      {
      cmd += '[';
      cmd += obj->GetTclName();
      cmd += " Get";
      cmd += l_it->SlotName;
      cmd += ']';
      }
    else
      {
      cmd += obj->GetTclName();
      }
    cmd += ' ';
    cmd += l_it->Command;
    cmd += ' ';
    cmd += l_it->Value;
    cmd += "}\n";
    }

  if (cmd.size())
    {
    vtkKWTkUtilities::EvaluateSimpleString(obj->GetApplication(), cmd.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::AddBackgroundColorOptions(
  double r, double g, double b)
{
  double bgcolor[3];
  bgcolor[0] = r;
  bgcolor[1] = g;
  bgcolor[2] = b;

  this->AddEntryAsDouble3(
    "vtkKWWidget", "SetBackgroundColor", bgcolor);
  this->AddEntryAsDouble3(
    "vtkKWWidget", "SetActiveBackgroundColor", bgcolor);

  this->AddEntryAsDouble3(
    "vtkKWEntry", "SetDisabledBackgroundColor", bgcolor);
  this->AddEntryAsDouble3(
    "vtkKWEntry", "SetReadOnlyBackgroundColor", bgcolor);
  this->AddEntryAsDouble3(
    "vtkKWSpinBox", "SetDisabledBackgroundColor", bgcolor);
  this->AddEntryAsDouble3(
    "vtkKWSpinBox", "SetReadOnlyBackgroundColor", bgcolor);
  this->AddEntryAsDouble3(
    "vtkKWSpinBox", "SetButtonBackgroundColor", bgcolor);
  this->AddEntryAsDouble3(
    "vtkKWScale", "SetTroughColor", bgcolor);
  this->AddEntryAsDouble3(
    "vtkKWScrollbar", "SetTroughColor", bgcolor);
  this->AddEntryAsDouble3(
    "vtkKWMultiColumnList", "SetColumnLabelBackgroundColor", bgcolor);
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::AddFontOptions(
  const char *font)
{
  if (!font)
    {
    return;
    }

  vtksys_stl::string font_spec(font);
  if (font_spec[0] != '{')
    {
    font_spec.insert(0, "{");
    font_spec += '}';
    }

#if 0
  this->AddEntry("vtkKWWidget", "SetFont", font_spec.c_str());
#else
  this->AddEntry("vtkKWCheckButton", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWEntry", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWLabel", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWListBox", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWMenu", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWMenuButton", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWMessage", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWPushButton", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWScale", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWSpinBox", "SetFont", font_spec.c_str());
  this->AddEntry("vtkKWText", "SetFont", font_spec.c_str());
#endif
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
