/*=========================================================================

  Module:    vtkKWInternationalization.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWInternationalization.h"

#ifdef _WIN32
#include "windows.h"
#endif

#include <string.h>
#include <stdarg.h>
#include "vtkObjectFactory.h"
#include "vtkDirectory.h"
#include "vtkTcl.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/set>
#include <vtksys/stl/vector>
#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWInternationalization);
vtkCxxRevisionMacro(vtkKWInternationalization, "1.2");

//----------------------------------------------------------------------------
void vtkKWInternationalization::SetCurrentTextDomain(const char *domain_name)
{
#ifdef KWWidgets_USE_INTERNATIONALIZATION
  textdomain(domain_name);
#endif
}

//----------------------------------------------------------------------------
const char* vtkKWInternationalization::GetCurrentTextDomain()
{
#ifdef KWWidgets_USE_INTERNATIONALIZATION
  return textdomain(NULL);
#else
  return NULL;
#endif
}

//----------------------------------------------------------------------------
void vtkKWInternationalization::SetTextDomainBinding(
  const char *domain_name, const char *dir)
{
#ifdef KWWidgets_USE_INTERNATIONALIZATION
  bindtextdomain(domain_name, dir);
#endif
}

//----------------------------------------------------------------------------
const char* vtkKWInternationalization::GetTextDomainBinding(
  const char *domain_name)
{
#ifdef KWWidgets_USE_INTERNATIONALIZATION
  return bindtextdomain(domain_name, NULL);
#else
  return NULL;
#endif
}

//----------------------------------------------------------------------------
const char* vtkKWInternationalization::FindTextDomainBinding(
  const char *domain_name)
{
  return vtkKWInternationalization::FindTextDomainBinding(domain_name, NULL);
}

//----------------------------------------------------------------------------
const char* vtkKWInternationalization::FindTextDomainBinding(
  const char *domain_name, const char *dirs_to_search)
{
#ifdef KWWidgets_USE_INTERNATIONALIZATION
  
  // First try to find likely path to search catalogs for
  
  vtksys_stl::set<vtksys_stl::string> search_dir_candidates;

  // User dirs

  vtksys_stl::vector<vtksys_stl::string> user_dir_candidates;
  if (dirs_to_search && *dirs_to_search)
    {
    vtksys::SystemTools::Split(dirs_to_search, user_dir_candidates, ';');
    }

  vtksys_stl::vector<vtksys_stl::string>::iterator user_dir_it = 
    user_dir_candidates.begin();
  vtksys_stl::vector<vtksys_stl::string>::iterator user_dir_end = 
    user_dir_candidates.end();
  for(; user_dir_it != user_dir_end; ++user_dir_it)
    {
    search_dir_candidates.insert(*user_dir_it);
    }

  // Try to find the path to the executable using Tcl

  const char *name_of_exec = Tcl_GetNameOfExecutable();
  if (name_of_exec && vtksys::SystemTools::FileExists(name_of_exec))
    {
    search_dir_candidates.insert(
      vtksys::SystemTools::GetFilenamePath(name_of_exec));
    }

#if _WIN32

  // Try to find path to exec using ::GetModuleFileName on the current module

  char module_path[_MAX_PATH];
  DWORD module_path_length = 
    ::GetModuleFileName(NULL, module_path, _MAX_PATH);
  if (module_path_length)
    {
    search_dir_candidates.insert(
      vtksys::SystemTools::GetFilenamePath(module_path));
    }

  // Try to find path to exec using ::GetModuleFileName on the module that has
  // the same name as the domain

  module_path_length = 
    ::GetModuleFileName(GetModuleHandle(domain_name), module_path, _MAX_PATH);
  if (module_path_length)
    {
    search_dir_candidates.insert(
      vtksys::SystemTools::GetFilenamePath(module_path));
    }

#endif _WIN32

  // Try to find the exec in the PATH environment variable

  vtksys_stl::string exec_path, error_msg;
  if (vtksys::SystemTools::FindProgramPath(
        domain_name, exec_path, error_msg, domain_name))
    {
    search_dir_candidates.insert(
      vtksys::SystemTools::GetFilenamePath(exec_path));
    }

  // Add the current working directory for kicks

  search_dir_candidates.insert(
    vtksys::SystemTools::GetCurrentWorkingDirectory());

  // Add some absolute paths to well known message catalog locations

#ifndef _WIN32
  search_dir_candidates.insert("/usr/share/locale");
  search_dir_candidates.insert("/usr/lib/locale");
  search_dir_candidates.insert("/usr/local/share/locale");
  search_dir_candidates.insert("/usr/local/lib/locale");
#endif

  // Try if the message catalog is located where the KWWidgets message
  // catalog already is

  const char *kwwidgets_binding = 
    vtkKWInternationalization::GetTextDomainBinding("KWWidgets");
  if (kwwidgets_binding)
    {
    search_dir_candidates.insert(kwwidgets_binding);
    }

  // In each search path, we will look in the following subdirs

  vtksys_stl::vector<vtksys_stl::string> subdir_candidates;

  const char *subdir_levels[] = { ".", "..", "../.." };
  int l;
  for (l = 0; l < sizeof(subdir_levels) / sizeof(subdir_levels[0]); l++)
    {
    vtksys_stl::string subdir_level = subdir_levels[l];

    user_dir_it = user_dir_candidates.begin();
    for(; user_dir_it != user_dir_end; ++user_dir_it)
      {
      subdir_candidates.push_back(
        subdir_level + "/" + *user_dir_it);
      subdir_candidates.push_back(
        subdir_level + "/" + *user_dir_it  + "/locale");
      subdir_candidates.push_back(
        subdir_level + *user_dir_it  + "/locale");
      }
    subdir_candidates.push_back(subdir_level);
    subdir_candidates.push_back(subdir_level + "/locale");
    subdir_candidates.push_back(subdir_level + "/share/locale");
    subdir_candidates.push_back(
      subdir_level + "/share/" + vtksys_stl::string(domain_name) + "/locale");
    subdir_candidates.push_back(subdir_level + "/po");
    subdir_candidates.push_back(subdir_level + "/mo");
    }

  // Iterate over the search paths, and try to find some catalogs for
  // that domain

  vtkDirectory *dir = vtkDirectory::New();
  vtksys_stl::set<vtksys_stl::string> catalog_candidates;

  vtksys_stl::set<vtksys_stl::string>::iterator search_dir_it = 
    search_dir_candidates.begin();
  vtksys_stl::set<vtksys_stl::string>::iterator search_dir_end = 
    search_dir_candidates.end();
  
  for(; search_dir_it != search_dir_end; ++search_dir_it)
    {
    vtksys_stl::string search_dir = *search_dir_it;
    vtksys::SystemTools::ConvertToUnixSlashes(search_dir);

    // For each exec dir, try several locale dir candidates

    vtksys_stl::vector<vtksys_stl::string>::iterator subdir_it = 
      subdir_candidates.begin();
    vtksys_stl::vector<vtksys_stl::string>::iterator subdir_end = 
      subdir_candidates.end();

    for(; subdir_it != subdir_end; ++subdir_it)
      {
      vtksys_stl::string try_locale_dir(search_dir);
      try_locale_dir += "/";
      try_locale_dir += *subdir_it;
      if (!vtksys::SystemTools::FileIsDirectory(try_locale_dir.c_str()))
        {
        continue;
        }

      // Look for the first locale subdir in this candidate

      dir->Open(try_locale_dir.c_str());
      int f, num_files = dir->GetNumberOfFiles();
      for (f = 0; f < num_files; ++f)
        {
        if (dir->GetFile(f)[0] != '.')
          {
          vtksys_stl::string try_catalog(try_locale_dir);
          try_catalog += "/";
          try_catalog += dir->GetFile(f);
          try_catalog += "/LC_MESSAGES";
          try_catalog += "/";
          try_catalog += domain_name;
          try_catalog += ".mo";
          if (vtksys::SystemTools::FileExists(try_catalog.c_str()))
            {
            catalog_candidates.insert(
              vtksys::SystemTools::CollapseFullPath(try_catalog.c_str()));
            }
          break;
          }
        }
      }
    }

  dir->Delete();

  // Use the most recent catalog

  if (catalog_candidates.size())
    {
    long int max_ctime = -1;
    vtksys_stl::string final_pick;
    
    vtksys_stl::set<vtksys_stl::string>::iterator cat_it = 
      catalog_candidates.begin();
    vtksys_stl::set<vtksys_stl::string>::iterator cat_end = 
      catalog_candidates.end();
    for(; cat_it != cat_end; ++cat_it)
      {
      long int ctime = vtksys::SystemTools::CreationTime(cat_it->c_str());
      if (ctime > max_ctime)
        {
        max_ctime = ctime;
        final_pick = *cat_it;
        }
      }
    
    vtkKWInternationalization::SetTextDomainBinding(
      domain_name, 
      vtksys::SystemTools::GetFilenamePath(
        vtksys::SystemTools::GetFilenamePath(
          vtksys::SystemTools::GetFilenamePath(final_pick))).c_str());

    return vtkKWInternationalization::GetTextDomainBinding(domain_name);
    }

#endif
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWInternationalization::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
char* kww_sgettext(const char *msgid)
{
#ifdef KWWidgets_USE_INTERNATIONALIZATION
  char *msgval = gettext(msgid);
  if (msgval == msgid)
#else
  char *msgval = const_cast<char*>(msgid);
#endif
    {
    char *sep = const_cast<char*>(strrchr(msgid, '|'));
    if (sep)
      {
      msgval = sep + 1;
      }
    }
  return msgval;
}

//----------------------------------------------------------------------------
char* kww_sdgettext(const char *domain_name, const char *msgid)
{
#ifdef KWWidgets_USE_INTERNATIONALIZATION
  char *msgval = dgettext(domain_name, msgid);
  if (msgval == msgid)
#else
  char *msgval = const_cast<char*>(msgid);
#endif
    {
    char *sep = const_cast<char*>(strrchr(msgid, '|'));
    if (sep)
      {
      msgval = sep + 1;
      }
    }
  return msgval;
}
