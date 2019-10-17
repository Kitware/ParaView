# QTTesting additions

* ItemView widgets record and replay a double-click in addition to an edit event,
  so TreeViews that respond to double-click but not edit can be tested.
* ItemView widgets record and replay selection events in addition to setCurrentItem,
  so they can test multiple selections. Useful in spreadsheet view and color annotation
  widget, for example.
