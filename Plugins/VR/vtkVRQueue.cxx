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
