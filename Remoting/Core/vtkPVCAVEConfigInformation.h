// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVCAVEConfigInformation
 *
 * A vtkClientServerStream serializable container intended to expose the
 * api and information presented by vtkDisplayConfiguration.  The same pvx
 * file is read on all server processes, so merging info from other servers
 * is simply a matter of overwriting the current state with the one read
 * from the stream or other object.
 */

#ifndef vtkPVCAVEConfigInformation_h
#define vtkPVCAVEConfigInformation_h

#include "vtkPVInformation.h" // for base class
#include "vtkTuple.h"         // for vtkTuple

#include <memory> // for std::unique_ptr

class vtkClientServerStream;

class VTKREMOTINGCORE_EXPORT vtkPVCAVEConfigInformation : public vtkPVInformation
{
public:
  static vtkPVCAVEConfigInformation* New();
  vtkTypeMacro(vtkPVCAVEConfigInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  ///@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  ///@}

  ///@{
  /**
   * Return the whether the system is in CAVE mode
   */
  virtual bool GetIsInCAVE();
  ///@}

  ///@{
  /**
   * Return the eye separation.
   */
  virtual double GetEyeSeparation();
  ///@}

  ///@{
  /**
   * Return the number of displays.
   */
  virtual int GetNumberOfDisplays();
  ///@}

  ///@{
  /**
   * Return whether or not borders are shown.
   */
  virtual bool GetShowBorders();
  ///@}

  ///@{
  /**
   * Return whether or not displays are fullscreen.
   */
  virtual bool GetFullScreen();
  ///@}

  ///@{
  /**
   * Return the geometry of the indexed display.
   */
  virtual vtkTuple<int, 4> GetGeometry(int index);
  ///@}

  ///@{
  /**
   * Return whether or not the indexed display has screen corners.
   */
  virtual bool GetHasCorners(int index);
  ///@}

  ///@{
  /**
   * Return the lower left corner of the indexed display.
   */
  virtual vtkTuple<double, 3> GetLowerLeft(int index);
  ///@}

  ///@{
  /**
   * Return the lower right corner of the indexed display.
   */
  virtual vtkTuple<double, 3> GetLowerRight(int index);
  ///@}

  ///@{
  /**
   * Return the upper right corner of the indexed display.
   */
  virtual vtkTuple<double, 3> GetUpperRight(int index);
  ///@}

protected:
  vtkPVCAVEConfigInformation();
  ~vtkPVCAVEConfigInformation() override;

private:
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internal;

  vtkPVCAVEConfigInformation(const vtkPVCAVEConfigInformation&) = delete;
  void operator=(const vtkPVCAVEConfigInformation&) = delete;
};

#endif
