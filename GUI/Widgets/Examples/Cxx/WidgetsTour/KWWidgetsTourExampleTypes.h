#ifndef __KWWidgetsTourExampleTypes_h
#define __KWWidgetsTourExampleTypes_h

/* 
   Create widgets map
 */


enum WidgetType 
{
  InvalidWidget = 0,
  CoreWidget,
  CompositeWidget,
  VTKWidget
};

typedef WidgetType (*WidgetEntryPointFunctionPointer)(vtkKWWidget *parent, vtkKWWindow *win);

typedef struct
{
  const char *Name;
  WidgetEntryPointFunctionPointer EntryPoint;
} WidgetNode;

#endif
