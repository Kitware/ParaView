## Fix keyboard focus grabs by property widgets

When a property widget (generally, pqInteractivePropertyWidget
instances such as plane widgets, coordinate-frame widgets, etc.)
registered keyboard shortcuts with a context widget, the context
widget would become focused any time the property widget became
visible. This was annoying in situations where users were actively
pressing keys. For example, you might be using the arrow keys
in the Pipeline Browser to switch between pipeline filters when
suddenly the keyboard focus switched to the active 3-D view.

This situation occurred because the pqShortcutDecorator would
always switch the focus to a pqModalShortcut's "context widget"
(generally the active 3-D view) whenever the property widget
became visible.

Going forward, the focus is only diverted to the context widget
when you explicitly click on a property-widget's frame (i.e.,
you are using the mouse not the keyboard and interacting with
the property widget, not some other widget). When a pipeline
object is shown in the Properties panel, keyboard shortcuts
are activated for the first item that has them. Otherwise, you
must click on the context widget for the shortcuts to be accepted.
