
/// \file pqPipelineObject.h
///
/// \date 11/16/2005

#ifndef _pqPipelineObject_h
#define _pqPipelineObject_h


class pqPipelineObjectInternal;
class pqPipelineServer;
class QWidget;
class vtkSMDisplayProxy;
class vtkSMProxy;


class pqPipelineObject
{
public:
  enum ObjectType {
    Source,
    Filter,
    Bundle,
    Window
  };

public:
  pqPipelineObject(vtkSMProxy *proxy, ObjectType type);
  pqPipelineObject(QWidget *widget);
  ~pqPipelineObject();

  ObjectType GetType() const {return this->Type;}
  void SetType(ObjectType type);

  vtkSMProxy *GetProxy() const;
  void SetProxy(vtkSMProxy *proxy);

  QWidget *GetWidget() const;
  void SetWidget(QWidget *widget);

  bool IsVisible() const {return this->Display != 0;}
  vtkSMDisplayProxy *GetDisplayProxy() const {return this->Display;}
  void SetDisplayProxy(vtkSMDisplayProxy *display) {this->Display = display;}

  pqPipelineObject *GetParent() const;
  void SetParent(pqPipelineObject *parent);

  pqPipelineServer *GetServer() const;
  void SetServer(pqPipelineServer *server);

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
  union {
    vtkSMProxy *Proxy;                ///< Stores the proxy pointer.
    QWidget *Widget;                  ///< Stores the widget pointer.
  } Data;
  union {
    pqPipelineObject *Window;         ///< Stores the parent window.
    pqPipelineServer *Server;         ///< Stores the parent server.
  } Parent;
};

#endif
