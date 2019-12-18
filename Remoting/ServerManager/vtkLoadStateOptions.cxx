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
using namespace vtksys;

#include <sstream>

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
std::string vtkLoadStateOptions::LocateFileInDirectory(const std::string& filepath, int isPath)
{
  std::string result = "";
  std::string modifiedDataDirectory = this->DataDirectory;
  std::vector<std::string> directoryPathComponents;

  // Replace any environment variable defined locations
  if (modifiedDataDirectory.compare(0, 1, "$") == 0)
  {
    SystemTools::SplitPath(modifiedDataDirectory, directoryPathComponents);
    std::string variablePath;
    if (SystemTools::GetEnv(directoryPathComponents[1].erase(0, 1), variablePath))
    {
      directoryPathComponents.erase(
        directoryPathComponents.begin(), directoryPathComponents.begin() + 2);
      std::vector<std::string> variablePathComponents;
      SystemTools::SplitPath(variablePath, variablePathComponents);
      directoryPathComponents.insert(directoryPathComponents.begin(),
        variablePathComponents.begin(), variablePathComponents.end());
    }
    else
    {
      vtkErrorMacro("Environment variable " << directoryPathComponents[1] << " is not set.");
      return result;
    }
  }
  else
  {
    SystemTools::SplitPath(
      SystemTools::CollapseFullPath(modifiedDataDirectory), directoryPathComponents);
  }

  std::vector<std::string> pathComponents;
  SystemTools::SplitPath(SystemTools::GetParentDirectory(filepath), pathComponents);
  size_t insertIndex = directoryPathComponents.size();

  while (pathComponents.size() >= 1)
  {
    std::string searchPath = SystemTools::JoinPath(directoryPathComponents);

    // If we care only about whether the path exists for a file , check that here.
    if (isPath)
    {
      // File path has old path. Replace old path with new path and
      // check that it exists.
      std::string filePrefix = SystemTools::GetFilenameName(filepath);
      std::string newPath = SystemTools::JoinPath(directoryPathComponents);
      if (SystemTools::FileExists(newPath.c_str(), false /*isFile*/))
      {
        std::vector<std::string> newPrefixComponents(directoryPathComponents);
        newPrefixComponents.push_back(filePrefix);
        return SystemTools::JoinPath(newPrefixComponents);
      }
    }
    else
    {
      if (SystemTools::LocateFileInDir(filepath.c_str(), searchPath.c_str(), result))
      {
        return result;
      }
    }
    directoryPathComponents.insert(
      directoryPathComponents.begin() + insertIndex, pathComponents.back());
    pathComponents.pop_back();
  }

  std::stringstream str;
  str << "Cannot find '" << SystemTools::GetFilenameName(filepath) << "' in '"
      << this->DataDirectory.c_str() << "'. Using '" << filepath << "'.\n\n";
  vtkOutputWindowDisplayDebugText(str.str().c_str());
  return result;
}
