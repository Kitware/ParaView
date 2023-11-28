## Filters categories are configurable

The ParaView `Filters` menu presents filters through a list of categories.
Those submenus can now be configured and can be nested!

Go to the `Tools / Configure Categories` dialog to create your own tree of categories.
There you can: create and rename categories, move filters around, and event create toolbar
from an existing category.

Some predefined categories have a specific meaning (and thus cannot be deleted):
 - `Alphabetical` category lists as usual every filter.
 - `Miscellaneous` section contains filters that are not part of any other category.
 - `Favorites` is now its own category, so you can configure it the same way.

> ![The categories configuration dialog](ConfiguragleCategories.png)
>
> The categories configuration dialog

Also note that the `hide_for_tests` attribute in Category XML is no
longer supported.
