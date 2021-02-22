/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerModelItem.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef _pqServerManagerModelItem_h
#define _pqServerManagerModelItem_h

#include "pqCoreModule.h"
#include <QObject>

class vtkEventQtSlotConnect;

/**
* pqServerManagerModelItem is a element maintained by pqServerManagerModel.
* pqServerManagerModel creates instances of pqServerManagerModelItem (and its
* subclasses) for every signification Server-Manager object such as a session
* (pqServer), source proxy (pqPipelineSource), filter proxy
* (pqPipelineFilter), view proxy (pqView) and so on.
*/
class PQCORE_EXPORT pqServerManagerModelItem : public QObject
{
  Q_OBJECT

public:
  pqServerManagerModelItem(QObject* parent = nullptr);
  ~pqServerManagerModelItem() override;

protected:
  /**
  * All subclasses generally need some vtkEventQtSlotConnect instance to
  * connect to VTK events. This provides access to a vtkEventQtSlotConnect
  * instance which all subclasses can use for listening to events.
  */
  vtkEventQtSlotConnect* getConnector();

private:
  vtkEventQtSlotConnect* Connector;
};

#endif
