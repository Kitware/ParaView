
#ifndef _pqAdaptiveRenderView_h
#define _pqAdaptiveRenderView_h

#include "pqRenderView.h"

class vtkSMRenderViewProxy;
class vtkSMAdaptiveViewProxy;

class pqAdaptiveRenderView : public pqRenderView
{
  Q_OBJECT
  typedef pqRenderView Superclass;
public:
  static QString adaptiveRenderViewType() { return "AdaptiveRenderView"; }
  static QString adaptiveRenderViewTypeName() { return "3D View (refining)"; }

  /// constructor takes a bunch of init stuff and must have this signature to 
  /// satisfy pqView
  pqAdaptiveRenderView(
         const QString& viewtype, 
         const QString& group, 
         const QString& name, 
         vtkSMViewProxy* viewmodule, 
         pqServer* server, 
         QObject* p);
  ~pqAdaptiveRenderView();

  /// Returns proxy for this
  virtual vtkSMAdaptiveViewProxy* getAdaptiveViewProxy() const;

  /// Returns proxy this one manages
  virtual vtkSMRenderViewProxy* getRenderViewProxy() const;

  /// Load streaming settings.
  virtual void setDefaultPropertyValues();

protected:

  /// Overridden to disable Qt caching of front buffer. 
  // The multipass rendering doesn't play well with that.
  virtual QWidget* createWidget();

private:
  pqAdaptiveRenderView(const pqAdaptiveRenderView&); // Not implemented.
  void operator=(const pqAdaptiveRenderView&); // Not implemented.

};

#endif // _pqAdaptiveRenderView_h

