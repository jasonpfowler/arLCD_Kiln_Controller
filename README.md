arLCD_Kiln_Controller
=====================

arLCD Arduino Glass fusing kiln controller project.

To get started you will need the code project file, hopefully you can find it right next to this file. You will need a couple graphics for icons a check mark and something for cancel. I use a couple from www.visualpharm.com (Check.gif & Cancel.gif). While I am at it here are the parts that I used.

Arduino arLCD
http://earthmake.com

Thermocouple
https://www.adafruit.com/products/269

Proto board
http://www.oddwires.com/prototype-shield-for-arduino-bare-board/#PhotoSwipe1397848331451

*Almost forgot you will need a Solid State Relay! My Dad just had them laying around but you will need to find one that meets your project needs. 

Once you have all of the hardware together you will need to drop the graphics in the proper folder according to the arLCD manual. Also in the root of the arLCD usb drive create a folder called “Schedule”. In this folder you are going to create 9 files preset1.txt through 9. These files are how you program your glass melting schedules. 
The schedule is setup for steps with each step having a ramp rate per hour, target temp and finally how long to hold that temp. A full schedule will look like this.

300,900,15,;500,1100,0,;100,1250,30,;9999,1475,10,;Test Schedule 1-

This should be easy enough to understand, sorry for all the comas and semicolons. Maybe I could have made it prettier but I am new. So Step 1 says ramp up at the rate of 300 degrees per hour until 900 and then hold for 15 minutes. Some quick math says that it then should take 3 hours to reach 900 degrees.  

I apologize for the poor nature of this information. I did this weeks ago and I keep slacking on putting a post together. While I don’t have the time for a full step by step I figured at least this may help someone looking to do the same type project. Track me down over at Google+ and I will help the best that I can. 

Head over here for pictures!
https://plus.google.com/+JasonFowler74/posts/H4itKN24AcK
