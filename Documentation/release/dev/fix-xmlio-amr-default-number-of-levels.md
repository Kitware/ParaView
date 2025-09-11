## XML UniformGrid AMR reader: Change default behavior so it reads all levels by default

The XML UniformGrid AMR Reader used to read only the first level of the AMR through the property
"DefaultNumberOfLevels" default value of 1. It now reads all levels as the default value of this property is now 0.

This is a change of behavior and considered a bugfix as previous behavior was incorrect, but it may result
in unexpected results for anyone relying on this previous behavior.
