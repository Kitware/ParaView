// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVIOSettings
 * @brief   singleton used to control how ParaView open and save files
 *
 * Represents the "IO" section of ParaView settings.
 *
 * vtkPVIOSettings is a singleton used to keep track
 * of readers/writers settings in ParaView, as well as UI for the `Open
 * File Dialog`.
 *
 * All calls to
 * vtkPVIOSettings::New() returns a reference to the
 * singleton instance.
 */

#ifndef vtkPVIOSettings_h
#define vtkPVIOSettings_h

#include "vtkObject.h"
#include "vtkRemotingViewsModule.h" // needed for exports
#include "vtkSmartPointer.h"        // needed for vtkSmartPointer

#include <memory> // needed for unique_ptr

class vtkStringArray;
class vtkDataArraySelection;

class VTKREMOTINGVIEWS_EXPORT vtkPVIOSettings : public vtkObject
{
public:
  static vtkPVIOSettings* New();
  vtkTypeMacro(vtkPVIOSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Access the singleton.
   */
  static vtkPVIOSettings* GetInstance();

  ///@{
  /**
   * Set/get the number of excluded name filters.
   */
  virtual void SetNumberOfExcludedNameFilters(int n);
  virtual int GetNumberOfExcludedNameFilters();
  ///@}

  ///@{
  /**
   * Set/get the excluded name filter at index i. If the index is
   * outside the valid range, this call is a noop.
   */
  virtual void SetExcludedNameFilter(int i, const char* expression);
  virtual const char* GetExcludedNameFilter(int i);
  ///@}

  /**
   * Get all reader names, used by the Reader selection logic
   * XXX: This method and property names should be improved
   */
  vtkStringArray* GetAllNameFilters();

protected:
  vtkPVIOSettings();
  ~vtkPVIOSettings() override;

private:
  vtkPVIOSettings(const vtkPVIOSettings&) = delete;
  void operator=(const vtkPVIOSettings&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
