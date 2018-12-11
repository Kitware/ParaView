/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDataInformationHelper
 * @brief   allows extension of types that
 * PVDataInformation can handle
 *
 *
 * Plugins can subclass this and call vtkPVDataInformation::RegisterHelper()
 * in order to allow vtkPVDataInformation (and thus ParaView) to handle new
 * data types.
*/

#ifndef vtkPVDataInformationHelper_h
#define vtkPVDataInformationHelper_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkPVDataInformation;
class vtkDataObject;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVDataInformationHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkPVDataInformationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This class is a friend of PVDI, so the work of copying
   * into self happens here.
   */
  void CopyFromDataObject(vtkPVDataInformation* self, vtkDataObject* data);

  virtual const char* GetPrettyDataTypeString() = 0;

protected:
  vtkPVDataInformationHelper();
  ~vtkPVDataInformationHelper() override;

  vtkDataObject* Data; // not reference counted

  virtual bool ValidateType(vtkDataObject* data) = 0;

  // API to access information from data that I fill the
  // PVDataInformation I am friend of with.
  virtual double* GetBounds() = 0;
  virtual int GetNumberOfDataSets() = 0;
  virtual vtkTypeInt64 GetNumberOfCells() = 0;
  virtual vtkTypeInt64 GetNumberOfPoints() = 0;
  virtual vtkTypeInt64 GetNumberOfRows() = 0;

private:
  vtkPVDataInformationHelper(const vtkPVDataInformationHelper&) = delete;
  void operator=(const vtkPVDataInformationHelper&) = delete;
};

#endif
