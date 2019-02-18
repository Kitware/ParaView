/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTransferFunctionPresets.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMTransferFunctionPresets
 * @brief   manages presets for color, opacity,
 * and annotation presets.
 *
 * vtkSMTransferFunctionPresets is a manager to manage all color, opacity, and
 * annotation presets. It also uses vtkSMSettings to support persistent
 * customizations besides hard-coded/builtin presets.
 *
 * vtkSMTransferFunctionPresets designed to be instantiated, used and then
 * destroyed. While there is no explicit synchronization between multiple
 * instances of vtkSMTransferFunctionPresets, there should not be any need to
 * have multiple instances alive at the same time.
*/

#ifndef vtkSMTransferFunctionPresets_h
#define vtkSMTransferFunctionPresets_h

#include "vtkSMObject.h"

#include "vtkPVServerManagerRenderingModule.h" // needed for exports
#include "vtkStdString.h"                      // needed for vtkStdString.
#include <vtk_jsoncpp_fwd.h>                   // for forward declarations

class vtkPVXMLElement;
class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMTransferFunctionPresets : public vtkSMObject
{
public:
  static vtkSMTransferFunctionPresets* New();
  vtkTypeMacro(vtkSMTransferFunctionPresets, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the number of presets current available (including builtin and
   * custom).
   */
  unsigned int GetNumberOfPresets();

  /**
   * Gets the raw text for a preset given its index. The preset is provided as a JSON string.
   * Returns an empty string when not available.
   */
  vtkStdString GetPresetAsString(unsigned int index);

  /**
   * Add a new preset. This will get saved across sessions using vtkSMSettings,
   * as appropriate. If name provided already exists, this will override the
   * existing preset (even for builtin presets).
   * \c preset must be a valid JSON string. If not, this will return failure.
   */
  bool AddPreset(const char* name, const vtkStdString& preset);

  /**
   * Remove a preset. This has no effect for builtin presets.
   */
  bool RemovePreset(unsigned int index);

  /**
   * Returns a preset JSON given the name. Since multiple presets can have the
   * same name, this returns the 'first' preset with the specified name.
   */
  const Json::Value& GetFirstPresetWithName(const char* name);

  /**
   * Returns a preset at a given index.
   */
  const Json::Value& GetPreset(unsigned int index);

  /**
   * Returns the name for a preset at the given index.
   */
  vtkStdString GetPresetName(unsigned int index);

  /**
   * Returns true if a present with given name exists.
   */
  bool HasPreset(const char* name);

  /**
   * Returns true if the preset has opacities i.e. values for a piecewise function.
   */
  bool GetPresetHasOpacities(const Json::Value& preset);
  bool GetPresetHasOpacities(unsigned int index)
  {
    return this->GetPresetHasOpacities(this->GetPreset(index));
  }

  /**
   * Returns true is the preset has indexed colors.
   */
  bool GetPresetHasIndexedColors(const Json::Value& preset);
  bool GetPresetHasIndexedColors(unsigned int index)
  {
    return this->GetPresetHasIndexedColors(this->GetPreset(index));
  }

  /**
   * Returns true is the preset has annotations.
   */
  bool GetPresetHasAnnotations(const Json::Value& preset);
  bool GetPresetHasAnnotations(unsigned int index)
  {
    return this->GetPresetHasAnnotations(this->GetPreset(index));
  }

  /**
   * Returns the preset's JSON-defined default position (if any)
   * or -1 if none.
   */
  bool IsPresetDefault(const Json::Value& preset);
  bool IsPresetDefault(unsigned int index) { return this->IsPresetDefault(this->GetPreset(index)); }

  /**
   * Add a preset give the Json::Value object.
   */
  bool AddPreset(const char* name, const Json::Value& preset);

  /**
   * Same as AddPreset() expect it create a unique name using the prefix
   * provided. If no prefix is specified, "Preset" will be used as the prefix.
   */
  vtkStdString AddUniquePreset(const Json::Value& preset, const char* prefix = NULL);

  /**
   * Returns true if the preset is a builtin preset.
   * The preset is identified by its index.
   */
  bool IsPresetBuiltin(unsigned int index);

  /**
   * Rename a preset.
   */
  bool RenamePreset(unsigned int index, const char* newname);

  //@{
  /**
   * Load presets from a file. All presets are added to "custom" presets list
   * and are considered as non-builtin.
   * If the filename ends with a .xml, it's assumed to be a legacy color map XML
   * and will be converted to the new format before processing.
   */
  bool ImportPresets(const char* filename);
  bool ImportPresets(const Json::Value& presets);
  //@}

protected:
  vtkSMTransferFunctionPresets();
  ~vtkSMTransferFunctionPresets() override;

private:
  vtkSMTransferFunctionPresets(const vtkSMTransferFunctionPresets&) = delete;
  void operator=(const vtkSMTransferFunctionPresets&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
