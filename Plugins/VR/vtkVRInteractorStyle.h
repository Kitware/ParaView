/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef __vtkVRInteractorStyle_h
#define __vtkVRInteractorStyle_h

#include <QObject>

class vtkPVXMLElement;
class vtkSMProxyLocator;
struct vtkVREventData;

class vtkVRInteractorStyle : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  vtkVRInteractorStyle(QObject* parent=0);
  virtual ~vtkVRInteractorStyle();

  /// called to handle an event. If the style does not handle this event or
  /// handles it but does not want to stop any other handlers from handlign it
  /// as well, they should return false. Other return true. Return true
  /// indicates that vtkVRQueueHandler the event has been "consumed".
  virtual bool handleEvent(const vtkVREventData& data)=0;

  virtual bool update()=0;

  ///--------------------------------------------------------------------------
  /// Identifies the device state when this style becomes active.

  /// Get/Set the device name.
  void setName(const QString& name)
    { this->Name = name; }
  const QString& name() const
    { return this->Name; }

  void setType(const QString& type)
    { this->Type = type; }
  const QString& type() const
    { return this->Type; }

  ///--------------------------------------------------------------------------
  /// Used to save/load the style in XML for ParaView state files.

  /// configure the style using the xml configuration.
  virtual bool configure(vtkPVXMLElement* child, vtkSMProxyLocator*);

  /// save the xml configuration.
  virtual vtkPVXMLElement* saveConfiguration() const;

protected:
  QString Name;
  QString Type;
private:
  Q_DISABLE_COPY(vtkVRInteractorStyle)
};

#endif
