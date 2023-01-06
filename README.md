# ESP32_SMA-Inverter
Arduino Project to read SMA Inverter data via ESP32 bluetooth.
It ist tested with my SMA SMC6000TL with a plugin SMA bluetooth module.
Please let me know when you have tested the software on other SMA Inverters.

The starting point for this project was the code posted by "SBFspot" and "ESP32_to_SMA" on github.
Many thanks for the work on these projects!

SETUP:
Copy Config_example.h to Config.h and modify SmaBTAddress and SmaInvPass.
If SMA_WEBSERVER is usend, modify WIFI ssid and password too.
The SmaInvPass has always 12 character with 0x00 on the unused trailing character.

NOTES:

TODO:

KNOWN BUGS:

