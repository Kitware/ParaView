/*=========================================================================

   Program: ParaView
   Module:    pqVRUIStarter.cxx

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
#include "pqVRUIStarter.h"

// Server Manager Includes.

// Qt Includes.
#include <QtDebug>
#include <QTimer>
// ParaView Includes.
#include "ParaViewVRUI.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"

//-----------------------------------------------------------------------------
pqVRUIStarter::pqVRUIStarter(QObject* p/*=0*/)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
pqVRUIStarter::~pqVRUIStarter()
{
}


//-----------------------------------------------------------------------------
void pqVRUIStarter::onStartup()
{
  qWarning() << "Message from pqVRUIStarter: Application Started";
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions *options = (vtkPVOptions*)pm->GetOptions();
  if(options->GetUseVRUI())
    {
    // VRUI input events.
    this->VRUITimer=new QTimer(this);
    this->VRUITimer->setInterval(40); // in ms
    // to define: obj and callback()
    this->InputDevice=new ParaViewVRUI;
    this->InputDevice->SetName(options->GetVRUIAddress());
    this->InputDevice->SetPort(8555);
    this->InputDevice->Init();
    connect(this->VRUITimer,SIGNAL(timeout()),
            this->InputDevice,SLOT(callback()));
    this->VRUITimer->start();
    }

}

//-----------------------------------------------------------------------------
void pqVRUIStarter::onShutdown()
{
  qWarning() << "Message from pqVRUIStarter: Application Shutting down";
}
