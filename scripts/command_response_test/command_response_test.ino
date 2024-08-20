/*
command_response_test

This Arduino sketch serves as an example to help students understand command data
parsing, response construction, and packetization. It will take any command received
over the serial connection, split out the command ID and data values, and parse the
data value as a floating point number. It will then respond to the command by echoing
back the data value in the correct response format.

*/

/* Declarations and Constants */
// Declare some string variables
String packet;
String cmdID;
String dataValue;

bool DEBUG = false; // If not debugging, set this to false to suppress debug messages
char FRAMESTART = '[';
char FRAMEEND = ']';
int TIMEOUT = 250; // Serial timeout in milliseconds
double DIFFERENCE = 1.2;
int MAX_PACKET_LENGTH = 143; // equivalent to 16 8-byte commands of format "xx-#####", with 15 delimiting commas between them

/* Create a debug message */
void debugMessage(String msg) {
  // If DEBUG is true, send some debug messages over the serial port
  if (DEBUG)
    {
      Serial.println(msg);
    }
}

/* Serial receive function */
String receiveSerial() {
  // Declare variables, making sure to set types explicitly
  String frontmatter = "";
  String msg = "";
  char front_char = 0;
  char msg_char = 0;
  unsigned long start_time = 0; // This MUST be usigned long or else timeouts will be wonky

  // If there's anything available in the serial buffer, get it
  if (Serial.available()) {

    // Read characters until the FRAMESTART character is found, dumping them all into frontmatter
    // frontmatter is only stored for debugging purposes
    start_time = millis();
    while (millis() < start_time + TIMEOUT) {
      if (Serial.available()) {
        front_char = Serial.read();
        if (front_char == FRAMESTART) {
          msg += front_char;
          break;
        } else {
          frontmatter += front_char;
        }
      }
    }
    if (frontmatter.length() > 0) {
      debugMessage("Prior to FRAMESTART, received: " + frontmatter);
    }

    // Read more serial characters into msg until the FRAMEEND character is reached
    while (millis() < start_time + TIMEOUT) {
      if (Serial.available()) {
        msg_char = Serial.read();

        // If a second frame start character is received, then restart the message
        if (msg_char == FRAMESTART) {
          debugMessage("A new framestart character was received, dumping: " + msg);
          msg = "";
        }

        // If any other character is received, then add it to the message
        // Break if FRAMEEND character is received.
        msg += msg_char;
        if (msg_char == FRAMEEND) {
          break;
        }
      }
    }

    // Check if the message timed out
    if (msg.length() < 1) {
      debugMessage("Timed out without receiving any message.");
    } else if (msg_char != FRAMEEND) {
      debugMessage("Timed out while receiving a message.");
    } else {
      debugMessage("Depacketizing received message:" + msg);
      return depacketize(msg);
    }
  }

  // If a correctly packed string isn't found, flush the serial port and return an empty string
  return "";
}

/* Remove packet framing information */
String depacketize(String msg) {
  // If the message is correctly framed (packetized), trim framing characters and return it
  if (msg.length() > 1 && msg[0] == FRAMESTART) {
    if (msg[msg.length()-1] == FRAMEEND) {
      return msg.substring(1, msg.length()-1);
    }
  }
  // If anything doesn't match the expected format, return an empty string
  debugMessage("Missing valid packet framing characters, instead detected these: " + String(msg[0]) + ',' + String(msg[msg.length()-1]));
  return "";
}

/* Add packet framing information */
String packetize(String msg) {
  return FRAMESTART + msg + FRAMEEND;
}

/* Handle the received commands (in this case just sending back the command and the data + DIFFERENCE)*/
String parseCmd(String cmdString) {
  String cmdID = "";
  double data = 0;
  debugMessage("Parsed command: " + cmdString);

  // Get the command ID
  cmdID = cmdString.substring(0,min(2, cmdString.length()));

  // Get the data, if the command is long enough to contain it
  if (cmdString.length() >= 4) {
    data = cmdString.substring(3).toDouble();
  }

  // Debug print messages
  debugMessage("Command ID is: " + cmdID);
  debugMessage("The parsed data string is:" + String(data));

  /*
  Here you would insert code to do something with the received cmdID and data
  */

  // Create a string response
  return cmdID + '-' + String(data + DIFFERENCE);

}



/* Setup Function */
void setup() {
  // initialize digital pin LED_BUILTIN as output in case it's needed
  pinMode(LED_BUILTIN, OUTPUT);

  // Set serial parameters
  Serial.begin(9600);
  Serial.setTimeout(TIMEOUT);
  Serial.flush();

  debugMessage("Arduino is ready");
}

/* Main Loop */
void loop() {

  // Get a string from the serial port
  packet = receiveSerial();
  if (packet.length() > 0) {
    debugMessage("Received good packet: " + packet);
  }

  // Parse a correctly formatted command string
  if (packet.length() >= 2 && packet.length() <= MAX_PACKET_LENGTH)
  {
    // Loop through the received string, splitting any commands by the ',' character and parsing each
    int cmdStartIndex = 0;
    String responseString = String();
    for (int ct = 0; ct < packet.length(); ct++) {
      if (packet[ct] == ',') {
        responseString += parseCmd(packet.substring(cmdStartIndex, ct)) + ',';
        cmdStartIndex = ct + 1;
      }
    }
    // For the last command (or if there were no ',' characters), parse it as well
    responseString += parseCmd(packet.substring(cmdStartIndex));
    debugMessage("Response String is: " + responseString);

    // Packetize and send the response string
    Serial.print(packetize(responseString));
    debugMessage("");
  }
}