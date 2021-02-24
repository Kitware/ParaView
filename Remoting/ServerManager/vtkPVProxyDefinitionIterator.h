/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProxyDefinitionIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVProxyDefinitionIterator
 * @brief   iterates over all proxy definitions
 * from which the vtkSMProxyManager can use to create new proxy.
 * It can also iterate over registered compound proxy definitions.
 *
 * vtkPVProxyDefinitionIterator iterates over all proxy definitions known to
 * the proxy manager. The iterator allow to filter the iteration on a
 * subset of group and/or on the global or custom proxies.
 * Custom and Compound proxy are exactly the same thing. We should stick with
 * only one name.
 * @sa
 * vtkSMProxyManager
*/

#ifndef vtkPVProxyDefinitionIterator_h
#define vtkPVProxyDefinitionIterator_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

class vtkPVXMLElement;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVProxyDefinitionIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkPVProxyDefinitionIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // ********* Iterator Common methods **********

  /**
   * Move the iterator to the beginning.
   */
  virtual void GoToFirstItem() = 0;

  /**
   * Reset the iterator and move it to the head.
   */
  virtual void InitTraversal() { this->GoToFirstItem(); }

  /**
   * Move the iterator to the next item. This will also move to next group
   * automatically if needed.
   */
  virtual void GoToNextItem() = 0;

  /**
   * Move the iterator to the next group.
   */
  virtual void GoToNextGroup() = 0;

  /**
   * Test whether the iterator is currently pointing to a valid
   * item.
   */
  virtual bool IsDoneWithTraversal() = 0;

  // ********* Configuration methods **********

  virtual void AddTraversalGroupName(const char* groupName) = 0;

  // ********* Access methods **********

  // Access methods
  /// Return the current group name or nullptr if Next() was never called.
  virtual const char* GetGroupName() = 0;
  /// Return the current proxy name or nullptr if Next() was never called.
  virtual const char* GetProxyName() = 0;
  /// Return true if the current definition has been defined in the Custom scope
  virtual bool IsCustom() { return false; };
  /// Return the current XML proxy definition
  virtual vtkPVXMLElement* GetProxyDefinition() = 0;
  /// Return the current XML proxy hints
  virtual vtkPVXMLElement* GetProxyHints() = 0;

protected:
  vtkPVProxyDefinitionIterator();
  ~vtkPVProxyDefinitionIterator() override;

private:
  vtkPVProxyDefinitionIterator(const vtkPVProxyDefinitionIterator&) = delete;
  void operator=(const vtkPVProxyDefinitionIterator&) = delete;
};

#endif
