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
*/

#ifndef vtkSMSILDomain_h
#define vtkSMSILDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMArraySelectionDomain.h"

class vtkGraph;
class vtkPVSILInformation;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMSILDomain : public vtkSMArraySelectionDomain
{
public:
  static vtkSMSILDomain* New();
  vtkTypeMacro(vtkSMSILDomain, vtkSMArraySelectionDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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
  virtual int SetDefaultValues(vtkSMProperty*, bool) VTK_OVERRIDE { return 1; }

protected:
  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem) VTK_OVERRIDE;

  // Internal method used to store the SubTree information from the XML
  vtkSetStringMacro(SubTree);

  vtkSMSILDomain();
  ~vtkSMSILDomain();

  char* SubTree;
  vtkPVSILInformation* SIL;
  vtkIdType SILTimeStamp;

private:
  vtkSMSILDomain(const vtkSMSILDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMSILDomain&) VTK_DELETE_FUNCTION;
};

#endif
