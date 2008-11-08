
#ifndef _pqStreamingRenderView_h
#define _pqStreamingRenderView_h

#include "pqRenderView.h"

class vtkSMRenderViewProxy;
class vtkSMStreamingViewProxy;

class pqStreamingRenderView : public pqRenderView
{
  Q_OBJECT
  typedef pqRenderView Superclass;
public:
  static QString streamingRenderViewType() { return "StreamingRenderView"; }
  static QString streamingRenderViewTypeName() { return "3D View (streaming)"; }

  /// constructor takes a bunch of init stuff and must have this signature to 
  /// satisfy pqView
  pqStreamingRenderView(
         const QString& viewtype, 
         const QString& group, 
         const QString& name, 
         vtkSMViewProxy* viewmodule, 
         pqServer* server, 
         QObject* p);
  ~pqStreamingRenderView();

  /// Returns proxy for this
  virtual vtkSMStreamingViewProxy* getStreamingViewProxy() const;

  /// Returns proxy this one manages
  virtual vtkSMRenderViewProxy* getRenderViewProxy() const;

public slots:

  /// Reads streaming settings from pqSettings
  virtual void restoreSettings();

protected:

private:
  pqStreamingRenderView(const pqStreamingRenderView&); // Not implemented.
  void operator=(const pqStreamingRenderView&); // Not implemented.

};

#endif // _pqStreamingRenderView_h

