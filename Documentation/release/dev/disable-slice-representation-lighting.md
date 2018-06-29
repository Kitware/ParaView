# Slice representation lighting is turned off

Lighting on the Slice representation has the undesirable effect of
darkening the Slice with the default light, which is off center with
respect to the view and focal points. We assume the common use case
for this representation is to view the data as directly as possible to
match colors on the slice with colors in the color legend. To better
support this use case, lighting is disabled for this representation.
