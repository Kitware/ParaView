# Keyboard shortcuts and property widgets

ParaView now has a singleton object, `pqKeySequences`, that
can be used to create and manage Qt shortcuts so that there
are no collisions between widgets/actions that both want the
same keyboard shortcut.

An example of this is when a property panel might wish to
use both a handle widget and a line widget. Each of these
registers shortcuts for the P and Ctrl+P/Cmd+P keys.
Because multiple widgets registered shortcuts for the same
key, Qt never signals *either* widget unambiguously when
these keys are pressed.

By registering shortcuts with this class, widgets will have
their shortcuts enabled and disabled so that only one is
using the shortcut at a time. The user is shown which widgets
have shortcuts registered (via a frame highlighted with a
thin blue line, the same as the active view's highlighting)
and can enable/disable widgets by clicking on the Qt widget's
background.

## Developer-facing changes

To accomplish the above, several changes to pqPropertyWidget
and its subclasses as well as to the pqPointPickingHelper
were required:

+ `pqPropertyWidget` inherits QFrame instead of QWidget.
  By default, the frame is configured not to render or
  take any additional space. However, when shortcuts are
  registered with `pqKeySequences`, a new decorator class
  named `pqShortcutDecorator` configures the frame to
  reflect when shortcuts are registered and enabled.
+ `pqPointPickingHelper` previously accepted any QObject
  as a parent. However, all occurrences of this class used
  property widgets as the parent. By forcing parents to be
  of this type, subclasses of `pqInteractivePropertyWidget`
  that used the picking helper do not need to manage visibility
  changes. (Instead `pqShortcutDecorator` enables/disables
  shortcuts to match visibility changes.)
  If you use `pqPointPickingHelper` in your code, ensure its
  parent class is a `pqPropertyWidget` and remove any usage
  of the `setShortcutEnabled()` method.
