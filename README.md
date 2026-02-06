A light based Bulletin Board System.

Aurora-Link: The Optical BBS Anchor (v3.3)
Aurora-Link is a human-scale, line-of-sight data bridge designed to provide local community connectivity without relying on radio frequencies or traditional ISPs. By combining 1200-baud optical telescopes with the power of Synchronet BBS, this system creates a secure, "Geometric Bridge" between a home anchor and a mobile visitor.

📡 The Hardware Stack

The Brain: A legacy Windows 7 (32-bit) laptop running Synchronet v3. 
+1


The Optical Interface: An Arduino-controlled 525nm Green LED (Beacon) and a CdS photoresistor sensor. 
+1

The Sight: Dual telescopes mounted on 3D-printed PLA Picatinny rails for rigid, repeatable manual alignment.


The Receiver: A single-potentiometer voltage divider on Analog Pin A0 that allows hardware-level sensitivity tuning for varying ambient light conditions. 
+1

🛠️ The "Self-Healing" .ino Logic
The firmware serves as a "Smart Bridge" with three core modes:


Sighting Mode: The system defaults to a steady Green Beacon, acting as a lighthouse to guide visitors for manual telescope alignment. 
+2


Data Mode: Upon detecting serial activity from the PC or incoming light from a visitor, the system instantly switches to 1200-baud data transmission. 


Self-Healing: If the link is silent for 15 seconds, the system automatically reverts to a steady beacon, allowing for easy re-alignment if a telescope is bumped. 
+1


Manual Override: Sending a !!! command via the Serial terminal forces the system back into Sighting Mode immediately. 

🏠 The Steward Philosophy
Eco-Friendly: Constructed using food-grade, UV-stabilized PLA to ensure long-term environmental safety.

Safe & Precise: Uses low-power LEDs instead of high-powered lasers, making it safe for eyes, neighbors, and local wildlife.

Zero RF Footprint: Operates entirely on the visible light spectrum, eliminating radio interference in the neighborhood.

🚀 Getting Started
Mount your telescopes to the Picatinny rail system.

Install Synchronet to C:\SBBS on a Windows 7 machine.

Upload the AuroraLink15.ino (or latest) to your Arduino.

Align the telescopes until the signal LED (Pin 13) locks on.

Enter the BBS and join the local conversation.
