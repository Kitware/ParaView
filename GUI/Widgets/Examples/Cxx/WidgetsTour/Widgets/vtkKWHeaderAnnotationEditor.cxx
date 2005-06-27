#include "vtkImageViewer2.h"
#include "vtkKWApplication.h"
#include "vtkKWHeaderAnnotationEditor.h"
#include "vtkKWFrame.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLImageDataReader.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWHeaderAnnotationEditorItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeVTK; };
  vtkKWHeaderAnnotationEditorItem(vtkKWWidget *parent, vtkKWWindow *);
  virtual ~vtkKWHeaderAnnotationEditorItem();

protected:
  vtkKWRenderWidget *RenderWidget;
  vtkXMLImageDataReader *Reader;
  vtkImageViewer2 *Viewer;
  vtkRenderWindowInteractor *Interactor;
  vtkKWHeaderAnnotationEditor *HeaderAnnotationEditor;
};

vtkKWHeaderAnnotationEditorItem::vtkKWHeaderAnnotationEditorItem(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a render widget
  // Set the header annotation visibility and set some text

  this->RenderWidget = vtkKWRenderWidget::New();
  this->RenderWidget->SetParent(parent);
  this->RenderWidget->Create(app);

  this->RenderWidget->HeaderAnnotationVisibilityOn();
  this->RenderWidget->SetHeaderAnnotationText("Hello, World!");

  app->Script("pack %s -side right -fill both -expand y -padx 0 -pady 0", 
              this->RenderWidget->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a volume reader

  this->Reader = vtkXMLImageDataReader::New();
  this->Reader->SetFileName(
    KWWidgetsTourItem::GetPathToExampleData(app, "head100x100x47.vti"));

  // Create an image viewer
  // Use the render window and renderer of the renderwidget

  this->Viewer = vtkImageViewer2::New();
  this->Viewer->SetRenderWindow(this->RenderWidget->GetRenderWindow());
  this->Viewer->SetRenderer(this->RenderWidget->GetRenderer());
  this->Viewer->SetInput(this->Reader->GetOutput());

  this->Interactor = vtkRenderWindowInteractor::New();
  this->Viewer->SetupInteractor(this->Interactor);

  this->RenderWidget->ResetCamera();

  // -----------------------------------------------------------------------

  // Create a header annotation editor
  // Connect it to the render widget
  
  this->HeaderAnnotationEditor = vtkKWHeaderAnnotationEditor::New();
  this->HeaderAnnotationEditor->SetParent(parent);
  this->HeaderAnnotationEditor->Create(app);
  this->HeaderAnnotationEditor->SetRenderWidget(this->RenderWidget);

  app->Script("pack %s -side left -anchor nw -expand n -padx 2 -pady 2", 
              this->HeaderAnnotationEditor->GetWidgetName());
}

vtkKWHeaderAnnotationEditorItem::~vtkKWHeaderAnnotationEditorItem()
{
  this->HeaderAnnotationEditor->Delete();
  this->Reader->Delete();
  this->Interactor->Delete();
  this->RenderWidget->Delete();
  this->Viewer->Delete();
}

KWWidgetsTourItem* vtkKWHeaderAnnotationEditorEntryPoint(vtkKWWidget *parent, vtkKWWindow *window)
{
  return new vtkKWHeaderAnnotationEditorItem(parent, window);
}
