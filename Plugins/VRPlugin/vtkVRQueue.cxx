/*=========================================================================

   Program: ParaView
   Module:  vtkVRQueue.cxx

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
#include "vtkVRQueue.h"

#include "vtkConditionVariable.h"
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRQueue);

//----------------------------------------------------------------------------
vtkVRQueue::vtkVRQueue()
  : Superclass()
{
}

//----------------------------------------------------------------------------
vtkVRQueue::~vtkVRQueue()
{
}

//----------------------------------------------------------------------------
void vtkVRQueue::Enqueue(const vtkVREvent& event)
{
  this->Mutex->Lock();
  this->Queue.push(event);
  this->Mutex->Unlock();
  this->CondVar->Signal();
}

//----------------------------------------------------------------------------
bool vtkVRQueue::IsEmpty() const
{
  this->Mutex->Lock();
  bool result = this->Queue.empty();
  this->Mutex->Unlock();
  return result;
}

//----------------------------------------------------------------------------
bool vtkVRQueue::TryDequeue(vtkVREvent& event)
{
  this->Mutex->Lock();
  bool result = false;
  if (!this->Queue.empty())
  {
    result = true;
    event = this->Queue.front();
    this->Queue.pop();
  }
  this->Mutex->Unlock();

  return result;
}

//----------------------------------------------------------------------------
void vtkVRQueue::Dequeue(vtkVREvent& event)
{
  this->Mutex->Lock();
  while (this->Queue.empty())
  {
    this->CondVar->Wait(this->Mutex.GetPointer());
  }

  event = this->Queue.front();
  this->Queue.pop();
  this->Mutex->Unlock();
}

//----------------------------------------------------------------------------
bool vtkVRQueue::TryDequeue(std::queue<vtkVREvent>& event)
{
  this->Mutex->Lock();
  if (!this->Queue.empty())
  {
    event = this->Queue;
    while (!this->Queue.empty())
    {
      this->Queue.pop();
    }
  }
  this->Mutex->Unlock();
  return true;
  return true; // WRS-TODO: for the other TryDequeue, we return the result.  Why not here? */
}

//----------------------------------------------------------------------------
void vtkVRQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Queued Events: " << this->Queue.size() << endl;
  os << indent << "Mutex:" << endl;
  this->Mutex->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CondVar:" << endl;
  this->CondVar->PrintSelf(os, indent.GetNextIndent());
}
