## Fix a UI glitch when annotation filtering is enabled

When the `pqTabbedMultiViewWidget` has annotation filtering
turned on (so only relevant views/layouts are displayed),
then it is possible to get into a situation where no layout
tabs are visible and no new tab can be created; you are
stuck unable to create any views.

We modify the behavior of `pqTabbedMultiViewWidget` to work
around this and make it possible to create new views like so:
1. Change `pqTabbedMultiViewWidget::pqInternals::updateVisibleTabs()`
   to display (rather than hide) tabs with 0 views (i.e., the tabs
   that are showing a view selector) even if the annotation filters
   do not apply.
2. Change `pqTabbedMultiViewWidget::createTab(pqServer*)` to call
   `updateVisibleTabs()` so that a tab index is assigned to a new
   layout created by clicking on the "+" tab even when annotation
   filters would normally hide it.
3. Adds an observer to the `QTabWidget::tabBarClicked` signal that
   calls `pqTabbedMultiViewWidget::createTab()` when only the "+"
   tab is visible.

With all of these changes, it is possible to create a new view even
with only the "+" tab showing. Be aware that if you create a new
view by clicking on a view type, then the next time you click the "+" tab,
that layout may be hidden (because now it has views that do not match
the annotation filter).
