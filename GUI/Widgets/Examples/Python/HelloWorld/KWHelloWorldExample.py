# Load the KWWidgets package

from kwwidgets import *

# Load requires system modules

import sys

# Process some command-line arguments
# The --test option here is used to run this example as a non-interactive test
# for software quality purposes. Ignore this feature in your own application.

if "--test" in sys.argv:
    option_test = 1
else:
    option_test = 0

def main(argv):
    
    # Initialize Tcl

    vtkKWApplication.InitializeTcl(len(argv), argv)

    # Create the application
    # If --test was provided, ignore all registry settings, and exit silently
    # Restore the settings that have been saved to the registry, like
    # the geometry of the user interface so far.

    app = vtkKWApplication()
    app.SetName("KWHelloWorldExample")
    if option_test:
        app.SetRegistryLevel(0)
        app.PromptBeforeExitOff()

    app.RestoreApplicationSettingsFromRegistry()
    
    # Set a help link. Can be a remote link (URL), or a local file

    app.SetHelpDialogStartingPage("http://www.kwwidgets.org")

    # Add a window
    # Set 'SupportHelp' to automatically add a menu entry for the help link

    win = vtkKWWindowBase()
    win.SupportHelpOn()
    app.AddWindow(win)
    win.Create()

    # Add a label, attach it to the view frame, and pack

    hello_label = vtkKWLabel()
    hello_label.SetParent(win.GetViewFrame())
    hello_label.Create()
    hello_label.SetText("Hello, World!")
    app.Script("pack %s -side left -anchor c -expand y",
               hello_label.GetWidgetName())

    # Start the application
    # If --test was provided, do not enter the event loop and run this example
    # as a non-interactive test for software quality purposes.

    ret = 0
    win.Display()
    if not option_test:
        app.Start(len(sys.argv), sys.argv)
        ret = app.GetExitStatus()

    win.Close()

    sys.exit(ret)

if __name__ == "__main__":
    main(sys.argv)
