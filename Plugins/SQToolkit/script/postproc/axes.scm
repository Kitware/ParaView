; Adds Axes to a plot
;
; Todo: 
;       1. make the ylabel 90degree orientation....DONE
;       2. Create the brush........................DONE
;       3. Make the fonts of the axes offset correcly
;          so that they don't intersect the axes...
;
;       4. 
;
; Temporary function that gets around a limitation of (my)
; gimp that was preventing using the script-fu variety 
; of this code.
(define (sciber-make-brush-rectangular name width height spacing )
  (let* ((img (car (gimp-image-new width height GRAY)))
         (drawable (car (gimp-layer-new img
                                        width height GRAY-IMAGE
                                        "MakeBrush" 100 NORMAL-MODE)))

         (filename (string-append gimp-directory
                                  "/brushes/r"
                                  (number->string width)
                                  "x"
                                  (number->string height)
                                  ".gbr")))
    (gimp-context-push)
    (gimp-image-undo-disable img)
    (gimp-image-add-layer img drawable 0)
    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-fill drawable BACKGROUND-FILL)
    (gimp-rect-select img 0 0 width height CHANNEL-OP-REPLACE FALSE 0)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill drawable BACKGROUND-FILL)
    (file-gbr-save 1 img drawable filename "" spacing name)
    (gimp-image-delete img)

    (gimp-context-pop)

    (gimp-brushes-refresh)
    (gimp-context-set-brush name)
   );let
);define

;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
; plugin::name= sciber-axes
; plugin::desc= Draws X/Y axis for a picture
;
; plugin::notes= 
;                1. The fontsize for the title is 2x the regular fontsize
;                2. DEBUG as an argument allows the gimp-messages to be 
;                   displayed which can be used for debugging the size
;                   and location of the labels and axes.
;
; plugin::todo=
;                1. Determine the font size by the justified font
;                   selection
;                    
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
( define (sciber-axes inFile outFile x0 x1 y0 y1 nx ny xLabel yLabel title fontSize fontType  )
  (let* (
         (frameIm (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
         (frameDw (car (gimp-image-get-active-layer frameIm)))
         (imwc (car (gimp-image-width frameIm)))
         (imhc (car (gimp-image-height frameIm)))
         (oImageWidth (car (gimp-image-width frameIm)))
         (oImageHeight (car (gimp-image-height frameIm)))
         (fpx (floor (* imhc fontSize )))
         (lm 50 )
         (bm 50 ) 
         (rm 50 ) 
         (tm 50 )
         (th 50 )
         (tw 50 )
         (foo 0 )
         (PI_2 (/ 3.14159 2 ))
         (fpTitle  (* 2 fpx ))
         (yPadding  15 )
         (yAxisPadding 10 )
         (increment 0 )
         (yLabelYPosition 0)
         (yLabelXPosition 0)
         (xLabelYPosition 0)
         (xLabelXPosition 0)
         (yInitial 0 )
         (xInitial 0 )
         (counter 0 )
         (delta 0 )
         (xPosition ) ( yPosition 0 )
         (displayNumber 0 )
         (yLabelObj #f )
         (xLabelObj #f )
         (textWidth 0 )
         (textHeight 0 )
         (nImageX 0 )
         (nImageY 0 )
         (xTitle "" )
         (yTitle "" )
         (yInitial 0 )
         (xInitial 0 )
         (xAxisOffset 0 )
         (yAxisOffset 0 )
         (yAxisYValue 0 )
         (yAxisXValue 0 )
         (xAxisXValue 0 )
         (xAxisYValue 0 )
         (yLabelOffset 0 )
         (maxwidth 0 )
         (tmpObj #f )
         (fs 0 )
         (imcx 0 )
         (imcy 0 )
         (xAxisDelta 0 )
         (yAxisDelta 0 )
         (points 0 )
         )
  

        
;    (set! originalWidth (car (gimp-image-height  frameIm )))
;    (set! originalHeight (car (gimp-image-width  frameIm )))
;    (gimp-message (string-append "Original width: " (number->string oImageWidth )))
;    (gimp-message (string-append "Original Height is : " (number->string oImageHeight )))
    (plug-in-autocrop RUN-NONINTERACTIVE frameIm frameDw)
    ;
    ; Determine the size of the fonts in 
    ; the margin

    (set! increment (/ (- (string->number y1) (string->number y0 )) (- ny 1 )))
    (set! yLabelYPosition (/ imhc 2 ))
    (set! yLabelXPosition (- (floor (/ lm 3 )) (* 2 fpx) ))
    (set! yInitial imhc )
    (set! yPosition (- yInitial ( * ( - ny counter)  delta )))
    (set! yPosition (- yPosition (/ fpx 2 )))
    (set! displayNumber  (+ (* (- ny counter ) increment ) (string->number y0 )))

    (set! yLabelObj (car (gimp-text-fontname frameIm -1 0 yPosition (number->string displayNumber ) 0 TRUE fpx PIXELS fontType )))
    (set! textWidth  (car (gimp-drawable-width yLabelObj )))
    (set! textHeight (car (gimp-drawable-height yLabelObj )))
    (set! yLabelOffset textWidth )

;    (gimp-message (string-append "FIX THIS  New Width " 
;                                 (number->string (car (gimp-drawable-width yLabelObj )))
;                                 " , "
;                                 (number->string (car (gimp-drawable-height yLabelObj )))
;                                 "  fpx="
;                                 (number->string fpx )
;                  )
;    )

    
    (set! fs  0.25) 

;    (set! lm (car (gimp-drawable-width yLabelObj )))
;    (set! lm (ceiling (+ (car (gimp-drawable-width yLabelObj ))) fpx ))
;    (gimp-message (string-append "Original width " 
;                                 (number->string (car (gimp-drawable-width yLabelObj )))
;                                 " fpx " 
;                                 (number->string fpx )
;                   )
;    )

    (set! lm ( + textWidth fpx yPadding yAxisPadding ))

;    (gimp-message (string-append "Lm= "
;                                 (number->string lm )))

    (set! tm (ceiling (* fpTitle (+ fs 1.2))))
    (set! bm (ceiling (* fpx     (+ fs 4 ))))

                                        ; resize
    (gimp-context-set-background '(255 255 255))
    (gimp-context-set-foreground '(  0   0   0))


;    (gimp-message "Removing temp labelobj")
    (gimp-image-remove-layer frameIm yLabelObj )

    ; resize the image
;    (gimp-message (string-append "Resizing to imwc="  
;;                                 (number->string imwc )
;                                 " lm=" 
;                                 (number->string lm )
;                                 " rm=" 
;                                 (number->string rm )
;                                 " imhc="
;                                 (number->string imhc )
;                  )
;    );gimp-message

    (gimp-image-resize frameIm (+ oImageWidth lm rm) (+ imhc tm bm) lm tm)
    (gimp-layer-resize frameDw (+ oImageWidth lm rm) (+ imhc tm bm) lm tm)


    (set! imcx (+ lm (/ imwc 2)))
    (set! imcy (+ tm (/ imhc 2)))
    (set! nImageX (+ lm (/ oImageWidth 2 )))
    (set! nImageY (+ tm (/ oImageHeight 2 )))

;    (gimp-message (string-append "Image Marks: "
;                                 (number->string lm )
;                                 " , " 
;                                 (number->string imwc )
;                                 " , " 
;                                 (number->string rm )
;                  )
;    )
                                 

;    (set! newWidth  (gimp-image-width frameIm ))
;    (set! newHeight (gimp-image-width frameIm ))
    
    
    (set! imwc (car (gimp-image-width  frameIm )))
    (set! imhc (car (gimp-image-height frameIm )))

;    (set! yInitial  (- imhc (+ tm bm) ))
    (set! yInitial ( - imhc bm ))
    (set! xInitial  lm )

    ; A tweaking option
    (set! xAxisDelta 0 )
    (set! xAxisOffset (- (floor (/ fpx 2 )) xAxisDelta ))
    (set! yAxisOffset (floor (/ lm 2 )))

;    (set! delta   (floor (/ (- imwc (+ lm rm )) (- nx  1))))
    (set! delta   (floor (/ (- imwc (+ lm rm )) (- nx  1))))
    (set! increment (/ (- (string->number x1) (string->number x0 )) (- nx 1 )))
    (set! counter 0 )
    (set! xPosition xInitial )
;    (gimp-message (string-append "New width: " (number->string imwc )
;                                 " "
;                                 "Delta: " (number->string delta )
;                                 " "
;                                 "Width: " (number->string (- imwc lm rm ))
;                                 " "
;                                 "Increment :" (number->string increment )
;                                 " "
;                                 "xInitial: " (number->string xInitial )
;                  );string-append
;     );
;    (gimp-message (string-append "yinitial: "
;                                 (number->string yInitial )
;                  )
;    )
    ; 
    ; Setup the Paint brush to draw the small lines
    ; 1.Change paint brush to small 
    ; This is temporary until its possible to use the 
    ; Built in brushes
    (sciber-make-brush-rectangular  "AxisBrush" 2.0 2.0 0.0 )



    ;
    ; XAxis
    ;
    (set! xAxisYValue (+ yInitial xAxisOffset))

    (while (< counter nx )
           (set! xPosition (+ xInitial (* counter delta )))
           (draw-line frameDw xPosition (- imhc bm ) xPosition (+ (- imhc bm ) 5 ) )
           (set! displayNumber  (+ (* counter increment ) (string->number x0 )))

           (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw (- xPosition (/ fpx 2)) xAxisYValue (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))

           ;
           ; Draw the ticks
           (set! counter (+ 1 counter ))
     );while

    ;
    ; Xlabel
    ;
;    (gimp-message (string-append "Half way point: " 
;                                 (number->string (ceiling (/ imwc 2 )))
;                  )                                
;    )

    (set! xLabelYPosition (ceiling (- imhc (+ (/ bm 2 ) (/ fpx 2 )))))
    (set! xLabelXPosition (ceiling (/ imwc 2 )))

    (set! xLabelObj (car (gimp-text-fontname frameIm frameDw xLabelXPosition xLabelYPosition xLabel 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))


    (set! textWidth  (car (gimp-drawable-width xLabelObj )))
    (set! textHeight (car (gimp-drawable-height xLabelObj )))
    ;
    ;Reposition the xlabel
    ; 
    ; Drawable in the middle of the screen

;    (gimp-message (string-append "Using Lm of "
;                                 (number->string lm )))
;    (gimp-message (string-append "Image width: " 
;                                 (number->string imwc )
;                                 "  textWidth: "
;                                 (number->string textWidth )
;                                 )
;     )



    (gimp-layer-set-offsets xLabelObj (+ lm (/ (- oImageWidth textWidth ) 2 ))   xLabelYPosition  )



    (gimp-floating-sel-anchor xLabelObj )


    ;
    ; Yaxis
    ;
    (set! delta   (floor (/ (- imhc (+ tm bm )) (- ny 1 ))))
    (set! increment (/ (- (string->number y1) (string->number y0 )) (- ny 1 )))
    (set! counter ny )
    (set! yInitial (- imhc bm ))
    (set! yPosition yInitial )
    (set! yAxisXValue (floor (- xInitial yAxisOffset )))

    (while (> counter 0 )
           (set! yPosition (- yInitial ( * ( - ny counter)  delta )))
           (draw-line frameDw (- lm 5) yPosition lm yPosition )
           (set! yPosition (- yPosition (/ fpx 2 )))

           (set! displayNumber  (+ (* (- ny counter ) increment ) (string->number y0 )))
           (set! yLabelObj (car (gimp-text-fontname frameIm frameDw yAxisXValue (- yPosition (/ fpx 2 )) (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
           (gimp-layer-set-offsets yLabelObj (+ fpx yPadding ) ( - yPosition (/ fpx 2 )))

           (gimp-floating-sel-anchor yLabelObj )

;           (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw yAxisXValue (- yPosition (/ fpx 2 )) (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))

           (set! counter (- counter 1 ))

     );while


;    (gimp-message (string-append "Final Y is : " (number->string yPosition )))
;           (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw (floor (- xInitial yAxisOffset ))   yPosition (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))

    ;
    ; Ylabel for the Yaxis
    ;
    (set! yLabelYPosition (/ imhc 2 ))
    (set! yLabelXPosition (- (floor (/ lm 3 )) (* 2 fpx) ))
;    (gimp-message (string-append "Putting in label at x= " 
;                                 (number->string yLabelXPosition )
;                                 "  y="
;                                 (number->string yLabelYPosition ) )
;     )


    (set! yLabelObj (car (gimp-text-fontname frameIm frameDw yLabelXPosition yLabelYPosition yLabel 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
    (set! textWidth  (car (gimp-drawable-width yLabelObj )))
    (set! textHeight (car (gimp-drawable-height yLabelObj )))

    ;
    ; Need to position the text to a better location
    ; Positioning seems to be difficult in terms of figuring out the 
    ; size of widths and lengths

    (gimp-drawable-transform-rotate-default yLabelObj (* -1 PI_2 )  1 ( / textWidth 2 ) (/ textHeight 2 ) 0 0)
;    (gimp-layer-set-offsets yLabelObj yLabelOffset (- imhc (+ (/ oImageHeight 2 ) bm ))   xLabelYPosition  )
;    (gimp-layer-set-offsets yLabelObj (- lm fpx ) (+ oImageHeight tm )   xLabelYPosition  )
;    (gimp-layer-set-offsets yLabelObj (- lm fpx ) (+ (/ (- oImageHeight textWidth )  2)  tm )   xLabelYPosition  )



;    (gimp-message (string-append "Values: "
;                  "xposition: " 
;                  (number->string 0)
;                  "Yposition: "
;;                  (number->string (+ (/ (- oImageHeight textWidth )  2)  tm ))
;                  (number->string (+ (/ (- oImageHeight textWidth )  2)  tm ))
;                  "Xlabelposition: "
;                  (number->string xLabelYPosition)
;                  )
;                  )

    
;    (gimp-layer-set-offsets yLabelObj  0 (+ (/ (- oImageHeight textWidth )  2)  tm )   xLabelYPosition  )

    (gimp-layer-set-offsets yLabelObj  0 (+ (/ (- oImageHeight textWidth )  2)  tm )  )



;    (gimp-message (string-append 
;                   "Width of rotated text is " 
;                  (number->string textWidth )
;                  ))
    


    ; Now position the yLabel halfway into the
    
    (gimp-floating-sel-anchor yLabelObj )

;    (sciber-make-brush-rectangular  "AxisBrush" 2.0 2.0 0.0 )


    (draw-line frameDw xInitial (- imhc bm ) (- imwc rm ) (- imhc bm ))
    (set! points (cons-array 4 'double))
    (gimp-palette-set-foreground '(0 0 0 ))
    (draw-line frameDw xInitial yInitial xInitial tm )

    ;
    ; Title
    ;
;    (gimp-message (string-append "AGAIN imcx="
;                                 (number->string imcx )
;    ))                                 
    
    (set! xTitle (- imcx (* (/ (string-length title) 2.0) fpx)))

    (set! yTitle 0 )
    (set! fpTitle  (* 2 fpx ) )

; (gimp-message (string-append "Adding title at: x=" 
;                                 (number->string xTitle ) 
;                                 " y=" 
;                                 (number->string yTitle )
;                                 " and font= " 
;                                 (number->string fpx ))
;    );message
    
    (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw xTitle yTitle title 0 TRUE fpTitle PIXELS "Nimbus Mono L Bold Oblique")))
           

;    (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw xTitle yTitle title 0 TRUE 100 PIXELS "Nimbus Mono L Bold Oblique")))

    (gimp-displays-flush )
    (gimp-file-save RUN-NONINTERACTIVE frameIm frameDw outFile outFile)
    );let*
);define


(script-fu-register "sciber-axes" 
                    "Axes"
                    "Automatically adds axes to an image "
                    "Jimi Damon <jdamon@gmail.com>"
                    "Jimi Damon"
                    "2010-06-08"
                    ""
;inFile outFile x0 x1 y0 y1 nx ny xLabel yLabel title fontSize fontType . DEBUG
                    SF-STRING "InputFile" "SciberExample.png"
                    SF-STRING "Output File" "out.png"
                    SF-STRING "xInitial (x0) as a string" "0"
                    SF-STRING "xFinal   (xF) as a string" "10"
                    SF-STRING "yInitial (y0) as a string" "0"
                    SF-STRING "yFinal   (yF) as a string" "10"
                    SF-VALUE "Number of X ticks (nx)" "5"
                    SF-VALUE "Number of Y ticks (ny)" "5"
                    SF-STRING "Xlabel " "XLabel"
                    SF-STRING "Ylabel " "YLabel"
                    SF-STRING "Tile" "TITLE"
                    SF-VALUE "Font Size for Label as a %" "0.05"
                    SF-STRING "Font Type"  "San"
;                    SF-VALUE  "DEBUG ? default nil" "0" 
                    )
(script-fu-menu-register "sciber-axes" "<Toolbox>/Xtns/Script-Fu/SciberQuest" )


;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
; Helper functions
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

(define (add-message message x y ) 
  (let*(()
        (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw x y message 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
        )
    )
);define


(define (draw-line layer x0 y0 x1 y1 )
  (let* ((points (cons-array 4 'double))
         )
    (aset points 0 x0 )
    (aset points 1 y0 )
    (aset points 2 x1 )
    (aset points 3 y1 )
    (gimp-palette-set-foreground '(0 0 0 ))
    (gimp-pencil layer 4 points )
    );let
);define


(define (ceiling  x ) 
  (+ x (- 1 (fmod x 1)))
);

(define (floor  x ) 
  (- x (fmod x 1))
);


(define (font-pixelsize-from-percent-size image )
  (2 )
);define


;(sciber-axes  "/home/jdamon/TestCase1.png" "/home/jdamon/foo2.png" "2" "10" "0" "10" 5 15  "xlabel" "ylabel" "NEW TITLE" 12 13  0.23 "San" )
