
/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   pqXRInterfaceControls
 * @brief   ParaView GUI for use within virtual reality
 *
 * This class brings elements of the ParaView GUI into VR where they
 * can be used. Instantiated by the pqXRInterfaceDockPanel.
 */

#ifndef pqXRInterfaceControls_h
#define pqXRInterfaceControls_h

#include "vtkPVXRInterfaceHelper.h" // for enum
#include "vtkSmartPointer.h"        // for vtkSmartPointer
#include "vtkVRInteractorStyle.h"   // for enum

#include <QStringList>
#include <QWidget>

class pqPipelineSource;
class pqVCRController;

class pqXRInterfaceControls : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqXRInterfaceControls(vtkPVXRInterfaceHelper* val, QWidget* p = nullptr)
    : Superclass(p)
  {
    this->constructor(val);
  }
  ~pqXRInterfaceControls() override;

  pqPipelineSource* GetSelectedPipelineSource();

  /**
   * Set the value of the Right Trigger combobox.
   */
  void SetRightTriggerMode(vtkPVXRInterfaceHelper::RightTriggerAction action);

  /**
   * Set the value of the Movement Style combobox.
   */
  void SetMovementStyle(vtkVRInteractorStyle::MovementStyle style);

  /**
   * Set the available camera pose indices of the Load Camera Pose combobox.
   */
  void SetAvailablePositions(std::vector<int> const& slots);

  /**
   * Set the value of the Save Camera Pose combobox.
   */
  void SetCurrentSavedPosition(int val);

  /**
   * Set the value of the Load Camera Pose combobox.
   */
  void SetCurrentPosition(int val);

  /**
   * Set the value of the Motion Factor combobox.
   */
  void SetCurrentMotionFactor(double val);

  /**
   * Set the value of the Scale Factor combobox.
   */
  void SetCurrentScaleFactor(double val);

  /**
   * Set the value of the View Up combobox.
   */
  void SetCurrentViewUp(std::string dir);

  /**
   * Set the available values of the Field Value combobox.
   */
  void SetFieldValues(const QStringList& values);

  /**
   * Set check state of the Show Floor checkbox.
   */
  void SetShowFloor(bool checked);

  /**
   * Set check state of the Interactive Ray checkbox.
   */
  void SetInteractiveRay(bool checked);

  /**
   * Set check state of the Navigation Panel checkbox.
   */
  void SetNavigationPanel(bool checked);

  /**
   * Set check state of the Snap Crop Planes checkbox.
   */
  void SetSnapCropPlanes(bool checked);

protected:
  vtkSmartPointer<vtkPVXRInterfaceHelper> Helper;
  bool NoForward = false;

protected Q_SLOTS:
  void resetPositions();
  void assignFieldValue();

private:
  void constructor(vtkPVXRInterfaceHelper* val);

  class pqInternals;
  pqInternals* Internals;

  pqVCRController* Controller;
};

#endif
