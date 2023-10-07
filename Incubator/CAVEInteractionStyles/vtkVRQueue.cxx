// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRQueue.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRQueue);

//----------------------------------------------------------------------------
vtkVRQueue::vtkVRQueue() = default;

//----------------------------------------------------------------------------
vtkVRQueue::~vtkVRQueue() = default;

//----------------------------------------------------------------------------
void vtkVRQueue::Enqueue(const vtkVREvent& event)
{
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    (void)lock;
    this->Queue.push(event);
  }
  this->CondVar.notify_all();
}

//----------------------------------------------------------------------------
bool vtkVRQueue::IsEmpty() const
{
  bool result;
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    (void)lock;
    result = this->Queue.empty();
  }
  return result;
}

//----------------------------------------------------------------------------
bool vtkVRQueue::TryDequeue(vtkVREvent& event)
{
  bool result;
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    (void)lock;
    result = false;
    if (!this->Queue.empty())
    {
      result = true;
      event = this->Queue.front();
      this->Queue.pop();
    }
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkVRQueue::Dequeue(vtkVREvent& event)
{
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    (void)lock;
    while (this->Queue.empty())
    {
      this->CondVar.wait(this->Mutex);
    }

    event = this->Queue.front();
    this->Queue.pop();
  }
}

//----------------------------------------------------------------------------
bool vtkVRQueue::TryDequeue(std::queue<vtkVREvent>& event)
{
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    (void)lock;
    if (!this->Queue.empty())
    {
      event = this->Queue;
      while (!this->Queue.empty())
      {
        this->Queue.pop();
      }
    }
  }
  return true; // WRS-TODO: for the other TryDequeue, we return the result.  Why not here?
}

//----------------------------------------------------------------------------
void vtkVRQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Queued Events: " << this->Queue.size() << endl;
}
