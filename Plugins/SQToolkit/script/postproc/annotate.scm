;
;   ____    _ __           ____               __    ____
;  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
; _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
;/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
;
;Copyright 2008 SciberQuest Inc.
;

(define 
  (annotate inFile outFile text fontSize fontColor)
    (let* 
      (
      (imw  0)    ; image width
      (imh  0)    ; image ht
      (fpx  0)    ; font size px
      (lm   0)    ; left margin
      (rm   0)    ; right margin
      (tm   0)    ; top margin
      (bm   0)    ; bottom margin
      (fs   0)    ; font spacer
      (tl   0)    ; text layer
      (tw   0)    ; text width
      (th   0)    ; text height
      (x    0)    ; text position
      (y    0)    ; text position
      (im   0)    ; image
      (dw   0)    ; drawable
      )

      ; load the original
      (set! im (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
      (set! dw (car (gimp-image-get-active-layer im)))

      (set! imw (car (gimp-drawable-width dw)))
      (set! imh (car (gimp-drawable-height dw)))

      (set! fpx (string->number fontSize))

      (set! lm (* fpx 1))
      (set! rm (* fpx 0))
      (set! tm (* fpx 1))
      (set! bm (* fpx 1))

      (set! tw (* (string-length text) fpx))
      (set! th fpx)

      ; lower right position
      (set! x (- imw (+ rm tw)))
      (set! y (- imh (+ bm th)))

      ; add annotation
      (cond
        ((string=? fontColor "w")
          (gimp-context-set-background '(  0   0   0))
          (gimp-context-set-foreground '(255 255 255)))
        ((string=? fontColor "b")
          (gimp-context-set-background '(255 255 255))
          (gimp-context-set-foreground '(  0   0   0)))
        ((string=? fontColor "g")
          (gimp-context-set-background '(  0   0   0))
          (gimp-context-set-foreground '(145 145 145))))

      (gimp-floating-sel-to-layer 
        (car 
          (gimp-text-fontname 
              im
              dw
              x
              y
              text
              0
              TRUE
              fpx
              PIXELS
              "Nimbus Mono L Bold Oblique")))

      ; save the annotated image
      (gimp-image-merge-visible-layers im  CLIP-TO-IMAGE)
      (set! dw (car (gimp-image-get-active-layer im)))
      (gimp-file-save RUN-NONINTERACTIVE im dw outFile outFile)
      (gimp-image-delete im)
   )
)

