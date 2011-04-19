/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLoadStateContext.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLoadStateContext - Class use to hold context information
// for state loading.
// .SECTION Description
// vtkSMLoadStateContext is use when we load a state to handle specific
// loading mechanisme where we only want to load the proxy definition and
// keep the properties values aside or in collaboration mode where properties
// should not be updated when another client change it.
// (i.e. Views synchronization)

#ifndef __vtkSMLoadStateContext_h
#define __vtkSMLoadStateContext_h

#include "vtkSMObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" // needed for vtkWeakPointer

class vtkSMSession;
class vtkSMStateLocator;

class VTK_EXPORT vtkSMLoadStateContext : public vtkSMObject
{
public:
  static vtkSMLoadStateContext* New();
  vtkTypeMacro(vtkSMLoadStateContext,vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Flag used to know if only the definition of proxy should be used by leaving
  // properties state loading aside.
  vtkSetMacro(LoadDefinitionOnly, bool);
  vtkGetMacro(LoadDefinitionOnly, bool);

//BTX

  enum StateContextOrigin
    {
      UNDEFINED                     = 0,
      UNDO_REDO                     = 1,
      COLLABORATION_NOTIFICATION    = 2,
      RE_NEW_PROXY                  = 3,
      UPDATE_INFORMATION_PROPERTIES = 4
    };

  StateContextOrigin GetRequestOrigin()
    {
    return this->RequestOrigin;
    }

  void SetRequestOrigin(StateContextOrigin origin)
    {
    this->RequestOrigin = origin;
    }

protected:
  // Description:
  // Default constructor.
  vtkSMLoadStateContext();

  // Description:
  // Destructor.
  virtual ~vtkSMLoadStateContext();

  // Internal vars
  bool LoadDefinitionOnly;
  StateContextOrigin RequestOrigin;

private:
  vtkSMLoadStateContext(const vtkSMLoadStateContext&); // Not implemented
  void operator=(const vtkSMLoadStateContext&);       // Not implemented
//ETX
};

#endif
