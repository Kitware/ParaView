SET CONVERTIMAGE=ConvertImage
SET OPTIONS=ZLIB BASE64 %1

%CONVERTIMAGE% icons.h annotate.png contours.png cut.png error.png expand.png filters.png general.png helpbubble.png info_mini.png layout.png macros.png material.png preferences.png question.png shrink.png smallerror.png smallerrorred.png transfer.png warning.png warning_mini.png %OPTIONS%
