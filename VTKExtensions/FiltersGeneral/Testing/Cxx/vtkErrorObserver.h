// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
  void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* calldata) override;

  std::string GetErrorMessage() const;
  std::string GetWarningMessage() const;

protected:
  vtkErrorObserver() = default;
  ~vtkErrorObserver() override = default;

private:
  vtkErrorObserver(const vtkErrorObserver&) = delete;
  void operator=(const vtkErrorObserver&) = delete;

  bool Error = false;
  bool Warning = false;
  std::string ErrorMessage;
  std::string WarningMessage;
};

#endif
