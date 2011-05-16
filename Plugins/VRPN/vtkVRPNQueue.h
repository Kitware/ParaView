/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRPNQueue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVRPNQueue -
// .SECTION Description
// vtkVRPNQueue

#ifndef __vtkVRPNQueue_h
#define __vtkVRPNQueue_h

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>

#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QMutexLocker>

#define BUTTON_EVENT 1
#define ANALOG_EVENT 2
#define TRACKER_EVENT 3


union vrpn_CommonData
{
  vrpn_TRACKERCB tracker;
  vrpn_ANALOGCB analog;
  vrpn_BUTTONCB button;
};

struct vtkVRPNEventData
{
  std::string connId;
  unsigned int eventType;
  vrpn_CommonData data;
  unsigned int timeStamp;
};

//typedef QQueue<vtkVRPNEventData> vtkVRPNQueue;

class vtkVRPNQueue
{
public:
    void enqueue (const vtkVRPNEventData& data)
    {
        QMutexLocker lock(&this->Mutex);
        this->Queue.enqueue(data);
        lock.unlock();
        this->CondVar.wakeOne();
    }

    bool isEmpty() const
    {
      QMutexLocker lock(&this->Mutex);
        return this->Queue.isEmpty();
    }

    bool tryDequeue(vtkVRPNEventData& data)
    {
        QMutexLocker lock(&this->Mutex);
        if(this->Queue.isEmpty())
        {
            return false;
        }

        data=this->Queue.dequeue();
        return true;
    }

    void dequeue(vtkVRPNEventData&  data)
    {
        QMutexLocker lock(&this->Mutex);
        while(this->Queue.isEmpty())
        {
            this->CondVar.wait(lock.mutex());
        }

        data=this->Queue.dequeue();
    }

private:
  QQueue<vtkVRPNEventData> Queue;
  mutable QMutex Mutex;
  QWaitCondition CondVar;

};
#endif // __vtkVRPNQueue_h
