/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAlgorithmPortsInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVAlgorithmPortsInformation
 * @brief   Holds number of outputs
 *
 * This information object collects the number of outputs from the
 * sources.  This is separate from vtkPVDataInformation because the number of
 * outputs can be determined before Update is called.
*/

#ifndef vtkPVAlgorithmPortsInformation_h
#define vtkPVAlgorithmPortsInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVAlgorithmPortsInformation : public vtkPVInformation
{
public:
  static vtkPVAlgorithmPortsInformation* New();
  vtkTypeMacro(vtkPVAlgorithmPortsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get number of outputs for a particular source.
   */
  vtkGetMacro(NumberOfOutputs, int);
  //@}

  //@{
  /**
   * Get the number of required inputs for a particular algorithm.
   */
  vtkGetMacro(NumberOfRequiredInputs, int);
  //@}

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*) VTK_OVERRIDE;

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation*) VTK_OVERRIDE;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE;
  virtual void CopyFromStream(const vtkClientServerStream*) VTK_OVERRIDE;
  //@}

protected:
  vtkPVAlgorithmPortsInformation();
  ~vtkPVAlgorithmPortsInformation();

  int NumberOfOutputs;
  int NumberOfRequiredInputs;

  vtkSetMacro(NumberOfOutputs, int);

private:
  vtkPVAlgorithmPortsInformation(const vtkPVAlgorithmPortsInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVAlgorithmPortsInformation&) VTK_DELETE_FUNCTION;
};

#endif
