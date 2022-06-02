
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

#include <QWidget>

class pqPipelineSource;
class pqView;
class vtkPVXRInterfaceHelper;
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

  // set the right trigger menu to the passed value
  // used when the helper programmatically adjusts
  // the mode
  void SetRightTriggerMode(std::string const& text);

  void SetAvailablePositions(std::vector<int> const& slots);

  void SetCurrentPosition(int val);
  void SetCurrentMotionFactor(double val);
  void SetCurrentScaleFactor(double val);

  void SetFieldValues(std::string vals);

protected:
  vtkPVXRInterfaceHelper* Helper;
  bool NoForward;

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
