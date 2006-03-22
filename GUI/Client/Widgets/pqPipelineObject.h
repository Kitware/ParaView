
/// \file pqPipelineObject.h
///
/// \date 11/16/2005

#ifndef _pqPipelineObject_h
#define _pqPipelineObject_h


#include "QtWidgetsExport.h"
#include <QString> // Needed for proxy name.

class pqPipelineObjectInternal;
class pqPipelineWindow;
class vtkSMDisplayProxy;
class vtkSMProxy;


class QTWIDGETS_EXPORT pqPipelineObject
{
public:
  enum ObjectType {
    Source,
    Filter,
    Bundle
  };

public:
  pqPipelineObject(vtkSMProxy *proxy, ObjectType type);
  ~pqPipelineObject();

  ObjectType GetType() const {return this->Type;}
  void SetType(ObjectType type) {this->Type = type;}

  const QString &GetProxyName() const {return this->ProxyName;}
  void SetProxyName(const QString &name) {this->ProxyName = name;}

  vtkSMProxy *GetProxy() const {return this->Proxy;}
  void SetProxy(vtkSMProxy *proxy) {this->Proxy = proxy;}

  // TODO: Store the display proxy name.
  vtkSMDisplayProxy *GetDisplayProxy() const {return this->Display;}
  void SetDisplayProxy(vtkSMDisplayProxy *display) {this->Display = display;}

  pqPipelineWindow *GetParent() const {return this->Window;}
  void SetParent(pqPipelineWindow *parent) {this->Window = parent;}

  /// \name Connection Methods
  //@{
  int GetInputCount() const;
  pqPipelineObject *GetInput(int index) const;
  bool HasInput(pqPipelineObject *input) const;

  void AddInput(pqPipelineObject *input);
  void RemoveInput(pqPipelineObject *input);

  int GetOutputCount() const;
  pqPipelineObject *GetOutput(int index) const;
  bool HasOutput(pqPipelineObject *output) const;

  void AddOutput(pqPipelineObject *output);
  void RemoveOutput(pqPipelineObject *output);

  void ClearConnections();
  //@}

private:
  pqPipelineObjectInternal *Internal; ///< Stores the object connections.
  vtkSMDisplayProxy *Display;         ///< Stores the display proxy;
  ObjectType Type;                    ///< Stores the object type.
  QString ProxyName;                  ///< Stores the proxy name.
  vtkSMProxy *Proxy;                  ///< Stores the proxy pointer.
  pqPipelineWindow *Window;           ///< Stores the parent window.
};

#endif
