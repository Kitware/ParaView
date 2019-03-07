# Default coincident topology resolution method has changed

The method used to resolve coincident topology such as lines on surfaces has been changed to "Offset faces relative to lines and points". This method does not suffer from the same problem with lines appearing to float above surfaces that were present in some zoomed-in views with the previous default method "Shift z-buffer when rendering lines and points".
