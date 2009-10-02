/*=========================================================================

   Program: ParaView
   Module:    PrismDisplayProxyEditor.h

=========================================================================*/
#ifndef _PrismDisplayProxyEditor_h
#define _PrismDisplayProxyEditor_h

#include <QWidget>
#include <QVariant>
#include "pqDisplayPanelInterface.h"
#include "pqPipelineRepresentation.h"
#include "vtkSMProxy.h"
#include "pqDisplayProxyEditor.h"
#include "vtkSMPrismCubeAxesRepresentationProxy.h"




/// Widget which provides an editor for the properties of a display.
class  PrismDisplayProxyEditor : public pqDisplayProxyEditor
{
  Q_OBJECT
  
public:
  /// constructor
  PrismDisplayProxyEditor(pqPipelineRepresentation* display, QWidget* p = NULL);
  /// destructor
  ~PrismDisplayProxyEditor();
  protected slots:
      virtual void editCubeAxes();
      virtual void cubeAxesVisibilityChanged();

protected:
  vtkSMPrismCubeAxesRepresentationProxy* CubeAxesActor;
  QPointer<pqPipelineRepresentation> Representation;
  pqServer* getActiveServer() const;
};

#endif

