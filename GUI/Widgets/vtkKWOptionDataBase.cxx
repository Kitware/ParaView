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
#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/vector>
#include <vtksys/stl/string>
#include <vtksys/stl/map>
#include <vtksys/RegularExpression.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWOptionDataBase);
vtkCxxRevisionMacro(vtkKWOptionDataBase, "1.3");

//----------------------------------------------------------------------------
class vtkKWOptionDataBaseInternals
{
public:

  // Option entry

  typedef vtksys_stl::string EntryKeyType;

  class EntryNodeType
  {
  public:
    int Id;
    vtksys_stl::string Pattern;
    vtksys_stl::string Option;
    vtksys_stl::string Value;
  };

  // Option pool

  typedef vtksys_stl::vector<EntryNodeType> EntryListType;
  typedef vtksys_stl::vector<EntryNodeType>::iterator EntryListIterator;

  typedef vtksys_stl::map<EntryKeyType, EntryListType> EntryPoolType;
  typedef vtksys_stl::map<EntryKeyType, EntryListType>::iterator EntryPoolIterator;

  EntryPoolType EntryPool;

  // Superclass cache

  typedef vtksys_stl::vector<vtksys_stl::string> SuperclassListType;
  typedef vtksys_stl::vector<vtksys_stl::string>::iterator SuperclassListIterator;
  typedef vtksys_stl::map<vtksys_stl::string, SuperclassListType> SuperclassCacheType;
  typedef vtksys_stl::map<vtksys_stl::string, SuperclassListType>::iterator SuperclassCacheIterator;

  int EntryCounter;
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
                                  const char *option, 
                                  const char *value)
{
  if (!this->Internals || 
      !pattern || !*pattern || 
      !option || !*option)
    {
    return -1;
    }

  int id =  this->Internals->EntryCounter++;

  vtkKWOptionDataBaseInternals::EntryNodeType node;

  node.Id = id;
  node.Pattern = pattern;
  node.Option = option;
  if (value)
    {
    node.Value = value;
    }

  this->Internals->EntryPool[node.Pattern].push_back(node);

  return id;
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsInt(const char *pattern, 
                                       const char *option, 
                                       int value)
{
  char buffer[20];
  sprintf(buffer, "%d", value);
  return this->AddEntry(pattern, option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsDouble(const char *pattern, 
                                          const char *option, 
                                          double value)
{
  char buffer[256];
  sprintf(buffer, "%lf", value);
  return this->AddEntry(pattern, option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsInt3(const char *pattern, 
                                        const char *option, 
                                        int v0, int v1, int v2)
{
  char buffer[256];
  sprintf(buffer, "%d %d %d", v0, v1, v2);
  return this->AddEntry(pattern, option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsInt3(const char *pattern, 
                                        const char *option, 
                                        int value3[3])
{
  return 
    this->AddEntryAsDouble3(pattern, option, value3[0], value3[1], value3[2]);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsDouble3(const char *pattern, 
                                           const char *option, 
                                           double v0, double v1, double v2)
{
  char buffer[256];
  sprintf(buffer, "%lf %lf %lf", v0, v1, v2);
  return this->AddEntry(pattern, option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWOptionDataBase::AddEntryAsDouble3(const char *pattern, 
                                           const char *option, 
                                           double value3[3])
{
  return 
    this->AddEntryAsDouble3(pattern, option, value3[0], value3[1], value3[2]);
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::DeepCopy(vtkKWOptionDataBase *p)
{
  if (!p)
    {
    return;
    }

  this->RemoveAllEntries();

  vtkKWOptionDataBaseInternals::EntryPoolIterator p_it = 
    p->Internals->EntryPool.begin();
  vtkKWOptionDataBaseInternals::EntryPoolIterator p_end = 
    p->Internals->EntryPool.end();
  for (; p_it != p_end; ++p_it)
    {
    vtkKWOptionDataBaseInternals::EntryListIterator l_it = 
      p_it->second.begin();
    vtkKWOptionDataBaseInternals::EntryListIterator l_end = 
      p_it->second.end();
    for (; l_it != l_end; ++l_it)
      {
      this->AddEntry(
        l_it->Pattern.c_str(), l_it->Option.c_str(), l_it->Value.c_str());
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

  vtkKWOptionDataBaseInternals::EntryPoolIterator p_it = 
    this->Internals->EntryPool.begin();
  vtkKWOptionDataBaseInternals::EntryPoolIterator p_end = 
    this->Internals->EntryPool.end();
  for (; p_it != p_end; ++p_it)
    {
    total += p_it->second.size();
    }

  return total;
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::ConfigureObject(vtkKWObject *obj)
{
  if (!obj || !this->Internals->EntryPool.size())
    {
    return;
    }

  this->ConfigureObject(obj, "*");

  // Process the class hierarchy
  // TODO: this should be class in a map of list of class names
  // i.e.: vtkKWScale: vtkObject, vtkKWObject, vtkkWWidget, vtkKWCoreWidget...

  ostrstream revisions;
  obj->PrintRevisions(revisions);
  revisions << ends;

  char buffer[256];
  const char *c = revisions.str();
  while (*c)
    {
    const char *begin_class = c;
    while (*c != ' ') ++c;
    memcpy(buffer, begin_class, c - begin_class);
    buffer[c - begin_class] = '\0';
    this->ConfigureObject(obj, buffer);
    while (*c != '\n') ++c;
    ++c;
    }

  revisions.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::ConfigureObject(vtkKWObject *obj, 
                                          const char *pattern)
{
  if (!obj || !this->Internals->EntryPool.size())
    {
    return;
    }

  // TODO: this should be made much faster
  // i.e. construct a string for all options to apply, *then* script it
  vtkKWOptionDataBaseInternals::EntryPoolIterator p_it = 
    this->Internals->EntryPool.find(pattern);
  if (p_it == this->Internals->EntryPool.end())
    {
    return;
    }

  vtkKWOptionDataBaseInternals::EntryListIterator l_it = 
    p_it->second.begin();
  vtkKWOptionDataBaseInternals::EntryListIterator l_end = 
    p_it->second.end();
  for (; l_it != l_end; ++l_it)
    {
    obj->Script("catch {%s %s %s}",
                obj->GetTclName(), l_it->Option.c_str(), l_it->Value.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWOptionDataBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
