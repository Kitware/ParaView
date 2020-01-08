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
/**
 * @class   vtkSMProxySelectionModel
 * @brief   selects proxies.
 *
 * vtkSMProxySelectionModel is used to select proxies. vtkSMProxyManager uses
 * two instances of vtkSMProxySelectionModel for keeping track of the
 * selected/active sources/filters and the active view.
 * @sa
 * vtkSMProxyManager
*/

#ifndef vtkSMProxySelectionModel_h
#define vtkSMProxySelectionModel_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"            // Needed for vtkSMMessage*
#include "vtkSMRemoteObject.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkSMProxySelectionModelInternal;
class vtkCollection;
class vtkSMProxy;

#include <list> // needed for vtkset::list.

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMProxySelectionModel : public vtkSMRemoteObject
{
public:
  static vtkSMProxySelectionModel* New();
  vtkTypeMacro(vtkSMProxySelectionModel, vtkSMRemoteObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override the set session, so we can attach an observer to the Collaboration
   * manager in order to monitor master/slave changes.
   */
  void SetSession(vtkSMSession*) override;

  //@{
  /**
   * Allow to synchronize the active object with master or just keep remote object
   * out-of-synch. Only the state from the master will be loaded.
   */
  void SetFollowingMaster(bool following);
  bool IsFollowingMaster();
  //@}

  /**
   * Type for selection.
   */
  typedef std::list<vtkSmartPointer<vtkSMProxy> > SelectionType;

  // vtkSMProxy selection flags
  enum ProxySelectionFlag
  {
    NO_UPDATE = 0,
    CLEAR = 1,
    SELECT = 2,
    DESELECT = 4,
    CLEAR_AND_SELECT = CLEAR | SELECT
  };

  /**
   * Returns the proxy that is current, NULL if there is no current.
   */
  vtkSMProxy* GetCurrentProxy();

  /**
   * Sets the current proxy. \c command is used to control how the current
   * selection is affected.
   * \li NO_UPDATE: change the current without affecting the selected set of
   * proxies.
   * \li CLEAR: clear current selection
   * \li SELECT: also select the proxy being set as current
   * \li DESELECT: deselect the proxy being set as current.
   */
  void SetCurrentProxy(vtkSMProxy* proxy, int command);

  /**
   * Returns true if the proxy is selected.
   */
  bool IsSelected(vtkSMProxy* proxy);

  // Returns the number of selected proxies.
  unsigned int GetNumberOfSelectedProxies();

  /**
   * Returns the selected proxy at the given index.
   */
  vtkSMProxy* GetSelectedProxy(unsigned int index);

  /**
   * Returns the selection set.
   */
  const SelectionType& GetSelection() { return this->Selection; }

  /**
   * Update the selected set of proxies. \c command affects how the selection is
   * updated.
   * \li NO_UPDATE: don't affect the selected set of proxies.
   * \li CLEAR: clear selection
   * \li SELECT: add the proxies to selection
   * \li DESELECT: deselect the proxies
   * \li CLEAR_AND_SELECT: clear selection and then add the specified proxies as
   * the selection.
   */

  void Select(const SelectionType& proxies, int command);

  void Select(vtkSMProxy* proxy, int command);

  /**
   * Wrapper friendly methods to doing what Select() can do.
   */
  void Clear() { this->Select(NULL, CLEAR); }
  void Select(vtkSMProxy* proxy) { this->Select(proxy, SELECT); }
  void Deselect(vtkSMProxy* proxy) { this->Select(proxy, DESELECT); }
  void ClearAndSelect(vtkSMProxy* proxy) { this->Select(proxy, CLEAR_AND_SELECT); }

  /**
   * Utility method to get the data bounds for the currently selected items.
   * This only makes sense for selections comprising of source-proxies or
   * output-port proxies.
   * Returns true is the bounds are valid.
   */
  bool GetSelectionDataBounds(double bounds[6]);

  /**
   * This method return the full object state that can be used to create that
   * object from scratch.
   * This method will be used to fill the undo stack.
   * If not overridden this will return NULL.
   */
  const vtkSMMessage* GetFullState() override;

  /**
   * This method is used to initialise the object to the given state
   * If the definitionOnly Flag is set to True the proxy won't load the
   * properties values and just setup the new proxy hierarchy with all subproxy
   * globalID set. This allow to split the load process in 2 step to prevent
   * invalid state when property refere to a sub-proxy that does not exist yet.
   */
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;

protected:
  vtkSMProxySelectionModel();
  ~vtkSMProxySelectionModel() override;

  void InvokeCurrentChanged(vtkSMProxy* proxy);
  void InvokeSelectionChanged();

  vtkSmartPointer<vtkSMProxy> Current;
  SelectionType Selection;

  /**
   * When the state has changed we call that method so the state can be shared
   * is any collaboration is involved
   */
  void PushStateToSession();

  // Cached version of State
  vtkSMMessage* State;

private:
  vtkSMProxySelectionModel(const vtkSMProxySelectionModel&) = delete;
  void operator=(const vtkSMProxySelectionModel&) = delete;

  class vtkInternal;
  friend class vtkInternal;
  vtkInternal* Internal;
};

#endif
