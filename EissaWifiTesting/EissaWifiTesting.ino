/*
Connect the ESP8266 as follows (note that rx and tx are reversed):
* RX is digital pin 10 (connect to TX of other device)
* TX is digital pin 11 (connect to RX of other device)
 
 
Basic AT commands to operate ESP-07

AT
AT+RST
AT+CWMODE=1
AT+CWLAP
AT+CWJAP="network name","password"
AT+CIFSR
AT+CIPSTART="TCP","www.google.com",80"

AT+CIPSTART="TCP","servers_IP_address",27015
AT+CIPSEND
> Hello World?
AT+CIPCLOSE

Note1: Windows uses \r\n to signify the enter key was pressed, while Linux and Unix use \n to signify that the enter key was pressed.
Note2: Its important to match the mux setting with the cipstart and cipsend commands. If using mux 0, do not send a connection id to them but you must send one for mux=1
 */

//#include "VSPDE.h"
#include <SoftwareSerial.h>


#define TIMEOUT     10000          // mS
#define CONTINUE    false
#define HALT        true
//#define ECHO_COMMANDS
#define NetID       "Google Starbucks"	// "DrPepper"
#define Pass        ""					// "grandizer77"
#define ServerIP    "192.168.1.102"


// TX on the ESP goes into port 10 on UNO
// RX on the ESP goes into port 11 on UNO
SoftwareSerial mySerial(10, 11); // RX, TX

// Print error message and loop stop.
void errorHalt(String msg)
{
  Serial.println(msg);
  Serial.println("HALT");
  while(true){};
}

// Read characters from WiFi module and echo to serial until keyword occurs or timeout.
boolean echoFind(String keyword)
{
  byte current_char   = 0;
  byte keyword_length = keyword.length();
  
  // Fail if the target string has not been sent by deadline.
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline)
  {
    if (mySerial.available())
    {
      char ch = mySerial.read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println();
          return true;
        }
    }
  }
  return false;  // Timed out
}

// Read and echo all available module output.
// (Used when we're indifferent to "OK" vs. "no change" responses or to get around firmware bugs.)
void echoFlush()
  {while(mySerial.available()) Serial.write(mySerial.read());}
  
// Echo module output until 3 newlines encountered.
// (Used when we're indifferent to "OK" vs. "no change" responses.)
void echoSkip()
{
  echoFind("\n");        // Search for nl at end of command echo
  echoFind("\n");        // Search for 2nd nl at end of response.
  echoFind("\n");        // Search for 3rd nl at end of blank line.
}

// Send a command to the module and wait for acknowledgement string
// (or flush module output if no ack specified).
// Echoes all data received to the serial monitor.
boolean echoCommand(String cmd, String ack, boolean halt_on_fail)
{
	
	// Execute command on ESP
	mySerial.println(cmd);

	#ifdef ECHO_COMMANDS
	Serial.print("Serial: "); Serial.println(cmd);
	#endif
  
	// If no ack response specified, skip all available module output.
	if (ack == "")
		echoSkip();
	else
		// Otherwise wait for ack.
		if (!echoFind(ack))          // timed out waiting for ack string 
			if (halt_on_fail)
				errorHalt(cmd+" failed");// Critical failure halt.
			else
				return false;            // Let the caller handle it.
	return true;                   // ack blank or ack found
}

// Connect to the specified wireless network.
boolean connectWiFi()
{
	// List all Access Points
	// echoCommand("AT+CWLAP", "OK", CONTINUE);

	// Create the join access point command with SSID, PWD
	String	cmd = "AT+CWJAP=\""; 
			cmd += NetID; 
			cmd += "\",\""; 
			cmd += Pass; 
			cmd += "\"";
  
	if (echoCommand(cmd, "OK", HALT)) {
		Serial.println("Connected to WiFi.");

		// Get local IP Address
		// Serial.print("IP: ");
		echoCommand("AT+CIFSR", "", CONTINUE);

		// Get connection information
		Serial.println("****Getting ESP Connection Status****");
		echoCommand("AT+CIPSTATUS", "OK", CONTINUE);
		Serial.println("****End****");

		return true;
	}
	else {
		Serial.println("Connection to WiFi failed.");
		return false;
	}
}

// Connect to socket
boolean connectPort()
{
  String cmd = "AT+CIPSTART=1,\"TCP\",\"";  // for mux=1
  //String cmd = "AT+CIPSTART=\"TCP\",\"";  // for mux=0
  cmd +=  ServerIP; 
  cmd += "\", 21";                           // select a port

  if (echoCommand(cmd, "", HALT)) { // this is server specific
    Serial.println("Socket connected.");
    return true;
  }
  else {
    Serial.println("Socket connection failed.");
    return false;
  }
}

// FTP Login
boolean ftpLogin()
{
  String FTPUser = "USER mark";
  String FTPPass = "PASS donaldduck";
   
  String cmd = "AT+CIPSEND=1,"+String(FTPUser.length()+2);  // +2 because we are also sending an automatic \r\n
    
  if (echoCommand(cmd, ">", CONTINUE)) {
    //Serial.println("Username accepted.");
    echoCommand(FTPUser, "OK", CONTINUE);
  }
  else {
    Serial.println("Username failed.");
    return false;
  }

  cmd = "AT+CIPSEND=1,"+String(FTPPass.length()+2);
  if (echoCommand(cmd, ">", CONTINUE)) {
    //Serial.println("Password accepted.");
    echoCommand(FTPPass, "OK", CONTINUE);
    return true;
  }
  else {
    Serial.println("Password failed.");
    return false;
  }
}


void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);      // usb port com
  mySerial.begin(9600);      // softwareserial port for ESP8266
  
  mySerial.setTimeout(TIMEOUT);
  Serial.println("Starting Arduino..."); 
  
  delay(2000);

  echoCommand("AT+RST", "ready", HALT);    // Reset & test if the module is ready  
  Serial.println("Module is ready.");
  echoCommand("AT+GMR", "OK", CONTINUE);   // Retrieves the firmware ID (version number) of the module. 
  echoCommand("AT+CWMODE?","OK", CONTINUE);// Get module access mode. 
  
  // echoCommand("AT+CWLAP", "OK", CONTINUE);
  
  echoCommand("AT+CWMODE=1", "", HALT);    // 1 client, 2 AP, 3 client and AP. Note, AP may not have dhcp
  echoCommand("AT+CIPMUX=1", "", HALT);    // Use many connection

  // Connect to WIFI
  boolean connection_established = false;
  for(int i=0;i<5;i++)
  {
    if(connectWiFi())
    {
      connection_established = true;
      break;
    }
  }
  if (!connection_established) errorHalt("Connection failed");
  
  delay(3000);

  //echoCommand("AT+CWSAP=?", "OK", CONTINUE); // test
  //echoCommand("AT+CIFSR", "", HALT);         // return IP address
  
  
  // Connect to Socket
  boolean socket_established = false;
  for(int i=0;i<5;i++) {
    if(connectPort()) {
      socket_established = true;

      break;
    } else { delay(1000); }
  }
  if (!socket_established) errorHalt("Socket connection failed");

  if (socket_established) {
	  String cmd = "GET /status.html HTTP/1.0\r\n";
	  cmd += "Host: www.google.com\r\n\r\n";
	  Serial.print("AT+CIPSEND=0,110");
	  // echoCommand("AT+CIPSEND=0,110", "OK", CONTINUE);
	  //Look for the > prompt from the esp8266
	  if (Serial.find(">")) {
		  echoCommand(cmd, "OK", CONTINUE);
	  }
	  else
		  Serial.println("Failed to GET Data");
  }
}

void loop()
{
	//Serial.println("Looping");
	
    //boolean ftp_login = false;
    //for(int i=0;i<5;i++) {
    //  if(ftpLogin()) {
    //    ftp_login = true;
    //    break;
    //  } else { delay(1000); }
    //}
    //if (!ftp_login) errorHalt("Socket connection failed");
  
  
  
    //String command = "SYST"; 
    //echoCommand("AT+CIPSEND=1,"+String(command.length()+2), "OK", CONTINUE);
    //echoCommand(command, "", HALT);
    //delay(2000);
    //
    //command = "PWD"; 
    //echoCommand("AT+CIPSEND=1,"+String(command.length()+2), "OK", CONTINUE);
    //echoCommand(command, "", HALT);
    //delay(2000);
    //
    //command = "PASV"; 
    //echoCommand("AT+CIPSEND=1,"+String(command.length()+2), "OK", CONTINUE);
    //echoCommand(command, "", HALT);
    //delay(2000);
    //
    //command = "LIST"; 
    //echoCommand("AT+CIPSEND=1,"+String(command.length()+2), "OK", CONTINUE);
    //echoCommand(command, "", HALT);
  
    // Loop forever echoing data received from destination server.
    //while(true) {
    //  while (mySerial.available()) {
    //    Serial.write(mySerial.read());
    //  }
    //}
}
