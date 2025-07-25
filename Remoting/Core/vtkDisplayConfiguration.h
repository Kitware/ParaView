// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDisplayConfiguration
 * @brief display configuration container
 *
 * vtkDisplayConfiguration is a container used by vtkRemotingCoreConfiguration
 * to manage display configurations which are needed for CAVE and other
 * immersive displays.
 */

#ifndef vtkDisplayConfiguration_h
#define vtkDisplayConfiguration_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" // for exports
#include "vtkTuple.h"              // for vtkTuple
#include <memory>                  // for std::unique_ptr
#include <string>                  // for std::string

class VTKREMOTINGCORE_EXPORT vtkDisplayConfiguration : public vtkObject
{
public:
  static vtkDisplayConfiguration* New();
  vtkTypeMacro(vtkDisplayConfiguration, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get whether to use window borders / frames are to be shown.
   */
  vtkGetMacro(ShowBorders, bool);

  /**
   * Returns true of each display should simply be full-screen. If so, display
   * geometry is ignored.
   */
  vtkGetMacro(FullScreen, bool);

  /**
   * Get eye separation.
   */
  vtkGetMacro(EyeSeparation, double);

  /**
   * Get use of off axis projection
   */
  vtkGetMacro(UseOffAxisProjection, bool);

  /**
   * Returns the count for display specified.
   */
  int GetNumberOfDisplays() const;

  ///@{
  /**
   * Returns optional name string for the given display index, or nullptr if
   * the Machine element has no Name attribute.
   */
  const char* GetName(int index) const;
  ///@}

  ///@{
  /**
   * Returns environment string for the given display index, or nullptr if
   * the Machine element has no Environment attribute.
   */
  const char* GetEnvironment(int index) const;
  ///@}

  ///@{
  /**
   * Returns geometry (screen space size and position) for the given display
   * index.
   */
  vtkTuple<int, 4> GetGeometry(int index) const;
  ///@}

  ///@{
  /**
   * Returns positions of screen corners (in physical coordinates) for the
   * given display.
   */
  vtkTuple<double, 3> GetLowerLeft(int index) const;
  vtkTuple<double, 3> GetLowerRight(int index) const;
  vtkTuple<double, 3> GetUpperRight(int index) const;
  ///@}

  ///@{
  /**
   * Returns whether given display index has lower left, lower right, and
   * upper right screen corners defined.
   */
  bool GetHasCorners(int index) const;
  ///@}

  ///@{
  /**
   * Only used on X windows systems. Indicates that the window should be
   * manageable by the window manager.  This means it can be covered by
   * other windows, and a taskbar item is available to bring it back to the
   * front.
   */
  bool GetCoverable(int index) const;
  ///@}

  ///@{
  /**
   * Allows user to select which displays show/hide things like scalar bar,
   * annotation, etc.
   */
  bool GetShow2DOverlays(int index) const;
  ///@}

  ///@{
  /**
   * Returns the configured viewer id for the given display index. If not
   * configured, all displays are associated with a viewer id of 0 by default.
   */
  int GetViewerId(int index) const;
  ///@}

  ///@{
  /**
   * Returns the stereo type for the given display index. Returns -1 if no stereo
   * type attribute was present on the element, or if the attribute could not be
   * parsed as one of the understood types
   */
  int GetStereoType(int index) const;
  ///@}

  ///@{
  /**
   * Returns the number of child elements under the optional IndependentViewers
   * element.  If provided, these are used to set default EyeSeparation for each
   * of the independent viewers listed in the Machine elements.
   */
  int GetNumberOfViewers() const;
  ///@}

  ///@{
  /**
   * Returns the Id attribute of the given independent viewer index. In order to
   * be used, this should match the ViewerId attribute of one or more Machine
   * elements.
   */
  int GetId(int viewerIndex) const;
  ///@}

  ///@{
  /**
   * Returns the EyeSeparation attribute of the given independent viewer index.
   * In order to be used, there should be a sibling "Id" element that matches
   * the ViewerId attribute of one or more Machine elements.
   */
  double GetEyeSeparation(int viewerIndex) const;
  ///@}

  /**
   * Parses a PVX file to load display configuration information.
   */
  bool LoadPVX(const char* fname);

  ///@{
  /**
   * Return an integer stereo type corresponding to the given string value,
   * or -1 if the string value is not recognized as a stereo type.
   */
  static int ParseStereoType(const std::string& value);
  ///@}

  ///@{
  /**
   * Returns the string representation of the given stereo type. Returns
   * the empty string if no stereo type attribute was present on the element, or
   * if the attribute could not be parsed as one of the understood types.
   */
  static const char* GetStereoTypeAsString(int stereoType);
  ///@}

protected:
  vtkDisplayConfiguration();
  ~vtkDisplayConfiguration() override;

private:
  vtkDisplayConfiguration(const vtkDisplayConfiguration&) = delete;
  void operator=(const vtkDisplayConfiguration&) = delete;

  bool ShowBorders = false;
  bool Coverable = false;
  bool FullScreen = false;
  double EyeSeparation = 0.0;
  bool UseOffAxisProjection = true;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
