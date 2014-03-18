/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonInformation - Gets python features.
// .SECTION Description
// Get details of python availabilty on the root server.


#ifndef __vtkPVPythonInformation_h
#define __vtkPVPythonInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

#include <string> // for string type

class vtkClientServerStream;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVPythonInformation
    : public vtkPVInformation
{
public:
  static vtkPVPythonInformation* New();
  vtkTypeMacro(vtkPVPythonInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  void DeepCopy(vtkPVPythonInformation *info);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Same as DeepCopy for this object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Whether the server was compiled with python support.
  vtkSetMacro(PythonSupport, bool)
  vtkGetMacro(PythonSupport, bool)
  vtkBooleanMacro(PythonSupport, bool)

  // Description:
  // If GetPythonSupport() is true, returns the version of python detected on
  // the server.
  void SetPythonVersion(const std::string &arg)
  {
    if (arg != this->PythonVersion)
      {
      this->PythonVersion = arg;
      this->Modified();
      }
  }
  const std::string& GetPythonVersion()
  {
    return this->PythonVersion;
  }

  // Description:
  // If GetPythonSupport() is true, returns the path to the python libraries
  // detected on the server.
  void SetPythonPath(const std::string &arg)
  {
    if (arg != this->PythonPath)
      {
      this->PythonPath = arg;
      this->Modified();
      }
  }
  const std::string& GetPythonPath()
  {
    return this->PythonPath;
  }

  // Description:
  // Whether the numpy module is available on the server.
  vtkSetMacro(NumpySupport, bool)
  vtkGetMacro(NumpySupport, bool)
  vtkBooleanMacro(NumpySupport, bool)

  // Description:
  // If GetNumpySupport() is true, returns the version of numpy detected on the
  // server.
  void SetNumpyVersion(const std::string &arg)
  {
    if (arg != this->NumpyVersion)
      {
      this->NumpyVersion = arg;
      this->Modified();
      }
  }
  const std::string& GetNumpyVersion()
  {
    return this->NumpyVersion;
  }

  // Description:
  // If GetNumpySupport() is true, returns the path to numpy detected on the
  // server.
  void SetNumpyPath(const std::string &arg)
  {
    if (arg != this->NumpyPath)
      {
      this->NumpyPath = arg;
      this->Modified();
      }
  }
  const std::string& GetNumpyPath()
  {
    return this->NumpyPath;
  }

  // Description:
  // Whether the matplotlib module is available on the server.
  vtkSetMacro(MatplotlibSupport, bool)
  vtkGetMacro(MatplotlibSupport, bool)
  vtkBooleanMacro(MatplotlibSupport, bool)

  // Description:
  // If GetMatplotlibSupport() is true, returns the version of matplotlib
  // detected on the server.
  void SetMatplotlibVersion(const std::string &arg)
  {
    if (arg != this->MatplotlibVersion)
      {
      this->MatplotlibVersion = arg;
      this->Modified();
      }
  }
  const std::string& GetMatplotlibVersion()
  {
    return this->MatplotlibVersion;
  }

  // Description:
  // If GetMatplotlibSupport() is true, returns the path to matplotlib detected
  // on the server.
  void SetMatplotlibPath(const std::string &arg)
  {
    if (arg != this->MatplotlibPath)
      {
      this->MatplotlibPath = arg;
      this->Modified();
      }
  }
  const std::string& GetMatplotlibPath()
  {
    return this->MatplotlibPath;
  }

protected:
  vtkPVPythonInformation();
  ~vtkPVPythonInformation();

  bool PythonSupport;
  std::string PythonPath;
  std::string PythonVersion;
  bool NumpySupport;
  std::string NumpyVersion;
  std::string NumpyPath;
  bool MatplotlibSupport;
  std::string MatplotlibVersion;
  std::string MatplotlibPath;

private:
  vtkPVPythonInformation(const vtkPVPythonInformation&); // Not implemented
  void operator=(const vtkPVPythonInformation&); // Not implemented
};

#endif
