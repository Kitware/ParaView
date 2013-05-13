
#include <vtkObject.h>

class vtkTPTest : public vtkObject
{
public:
  vtkTypeMacro(vtkTPTest, vtkObject);

  static vtkTPTest* New();

  bool TestPlugin();
};
