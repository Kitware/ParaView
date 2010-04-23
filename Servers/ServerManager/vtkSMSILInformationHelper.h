/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSILInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSILInformationHelper - populates a vtkSMStringVectorProperty using
// the list of arrays obtained via a SIL.
// .SECTION Description
// vtkSMSILInformationHelper populates a vtkSMStringVectorProperty using
// the list of arrays obtained via a SIL.
//
// The xml can has 2 attributes:
// \li subtree -- identifies the subtree in the SIL whose leaf nodes are to
// returned as the value in the property.
// \li timestamp_command -- specifies a method name that can be used to get the
// last time the SIL was updated on the server. This makes it possible to avoid
// re-fetches of the SIL when not necessary (since SIL's can be huge and
// fetching them can be time consuming).
//
// Note: array status is not provided by this helper. SIL
// does not provide information about the current selection and hence this
// information helper has no way of fetching that information.

#ifndef __vtkSMSILInformationHelper_h
#define __vtkSMSILInformationHelper_h

#include "vtkSMInformationHelper.h"

class vtkGraph;
class vtkSMStringVectorProperty;

class VTK_EXPORT vtkSMSILInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMSILInformationHelper* New();
  vtkTypeMacro(vtkSMSILInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using value obtained for server. It creates
  // an instance of the server helper class vtkPVServerArraySelection
  // and passes the objectId (which the helper class gets as a pointer)
  // and populates the property using the values returned.
  // Each array is represented by two components:
  // name, state (on/off)  
  virtual void UpdateProperty(
    vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

  // Description:
  vtkGetObjectMacro(SIL, vtkGraph);
  vtkGetStringMacro(Subtree);

//BTX
protected:
  vtkSMSILInformationHelper();
  ~vtkSMSILInformationHelper();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty*, vtkPVXMLElement*);

  // Description:
  // checks if the server side mtime has changed since last call, if so then
  // alone do we refetch the SIL.
  bool CheckMTime(vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId);

  void UpdateArrayList(vtkSMStringVectorProperty*);

  vtkSetStringMacro(TimestampCommand);
  vtkSetStringMacro(Subtree);

  char* TimestampCommand;
  char* Subtree;

  int LastUpdateTime;

  vtkGraph* SIL;
private:
  vtkSMSILInformationHelper(const vtkSMSILInformationHelper&); // Not implemented
  void operator=(const vtkSMSILInformationHelper&); // Not implemented
  
  // Internal method to set the SIL.
  void SetSIL(vtkGraph*);
//ETX
};

#endif

