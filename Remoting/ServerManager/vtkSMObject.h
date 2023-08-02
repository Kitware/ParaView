// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMObject
 * @brief   superclass for most server manager classes
 *
 * vtkSMObject is mostly to tag a class hierarchy that it belong to the
 * servermanager.
 */

#ifndef vtkSMObject_h
#define vtkSMObject_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMObject : public vtkObject
{
public:
  static vtkSMObject* New();
  vtkTypeMacro(vtkSMObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a well-formated label using provided name by adding
   * spaces between lower cases and upper cases:
   *
   * "MYSpace" ==> "MY Space"
   * "MYSPACE" ==> "MYSPACE"
   * "My Space" ==> "My Space"
   * "MySPACE" ==> "My SPACE"
   * "MySPace" ==> "My S Pace"
   * "MySPAce" ==> "My SP Ace"
   * "MySPACE" ==> "My SPACE"
   * "MySpACE" ==> "My Sp ACE"
   * "MYSuperSpacer" ==> "MY Super Spacer"
   *
   * Please note non-alphabetical char are not handled and may require manual
   * creation of a label instead of relying on this method.
   */
  static std::string CreatePrettyLabel(const std::string& name);

protected:
  vtkSMObject();
  ~vtkSMObject() override;

private:
  vtkSMObject(const vtkSMObject&) = delete;
  void operator=(const vtkSMObject&) = delete;
};

#endif
