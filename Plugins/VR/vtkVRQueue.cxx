/*=========================================================================

   Program: ParaView
   Module:    vtkVRQueue.cxx

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

//----------------------------------------------------------------------------
vtkVRQueue::vtkVRQueue(QObject* parentObject) : Superclass(parentObject)
{
}

//----------------------------------------------------------------------------
void vtkVRQueue::enqueue(const vtkVREventData& data)
{
  QMutexLocker lock(&this->Mutex);
  this->Queue.enqueue(data);
  lock.unlock();
  this->CondVar.wakeOne();
}

//----------------------------------------------------------------------------
bool vtkVRQueue::isEmpty() const
{
  QMutexLocker lock(&this->Mutex);
  return this->Queue.isEmpty();
}

//----------------------------------------------------------------------------
bool vtkVRQueue::tryDequeue(vtkVREventData& data)
{
  QMutexLocker lock(&this->Mutex);
  if(this->Queue.isEmpty())
    {
    return false;
    }

  data=this->Queue.dequeue();
  return true;
}

//----------------------------------------------------------------------------
void vtkVRQueue::dequeue(vtkVREventData& data)
{
  QMutexLocker lock(&this->Mutex);
  while(this->Queue.isEmpty())
    {
    this->CondVar.wait(lock.mutex());
    }

  data=this->Queue.dequeue();
}

//----------------------------------------------------------------------------
bool vtkVRQueue::tryDequeue(QQueue<vtkVREventData>& data)
{
  QMutexLocker lock(&this->Mutex);
  if (this->Queue.isEmpty())
    {
    return false;
    }
  data = this->Queue;
  this->Queue.clear();
  return true;
}
