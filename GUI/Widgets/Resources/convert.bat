SET CONVERTIMAGE=ConvertImage
SET OPTIONS=ZLIB BASE64 %1

%CONVERTIMAGE% KWWindowLayout.h KWWindowLayout1x1.png KWWindowLayout1x1c.png KWWindowLayout1x2.png KWWindowLayout1x2c.png KWWindowLayout2x1.png KWWindowLayout2x1c.png KWWindowLayout2x2.png KWWindowLayout2x2c.png KWWindowLayout2x3.png KWWindowLayout2x3c.png KWWindowLayout3x2.png KWWindowLayout3x2c.png %OPTIONS%
