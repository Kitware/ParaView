#include <string>
#include <vtkCommand.h>

class vtkErrorObserver : public vtkCommand
{
public:
  vtkErrorObserver();
  static vtkErrorObserver* New();
  bool GetError() const;
  bool GetWarning() const;
  void Clear();
  virtual void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* calldata);

  std::string GetErrorMessage() const;
  std::string GetWarningMessage() const;

private:
  bool Error;
  bool Warning;
  std::string ErrorMessage;
  std::string WarningMessage;
};
