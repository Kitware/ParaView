## IOSSReader: Add GroupAlphabeticVectorFieldComponents and GlobalFields properties

`IOSSReader` used to have a method `ReadGlobalFields` to control if the global fields would be reader or not. This
method has been deprecated in favor of a more flexible approach using `GlobalFields` to allow users to select specific
global fields to be read. This is particularly useful when dealing with datasets with a large number of global fields,
as it allows users to read only the fields they are interested in, improving performance and reducing memory usage.

Additionally, `IOSSReader` can now control whether to group alphabetic vector field components, such as `vel_x`,
`vel_y`, `vel_z` into a single vector field, such as `vel`, something which was automatically done before.

Finally, the `IOSSReader`'s `ReadAllFilesToDetermineStructure` default value has been changed to false.
This change improves performance on the common case, and the user will still be notified if the flag needs
to be turned on.
