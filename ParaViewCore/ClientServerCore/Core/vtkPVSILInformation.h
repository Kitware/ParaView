/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSILInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVSILInformation
 *
 * Information object used to retreived the SIL graph from a file reader or
 * any compatible source.
*/

#ifndef vtkPVSILInformation_h
#define vtkPVSILInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkGraph;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVSILInformation : public vtkPVInformation
{
public:
  static vtkPVSILInformation* New();
  vtkTypeMacro(vtkPVSILInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*) VTK_OVERRIDE;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE;
  virtual void CopyFromStream(const vtkClientServerStream*) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Returns the SIL.
   */
  vtkGetObjectMacro(SIL, vtkGraph);
  //@}

protected:
  vtkPVSILInformation();
  ~vtkPVSILInformation();

  void SetSIL(vtkGraph*);
  vtkGraph* SIL;

private:
  vtkPVSILInformation(const vtkPVSILInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVSILInformation&) VTK_DELETE_FUNCTION;
};

#endif
