
#include <QDockWidget>

class pqView;
class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkPVOpenVRHelper;

class pvOpenVRDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  pvOpenVRDockPanel(const QString& t, QWidget* p = 0, Qt::WindowFlags f = 0)
    : Superclass(t, p, f)
  {
    this->constructor();
  }
  pvOpenVRDockPanel(QWidget* p = 0, Qt::WindowFlags f = 0)
    : Superclass(p, f)
  {
    this->constructor();
  }

  ~pvOpenVRDockPanel();

protected:
  vtkPVOpenVRHelper* Helper;

protected slots:
  void sendToOpenVR();

  void exportLocationsAsSkyboxes();
  void exportLocationsAsView();

  void multiSampleChanged(int state);
  void defaultCropThicknessChanged(const QString& text);
  void editableFieldChanged(const QString& text);
  void fieldValuesChanged(const QString& text);

  void setActiveView(pqView*);

  void loadState(vtkPVXMLElement*, vtkSMProxyLocator*);
  void saveState(vtkPVXMLElement*);

  void prepareForQuit();

  void beginPlay();
  void endPlay();
  void updateSceneTime();

  void onViewAdded(pqView*);
  void onViewRemoved(pqView*);

  void collaborationConnect();
  void collaborationCallback(std::string const& data, void* cd);

private:
  void constructor();

  class pqInternals;
  pqInternals* Internals;
};
