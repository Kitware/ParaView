SET CONVERTIMAGE=ConvertImage
SET OPTIONS=ZLIB BASE64 %1

%CONVERTIMAGE% icons.h annotate.png axes.png connection.png contours.png cut.png error.png expand.png filters.png folder.png general.png helpbubble.png info_mini.png layout.png lock.png macros.png material.png minus.png move.png move_h.png move_v.png plus.png preferences.png question.png reload.png shrink.png smallerror.png smallerrorred.png stopwatch.png transfer.png trashcan.png warning.png warning_mini.png window_level.png %OPTIONS%
