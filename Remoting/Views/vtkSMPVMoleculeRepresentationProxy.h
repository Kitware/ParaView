/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVMoleculeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPVMoleculeRepresentationProxy
 * @brief vtkSMPVMoleculeRepresentationProxy is a representation proxy used for molecules.
 *
 * This class exposes a method to apply a preset onto molecule properties.
 */

#ifndef vtkSMPVMoleculeRepresentationProxy_h
#define vtkSMPVMoleculeRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMPVRepresentationProxy.h"

#include <map> // for std::map

struct MapperParametersPreset
{
  bool RenderAtoms;
  bool RenderBonds;
  int AtomicRadiusType;
  double AtomicRadiusFactor;
  bool UseMultiCylindersForBonds;
  double BondRadius;
  bool UseAtomColorForBonds;
  std::string Name;
  std::string DisplayName;

  bool IsSimilar(const MapperParametersPreset& p) const;
};

class VTKREMOTINGVIEWS_EXPORT vtkSMPVMoleculeRepresentationProxy : public vtkSMPVRepresentationProxy
{
public:
  static vtkSMPVMoleculeRepresentationProxy* New();
  vtkTypeMacro(vtkSMPVMoleculeRepresentationProxy, vtkSMPVRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Preset
  {
    None = 0,
    BallAndStick,
    Liquorice,
    VanDerWaals,
    Fast,
    NbOfPresets
  };

  //@{
  /**
   * Set Preset onto properties. If preset is None or 0, does nothing.
   */
  void SetPreset(int preset);
  void SetPreset(const char* name);
  //@}

  /**
   * Get the preset name. This name can be used with `SetPreset()` method.
   */
  const char* GetPresetName(int preset);

  /**
   * Get a display name for the preset.
   */
  const char* GetPresetDisplayName(int preset);

  /**
   * Get the current preset value.
   */
  int GetCurrentPreset();

protected:
  vtkSMPVMoleculeRepresentationProxy();
  ~vtkSMPVMoleculeRepresentationProxy() override = default;

private:
  vtkSMPVMoleculeRepresentationProxy(const vtkSMPVMoleculeRepresentationProxy&) = delete;
  void operator=(const vtkSMPVMoleculeRepresentationProxy&) = delete;

  std::map<int, MapperParametersPreset> Presets;
};

#endif
