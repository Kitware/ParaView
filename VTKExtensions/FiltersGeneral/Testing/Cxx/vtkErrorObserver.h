/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkErrorObserver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkErrorObserver
 * @brief   Observe errors and warnings
 *
 * Use during testing to verify errors and warnings were emitted
*/

#ifndef vtkErrorObserver_h
#define vtkErrorObserver_h

#include <string>
#include <vtkCommand.h>

class vtkErrorObserver : public vtkCommand
{
public:
  static vtkErrorObserver* New();

  bool GetError() const;
  bool GetWarning() const;
  void Clear();
  virtual void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* calldata) override;

  std::string GetErrorMessage() const;
  std::string GetWarningMessage() const;

protected:
  vtkErrorObserver() = default;
  ~vtkErrorObserver() = default;

private:
  vtkErrorObserver(const vtkErrorObserver&) = delete;
  void operator=(const vtkErrorObserver&) = delete;

  bool Error = false;
  bool Warning = false;
  std::string ErrorMessage = "";
  std::string WarningMessage = "";
};

#endif
