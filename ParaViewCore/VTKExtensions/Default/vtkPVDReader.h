/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDReader
 * @brief   ParaView-specific vtkXMLCollectionReader subclass
 *
 * vtkPVDReader subclasses vtkXMLCollectionReader to add
 * ParaView-specific methods.
*/

#ifndef vtkPVDReader_h
#define vtkPVDReader_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkXMLCollectionReader.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVDReader : public vtkXMLCollectionReader
{
public:
  static vtkPVDReader* New();
  vtkTypeMacro(vtkPVDReader, vtkXMLCollectionReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the required value for the timestep attribute.  The value
   * should be referenced by its index.  Only data sets matching this
   * value will be read.  An out-of-range index will remove the
   * restriction.
   */
  void SetTimeStep(int index) VTK_OVERRIDE;
  int GetTimeStep() VTK_OVERRIDE;
  //@}

protected:
  vtkPVDReader();
  ~vtkPVDReader();

  void ReadXMLData() VTK_OVERRIDE;

  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  // Set TimeStepRange
  virtual void SetupOutputInformation(vtkInformation* outInfo) VTK_OVERRIDE;

private:
  vtkPVDReader(const vtkPVDReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVDReader&) VTK_DELETE_FUNCTION;
};

#endif
