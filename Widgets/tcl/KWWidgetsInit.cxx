#include "vtkTclUtil.h"
int vtkCornerAnnotationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkCornerAnnotationNewCommand();
int vtkKWActorCompositeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWActorCompositeNewCommand();
int vtkKWApplicationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWApplicationNewCommand();
int vtkKWCallbackSpecificationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWCallbackSpecificationNewCommand();
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
int vtkKWEventNotifierCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWEventNotifierNewCommand();
int vtkKWExtentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWExtentNewCommand();
int vtkKWGenericCompositeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWGenericCompositeNewCommand();
int vtkKWLabelCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWLabelNewCommand();
int vtkKWLabeledEntryCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWLabeledEntryNewCommand();
int vtkKWLabeledFrameCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWLabeledFrameNewCommand();
int vtkKWListBoxCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWListBoxNewCommand();
int vtkKWMenuCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWMenuNewCommand();
int vtkKWMenuButtonCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWMenuButtonNewCommand();
int vtkKWMessageDialogCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWMessageDialogNewCommand();
int vtkKWNotebookCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWNotebookNewCommand();
int vtkKWObjectCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWObjectNewCommand();
int vtkKWOKCancelDialogCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWOKCancelDialogNewCommand();
int vtkKWOptionMenuCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWOptionMenuNewCommand();
int vtkKWProgressGaugeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWProgressGaugeNewCommand();
int vtkKWPushButtonCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWPushButtonNewCommand();
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
int vtkKWXtEmbeddedWidgetCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWXtEmbeddedWidgetNewCommand();

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
  vtkTclCreateNew(interp,(char *) "vtkCornerAnnotation", vtkCornerAnnotationNewCommand,
                  vtkCornerAnnotationCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWActorComposite", vtkKWActorCompositeNewCommand,
                  vtkKWActorCompositeCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWApplication", vtkKWApplicationNewCommand,
                  vtkKWApplicationCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWCallbackSpecification", vtkKWCallbackSpecificationNewCommand,
                  vtkKWCallbackSpecificationCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWChangeColorButton", vtkKWChangeColorButtonNewCommand,
                  vtkKWChangeColorButtonCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWCheckButton", vtkKWCheckButtonNewCommand,
                  vtkKWCheckButtonCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWCompositeCollection", vtkKWCompositeCollectionNewCommand,
                  vtkKWCompositeCollectionCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWCornerAnnotation", vtkKWCornerAnnotationNewCommand,
                  vtkKWCornerAnnotationCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWDialog", vtkKWDialogNewCommand,
                  vtkKWDialogCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWEntry", vtkKWEntryNewCommand,
                  vtkKWEntryCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWEventNotifier", vtkKWEventNotifierNewCommand,
                  vtkKWEventNotifierCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWExtent", vtkKWExtentNewCommand,
                  vtkKWExtentCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWGenericComposite", vtkKWGenericCompositeNewCommand,
                  vtkKWGenericCompositeCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWLabel", vtkKWLabelNewCommand,
                  vtkKWLabelCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWLabeledEntry", vtkKWLabeledEntryNewCommand,
                  vtkKWLabeledEntryCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWLabeledFrame", vtkKWLabeledFrameNewCommand,
                  vtkKWLabeledFrameCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWListBox", vtkKWListBoxNewCommand,
                  vtkKWListBoxCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWMenu", vtkKWMenuNewCommand,
                  vtkKWMenuCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWMenuButton", vtkKWMenuButtonNewCommand,
                  vtkKWMenuButtonCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWMessageDialog", vtkKWMessageDialogNewCommand,
                  vtkKWMessageDialogCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWNotebook", vtkKWNotebookNewCommand,
                  vtkKWNotebookCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWObject", vtkKWObjectNewCommand,
                  vtkKWObjectCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWOKCancelDialog", vtkKWOKCancelDialogNewCommand,
                  vtkKWOKCancelDialogCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWOptionMenu", vtkKWOptionMenuNewCommand,
                  vtkKWOptionMenuCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWProgressGauge", vtkKWProgressGaugeNewCommand,
                  vtkKWProgressGaugeCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWPushButton", vtkKWPushButtonNewCommand,
                  vtkKWPushButtonCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWRadioButton", vtkKWRadioButtonNewCommand,
                  vtkKWRadioButtonCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWSaveImageDialog", vtkKWSaveImageDialogNewCommand,
                  vtkKWSaveImageDialogCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWScale", vtkKWScaleNewCommand,
                  vtkKWScaleCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWSerializer", vtkKWSerializerNewCommand,
                  vtkKWSerializerCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWText", vtkKWTextNewCommand,
                  vtkKWTextCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWToolbar", vtkKWToolbarNewCommand,
                  vtkKWToolbarCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWWidget", vtkKWWidgetNewCommand,
                  vtkKWWidgetCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWWidgetCollection", vtkKWWidgetCollectionNewCommand,
                  vtkKWWidgetCollectionCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWViewCollection", vtkKWViewCollectionNewCommand,
                  vtkKWViewCollectionCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWVolumeComposite", vtkKWVolumeCompositeNewCommand,
                  vtkKWVolumeCompositeCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWWindow", vtkKWWindowNewCommand,
                  vtkKWWindowCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWWindowCollection", vtkKWWindowCollectionNewCommand,
                  vtkKWWindowCollectionCommand);
  vtkTclCreateNew(interp,(char *) "vtkKWXtEmbeddedWidget", vtkKWXtEmbeddedWidgetNewCommand,
                  vtkKWXtEmbeddedWidgetCommand);
  return TCL_OK;
}
