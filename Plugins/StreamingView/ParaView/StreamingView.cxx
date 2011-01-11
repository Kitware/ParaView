/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StreamingView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    StreamingView.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "StreamingView.h"

#include <QMainWindow>
#include <QStatusBar>
#include <QString>
#include <QTimer>

#include "vtkPNGWriter.h"
#include "vtkRenderWindow.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMStreamingViewProxy.h"
#include "vtkWindowToImageFilter.h"

#include <pqServer.h>
#include <pqApplicationCore.h>
#include <pqCoreUtilities.h>

//-----------------------------------------------------------------------------
StreamingView::StreamingView(
  const QString& viewType,
  const QString& group,
  const QString& name,
  vtkSMViewProxy* viewProxy,
  pqServer* server,
  QObject* p)
  : pqRenderView(viewType, group, name, viewProxy, server, p), Pass(0)
{
  QObject::connect(this, SIGNAL(beginRender()),
                   this, SLOT(watchPreRender()));
  QObject::connect(this, SIGNAL(endRender()),
                   this, SLOT(scheduleNextPass()));

  //we manage front buffer swapping, and have to do a series of renders to fill
  //it, so don't let the app try to cache the front buffer
  this->AllowCaching = false;

  //prevent paraview from updatine whole extent to gather info
  vtkSMOutputPort::SetUseStreaming(true);
}

//-----------------------------------------------------------------------------
StreamingView::~StreamingView()
{
}

//-----------------------------------------------------------------------------
void StreamingView::watchPreRender()
{
#if 0
  //cerr << "PRE RENDER " << this->Pass << endl;
  vtkSMStreamingViewProxy *vp = vtkSMStreamingViewProxy::SafeDownCast
    (this->getViewProxy());
  if (!vp)
    {
    return;
    }

  //save debug images of back and front buffer
  vtkRenderWindow *rw = vp->GetRenderWindow();
  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  vtkPNGWriter *writer = vtkPNGWriter::New();
  w2i->SetInput(rw);
  w2i->ShouldRerenderOff();
  w2i->ReadFrontBufferOff();
  w2i->Update();
  writer->SetInputConnection(w2i->GetOutputPort());
  QString s("/Users/demarle/Desktop/debugimgs/image_");
  s.append(QString::number(this->Pass));
  s.append("_0.png");
  writer->SetFileName(s.toAscii());
  writer->Write();

  w2i->ReadFrontBufferOn();
  w2i->Update();
  s = "/Users/demarle/Desktop/debugimgs/image_";
  s.append(QString::number(this->Pass));
  s.append("_1.png");
  writer->SetFileName(s.toAscii());
  writer->Write();

  w2i->Delete();
  writer->Delete();
#endif
}

//-----------------------------------------------------------------------------
void StreamingView::scheduleNextPass()
{
  //cerr << "POST RENDER " << this->Pass << endl;
  vtkSMStreamingViewProxy *vp = vtkSMStreamingViewProxy::SafeDownCast
    (this->getViewProxy());
  if (!vp)
    {
    return;
    }

#if 0
  //save debug images of back and front buffer
  vtkRenderWindow *rw = vp->GetRenderWindow();
  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  vtkPNGWriter *writer = vtkPNGWriter::New();
  w2i->SetInput(rw);
  w2i->ShouldRerenderOff();
  w2i->ReadFrontBufferOff();
  w2i->Update();
  writer->SetInputConnection(w2i->GetOutputPort());
  QString s("/Users/demarle/Desktop/debugimgs/image_");
  s.append(QString::number(this->Pass));
  s.append("_2.png");
  writer->SetFileName(s.toAscii());
  writer->Write();

  w2i->ReadFrontBufferOn();
  w2i->Update();
  s = "/Users/demarle/Desktop/debugimgs/image_";
  s.append(QString::number(this->Pass));
  s.append("_3.png");
  writer->SetFileName(s.toAscii());
  writer->Write();

  w2i->Delete();
  writer->Delete();
#endif

  QString message("streaming pass ");
  message.append(QString::number(this->Pass));

  if (!vp->IsDisplayDone())
    {
    //schedule next render pass
    QTimer *t = new QTimer(this);
    t->setSingleShot(true);
    QObject::connect(t, SIGNAL(timeout()),
                     this, SLOT(render()), Qt::QueuedConnection);
    t->start();
    this->Pass++;
    }
  else
    {
    this->Pass = 0;
    message.append(" DONE");
    }

  QMainWindow *mainWindow =
    qobject_cast<QMainWindow *>(pqCoreUtilities::mainWidget());
  mainWindow->statusBar()->showMessage(message, 500);
}
