/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClassNameInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVClassNameInformation
 * @brief   Holds class name
 *
 * This information object gets the class name of the input VTK object.  This
 * is separate from vtkPVDataInformation because it can be determined before
 * Update is called and because it operates on any VTK object.
*/

#ifndef vtkPVClassNameInformation_h
#define vtkPVClassNameInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVClassNameInformation : public vtkPVInformation
{
public:
  static vtkPVClassNameInformation* New();
  vtkTypeMacro(vtkPVClassNameInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Port number controls which output port the information is gathered from.
   * This is only applicable when the vtkObject from which the information being
   * gathered is a vtkAlgorithm. Set it to -1(default), to return the classname
   * of the vtkAlgorithm itself.
   * This is the only parameter that can be set on  the client-side before
   * gathering the information.
   */
  vtkSetMacro(PortNumber, int);
  //@}

  //@{
  /**
   * Get class name of VTK object.
   */
  vtkGetStringMacro(VTKClassName);
  //@}

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*);

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation*);

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //@}

  //@{
  /**
   * Serialize/Deserialize the parameters that control how/what information is
   * gathered. This are different from the ivars that constitute the gathered
   * information itself. For example, PortNumber on vtkPVDataInformation
   * controls what output port the data-information is gathered from.
   */
  virtual void CopyParametersToStream(vtkMultiProcessStream&);
  virtual void CopyParametersFromStream(vtkMultiProcessStream&);
  //@}

protected:
  vtkPVClassNameInformation();
  ~vtkPVClassNameInformation();

  int PortNumber;
  char* VTKClassName;
  vtkSetStringMacro(VTKClassName);

private:
  vtkPVClassNameInformation(const vtkPVClassNameInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVClassNameInformation&) VTK_DELETE_FUNCTION;
};

#endif
