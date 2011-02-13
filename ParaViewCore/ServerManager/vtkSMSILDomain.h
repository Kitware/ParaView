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
// .NAME vtkSMSILDomain - is a specialization for vtkSMArraySelectionDomain with
// access to the SIL.
// .SECTION Description
// vtkSMSILDomain is basically a vtkSMArraySelectionDomain with a method to
// access the SIL. Having a separate domain also makes it possible to
// automatically create SIL widgets in the GUI.
//
// vtkSMSILDomain needs a required property with function="ArrayList" which is
// typically an information property with the array selection statuses (exactly
// similar to vtkSMArraySelectionDomain) with one notable exception. This
// information property typically uses the vtkSMSILInformationHelper which is
// used to access the SIL if requested by using GetSIL().
#ifndef __vtkSMSILDomain_h
#define __vtkSMSILDomain_h

#include "vtkSMArraySelectionDomain.h"

class vtkGraph;
class vtkPVSILInformation;

class VTK_EXPORT vtkSMSILDomain : public vtkSMArraySelectionDomain
{
public:
  static vtkSMSILDomain* New();
  vtkTypeMacro(vtkSMSILDomain, vtkSMArraySelectionDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the SIL. This does not result in the re-fetching of the SIL, it simply
  // returns the most recently fetched SIL. To re-fetch the SIL, try calling
  // UpdatePropertyInformation() on the reader proxy. That will result in
  // requesting the vtkSMSILInformationHelper to fetch the SIL.
  vtkGraph* GetSIL();

  // Description:
  // Provide an access to the subtree attribute from the XML definition of
  // the sub-domaine
  vtkGetStringMacro(SubTree);

//BTX
protected:

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem);

  // Internal method used to store the SubTree information from the XML
  vtkSetStringMacro(SubTree);

  vtkSMSILDomain();
  ~vtkSMSILDomain();

  char* SubTree;
  vtkPVSILInformation* SIL;
  vtkIdType SILTimeStamp;

private:
  vtkSMSILDomain(const vtkSMSILDomain&); // Not implemented
  void operator=(const vtkSMSILDomain&); // Not implemented
//ETX
};

#endif

