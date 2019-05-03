# Layouts and Python support

Mutliple views and layouts are now better traced in Python. When the generated
trace is played back in the GUI, the views are laid out as expected. Also Python
state can now capture the view layout state faithfully.

Developers and Python users must note that view creation APIs no longer
automatically assign views to a layout. One must use explicit function calls to
assign a view to a layout after creation.
