;   ____    _ __           ____               __    ____
;  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
; _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
;/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
;
;Copyright 2008 SciberQuest Inc.

;------------------------------------------------------------------------------
(define
  (sq-crop inFile outFile width height offx offy)
    (let*
      (
      (w    0)
      (h    0)
      (x    0)
      (y    0)
      (im   0)
      (dw   0)
      )

      ; load the original
      (set! im (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
      (set! dw (car (gimp-image-get-active-layer im)))

      ; crop to the requested size
      (set! w (string->number width))
      (set! h (string->number height))
      (set! x (string->number offx))
      (set! y (string->number offy))
      (gimp-image-crop im w h x y)

      ; save the cropped image
      (gimp-file-save RUN-NONINTERACTIVE im dw outFile outFile)
    )
)
