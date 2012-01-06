/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxySelectionModel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxySelectionModel - selects proxies. 
// .SECTION Description
// vtkSMProxySelectionModel is used to select proxies. vtkSMProxyManager uses
// two instances of vtkSMProxySelectionModel for keeping track of the
// selected/active sources/filters and the active view.
// .SECTION See Also
// vtkSMProxyManager

#ifndef __vtkSMProxySelectionModel_h
#define __vtkSMProxySelectionModel_h

#include "vtkSMRemoteObject.h"
#include "vtkSMMessageMinimal.h" // Needed for vtkSMMessage*
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkSMProxySelectionModelInternal;
class vtkCollection;
class vtkSMProxy;

//BTX
#include <set> // needed for vtkset::set.
//ETX

class VTK_EXPORT vtkSMProxySelectionModel : public vtkSMRemoteObject
{
public:
  static vtkSMProxySelectionModel*  New();
  vtkTypeMacro(vtkSMProxySelectionModel,  vtkSMRemoteObject);
  void PrintSelf(ostream&  os,  vtkIndent  indent);

  // Description:
  // Override the set session, so we can attach an observer to the Collaboration
  // manager in order to monitor master/slave changes.
  virtual void SetSession(vtkSMSession*);

  // Description:
  // Allow to synchronize the active object with master or just keep remote object
  // out-of-synch. Only the state from the master will be loaded.
  void SetFollowingMaster(bool following);
  bool IsFollowingMaster();
 
//BTX
  // Description:
  // Type for selection.
  typedef std::set<vtkSmartPointer<vtkSMProxy> > SelectionType;

  // vtkSMProxy selection flags
  enum ProxySelectionFlag {
    NO_UPDATE        = 0,
    CLEAR            = 1,
    SELECT           = 2,
    DESELECT         = 4,
    CLEAR_AND_SELECT = CLEAR | SELECT
  };
//ETX

  // Description:
  // Returns the proxy that is current, NULL if there is no current.
  vtkSMProxy* GetCurrentProxy();

  // Description:
  // Sets the current proxy. \c command is used to control how the current
  // selection is affected. 
  // \li NO_UPDATE: change the current without affecting the selected set of
  // proxies.
  // \li CLEAR: clear current selection
  // \li SELECT: also select the proxy being set as current
  // \li DESELECT: deselect the proxy being set as current.
  void SetCurrentProxy(vtkSMProxy*  proxy,  int  command);

  // Description:
  // Returns true if the proxy is selected.
  bool IsSelected(vtkSMProxy*  proxy);

  // Returns the number of selected proxies.
  unsigned int GetNumberOfSelectedProxies();
  
  // Description:
  // Returns the selected proxy at the given index.
  vtkSMProxy* GetSelectedProxy(unsigned int  index);

  //BTX
  // Description:
  // Returns the selection set. 
  const SelectionType& GetSelection() { return this->Selection; }
  //ETX

  // Description:
  // Update the selected set of proxies. \c command affects how the selection is
  // updated.
  // \li NO_UPDATE: don't affect the selected set of proxies.
  // \li CLEAR: clear selection
  // \li SELECT: add the proxies to selection
  // \li DESELECT: deselect the proxies
  // \li CLEAR_AND_SELECT: clear selection and then add the specified proxies as
  // the selection.
  //BTX
  void Select(const SelectionType &proxies,  int command);
  //ETX
  void Select(vtkSMProxy* proxy,  int command);
 
  // Description:
  // Wrapper friendly methods to doing what Select() can do.
  void Clear()
    { this->Select(NULL, CLEAR); }
  void Select(vtkSMProxy*  proxy)
    { this->Select(proxy, SELECT); }
  void Deselect(vtkSMProxy*  proxy)
    { this->Select(proxy, DESELECT); }
  void ClearAndSelect(vtkSMProxy*  proxy)
    { this->Select(proxy, CLEAR_AND_SELECT); }

  // Description:
  // Utility method to get the data bounds for the currently selected items.
  // This only makes sense for selections comprising of source-proxies or
  // output-port proxies.
  // Returns true is the bounds are valid.
  bool GetSelectionDataBounds(double bounds[6]);

//BTX 

  // Description:
  // This method return the full object state that can be used to create that
  // object from scratch.
  // This method will be used to fill the undo stack.
  // If not overriden this will return NULL.
  virtual const vtkSMMessage* GetFullState();

  // Description:
  // This method is used to initialise the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalID set. This allow to split the load process in 2 step to prevent
  // invalid state when property refere to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* msg, vtkSMProxyLocator* locator);

protected:
  vtkSMProxySelectionModel();
  ~vtkSMProxySelectionModel();

  void InvokeCurrentChanged(vtkSMProxy*  proxy);
  void InvokeSelectionChanged();

  vtkSmartPointer<vtkSMProxy> Current;
  SelectionType Selection;

  // Description:
  // When the state has changed we call that method so the state can be shared
  // is any collaboration is involved
  void PushStateToSession();
  
  // Cached version of State
  vtkSMMessage* State;

private:
  vtkSMProxySelectionModel(const  vtkSMProxySelectionModel&); // Not implemented
  void operator = (const  vtkSMProxySelectionModel&); // Not implemented

  class vtkInternal;
  friend class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
