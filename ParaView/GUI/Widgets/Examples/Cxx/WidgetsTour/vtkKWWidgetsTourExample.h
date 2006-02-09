#ifndef __vtkKWWidgetsTourExample_h
#define __vtkKWWidgetsTourExample_h

#include "vtkKWObject.h"

class vtkKWWidget;
class vtkKWWindow;

//----------------------------------------------------------------------------
//BTX
class KWWidgetsTourItem
{
public:

  // Get the type

  enum WidgetType 
  {
    TypeCore,
    TypeComposite,
    TypeVTK
  };
  virtual int GetType() = 0;
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *win) = 0;

  KWWidgetsTourItem() {};
  virtual ~KWWidgetsTourItem() {};
};

typedef KWWidgetsTourItem* (*KWWidgetsTourItemEntryPoint)();
//ETX

//----------------------------------------------------------------------------
//BTX
typedef struct
{
  const char *Name;
  KWWidgetsTourItemEntryPoint EntryPoint;
} KWWidgetsTourNode;
//ETX

//----------------------------------------------------------------------------
class vtkKWTreeWithScrollbars;
class vtkKWWindow;
class vtkKWTextWithScrollbarsWithLabel;
class vtkKWWidgetsTourExampleInternals;

class vtkKWWidgetsTourExample : public vtkKWObject
{
public:
  static vtkKWWidgetsTourExample* New();
  vtkTypeRevisionMacro(vtkKWWidgetsTourExample,vtkKWObject);

  // Description:
  // Run the example.
  int Run(int argc, char *argv[]);

  // Description:
  // Select specific example
  virtual void SelectExample(const char *name);

  // Description:
  // Callbacks
  virtual void SelectionChangedCallback();

  // Description:
  // Get path to example data
  static const char *GetPathToExampleData(
    vtkKWApplication *app, const char *name);

protected:
  vtkKWWidgetsTourExample();
  ~vtkKWWidgetsTourExample();

  vtkKWTreeWithScrollbars          *WidgetsTree;
  vtkKWWindow                      *Window;
  vtkKWTextWithScrollbarsWithLabel *CxxSourceText;
  vtkKWTextWithScrollbarsWithLabel *PythonSourceText;
  vtkKWTextWithScrollbarsWithLabel *TclSourceText;

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWWidgetsTourExampleInternals *Internals;
  //ETX

private:
  vtkKWWidgetsTourExample(const vtkKWWidgetsTourExample&);   // Not implemented.
  void operator=(const vtkKWWidgetsTourExample&);  // Not implemented.
};

#endif
