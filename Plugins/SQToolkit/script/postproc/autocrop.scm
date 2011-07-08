;   ____    _ __           ____               __    ____
;  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
; _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
;/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
;
;Copyright 2008 SciberQuest Inc.

;------------------------------------------------------------------------------
(define
  (sq-auto-crop inFile outFile)
    (let*
      (
      (im   0)
      (dw   0)
      )

      ; load the original
      (set! im (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
      (set! dw (car (gimp-image-get-active-layer im)))

      ; auto-crop
      (plug-in-autocrop RUN-NONINTERACTIVE im dw)

      ; save the cropped image
      (gimp-file-save RUN-NONINTERACTIVE im dw outFile outFile)
    )
)

;EOF
 