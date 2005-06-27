#include "vtkImageViewer2.h"
#include "vtkKWApplication.h"
#include "vtkKWCornerAnnotationEditor.h"
#include "vtkKWFrame.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLImageDataReader.h"
#include "vtkCornerAnnotation.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWCornerAnnotationEditorItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeVTK; };
  vtkKWCornerAnnotationEditorItem(vtkKWWidget *parent, vtkKWWindow *);
  virtual ~vtkKWCornerAnnotationEditorItem();

protected:
  vtkKWRenderWidget *RenderWidget;
  vtkXMLImageDataReader *Reader;
  vtkImageViewer2 *Viewer;
  vtkRenderWindowInteractor *Interactor;
  vtkKWCornerAnnotationEditor *CornerAnnotationEditor;
};

vtkKWCornerAnnotationEditorItem::vtkKWCornerAnnotationEditorItem(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a render widget
  // Set the corner annotation visibility

  this->RenderWidget = vtkKWRenderWidget::New();
  this->RenderWidget->SetParent(parent);
  this->RenderWidget->Create(app);
  this->RenderWidget->CornerAnnotationVisibilityOn();

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

  // Reset the window/level and the camera

  this->Reader->Update();
  double *range = this->Reader->GetOutput()->GetScalarRange();
  this->Viewer->SetColorWindow(range[1] - range[0]);
  this->Viewer->SetColorLevel(0.5 * (range[1] + range[0]));

  this->RenderWidget->ResetCamera();

  // -----------------------------------------------------------------------

  // The corner annotation has the ability to parse "tags" and fill
  // them with information gathered from other objects.
  // For example, let's display the slice and window/level in one corner
  // by connecting the corner annotation to our image actor and
  // image mapper

  vtkCornerAnnotation *ca = this->RenderWidget->GetCornerAnnotation();
  ca->SetImageActor(this->Viewer->GetImageActor());
  ca->SetWindowLevel(this->Viewer->GetWindowLevel());
  ca->SetText(2, "<slice>");
  ca->SetText(3, "<window>\n<level>");
  ca->SetText(1, "Hello, World!");

  // -----------------------------------------------------------------------

  // Create a corner annotation editor
  // Connect it to the render widget
  
  this->CornerAnnotationEditor = vtkKWCornerAnnotationEditor::New();
  this->CornerAnnotationEditor->SetParent(parent);
  this->CornerAnnotationEditor->Create(app);
  this->CornerAnnotationEditor->SetRenderWidget(this->RenderWidget);

  app->Script("pack %s -side left -anchor nw -expand n -padx 2 -pady 2", 
              this->CornerAnnotationEditor->GetWidgetName());
}

vtkKWCornerAnnotationEditorItem::~vtkKWCornerAnnotationEditorItem()
{
  this->CornerAnnotationEditor->Delete();
  this->Reader->Delete();
  this->Interactor->Delete();
  this->RenderWidget->Delete();
  this->Viewer->Delete();
}

KWWidgetsTourItem* vtkKWCornerAnnotationEditorEntryPoint(vtkKWWidget *parent, vtkKWWindow *window)
{
  return new vtkKWCornerAnnotationEditorItem(parent, window);
}
