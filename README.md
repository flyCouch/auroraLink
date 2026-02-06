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
