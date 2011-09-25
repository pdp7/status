
#include "WiFly.h"

#define DEBUG_LEVEL 0

#include "Debug.h"

boolean WiFlyDevice::findInResponse(const char *toMatch,
				    unsigned int timeOut = 0) {

// TODO: if entered with return false too many times, just reset?
  /*

   */

  // TODO: Change 'sendCommand' to use 'findInResponse' and have timeouts,
  //       and then use 'sendCommand' in routines that call 'findInResponse'?

  int byteRead;

  unsigned int timeOutTarget; // in milliseconds


  Serial.println("Entered findInResponse:");
  Serial.print("toMatch:");
  Serial.println(toMatch);
  Serial.print("timeOut:");
  Serial.println(timeOut);
  Serial.println("Loop:");

  for (unsigned int offset = 0; offset < strlen(toMatch); offset++) {


    // Reset after successful character read
    timeOutTarget = millis() + timeOut; // Doesn't handle timer wrapping

    while (!uart.available()) {
Serial.print("!uart.available() ==> timeOutTarget: ");
Serial.println(timeOutTarget);
// Wait, with optional time out.
// TODO: print millis() ?
      if (timeOut > 0) {
        if (millis() > timeOutTarget) {
          return false;
	// TODO: if entered with return false too many times, just reset?
        }
      }
      delay(1); // This seems to improve reliability slightly
    }

    // We read this separately from the conditional statement so we can
    // log the character read when debugging.
    byteRead = uart.read();
    //Serial.print((char)byteRead);
    //Serial.print("_");
    delay(1); // Removing logging may affect timing slightly

Serial.print("toMatch["); Serial.print(offset); Serial.print("]="); Serial.println((char)toMatch[offset]);
Serial.println(timeOutTarget);
Serial.print("timeOutTarget: ");
Serial.println(timeOutTarget);
Serial.print("byteRead: ");
Serial.println((char)byteRead);
    if (byteRead != toMatch[offset]) {
     offset = 0;
      // Ignore character read if it's not a match for the start of the string
      if (byteRead != toMatch[offset]) {
        offset = -1;
      }
      continue;
    }

  }

  return true;
}



boolean WiFlyDevice::responseMatched(const char *toMatch) {
 /*
  */
  boolean matchFound = true;

  DEBUG_LOG(3, "Entered responseMatched");

  for (unsigned int offset = 0; offset < strlen(toMatch); offset++) {
    while (!uart.available()) {
      // Wait -- no timeout
    }
    if (uart.read() != toMatch[offset]) {
      matchFound = false;
      break;
    }
  }
  return matchFound;
}



#define COMMAND_MODE_ENTER_RETRY_ATTEMPTS 5

#define COMMAND_MODE_GUARD_TIME 250 // in milliseconds

boolean WiFlyDevice::enterCommandMode(boolean isAfterBoot) {
  /*

   */

  DEBUG_LOG(1, "Entered enterCommandMode");

  // Note: We used to first try to exit command mode in case we were
  //       already in it. Doing this actually seems to be less
  //       reliable so instead we now just ignore the errors from
  //       sending the "$$$" in command mode.

  for (int retryCount = 0;
       retryCount < COMMAND_MODE_ENTER_RETRY_ATTEMPTS;
       retryCount++) {

    // At first I tried automatically performing the
    // wait-send-wait-send-send process twice before checking if it
    // succeeded. But I removed the automatic retransmission even
    // though it makes things  marginally less reliable because it speeds
    // up the (hopefully) more common case of it working after one
    // transmission. We also now have automatic-retries for the whole
    // process now so it's less important anyway.

    if (isAfterBoot) {
      delay(1000); // This delay is so characters aren't missed after a reboot.
    }

    delay(COMMAND_MODE_GUARD_TIME);

    uart.print("$$$");

    delay(COMMAND_MODE_GUARD_TIME);

    // We could already be in command mode or not.
    // We could also have a half entered command.
    // If we have a half entered command the "$$$" we've just added
    // could succeed or it could trigger an error--there's a small
    // chance it could also screw something up (by being a valid
    // argument) but hopefully it's not a general issue.  Sending
    // these two newlines is intended to clear any partial commands if
    // we're in command mode and in either case trigger the display of
    // the version prompt (not that we actually check for it at the moment
    // (anymore)).

    // TODO: Determine if we need less boilerplate here.

    uart.println();
    uart.println();

    // TODO: Add flush with timeout here?

    // This is used to determine whether command mode has been entered
    // successfully.
    // TODO: Find alternate approach or only use this method after a (re)boot?
    uart.println("ver");

    if (findInResponse("\r\nWiFly Ver", 1000)) {
      // TODO: Flush or leave remainder of output?
      return true;
    }
  }
  return false;
}



void WiFlyDevice::skipRemainderOfResponse() {
  /*
   */

  DEBUG_LOG(3, "Entered skipRemainderOfResponse");

    while (!(uart.available() && (uart.read() == '\n'))) {
      // Skip remainder of response
    }
}


void WiFlyDevice::waitForResponse(const char *toMatch) {
  /*
   */

   // Note: Never exits if the correct response is never found
   while(!responseMatched(toMatch)) {
     skipRemainderOfResponse();
   }
}



WiFlyDevice::WiFlyDevice(SpiUartDevice& theUart) : uart (theUart) {
  /*

    Note: Supplied UART should/need not have been initialised first.

   */
  // The WiFly requires the server port to be set between the `reboot`
  // and `join` commands so we go for a "useful" default first.
  serverPort = DEFAULT_SERVER_PORT;
  serverConnectionActive = false;
}

// TODO: Create a constructor that allows a SpiUartDevice (or better a "Stream") to be supplied
//       and/or allow the select pin to be supplied.


void WiFlyDevice::begin() {
  /*
   */

  DEBUG_LOG(1, "Entered WiFlyDevice::begin()");

  Serial.println("uart.begin");
  uart.begin();
  Serial.println("reboot");
  reboot(); // Reboot to get device into known state
  Serial.println("requireFlowControl");
  requireFlowControl();
  Serial.println("setConfiguration");
  setConfiguration();
}

// TODO: Create a `begin()` that allows IP etc to be supplied.



#define SOFTWARE_REBOOT_RETRY_ATTEMPTS 5

boolean WiFlyDevice::softwareReboot(boolean isAfterBoot = true) {
  /*

   */

  DEBUG_LOG(1, "Entered softwareReboot");

  for (int retryCount = 0;
       retryCount < SOFTWARE_REBOOT_RETRY_ATTEMPTS;
       retryCount++) {

    // TODO: Have the post-boot delay here rather than in enterCommandMode()?

    if (!enterCommandMode(isAfterBoot)) {
      return false; // If the included retries have failed we give up
    }

    uart.println("reboot");

    // For some reason the full "*Reboot*" message doesn't always
    // seem to be received so we look for the later "*READY*" message instead.

    // TODO: Extract information from boot? e.g. version and MAC address

    if (findInResponse("*READY*", 2000)) {
      return true;
    }
  }

  return false;
}

boolean WiFlyDevice::hardwareReboot() {
  /*
   */
  uart.ioSetDirection(0b00000010);
  Serial.println(">>> uart.ioSetDirection(0b00000010)");
  uart.ioSetState(0b00000000);
  Serial.println(">>> uart.ioSetState(0b00000000);");
  delay(1);
  uart.ioSetState(0b00000010);
  Serial.println(">>> uart.ioSetState(0b00000010);");

  return findInResponse("*READY*", 2000);
}


#if USE_HARDWARE_RESET
#define REBOOT hardwareReboot
#else
#define REBOOT softwareReboot
#endif

void WiFlyDevice::reboot() {
  /*
   */

  DEBUG_LOG(1, "Entered reboot");
  Serial.println(">>> Entered reboot");

  #if USE_HARDWARE_RESET
  Serial.println(">>> hardwareReboot");
  #else
  Serial.println(">>> softwareReboot");
  #endif

  if (!REBOOT()) {
    Serial.println(">>> Failed to reboot. Halt");
    DEBUG_LOG(1, "Failed to reboot. Halting.");
    Serial.println("reboot> OFF");  
    digitalWrite(9, LOW);
    delay(3000);
    Serial.println("reboot> ON");
    digitalWrite(9, HIGH);
    delay(5000);
    Serial.println("reboot> POWERED UP");
  //  while (1) {}; // Hang. TODO: Handle differently?
  }
}


boolean WiFlyDevice::sendCommand(const char *command,
                                 boolean isMultipartCommand = false,
                                 const char *expectedResponse = "AOK") {
  /*
   */
  uart.print(command);

  if (!isMultipartCommand) {
    uart.flush();
    uart.println();

    // TODO: Handle other responses
    //       (e.g. autoconnect message before it's turned off,
    //        DHCP messages, and/or ERR etc)
    waitForResponse(expectedResponse);
  }

  return true;
}


void WiFlyDevice::requireFlowControl() {
  /*


    Note: If flow control has been set but not saved then this
          function won't handle it correctly.

    Note: Any other configuration changes made since the last
          reboot will also be saved by this function so this
	  function should ideally be called immediately after a
	  reboot.

   */

  DEBUG_LOG(1, "Entered requireFlowControl");

  enterCommandMode();

  // TODO: Reboot here to ensure we get an accurate response and
  //       don't unintentionally save a configuration we don't intend?

  sendCommand("get uart", false, "Flow=0x");

  while (!uart.available()) {
    // Wait to ensure we have the full response
  }

  char flowControlState = uart.read();

  uart.flush();

  if (flowControlState == '1') {
    return;
  }

  // Enable flow control
  sendCommand("set uart flow 1");

  sendCommand("save", false, "Storing in config");

  // Without this (or some delay--but this seemed more useful/reliable)
  // the reboot will fail because we seem to lose the response from the
  // WiFly and end up with something like:
  //     "*ReboWiFly Ver 2.18"
  // instead of the correct:
  //     "*Reboot*WiFly Ver 2.18"
  // TODO: Solve the underlying problem
  sendCommand("get uart", false, "Flow=0x1");

  reboot();
}

void WiFlyDevice::setConfiguration() {
  /*
   */
  enterCommandMode();

  // TODO: Handle configuration better
  // Turn off auto-connect
  sendCommand("set wlan join 0");

  // TODO: Turn off server functionality until needed
  //       with "set ip protocol <something>"

  // Set server port
  sendCommand("set ip localport ", true);
  // TODO: Handle numeric arguments correctly.
  uart.print(serverPort);
  sendCommand("");

  // Turn off remote connect message
  sendCommand("set comm remote 0");

  // Setup NTP server
  sendCommand("set time address 64.90.182.55");
  sendCommand("set time port 123");
  sendCommand("set t e 60");

  // Turn off status messages
  // sendCommand("set sys printlvl 0");

  // TODO: Change baud rate and then re-connect?

  // Turn off RX data echo
  // TODO: Should really treat as bitmask
  // sendCommand("set uart mode 0");
}


boolean WiFlyDevice::join(const char *ssid) {
  /*
   */
  // TODO: Handle other authentication methods
  // TODO: Handle escaping spaces/$ in SSID
  // TODO: Allow for timeout?

  // TODO: Do we want to set the passphrase/key to empty when they're
  //       not required? (Probably not necessary as I think module
  //       ignores them when they're not required.)

  // Set the SSID and configure the auto join.
  sendCommand("set wlan ssid ", true);
  sendCommand(ssid, false);
  sendCommand("set wlan join 1", false);

  sendCommand("join ", true);
  // TODO: Actually detect failure to associate
  // TODO: Handle connecting to Adhoc device
  if (sendCommand(ssid, false, "Associated!")) {
    // TODO: Extract information from complete response?
    // TODO: Change this to still work when server mode not active
    waitForResponse("Listen on ");
    skipRemainderOfResponse();
    return true;
  }
  return false;
}


boolean WiFlyDevice::join(const char *ssid, const char *passphrase,
			  boolean isWPA) {
  /*
   */
  // TODO: Handle escaping spaces/$ in passphrase and SSID

  // TODO: Do this better...
  sendCommand("set wlan ", true);

  if (isWPA) {
    sendCommand("passphrase ", true);
  } else {
    sendCommand("key ", true);
  }

  sendCommand(passphrase);

  return join(ssid);
}

#define UPTIME_SIZE 11 // store at least a year's uptime in seconds....

long WiFlyDevice::getUpTime(){
	/*
		Returns the time (in seconds) since last powerup or reboot.
	*/
	char newChar;
	byte offset = 0;

	static char upTime[UPTIME_SIZE] = "";

	enterCommandMode();

	sendCommand("show time", false, "UpTime=");
	delay(25);

	while(offset < UPTIME_SIZE){
		newChar = uart.read();

		if (newChar == ' ') {
		    upTime[offset] = '\x00';
		    break;
		} else if (newChar != -1) {
		    upTime[offset] = newChar;
		    offset++;
    	}
	}

	// This should skip the remainder of the output.
	// TODO: Handle this better?
	waitForResponse("<");
	while (uart.read() != ' ') {
		// Skip the prompt
	}

	// For some reason the "sendCommand" approach leaves the system
	// in a state where it misses the first/next connection so for
	// now we don't check the response.
	// TODO: Fix this
	uart.println("exit");
	//sendCommand("exit", false, "EXIT");

	return atol(upTime);
}

int WiFlyDevice::getSignalStrength(){
	/*
		Returns the signal strength of the associated ssid, if not associated 0 will be returned.
	*/
	char newChar;

	byte offset = 0;
	byte valueLength = 6; // max signal is -99 to 99 but we have ( and ) to deal with, this is a safe number
	static char signalStrenghCharArray[4] = "";

	enterCommandMode();

	sendCommand("show rssi", false, "RSSI=");
	delay(25); // required to allow the module to catch up..otherwise you'll get the funny y char

	//copy the signal value from the response
	while(offset < valueLength){
		newChar = uart.read();

		if(newChar == ')'){
			signalStrenghCharArray[offset] = '\x00';
			break;
		}

		if(newChar != '('){
			signalStrenghCharArray[offset] = newChar;
			offset++;
		}
	}

	// This should skip the remainder of the output.
    // TODO: Handle this better?
    waitForResponse("<");
    while (uart.read() != ' ') {
  	// Skip the prompt
    }

    // For some reason the "sendCommand" approach leaves the system
    // in a state where it misses the first/next connection so for
    // now we don't check the response.
    // TODO: Fix this
    uart.println("exit");
    //sendCommand("exit", false, "EXIT");

	return atoi(signalStrenghCharArray);
}


#define TIME_SIZE 11 // 1311006129

long WiFlyDevice::getTime(){

	/*
		Returns the time based on the NTP settings and time zone.
	*/

	char newChar;
	byte offset = 0;
	String timeStr;
	char buffer[TIME_SIZE];

	enterCommandMode();

	//sendCommand("time"); // force update if it's not already updated with NTP server
	sendCommand("show t t", false, "RTC=");

	// copy the time form the response into our buffer
	while (offset < TIME_SIZE) {
	    newChar = uart.read();

	   if (newChar != -1) {
	      timeStr += newChar;
	      offset++;
	    }
  }

  // This should skip the remainder of the output.
  // TODO: Handle this better?
  waitForResponse("<");
  while (uart.read() != ' ') {
    // Skip the prompt
  }

  // For some reason the "sendCommand" approach leaves the system
  // in a state where it misses the first/next connection so for
  // now we don't check the response.
  // TODO: Fix this
  uart.println("exit");
  //sendCommand("exit", false, "EXIT");


  // gotta figure out how to convert the RTC value to a unix date/time stamp.
  timeStr.toCharArray(buffer,TIME_SIZE);

  return strtol(buffer, NULL, 0);
}


#define IP_ADDRESS_BUFFER_SIZE 16 // "255.255.255.255\0"

const char * WiFlyDevice::ip() {
  /*

    The return value is intended to be dropped directly
    into calls to 'print' or 'println' style methods.

   */
  static char ip[IP_ADDRESS_BUFFER_SIZE] = "";

  // TODO: Ensure we're not in a connection?

  enterCommandMode();

  // Version 2.19 of the WiFly firmware has a "get ip a" command but
  // we can't use it because we want to work with 2.18 too.
  sendCommand("get ip", false, "IP=");

  char newChar;
  byte offset = 0;

  // Copy the IP address from the response into our buffer
  while (offset < IP_ADDRESS_BUFFER_SIZE) {
    newChar = uart.read();

    if (newChar == ':') {
      ip[offset] = '\x00';
      break;
    } else if (newChar != -1) {
      ip[offset] = newChar;
      offset++;
    }
  }

  // This handles the case when we reach the end of the buffer
  // in the loop. (Which should never happen anyway.)
  // And hopefully this prevents us from failing completely if
  // there's a mistake above.
  ip[IP_ADDRESS_BUFFER_SIZE-1] = '\x00';

  // This should skip the remainder of the output.
  // TODO: Handle this better?
  waitForResponse("<");
  while (uart.read() != ' ') {
    // Skip the prompt
  }

  // For some reason the "sendCommand" approach leaves the system
  // in a state where it misses the first/next connection so for
  // now we don't check the response.
  // TODO: Fix this
  uart.println("exit");
  //sendCommand("exit", false, "EXIT");

  return ip;
}


// Preinstantiate required objects
SpiUartDevice SpiSerial;
WiFlyDevice WiFly(SpiSerial);

