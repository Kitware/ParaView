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
  vtkSetMacro(PythonVersion, std::string)
  vtkGetMacro(PythonVersion, std::string)

  // Description:
  // If GetPythonSupport() is true, returns the path to the python libraries
  // detected on the server.
  vtkSetMacro(PythonPath, std::string)
  vtkGetMacro(PythonPath, std::string)

  // Description:
  // Whether the numpy module is available on the server.
  vtkSetMacro(NumpySupport, bool)
  vtkGetMacro(NumpySupport, bool)
  vtkBooleanMacro(NumpySupport, bool)

  // Description:
  // If GetNumpySupport() is true, returns the version of numpy detected on the
  // server.
  vtkSetMacro(NumpyVersion, std::string)
  vtkGetMacro(NumpyVersion, std::string)

  // Description:
  // If GetNumpySupport() is true, returns the path to numpy detected on the
  // server.
  vtkSetMacro(NumpyPath, std::string)
  vtkGetMacro(NumpyPath, std::string)

  // Description:
  // Whether the matplotlib module is available on the server.
  vtkSetMacro(MatplotlibSupport, bool)
  vtkGetMacro(MatplotlibSupport, bool)
  vtkBooleanMacro(MatplotlibSupport, bool)

  // Description:
  // If GetMatplotlibSupport() is true, returns the version of matplotlib
  // detected on the server.
  vtkSetMacro(MatplotlibVersion, std::string)
  vtkGetMacro(MatplotlibVersion, std::string)

  // Description:
  // If GetMatplotlibSupport() is true, returns the path to matplotlib detected
  // on the server.
  vtkSetMacro(MatplotlibPath, std::string)
  vtkGetMacro(MatplotlibPath, std::string)

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
