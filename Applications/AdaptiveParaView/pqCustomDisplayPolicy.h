/*=========================================================================

   Program: ParaView
   Module:    pqCustomDisplayPolicy.h

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
#ifndef __pqCustomDisplayPolicy_h
#define __pqCustomDisplayPolicy_h

#include <QObject>
#include <pqDisplayPolicy.h>

class pqDataRepresentation;
class pqOutputPort;
class pqPipelineSource;
class pqView;
class vtkSMProxy;

/// Display policy defines the application specific policy
/// for creating display proxies. Given a pair of a proxy to be displayed 
/// and a view proxy in which to display, this class must tell the type 
/// of display to create, if any. Custom applications can subclass 
/// this to define their own policy. The pqApplicationCore maintains
/// an instance of the policy used by the application. Custom applications
/// should set their own policy instance on the global application core 
/// instance.
class pqCustomDisplayPolicy : public pqDisplayPolicy
{
  Q_OBJECT
public:
  pqCustomDisplayPolicy(QObject* p);
  virtual ~pqCustomDisplayPolicy();

  /// Returns the type for the view that is indicated as the preferred view
  /// for the given output port. May return a null string if the no view type
  /// can be determined as the preferred view.
  /// If update_pipeline is set, then the pipeline will be update prior to
  /// fetching the data information from the port.
  virtual QString getPreferredViewType(pqOutputPort* opPort, bool update_pipeline) const;

  // Description
  // Adaptive paraview lets the user control when data is displayed. This
  // way, the culling operators get a chance to reduce the amount of data that
  // has to be drawn.
  virtual bool getHideByDefault() const {
    return false;
  }

  /// Adaptive ParaView overrides this so as not to reset the camera 
  // viewpoint as frequently.
  virtual pqDataRepresentation* setRepresentationVisibility(
    pqOutputPort* opPort, pqView* view, bool visible) const;

protected:
};

#endif

