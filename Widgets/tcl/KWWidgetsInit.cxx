#include "vtkTclUtil.h"
int vtkKWApplicationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWApplicationNewCommand();
int vtkKWChangeColorButtonCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWChangeColorButtonNewCommand();
int vtkKWCheckButtonCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWCheckButtonNewCommand();
int vtkKWCompositeCollectionCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWCompositeCollectionNewCommand();
int vtkKWCornerAnnotationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWCornerAnnotationNewCommand();
int vtkKWDialogCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWDialogNewCommand();
int vtkKWEntryCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWEntryNewCommand();
int vtkKWExtentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWExtentNewCommand();
int vtkKWGenericCompositeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWGenericCompositeNewCommand();
int vtkKWLabeledFrameCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWLabeledFrameNewCommand();
int vtkKWMenuCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWMenuNewCommand();
int vtkKWMessageDialogCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWMessageDialogNewCommand();
int vtkKWNotebookCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWNotebookNewCommand();
int vtkKWObjectCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWObjectNewCommand();
int vtkKWOptionMenuCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWOptionMenuNewCommand();
int vtkKWProgressGaugeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWProgressGaugeNewCommand();
int vtkKWRadioButtonCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWRadioButtonNewCommand();
int vtkKWSaveImageDialogCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWSaveImageDialogNewCommand();
int vtkKWScaleCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWScaleNewCommand();
int vtkKWSerializerCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWSerializerNewCommand();
int vtkKWTextCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWTextNewCommand();
int vtkKWToolbarCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWToolbarNewCommand();
int vtkKWWidgetCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWWidgetNewCommand();
int vtkKWWidgetCollectionCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWWidgetCollectionNewCommand();
int vtkKWViewCollectionCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWViewCollectionNewCommand();
int vtkKWVolumeCompositeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWVolumeCompositeNewCommand();
int vtkKWWindowCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWWindowNewCommand();
int vtkKWWindowCollectionCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWWindowCollectionNewCommand();

extern Tcl_HashTable vtkInstanceLookup;
extern Tcl_HashTable vtkPointerLookup;
extern Tcl_HashTable vtkCommandLookup;
extern void vtkTclDeleteObjectFromHash(void *);
extern void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);


extern "C" {int VTK_EXPORT Vtkkwwidgetstcl_SafeInit(Tcl_Interp *interp);}



extern "C" {int VTK_EXPORT Vtkkwwidgetstcl_Init(Tcl_Interp *interp);}



extern void vtkTclGenericDeleteObject(ClientData cd);



int VTK_EXPORT Vtkkwwidgetstcl_SafeInit(Tcl_Interp *interp)
{
  return Vtkkwwidgetstcl_Init(interp);
}


int VTK_EXPORT Vtkkwwidgetstcl_Init(Tcl_Interp *interp)
{
  vtkTclCreateNew(interp,"vtkKWApplication", vtkKWApplicationNewCommand,
                  vtkKWApplicationCommand);
  vtkTclCreateNew(interp,"vtkKWChangeColorButton", vtkKWChangeColorButtonNewCommand,
                  vtkKWChangeColorButtonCommand);
  vtkTclCreateNew(interp,"vtkKWCheckButton", vtkKWCheckButtonNewCommand,
                  vtkKWCheckButtonCommand);
  vtkTclCreateNew(interp,"vtkKWCompositeCollection", vtkKWCompositeCollectionNewCommand,
                  vtkKWCompositeCollectionCommand);
  vtkTclCreateNew(interp,"vtkKWCornerAnnotation", vtkKWCornerAnnotationNewCommand,
                  vtkKWCornerAnnotationCommand);
  vtkTclCreateNew(interp,"vtkKWDialog", vtkKWDialogNewCommand,
                  vtkKWDialogCommand);
  vtkTclCreateNew(interp,"vtkKWEntry", vtkKWEntryNewCommand,
                  vtkKWEntryCommand);
  vtkTclCreateNew(interp,"vtkKWExtent", vtkKWExtentNewCommand,
                  vtkKWExtentCommand);
  vtkTclCreateNew(interp,"vtkKWGenericComposite", vtkKWGenericCompositeNewCommand,
                  vtkKWGenericCompositeCommand);
  vtkTclCreateNew(interp,"vtkKWLabeledFrame", vtkKWLabeledFrameNewCommand,
                  vtkKWLabeledFrameCommand);
  vtkTclCreateNew(interp,"vtkKWMenu", vtkKWMenuNewCommand,
                  vtkKWMenuCommand);
  vtkTclCreateNew(interp,"vtkKWMessageDialog", vtkKWMessageDialogNewCommand,
                  vtkKWMessageDialogCommand);
  vtkTclCreateNew(interp,"vtkKWNotebook", vtkKWNotebookNewCommand,
                  vtkKWNotebookCommand);
  vtkTclCreateNew(interp,"vtkKWObject", vtkKWObjectNewCommand,
                  vtkKWObjectCommand);
  vtkTclCreateNew(interp,"vtkKWOptionMenu", vtkKWOptionMenuNewCommand,
                  vtkKWOptionMenuCommand);
  vtkTclCreateNew(interp,"vtkKWProgressGauge", vtkKWProgressGaugeNewCommand,
                  vtkKWProgressGaugeCommand);
  vtkTclCreateNew(interp,"vtkKWRadioButton", vtkKWRadioButtonNewCommand,
                  vtkKWRadioButtonCommand);
  vtkTclCreateNew(interp,"vtkKWSaveImageDialog", vtkKWSaveImageDialogNewCommand,
                  vtkKWSaveImageDialogCommand);
  vtkTclCreateNew(interp,"vtkKWScale", vtkKWScaleNewCommand,
                  vtkKWScaleCommand);
  vtkTclCreateNew(interp,"vtkKWSerializer", vtkKWSerializerNewCommand,
                  vtkKWSerializerCommand);
  vtkTclCreateNew(interp,"vtkKWText", vtkKWTextNewCommand,
                  vtkKWTextCommand);
  vtkTclCreateNew(interp,"vtkKWToolbar", vtkKWToolbarNewCommand,
                  vtkKWToolbarCommand);
  vtkTclCreateNew(interp,"vtkKWWidget", vtkKWWidgetNewCommand,
                  vtkKWWidgetCommand);
  vtkTclCreateNew(interp,"vtkKWWidgetCollection", vtkKWWidgetCollectionNewCommand,
                  vtkKWWidgetCollectionCommand);
  vtkTclCreateNew(interp,"vtkKWViewCollection", vtkKWViewCollectionNewCommand,
                  vtkKWViewCollectionCommand);
  vtkTclCreateNew(interp,"vtkKWVolumeComposite", vtkKWVolumeCompositeNewCommand,
                  vtkKWVolumeCompositeCommand);
  vtkTclCreateNew(interp,"vtkKWWindow", vtkKWWindowNewCommand,
                  vtkKWWindowCommand);
  vtkTclCreateNew(interp,"vtkKWWindowCollection", vtkKWWindowCollectionNewCommand,
                  vtkKWWindowCollectionCommand);
  return TCL_OK;
}
