Aurora Link: The Optical BBS Anchor (v3.9.31)
Aurora Link is a robust, line-of-sight data bridge designed to provide local community connectivity without reliance on radio frequencies or traditional ISPs. This "Geometric Bridge" leverages visible light to create a private, secure network link, ideal for independent Bulletin Board Systems (BBS) and off-grid communication.

📡 System Demonstration
The current implementation, as shown in the demonstration video, successfully bridges two independent terminal sessions.


Real-Time Data: Text entered in a PuTTY terminal on the "Anchor" laptop is converted into high-precision light pulses. 


Point-to-Point Accuracy: The signal is captured across a physical gap by a CdS-equipped receiver tube and translated back into text on a second terminal. 


Visual Verification: The hardware uses a 1200-baud Serial Monitor for diagnostics while maintaining a rock-solid 20-baud optical link for maximum reliability over distance. 

🛠️ Firmware Architecture (auroraLink73noBecaon61.ino)
The system utilizes a "Locked-Reference" logic designed for stability in varying environments:


Ambient Calibration: On startup, the system captures a 1-second baseline of ambient light to set a referenceLevel. 


Precision Triggering: Data transmission is triggered by a 5% change (deltaPercent = 0.05) from the baseline, allowing the system to ignore minor light flickers while catching every bit. 
+1


Common Emitter Logic: The emitter (Pin 2) remains LOW by default, pulsing only when data is being transmitted. 
+1


Timing: The system utilizes an 8-bit data protocol with a dedicated Start Bit (HIGH) and Stop Bit (LOW). 
+1

🏛️ The BBS Vision: Independent Local Networks
Aurora Link is the ideal physical layer for an Independent Bulletin Board System:

Galvanic Isolation: Because the link is light-based, there is no electrical path between nodes, protecting hardware from ground loops or lightning surges.

Privacy & Sovereignty: Data travels in a narrow beam; it cannot be "sniffed" by standard Wi-Fi or Radio scanners.

Synchronet Integration: By mapping the Arduino's serial output to Synchronet BBS software, users can host local message boards, file areas, and chat rooms that function entirely outside the global internet.

Community Resilience: Perfect for neighborhood "digital commons" or emergency communication during infrastructure failures.

🏠 The Steward Philosophy
Environmental Safety: Built with UV-stabilized, food-grade PLA. No copper lines to bury or RF interference to manage.

Eye Safe: Optimized for high-visibility Green LEDs (525nm) instead of high-power lasers for use in residential areas.

Sane Engineering: Uses robust Arduino Uno hardware with USB-B connectors to withstand the rigors of field deployment.
