#include "vtkKWApplication.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWText.h"
#include "vtkKWFrame.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLanguage.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/stl/string>

int my_main(int argc, char *argv[])
{
  // Initialize Tcl

  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc, argv, &cerr);
  if (!interp)
    {
    cerr << "Error: InitializeTcl failed" << endl ;
    return 1;
    }

  // Set the current text (translation) domain to an arbitrary name,
  // which will be used to look up the corresponding translation catalog
  // at runtime. 

  vtkKWInternationalization::SetCurrentTextDomain(
    "KWInternationalizationExample");
  
  // Try to find where the translation catalogs for the current text
  // domain can be found on disk. Several candidates are considered.

  vtkKWInternationalization::FindTextDomainBinding(
    vtkKWInternationalization::GetCurrentTextDomain());
  
  // Process some command-line arguments
  // The --test option here is used to run this example as a non-interactive 
  // test for software quality purposes. You can ignore it.

  int option_test = 0;
  vtksys_stl::string option_lang;
  vtksys::CommandLineArguments args;
  args.Initialize(argc, argv);
  args.AddArgument(
    "--test", vtksys::CommandLineArguments::NO_ARGUMENT, &option_test, "");
  args.AddArgument(
    "--lang", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &option_lang, "");
  args.Parse();
  
  // Create the application
  // If --test was provided, ignore all registry settings, and exit silently
  // Restore the settings that have been saved to the registry, like
  // the geometry of the user interface so far.

  vtkKWApplication *app = vtkKWApplication::New();
  app->SetName("KWInternationalizationExample");
  if (option_test)
    {
    app->SetRegistryLevel(0);
    }
  app->RestoreApplicationSettingsFromRegistry();

  // Let's create a simple message dialog with 3 buttons.
  // Two buttons will be used to switch from English to French, the last
  // one will close/cancel the message dialog.

  app->PromptBeforeExitOff();

  char buffer[500];

  int keep_going = !option_test;
  while (keep_going || !app->Exit())
    {
    vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
    dlg->SetApplication(app);
    dlg->SetStyleToOkOtherCancel();
    
    // Change the 'OK' and 'Other ' button label to the name of the
    // languages we want to switch to. Note how we use a special 
    // separator (|) here to provide more context to the translation
    // framework. This is pretty useful when translating very short 
    // strings that could require different translations within the same
    // translation catalog.

    dlg->SetOKButtonText(s_("Button|English"));
    dlg->SetOtherButtonText(s_("Button|French"));

    dlg->Create();
    dlg->SetTextWidth(400);
    
    // Set the title to a simple translatable string
    
    dlg->SetTitle(_("A simple dialog box!"));

    // Set the message to a more complex string with printf() arguments.
    // This demonstrates a very common issue where the arguments 
    // to printf() can be used in a different order depending on
    // the language. This is resolved by using a custom version of 
    // printf (provided transparently here by GNU gettext) that supports
    // POSIX positional parameters.
    // A positional parameter is an extension to the printf format that
    // allows explicit references to the rank of any of the arguments passed
    // to printf(), e.g. %2$s specifies that we want to use the second
    // argument (as a %s). Positional parameters are specified in the
    // translation catalog to re-shuffle the arguments at run-time, not in
    // the source code (check the french translation catalog for example).

    sprintf(buffer, _("The string '%s' has %d characters."), 
            app->GetName(), (int)strlen(app->GetName()));
    dlg->SetText(buffer);

    // Invoke the dialog, and switch the current language

    dlg->Invoke();
    if (dlg->GetStatus() == vtkKWMessageDialog::StatusOK)
      {
      vtkKWLanguage::SetCurrentLanguage(vtkKWLanguage::ENGLISH);
      }
    else if (dlg->GetStatus() == vtkKWMessageDialog::StatusOther)
      {
      vtkKWLanguage::SetCurrentLanguage(vtkKWLanguage::FRENCH);
      }
    keep_going = dlg->GetStatus() != vtkKWMessageDialog::StatusCanceled;
    dlg->Delete();
    }

  // If --test was provided, do not prompt the message dialog and run this
  // example as a non-interactive test for software quality purposes.

  int ret = 0;
  if (option_test)
    {
    vtkKWLanguage::SetCurrentLanguage(vtkKWLanguage::FRENCH);
    sprintf(buffer, _("Test foo = %s and 3 = %d."), "foo", 3);
#ifdef KWWidgets_USE_INTERNATIONALIZATION
    ret += strcmp(buffer, "Test 3 = 3 et foo = foo.") ? 1 : 0;
    ret += strcmp(s_("Button|English"), "Anglais") ? 1 : 0;
    vtkKWLanguage::SetCurrentLanguage(vtkKWLanguage::ENGLISH);
    sprintf(buffer, _("Test foo = %s and 3 = %d."), "foo", 3);
#endif
    ret += strcmp(buffer, "Test foo = foo and 3 = 3.") ? 1 : 0;
    ret += strcmp(s_("Button|English"), "English") ? 1 : 0;
    }

  // Deallocate and exit
  
  app->Delete();
  
  return ret;
}

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int)
{
  int argc;
  char **argv;
  vtksys::SystemTools::ConvertWindowsCommandLineToUnixArguments(
    lpCmdLine, &argc, &argv);
  int ret = my_main(argc, argv);
  for (int i = 0; i < argc; i++) { delete [] argv[i]; }
  delete [] argv;
  return ret;
}
#else
int main(int argc, char *argv[])
{
  return my_main(argc, argv);
}
#endif
