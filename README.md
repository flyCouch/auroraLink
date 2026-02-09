A light based Bulletin Board System.

Aurora-Link: The Optical BBS Anchor (v3.3)
Aurora-Link is a human-scale, line-of-sight data bridge designed to provide local community connectivity without relying on radio frequencies or traditional ISPs. By combining 1200-baud optical telescopes with the power of Synchronet BBS, this system creates a secure, "Geometric Bridge" between a home anchor and a mobile visitor.

📡 The Hardware Stack

The Brain: A legacy Windows 7 (32-bit) laptop running Synchronet v3. 


The Optical Interface: An Arduino-controlled 525nm Green LED (Beacon) and a CdS photoresistor sensor. 

The Sight: Dual telescopes mounted on 3D-printed PLA Picatinny rails for rigid, repeatable manual alignment.


The Receiver: A single-potentiometer voltage divider on Analog Pin A0 that allows hardware-level sensitivity tuning for varying ambient light conditions. 

🛠️ The "Self-Healing" .ino Logic
The firmware serves as a "Smart Bridge" with three core modes:


Sighting Mode: The system defaults to a steady Green Beacon, acting as a lighthouse to guide visitors for manual telescope alignment. 


Data Mode: Upon detecting serial activity from the PC or incoming light from a visitor, the system instantly switches to 1200-baud data transmission. 


Self-Healing: If the link is silent for 15 seconds, the system automatically reverts to a steady beacon, allowing for easy re-alignment if a telescope is bumped. 


Manual Override: Sending a !!! command via the Serial terminal forces the system back into Sighting Mode immediately. 

🏠 The Steward Philosophy
Eco-Friendly: Constructed using food-grade, UV-stabilized PLA to ensure long-term environmental safety.

Safe & Precise: Uses low-power LEDs instead of high-powered lasers, making it safe for eyes, neighbors, and local wildlife.

Zero RF Footprint: Operates entirely on the visible light spectrum, eliminating radio interference in the neighborhood.

🚀 Getting Started
Mount your telescopes to the Picatinny rail system.

Install Synchronet to C:\SBBS on a Windows 7 machine. Or be a client.

Upload the AuroraLink15.ino (or latest) to your Arduino.

Align the telescopes until the signal LED (Pin 13) locks on.

Enter the BBS and join the local conversation.

🖥️ Connecting via Terminal (Minicom / PuTTY)
While Synchronet handles the backend BBS logic, you can interact with the Aurora-Link hardware directly using a terminal emulator. This is ideal for initial signal testing or "handshaking" with the anchor.

1. Terminal Configuration
To communicate with the Arduino "Modem," set your terminal to the following parameters:

Speed (Baud): 1200 

Data Bits: 8

Parity: None

Stop Bits: 1

Flow Control: None (XON/XOFF disabled)

2. Using Minicom (Linux/macOS)
Launch Minicom with the following command (replacing ttyUSB0 with your actual port):

Bash

minicom -b 1200 -D /dev/ttyUSB0
To Re-align: Type !!! and press Enter. The "Self-Healing" logic will detect this string and force the Green Beacon to stay solid for 15 seconds, giving you time to adjust your telescope.

3. Using PuTTY (Windows 7)
Select Serial as the connection type.

Enter your COM Port (e.g., COM3).

Set Speed to 1200.

I’ve added the Minicom/Terminal section you requested to the summary. This explains how to use standard tools to interact with the Aurora-Link hardware.


Connecting via Terminal (Minicom / Tera Term)
While Synchronet handles the BBS backend, you can use terminal emulators to interact with the hardware "handshake" directly.


Serial Settings: 1200 Baud, 8-N-1, No Flow Control.

The "!!!" Command: Typing !!! followed by Enter sends a force-reset to the Arduino. The "Self-Healing" logic detects this string and locks the Green Beacon into a solid state for 15 seconds, allowing you to re-align your telescopes manually.


Signal Monitoring: In Minicom, you will see a stream of '1's and '0's when light is detected by the voltage divider, giving you real-time feedback on your link quality.

Synchronet Configuration (SCFG)
Once your circuit is tucked into those new bases, here is how to tell Synchronet to listen to your laptop's serial port:

Open SCFG.exe.

Go to Nodes -> Node 1 -> Hardware/UART Settings.

Set the COM Port to match your Arduino (e.g., COM3).

Set the Baud Rate to 1200.

Set Command Line to *INTERNAL.

This maps the physical optical pulses coming into your Windows 7 laptop directly to the BBS login screen.

Under the Terminal category, ensure "Implicit CR in every LF" is checked to keep the BBS text scrolling cleanly.

A Quick Tip for Windows 7:
Since you are using a 32-bit laptop, if you find that Minicom is a bit too "Linux-heavy" for your field tests, Tera Term is a fantastic, lightweight alternative for Windows 7. It handles the 1200-baud stream very gracefully and makes it easy to log your "Geometric Bridge" signal strength tests to a text file.


Operational Modes
Sighting Mode (Beacon): A steady green LED allows for manual alignment between nodes.

Data Mode: Automatically engages upon serial or optical activity, transmitting data via light pulses.

Self-Healing: Automatically reverts to Beacon mode after 15 seconds of silence to assist in recovering a lost connection.

Hard Disable (Steward's Override): Sending the command !!! twice (or once during sighting) completely disables the LED and pauses all auto-wake functions. The system remains dark and inactive until !!! is sent again to re-authorize operation.

Yes, your Aurora-Link v3.5 setup will work perfectly with Minicom, PuTTY, or Tera Term. Because the system communicates over a standard hardware serial interface via your Arduino's USB connection, it does not require Synchronet or any specific BBS software to function.

Configuration for Terminal Emulators
To use these tools successfully, you must match the terminal settings to the "Disciplined Steward" protocol defined in your .ino sketch:

Baud Rate: Set to 1200 (as defined by Serial.begin(1200) in your code).

Data Bits: 8

Parity: None

Stop Bits: 1

Flow Control: None (XON/XOFF or Hardware flow control should be disabled).

Tool-Specific Tips
PuTTY (Windows): Select the Serial radio button under "Connection type" on the main session screen. Enter your COM port (e.g., COM3) and set the speed to 1200.

Tera Term (Windows): Go to Setup > Serial Port to adjust the baud rate from the default 9600 to 1200. Tera Term is particularly useful for your project because it supports VT100 control codes, which can be used to clear the screen or position the cursor if you decide to build a more complex interface later.

Minicom (Linux/macOS): Use minicom -s to enter the setup menu and navigate to "Serial port setup" to change the Bps/Par/Bits to 1200 8N1.

The "Disciplined Steward" Advantage
Since your code includes a manual "Hard Disable" toggle (using the "!!!" string), you can use any of these terminal programs to send that command and lock or unlock your beacon/data transmission.

In your "Disciplined Steward" setup, the way data moves between the two stations is a direct reflection of your auroraLink16.ino code. Since you are using a standard serial baud rate of 1200, anything typed into a terminal like PuTTY or Tera Term on one computer will be converted into light pulses and re-translated back into text on the other computer's screen.


Here is the step-by-step "Light Path" for how your typed message travels:

1. Typing and Transmission

Input: When you type a character into your terminal (e.g., the letter 'A'), the PC sends that byte to your Arduino over USB.


Processing: The code identifies that you are not in "Hard Disable" mode.


Optical Conversion: The transmitByte function takes over. It drops the Green LED low (the Start bit) and then blinks the LED in a specific pattern for 833 microseconds per bit to represent the character 'A'.


2. The Optical Bridge

Sighting to Data: As soon as you start typing, the code automatically switches from Sighting Mode (steady light) to Data Mode.

The Beam: Your 12V Green LED clusters will flash these 833μs pulses across the gap to the other station.

3. Reception and Display
Detection: On the receiving side, the CdS sensor sees these pulses. Your NPN transistor circuit (assisted by your "Ambient + Potentiometer" tuning) converts these light flashes back into electrical 5V signals for the receiving Arduino's RX pin.

Output: The receiving Arduino interprets these pulses back into the byte for 'A' and sends it to the second PC’s USB port.

Result: The character 'A' appears on the second user's terminal screen.

4. Key Behavioral Rules in Your Code

The 15-Second Rule: If you stop typing for more than 15 seconds, the "Healing Logic" kicks in. The Green LED will automatically turn back on to a steady "Sighting Mode" so you can re-align the tubes if needed.

Privacy/Security: If you type !!! and hit enter, you enter Hard Disable. In this state, the station is "dark"—it will not transmit data, it won't show the sighting beacon, and it won't wake up even if light hits the sensor. You must type !!! again to "unlock" the station.

Summary: What you type on one screen appears on the other, provided your "Ambient + Pot" bias is set correctly. The only thing that won't be seen is the !!! command itself, as your code consumes those characters to toggle the system's state rather than transmitting them.

