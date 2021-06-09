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
 * @class vtkPVSILInformation
 * @brief vtkPVInformation subclass to fetch SIL.
 *
 * Information object used to retrieve the SIL from an algorithm. It supports
 * both legacy representation of SIL (using a vtkGraph).
 *
 * This is largely obsolete. Refer to vtkPVDataAssemblyInformation instead for
 * newer code for similar use-cases.
 */

#ifndef vtkPVSILInformation_h
#define vtkPVSILInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"       // needed for vtkSmartPointer.

class vtkGraph;

class VTKREMOTINGCORE_EXPORT vtkPVSILInformation : public vtkPVInformation
{
public:
  static vtkPVSILInformation* New();
  vtkTypeMacro(vtkPVSILInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  //@{
  /**
   * Returns the SIL. This returns the legacy SIL representation, if any.
   */
  vtkGetObjectMacro(SIL, vtkGraph);
  //@}

protected:
  vtkPVSILInformation();
  ~vtkPVSILInformation() override;

  void SetSIL(vtkGraph*);
  vtkGraph* SIL;

private:
  vtkPVSILInformation(const vtkPVSILInformation&) = delete;
  void operator=(const vtkPVSILInformation&) = delete;
};

#endif
