// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkVRQueue_h
#define vtkVRQueue_h

#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro

#include <condition_variable>
#include <mutex>
#include <queue>

#define BUTTON_EVENT 1
#define VALUATOR_EVENT 2
#define TRACKER_EVENT 3
#define VTK_VALUATOR_CHANNEL_MAX 128

struct vtkTracker
{
  long sensor;       // Which sensor is reporting
  double matrix[16]; // The matrix with transformations applied
};

struct vtkValuator
{
  int num_channels;                         // how many channels
  double channel[VTK_VALUATOR_CHANNEL_MAX]; // channel diliever valuator values
};

struct vtkButton
{
  int button; // Which button (zero-based)
  int state;  // New state (0 = off, 1 = on)
};

union vtkVREventCommonData {
  vtkTracker tracker;
  vtkValuator valuator;
  vtkButton button;
};

struct vtkVREvent
{
  std::string connId;
  std::string name; // Specified from configuration
  unsigned int eventType;
  vtkVREventCommonData data;
  unsigned int timeStamp;
};

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkVRQueue : public vtkObject
{
public:
  static vtkVRQueue* New();
  vtkTypeMacro(vtkVRQueue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Enqueue(const vtkVREvent& event);
  bool IsEmpty() const;
  bool TryDequeue(vtkVREvent& event);
  bool TryDequeue(std::queue<vtkVREvent>& event);
  void Dequeue(vtkVREvent& event);

protected:
  vtkVRQueue();
  ~vtkVRQueue();

private:
  vtkVRQueue(const vtkVRQueue&) = delete;
  void operator=(const vtkVRQueue&) = delete;

  std::queue<vtkVREvent> Queue;
  mutable std::mutex Mutex;
  std::condition_variable_any CondVar;
};
#endif // vtkVRQueue_h
