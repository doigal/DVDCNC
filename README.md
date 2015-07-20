# DVDCNC
The Really Small and Really Cheap CNC Machine

How cheap can a CNC machine be?

This is designed entirely as a cheap proof of concept and a way to learn a lot of skills on a practical project with absolute minimal financial outlay. Whilst this is probably over complicated, for me it was a good chance to learn some design, Gcode and laser cutting along the way.

Its important to keep expectations realistic here. This is not going to move a heavy load, its not going to be super accurate and its not going to have a big working area. Its also going to be incredibly cheap, and a lot of the parts will be things that people will have lying around or be able to salvage reasonably easy.

At the moment its designed to be simple, but theres always the options of adding in endstops and changing out the pen holder for one of those laser cutters that people salvage from (yet another!) DVD drive. I don't intend to be attaching a 3D printing head or dremel type tool, as these would be too heavy to move around and/or the working area is far too small.

Parts used:
-	Arduino 
-	Adafruit Mk 1 Motor Shield equivalent	
-	Micro Servo for Z axis 			
-	16x2 LCD 	
-	Limit switches and LEDs for homing (Optional - planned for future use)	
-	Pair of DVD drives (with stepper motors)	
-	2 sheets of A4 3mm plywood for frame & pen mount - not everyone has access to a laser cutter, and thats cool! Feel free to come up with your own designs, for the frame (this was my excuse for learning laser cutting at Nottinghack!) 				
-	Various mounting screws

The code and the plans are uploaded on this Git. Note that the mounting holes worked for my drives, you might need to adjust to suit!

Inspiration for this goes to: https://github.com/adidax/mini_cnc_plotter_firmware & http://www.makerblog.at/2015/02/projekt-mini-cnc-plotter-aus-alten-cddvd-laufwerken/

Code to send to GCODE to the arduino: https://github.com/damellis/gctrl

Inkscape to GCODE extension (requires Inkscape 0.48.5 or earlier, and document to be in PX) https://github.com/martymcguire/inkscape-unicorn
