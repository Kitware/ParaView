/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionLinkProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSelectionLinkProxy - 
// .SECTION Description

#ifndef __vtkSMSelectionLinkProxy_h
#define __vtkSMSelectionLinkProxy_h

#include "vtkSMSourceProxy.h"

class vtkSelectionLink;
class vtkSMSelectionLinkProxyObserver;

class VTK_EXPORT vtkSMSelectionLinkProxy : public vtkSMSourceProxy
{
public:
  static vtkSMSelectionLinkProxy* New();
  vtkTypeRevisionMacro(vtkSMSelectionLinkProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the selection on the server and marks the client vtkSelectionLink
  // as modified.
  void SetSelection(vtkSMProxy* selectionProxy);
  
  // Description:
  // Retrieve the client's selection link.
  vtkSelectionLink* GetSelectionLink();
  
protected:
  vtkSMSelectionLinkProxy();
  ~vtkSMSelectionLinkProxy();

  // Description:
  // Adds listeners to the client side vtkSelectionLink when
  // the objects are created.
  virtual void CreateVTKObjects();
  
  // Description:
  // Called when the client's vtkSelectionLink selection changes.
  void ClientSelectionChanged();
  
  // Description:
  // Called when the client's vtkSelectionLink is executing.
  void ClientRequestData();
  
  // Description:
  // Whether the most recent selection is on the client or server.
  // This determines whether the selection should be moved
  // from client to server or vice versa.
  bool MostRecentSelectionOnClient;
  
  // Description:
  // A flag to indicate that this class is setting the client
  // selection directly, so we ignore the selection change event.
  bool SettingClientSelection;
  
  //BTX
  friend class vtkSMSelectionLinkProxyObserver;
  vtkSMSelectionLinkProxyObserver* ClientObserver;
  //ETX
  
private:
  vtkSMSelectionLinkProxy(const vtkSMSelectionLinkProxy&); // Not implemented
  void operator=(const vtkSMSelectionLinkProxy&); // Not implemented
};

#endif
