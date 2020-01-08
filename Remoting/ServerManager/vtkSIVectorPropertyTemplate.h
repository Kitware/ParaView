/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIVectorPropertyTemplate
 *
 *
*/

#ifndef vtkSIVectorPropertyTemplate_h
#define vtkSIVectorPropertyTemplate_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIVectorProperty.h"

template <class T, class force_idtype = int>
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIVectorPropertyTemplate : public vtkSIVectorProperty
{
public:
  typedef vtkSIVectorProperty Superclass;
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * If ArgumentIsArray is true, multiple elements are passed in as
   * array arguments. For example, For example, if
   * RepeatCommand is true, NumberOfElementsPerCommand is 2, the
   * command is SetFoo and the values are 1 2 3 4 5 6, the resulting
   * stream will have:
   * @verbatim
   * * Invoke obj SetFoo array(1, 2)
   * * Invoke obj SetFoo array(3, 4)
   * * Invoke obj SetFoo array(5, 6)
   * @endverbatim
   */
  vtkGetMacro(ArgumentIsArray, bool);
  //@}

protected:
  vtkSIVectorPropertyTemplate();
  ~vtkSIVectorPropertyTemplate() override;

  /**
   * Push a new state to the underneath implementation
   */
  bool Push(vtkSMMessage*, int) override;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

  /**
   * Parse the xml for the property.
   */
  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) override;

  /**
   * Implements the actual push.
   */
  bool Push(T* values, int number_of_elements);

  bool ArgumentIsArray;

private:
  vtkSIVectorPropertyTemplate(const vtkSIVectorPropertyTemplate&) = delete;
  void operator=(const vtkSIVectorPropertyTemplate&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkSIVectorPropertyTemplate.h
