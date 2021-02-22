/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeAnimationCueProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMComparativeAnimationCueProxy
 * @brief   cue used for parameter animation by
 * the comparative view.
 *
 * vtkSMComparativeAnimationCueProxy is a animation cue used for parameter
 * animation by the vtkSMComparativeViewProxy. It provides a non-conventional
 * API i.e. without using properties to allow the user to setup parameter
 * values over the comparative grid.
*/

#ifndef vtkSMComparativeAnimationCueProxy_h
#define vtkSMComparativeAnimationCueProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkPVComparativeAnimationCue;

class VTKREMOTINGVIEWS_EXPORT vtkSMComparativeAnimationCueProxy : public vtkSMProxy
{
public:
  static vtkSMComparativeAnimationCueProxy* New();
  vtkTypeMacro(vtkSMComparativeAnimationCueProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Methods simply forwarded to vtkPVComparativeAnimationCue.
   * Any of these methods changing the state of the proxy, also call
   * this->MarkModified(this).
   */
  void UpdateXRange(int y, double minx, double maxx);
  void UpdateYRange(int x, double miny, double maxy);
  void UpdateWholeRange(double mint, double maxt);
  void UpdateValue(int x, int y, double value);
  void UpdateXRange(int y, double* minx, double* maxx, unsigned int numvalues);
  void UpdateYRange(int x, double* minx, double* maxx, unsigned int numvalues);
  void UpdateWholeRange(double* mint, double* maxt, unsigned int numValues);
  void UpdateWholeRange(double* mint, double* maxt, unsigned int numValues, bool vertical_first);
  void UpdateValue(int x, int y, double* value, unsigned int numValues);
  double* GetValues(int x, int y, int dx, int dy, unsigned int& numValues);
  double GetValue(int x, int y, int dx, int dy);
  void UpdateAnimatedValue(int x, int y, int dx, int dy);
  //@}

  /**
   * Saves the state of the proxy. This state can be reloaded
   * to create a new proxy that is identical the present state of this proxy.
   * The resulting proxy's XML hieratchy is returned, in addition if the root
   * argument is not nullptr then it's also inserted as a nested element.
   * This call saves all a proxy's properties, including exposed properties
   * and sub-proxies. More control is provided by the following overload.
   */
  vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root) override
  {
    return this->Superclass::SaveXMLState(root);
  }

  /**
   * The iterator is use to filter the property available on the given proxy
   */
  vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root, vtkSMPropertyIterator* iter) override;

  /**
   * Loads the proxy state from the XML element. Returns 0 on failure.
   * \c locator is used to locate other proxies that may be referred to in the
   * state XML (which happens in case of properties of type vtkSMProxyProperty
   * or subclasses). If locator is nullptr, then such properties are left
   * unchanged.
   */
  int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator) override;

protected:
  vtkSMComparativeAnimationCueProxy();
  ~vtkSMComparativeAnimationCueProxy() override;

  /**
   * Given a class name (by setting VTKClassName) and server ids (by
   * setting ServerIDs), this methods instantiates the objects on the
   * server(s)
   */
  void CreateVTKObjects() override;

  // Method used to simplify the access to the concreate VTK class underneath
  friend class vtkSMComparativeAnimationCueUndoElement;
  vtkPVComparativeAnimationCue* GetComparativeAnimationCue();

private:
  vtkSMComparativeAnimationCueProxy(const vtkSMComparativeAnimationCueProxy&) = delete;
  void operator=(const vtkSMComparativeAnimationCueProxy&) = delete;

  class vtkInternal;
  vtkInternal* Internals;
};

#endif
