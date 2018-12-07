/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeKeeper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMTimeKeeper
 * @brief   a time keeper is used to keep track of the
 * pipeline time.
 *
 * TimeKeeper can be thought of as a application wide clock. In ParaView, all
 * views are registered with the TimeKeeper (using AddView()) so that all the
 * views render data at the same global time.
 *
 * TimeKeeper also keeps track of time steps and continuous time ranges provided
 * by sources/readers/filters. This expects that the readers have a
 * "TimestepValues" and/or "TimeRange" properties from which the time steps and
 * time ranges provided by the reader can be obtained. All sources whose
 * time steps/time ranges must be noted by the time keeper need to be registered
 * with the time keeper using AddTimeSource(). ParaView automatically registers
 * all created sources/filters/readers with the time keeper. The time steps and
 * time ranges are made accessible by two information properties
 * "TimestepValues" and "TimeRange" on the TimeKeeper proxy.
 *
 * To change the time shown by all the views, simply change the "Time" property
 * on the time keeper proxy (don't directly call SetTime() since otherwise
 * undo/redo, state etc. will not work as expected).
 *
 * This proxy has no VTK objects that it creates on the server.
*/

#ifndef vtkSMTimeKeeper_h
#define vtkSMTimeKeeper_h

#include "vtkObject.h"
#include "vtkPVServerManagerCoreModule.h" //needed for exports

class vtkSMProperty;
class vtkSMSourceProxy;
class vtkSMProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMTimeKeeper : public vtkObject
{
public:
  static vtkSMTimeKeeper* New();
  vtkTypeMacro(vtkSMTimeKeeper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the pipeline time.
   */
  void SetTime(double time);
  vtkGetMacro(Time, double);
  //@}

  //@{
  /**
   * Add/Remove view proxy linked to this time keeper.
   */
  void AddView(vtkSMProxy*);
  void RemoveView(vtkSMProxy*);
  void RemoveAllViews();
  //@}

  //@{
  /**
   * List of proxies that provide time. TimestepValues property has a set of
   * timesteps provided by all the sources added to this property alone.
   */
  void AddTimeSource(vtkSMSourceProxy*);
  void RemoveTimeSource(vtkSMSourceProxy*);
  void RemoveAllTimeSources();
  //@}

  //@{
  /**
   * List of proxies that provide time. TimestepValues property has a set of
   * timesteps provided by all the sources added to this property alone.
   */
  void AddSuppressedTimeSource(vtkSMSourceProxy*);
  void RemoveSuppressedTimeSource(vtkSMSourceProxy*);
  //@}

  //@{
  /**
   * Iterates over all sources providing time and calls
   * `vtkSMSourceProxy::UpdatePipelineInformation` on them. That ensures that
   * timekeeper is using the latest time information available to it.
   */
  void UpdateTimeInformation();
  //@}

protected:
  vtkSMTimeKeeper();
  ~vtkSMTimeKeeper() override;

  friend class vtkSMTimeKeeperProxy;
  void SetTimestepValuesProperty(vtkSMProperty*);
  void SetTimeRangeProperty(vtkSMProperty*);
  void SetTimeLabelProperty(vtkSMProperty*);

  void UpdateTimeSteps();

  vtkSMProperty* TimeLabelProperty;
  vtkSMProperty* TimeRangeProperty;
  vtkSMProperty* TimestepValuesProperty;
  double Time;

private:
  vtkSMTimeKeeper(const vtkSMTimeKeeper&) = delete;
  void operator=(const vtkSMTimeKeeper&) = delete;

  class vtkInternal;
  vtkInternal* Internal;

  bool DeferUpdateTimeSteps;
};

#endif
