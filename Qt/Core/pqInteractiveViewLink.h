/*=========================================================================

   Program: ParaView
   Module:    pqInteractiveViewLink.h

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
#ifndef pqInteractiveViewLink_h
#define pqInteractiveViewLink_h

#include "pqCoreModule.h"
#include <QObject>

class pqRenderView;
class vtkPVXMLElement;

/**
* pqInteractiveViewLink is uaed by pqLinksModel to create
* interactive view links, which are ViewLink allowing to see a view "trough"
* another view.
*/
class PQCORE_EXPORT pqInteractiveViewLink : public QObject
{
  Q_OBJECT;
  typedef QObject Superclass;

public:
  pqInteractiveViewLink(pqRenderView* displayView, pqRenderView* linkedView, double xPos = 0.375,
    double yPos = 0.375, double xSize = 0.25, double ySize = 0.25);
  ~pqInteractiveViewLink() override;

  // Save this interactive view link in xml node
  virtual void saveXMLState(vtkPVXMLElement* xml);

  // Set/Get the view link opacity, between 0 and 1
  virtual void setOpacity(double opacity);
  virtual double getOpacity();

  // Set/get whether to hide the background of the linked view
  virtual void setHideLinkedViewBackground(bool hide);
  virtual bool getHideLinkedViewBackground();

protected:
  // Generate a correct draw method call from current situation
  // on the back buffer
  virtual void drawViewLink();

  // draw pixels on the display Window at ViewLink position
  virtual void drawViewLink(int setFront);

  // update the linked window
  virtual void renderLinkedView();

  // method to set rendered flag to true
  virtual void linkedWindowRendered();

protected Q_SLOTS:

  // draw pixels on the front buffer
  virtual void finalRenderDisplayView();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
