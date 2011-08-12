/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRQueue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVRQueue -
// .SECTION Description
// vtkVRQueue

#ifndef __vtkVRQueue_h
#define __vtkVRQueue_h

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QMutexLocker>

#define BUTTON_EVENT 1
#define ANALOG_EVENT 2
#define TRACKER_EVENT 3

#define VTK_ANALOG_CHANNEL_MAX 128

struct vtkTracker
{
  long   sensor;		// Which sensor is reporting
  double matrix[16];		// The matrix with transformations applied
};

struct vtkAnalog
{
  int	 num_channel;		    // how many channels
  double channel[VTK_ANALOG_CHANNEL_MAX]; // channel diliever analog values
};

struct vtkButton
{
  int button;			// Which button (numbered from zero)
  int state;			// New state (0 = off, 1 = on)
};

union vtkVREventCommonData
{
  vtkTracker tracker;
  vtkAnalog analog;
  vtkButton button;
};

struct vtkVREventData
{
  std::string connId;
  std::string name;		// Specified from configuration
  unsigned int eventType;
  vtkVREventCommonData data;
  unsigned int timeStamp;
};


class vtkVRQueue : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  vtkVRQueue(QObject* parentObject=0);
  void enqueue (const vtkVREventData& data);
  bool isEmpty() const;
  bool tryDequeue(vtkVREventData& data);
  bool tryDequeue(QQueue<vtkVREventData>& data);
  void dequeue(vtkVREventData&  data) ;

private:
  QQueue<vtkVREventData> Queue;
  mutable QMutex Mutex;
  QWaitCondition CondVar;

};
#endif // __vtkVRQueue_h
