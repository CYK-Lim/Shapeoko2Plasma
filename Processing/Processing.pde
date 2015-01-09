/**
 * Simple Read
 * 
 * Read data from the serial port and change the color of a rectangle
 * when a switch connected to a Wiring or Arduino board is pressed and released.
 * This example works with the Wiring / Arduino program that follows below.
 */


import processing.serial.*;

PFont f;
Serial myPort;  // Create object from Serial class
String val;      // Data received from the serial port
String[] vals;
PrintWriter output;
boolean valRead = false;

void stop()
{
  output.flush();  // Writes the remaining data to the file
  output.close();  // Finishes the file  
}

void setup() 
{
  size(300, 100);
  // I know that the first port in the serial list on my mac
  // is always my  FTDI adaptor, so I open Serial.list()[0].
  // On Windows machines, this generally opens COM1.
  // Open whatever port is the one you're using.
  String portName = "/dev/ttyUSB0"; //Serial.list()[9];
  myPort = new Serial(this, portName, 115200);
  
  output = createWriter("thcTrbl.txt");
  
  output.println("thcON  vSP  vACT  thcOffset");
  

  f = createFont("Arial",16,true); // STEP 3 Create Font
  boolean first = true;

}

void draw()
{

 
  
  byte[] inbuffer = new byte[40];
  

  
  if ( myPort.available() > 0) 
  {  // If data is available,
    int numbytes = myPort.readBytes(inbuffer);
    if(numbytes >28 && ( inbuffer[0] == '0' || inbuffer[0] == '1') )
    {
      
      val = new String(inbuffer);
      output.print(val);
    //val = myPort.readStringUntil('\n');         // read it and store it in val
    //vals = split(val, ' ');
    //if (vals[1] != null)
        //valRead = true;

  //println(val);

     background(255);
    textFont(f,16);                 // STEP 4 Specify font to be used
  fill(0);                        // STEP 5 Specify font color 
     text("thcON  vSP  vACT  thcOffset", 5,20);
      text(val,10,40); 
          //text("thcON  vSP  vACT  thcOffset", 5,20);
          }
  }

}

