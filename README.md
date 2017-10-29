This repository contains sample applications and main application for bled112 smart dongle from bluegiga.

1.	Main application code resides in "ble_host_code" directory. Refer README file to futher info.
2.	bled112 dongle custom profile creation firmware sample code resides in "custom_profile_firmware" directory.
3.	The file which stores the encryption keys and permission levels resides in "configuration_files" follow
		the instructions provided in the "keys.txt" for adding the new keys to the keys database.
4.  The data.dat file contains the sensor data from the sensor this value gets transferred via ble.
	Sensor data is shared to smart phone only of the permission level of key greater than permission level of sensor.
	Modify this data.data file for updating sensor data.
4.	Samples directory contains some sample codes of bled112 smart dongle.

If you have any doubts regarding the linux side application, write me a mail. I am more than happy to help.
	My mail id: yadhu.bmsce@gmail.com
