#ifndef __KWWidgetsTourExampleTypes_h
#define __KWWidgetsTourExampleTypes_h

/* 
   Create widgets map
 */

class vtkKWApplication;

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

  KWWidgetsTourItem() {};
  virtual ~KWWidgetsTourItem() {};

  // Get path to example data

  static const char *GetPathToExampleData(
    vtkKWApplication *app, const char *name);
};


typedef KWWidgetsTourItem* (*KWWidgetsTourItemEntryPoint)(vtkKWWidget *parent, vtkKWWindow *win);

typedef struct
{
  const char *Name;
  KWWidgetsTourItemEntryPoint EntryPoint;
} KWWidgetsTourNode;

#endif
