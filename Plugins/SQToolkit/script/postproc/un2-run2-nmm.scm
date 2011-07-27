
(define 
  (un2-run2-nmm inFile outFile title x0 x1 y0 y1)
    (let* 
      (
      (imwc  0)    ; cropped image width
      (imhc  0)    ; cropped image ht
      (imwf  0)    ; final image width 
      (imhf  0)    ; final image ht
      (imcx  0)    ; cropped image horizontal center
      (imcy  0)    ; cropped image vertical center
      (fpx   0)    ; font size px
      (lm    0)    ; left margin
      (rm    0)    ; right margin
      (tm    0)    ; top margin
      (bm    0)    ; bottom margin
      (fs    0)    ; font spacer
      (tl    0)    ; title layer
      (tw    0)    ; title width
      (th    0)    ; title height
      
      ; load the original
      (frameIm (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
      (frameDw (car (gimp-image-get-active-layer frameIm)))

      ; load the axis labels
      (xArrFn "/home/burlen/ext/vis/AGU-09/poster/gimp/vx-arrow.png")
      (xArrL (car (gimp-file-load-layer RUN-NONINTERACTIVE frameIm xArrFn))) 
      (xArrW (car (gimp-drawable-width  xArrL)))
      (xArrH (car (gimp-drawable-height xArrL)))        

      (yArrFn "/home/burlen/ext/vis/AGU-09/poster/gimp/hy-arrow.png")
      (yArrL (car (gimp-file-load-layer RUN-NONINTERACTIVE frameIm yArrFn)))
      (yArrW (car (gimp-drawable-width  yArrL)))
      (yArrH (car (gimp-drawable-height yArrL))) 
      )
      ; auto-crop
      (plug-in-autocrop RUN-NONINTERACTIVE frameIm frameDw)
      (set! imwc (car (gimp-drawable-width frameDw)))
      (set! imhc (car (gimp-drawable-height frameDw)))

      (set! fpx 25)
      (set! fs  0.25) 
      (set! lm (* fpx (+ fs 2.0)))
      (set! tm (* fpx (+ fs 1.2)))
      (set! bm (* fpx (+ fs 1.2)))

      ; resize
      (gimp-context-set-background '(  0   0   0))
      (gimp-context-set-foreground '(255 255 255))
      (gimp-image-resize frameIm (+ imwc lm rm) (+ imhc tm bm) lm tm)
      (gimp-layer-resize frameDw (+ imwc lm rm) (+ imhc tm bm) lm tm)
       
      (set! imcx (+ lm (/ imwc 2)))
      (set! imcy (+ tm (/ imhc 2)))

      ; TITLE
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw (- imcx (* (/ (string-length title) 2.0) fpx)) (+ tm imhc fs) title 0 TRUE 30 PIXELS "Nimbus Mono L Bold Oblique")))
      
      ; AXIS-Ticks
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw (- lm (* fpx 2)) tm                    x0 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw (- lm (* fpx 2)) (- (+ tm imhc) fpx)   x1 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw lm                          (- tm fpx) y0 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw (- (+ lm imwc) (* fpx 2.0)) (- tm fpx) y1 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))

      ; AXIS LABELS
      (gimp-image-add-layer frameIm xArrL -1) 
      (gimp-layer-set-offsets xArrL (- (/ lm 2.0) (/ xArrW 2.0)) (- (+ tm (/ imhc 2.0)) (/ xArrH 2.0)))
      (gimp-image-add-layer frameIm yArrL -1)
      (gimp-layer-set-offsets yArrL (- (+ lm (/ imwc 2.0)) (/ yArrW 2.0)) (- (/ tm 2.0) (/ yArrH 2.0)))

      ; SAVE
      (gimp-image-merge-visible-layers frameIm  CLIP-TO-IMAGE)
      (set! frameDw (car (gimp-image-get-active-layer frameIm)))
      (gimp-file-save RUN-NONINTERACTIVE frameIm frameDw outFile outFile)
      (gimp-image-delete frameIm)

      (display (string-append inFile "->" outFile))
   )
)

