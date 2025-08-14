// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkStringList
 * @brief Manages allocation and freeing for a string list.
 *
 * A vtkStringList holds a list of strings.
 * We might be able to replace it in the future.
 */

#ifndef vtkStringList_h
#define vtkStringList_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkStringFormatter.h"           // for vtk::printf_to_std_format

#include <memory> // for std::unique_ptr

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkStringList : public vtkObject
{
public:
  static vtkStringList* New();
  vtkTypeMacro(vtkStringList, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Add a simple string.
   */
  void AddString(const char* str);
  void AddUniqueString(const char* str);
  ///@}

  /**
   * Add a command and format it any way you like.
   */
  template <typename... T>
  void AddFormattedString(const char* EventString, T&&... args)
  {
    std::string format = EventString ? EventString : "";
    if (vtk::is_printf_format(format))
    {
      // PARAVIEW_DEPRECATED_IN_6_1_0
      vtkWarningMacro(<< "The given format " << format << " is a printf format. The format will be "
                      << "converted to std::format. This conversion has been deprecated in 6.1.0");
      format = vtk::printf_to_std_format(format);
    }
    auto formatedString = vtk::format(format, std::forward<T>(args)...);
    this->AddString(formatedString.c_str());
  }

  /**
   * Initialize to empty.
   */
  void RemoveAllItems();

  /**
   * Random access.
   */
  void SetString(int idx, const char* str);

  /**
   * Get the length of the list.
   */
  int GetLength() { return this->GetNumberOfStrings(); }

  /**
   * Get the index of a string.
   */
  int GetIndex(const char* str);

  /**
   * Get a command from its index.
   */
  const char* GetString(int idx);

  /**
   * Returns the number of strings.
   */
  int GetNumberOfStrings();

protected:
  vtkStringList();
  ~vtkStringList() override;

private:
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  vtkStringList(const vtkStringList&) = delete;
  void operator=(const vtkStringList&) = delete;
};

#endif
