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
 * both legacy representation of SIL (using a vtkGraph) or the newer form (using
 * vtkSubsetInclusionLattice or subclass).
 *
 * If `CopyFromObject` is called with vtkAlgorithmOutput as the argument, it
 * looks for presence of `vtkSubsetInclusionLattice::SUBSET_INCLUSION_LATTICE()`
 * or `vtkDataObject::SIL()` (for legacy SIL) key in the output information for
 * the corresponding `vtkAlgorithm`.
*/

#ifndef vtkPVSILInformation_h
#define vtkPVSILInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"       // needed for vtkSmartPointer.

class vtkGraph;
class vtkSubsetInclusionLattice;

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

  //@{
  /**
   * Returns the SIL represented using vtkSubsetInclusionLattice.
   */
  vtkSubsetInclusionLattice* GetSubsetInclusionLattice() const;
  //@}

protected:
  vtkPVSILInformation();
  ~vtkPVSILInformation() override;

  void SetSIL(vtkGraph*);
  vtkGraph* SIL;
  vtkSmartPointer<vtkSubsetInclusionLattice> SubsetInclusionLattice;

private:
  vtkPVSILInformation(const vtkPVSILInformation&) = delete;
  void operator=(const vtkPVSILInformation&) = delete;
};

#endif
