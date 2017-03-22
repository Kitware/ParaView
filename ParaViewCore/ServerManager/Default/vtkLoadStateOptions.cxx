/*=========================================================================

  Program:   ParaView
  Module:    vtkLoadStateOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLoadStateOptions.h"

#include "vtkObjectFactory.h"
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkLoadStateOptions);
//----------------------------------------------------------------------------
vtkLoadStateOptions::vtkLoadStateOptions()
{
}

//----------------------------------------------------------------------------
vtkLoadStateOptions::~vtkLoadStateOptions()
{
}

//----------------------------------------------------------------------------
void vtkLoadStateOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
std::string vtkLoadStateOptions::LocateFileInDirectory(const std::string& filepath)
{
  std::string result = "";
  std::string modifiedDataDirectory = this->DataDirectory;

  // Check for $HOME/$PWD/$CWD/$EXE
  if (modifiedDataDirectory.compare(0, 5, "$HOME") == 0)
  {
    modifiedDataDirectory.replace(0, 5, "~");
  }
  else if (modifiedDataDirectory.compare(0, 4, "$PWD") == 0 ||
    modifiedDataDirectory.compare(0, 4, "$CWD") == 0 ||
    modifiedDataDirectory.compare(0, 4, "$EXE") == 0) // Should this be $PATH
  {
    modifiedDataDirectory.replace(0, 4, ".");
  }

  std::vector<std::string> directoryPathComponents;
  vtksys::SystemTools::SplitPath(
    vtksys::SystemTools::CollapseFullPath(modifiedDataDirectory), directoryPathComponents);
  std::vector<std::string> pathComponents;
  vtksys::SystemTools::SplitPath(vtksys::SystemTools::GetParentDirectory(filepath), pathComponents);
  int insertIndex = directoryPathComponents.size();

  while (pathComponents.size() > 1)
  {
    std::string searchPath = vtksys::SystemTools::JoinPath(directoryPathComponents);
    if (vtksys::SystemTools::LocateFileInDir(filepath.c_str(), searchPath.c_str(), result))
    {
      return result;
    }
    directoryPathComponents.insert(
      directoryPathComponents.begin() + insertIndex, pathComponents.back());
    pathComponents.pop_back();
  }
  vtkErrorMacro("Cannot find " << filepath << " in " << this->DataDirectory.c_str() << ".");
  return result;
}
