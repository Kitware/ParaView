/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVInformation
 * @brief   Superclass for information objects.
 *
 * Subclasses of this class are used to get information from the server.
*/

#ifndef vtkPVInformation_h
#define vtkPVInformation_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkClientServerStream;
class vtkMultiProcessStream;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVInformation : public vtkObject
{
public:
  vtkTypeMacro(vtkPVInformation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  virtual void CopyToStream(vtkClientServerStream*) = 0;
  virtual void CopyFromStream(const vtkClientServerStream*);
  //@}

  //@{
  /**
   * Serialize/Deserialize the parameters that control how/what information is
   * gathered. This are different from the ivars that constitute the gathered
   * information itself. For example, PortNumber on vtkPVDataInformation
   * controls what output port the data-information is gathered from.
   */
  virtual void CopyParametersToStream(vtkMultiProcessStream&){};
  virtual void CopyParametersFromStream(vtkMultiProcessStream&){};
  //@}

  //@{
  /**
   * Set/get whether to gather information only from the root.
   */
  vtkGetMacro(RootOnly, int);
  //@}

protected:
  vtkPVInformation();
  ~vtkPVInformation() override;

  int RootOnly;
  vtkSetMacro(RootOnly, int);

  vtkPVInformation(const vtkPVInformation&) = delete;
  void operator=(const vtkPVInformation&) = delete;
};

#endif
