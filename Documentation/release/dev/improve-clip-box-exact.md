## Improve performance of box clipping with exact flag on

The `Clipping` filter has been sped up overall. Box clipping with the `Exact` property turned on has been significantly
sped up using a smart strategy for determining the order of clips to apply.
