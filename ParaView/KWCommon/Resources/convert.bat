SET CONVERTIMAGE=ConvertImage
SET OPTIONS=ZLIB BASE64 %1

%CONVERTIMAGE% icons.h annotate.png axes.png contours.png cut.png error.png expand.png filters.png general.png helpbubble.png info_mini.png layout.png macros.png material.png minus.png move.png plus.png preferences.png question.png shrink.png smallerror.png smallerrorred.png transfer.png trashcan.png warning.png warning_mini.png %OPTIONS%
