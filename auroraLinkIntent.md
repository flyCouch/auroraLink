Project Intent:

The Aurora-Link is a high-sensitivity, air-gapped optical communication system designed for point-to-point data transfer using visible light. It is engineered as a  "slow science" experiment. The system provides a reliable handshake that remains stable across varying ambient light conditions, from indoor shop environments to outdoor daylight.

Hardware Summary
The hardware is designed for consistent alignment and long-term stability:

Controller: Arduino Uno 

Transmitter: High-brightness Green LED connected to Pin 2.

Receiver: Cadmium Sulfide (CdS) photoresistor in a voltage divider connected to Pin A0.

Visual Feedback: The onboard LED (Pin 13) acts as a real-time signal monitor, reflecting the raw state of the sensor.

Housing: Custom 3D-printed assemblies (CdSHolderSupport.stl) provide physical shrouding to prevent ambient light from washing out the signal.

Software Logic & Functionality
The system runs on the auroraLink61.ino firmware, supporting full-duplex interaction via PuTTY.

Locked Reference Calibration: To handle shifting light levels, the system locks its sensitivity during setup(). By shielding the receiver during a reset, you establish a "zero" baseline that remains fixed until the next manual reset.

Adaptive Timing: All communication timing is derived from a single baudRate variable. The software automatically calculates the bit-time for transmission and the 1.5x bit-time offset required for precise receiver sampling.

Constant Signal Monitoring: Pin 13 stays ON whenever the sensor detects a light jump of 5% or more above the baseline, providing a constant diagnostic of link integrity.

Intelligent State Management:

Sighting Mode: The LED remains steady (HIGH) to aid in physical alignment between units.

Data Mode: Upon activity, the system enters Data Mode, where the transmitter LED stays OFF when idle to avoid self-blinding the receiver.

Healing Logic: After 15 seconds of inactivity, the sentinel automatically reverts to Sighting Mode without losing its calibrated light reference.

Experimental Capabilities
The current firmware is optimized for 20 Baud, creating a "savoury" data transfer experience where each 50ms bit can be visually tracked. Because the system is full-duplex, users can send and receive data simultaneously.

