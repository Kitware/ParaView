/*=========================================================================

   Program: ParaView
   Module:    vtkVRQueue.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef vtkVRQueue_h
#define vtkVRQueue_h

#include "vtkNew.h"
#include "vtkObject.h"

class vtkConditionVariable;
class vtkMutexLock;

#include <queue>

#define BUTTON_EVENT 1
#define ANALOG_EVENT 2
#define TRACKER_EVENT 3
#define VTK_ANALOG_CHANNEL_MAX 128

struct vtkTracker
{
  long sensor;       // Which sensor is reporting
  double matrix[16]; // The matrix with transformations applied
};

struct vtkAnalog
{
  int num_channel;                        // how many channels
  double channel[VTK_ANALOG_CHANNEL_MAX]; // channel diliever analog values
};

struct vtkButton
{
  int button; // Which button (numbered from zero)
  int state;  // New state (0 = off, 1 = on)
};

union vtkVREventCommonData {
  vtkTracker tracker;
  vtkAnalog analog;
  vtkButton button;
};

struct vtkVREventData
{
  std::string connId;
  std::string name; // Specified from configuration
  unsigned int eventType;
  vtkVREventCommonData data;
  unsigned int timeStamp;
};

class vtkVRQueue : public vtkObject
{
public:
  static vtkVRQueue* New();
  vtkTypeMacro(vtkVRQueue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Enqueue(const vtkVREventData& data);
  bool IsEmpty() const;
  bool TryDequeue(vtkVREventData& data);
  bool TryDequeue(std::queue<vtkVREventData>& data);
  void Dequeue(vtkVREventData& data);

protected:
  vtkVRQueue();
  ~vtkVRQueue();

private:
  vtkVRQueue(const vtkVRQueue&) = delete;
  void operator=(const vtkVRQueue&) = delete;

  std::queue<vtkVREventData> Queue;
  mutable vtkNew<vtkMutexLock> Mutex;
  vtkNew<vtkConditionVariable> CondVar;
};
#endif // vtkVRQueue_h
