/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSILDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMSILDomain
 * @brief   is a specialization for vtkSMArraySelectionDomain with
 * access to the SIL.
 *
 * vtkSMSILDomain is basically a vtkSMArraySelectionDomain with a method to
 * access the SIL. Having a separate domain also makes it possible to
 * automatically create SIL widgets in the GUI.
 *
 * vtkSMSILDomain needs a required property with function="ArrayList" which is
 * typically an information property with the array selection statuses (exactly
 * similar to vtkSMArraySelectionDomain) with one notable exception. This
 * information property typically uses the vtkSMSILInformationHelper which is
 * used to access the SIL if requested by using GetSIL().
 *
 * @section vtkSMSILDomainLegacyWarning Legacy Warning
 *
 * While not deprecated, this class exists to support readers that use legacy
 * representation for SIL which used a `vtkGraph` to represent the SIL. It is
 * recommended that newer code uses vtkSubsetInclusionLattice (or subclass) to
 * represent the SIL. In that case, you should use
 * vtkSMSubsetInclusionLatticeDomain instead.
*/

#ifndef vtkSMSILDomain_h
#define vtkSMSILDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMArraySelectionDomain.h"

class vtkGraph;
class vtkPVSILInformation;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSILDomain : public vtkSMArraySelectionDomain
{
public:
  static vtkSMSILDomain* New();
  vtkTypeMacro(vtkSMSILDomain, vtkSMArraySelectionDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the SIL. This does not result in the re-fetching of the SIL, it simply
   * returns the most recently fetched SIL. To re-fetch the SIL, try calling
   * UpdatePropertyInformation() on the reader proxy. That will result in
   * requesting the vtkSMSILInformationHelper to fetch the SIL.
   */
  vtkGraph* GetSIL();

  //@{
  /**
   * Provide an access to the subtree attribute from the XML definition of
   * the sub-domaine
   */
  vtkGetStringMacro(SubTree);
  //@}

  /**
   * Overridden to leave defaults unchanged.
   */
  int SetDefaultValues(vtkSMProperty*, bool) override { return 1; }

protected:
  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem) override;

  // Internal method used to store the SubTree information from the XML
  vtkSetStringMacro(SubTree);

  vtkSMSILDomain();
  ~vtkSMSILDomain() override;

  char* SubTree;
  vtkPVSILInformation* SIL;
  vtkIdType SILTimeStamp;

private:
  vtkSMSILDomain(const vtkSMSILDomain&) = delete;
  void operator=(const vtkSMSILDomain&) = delete;
};

#endif
