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
#ifndef vtkVRVectorPropertyStyle_h
#define vtkVRVectorPropertyStyle_h

#include "vtkVRPropertyStyle.h"

/// vtkVRVectorPropertyStyle can be used to control any 3-element numeric
/// property. This style has modes which allows one to control how the property
/// is updated i.e. whether to use the orientation vector or use displacement
/// etc.
class vtkVRVectorPropertyStyle : public vtkVRPropertyStyle
{
  Q_OBJECT
  typedef vtkVRPropertyStyle Superclass;

public:
  enum eMode
  {
    DIRECTION_VECTOR = 1,
    DISPLACEMENT = 2
  };

public:
  vtkVRVectorPropertyStyle(QObject* parent = 0);
  virtual ~vtkVRVectorPropertyStyle();

  /// get/set the mode.
  void setMode(eMode mode) { this->Mode = mode; }
  eMode mode() const { return this->Mode; }

  /// handle the event.
  virtual bool handleEvent(const vtkVREventData& data);

  /// updates the server side
  virtual bool update();

  /// configure the style using the xml configuration.
  virtual bool configure(vtkPVXMLElement* child, vtkSMProxyLocator*);

  /// save the xml configuration.
  virtual vtkPVXMLElement* saveConfiguration() const;

protected:
  void setValue(double x, double y, double z);
  eMode Mode;

private:
  Q_DISABLE_COPY(vtkVRVectorPropertyStyle)
};

#endif
