Steps for running the ble code in the linux board.

How to set up and run the Middleware
Steps for running the ble code in the linux board.

Connect the dongle to board(Running Linux).

Run the "dmesg" command you will see, To which port dongle is connected(eg: /dev/ttyACM0). It can be from /dev/ttyACM0 to
/dev/ttyACM9.(TODO: Modify udev rules to restrict the port to specific node)

Download the code

Then, if do "ls" you will see lot of directories. main code is in "ble_host_code" directory. change to this directory.

If you don't have libssl-dev package installed you have install it.(If you are running debian based linux install it
using the command "sudo apt-get install libssl-dev". This package is needed for AES encryption api's.

If you don't have zip tool installed in your PC. Install it, by running the command. "sudo apt-get install zip"

In ble_host_code directory run the "cmake ." command without quotes. This will generate the required make files for
building the project. Then execute "make" command This command will generate executable from the source code.

If you have followed all the steps correctly. It will create a executable with the name "ble_host_app.out".

In the main repository directory you will see a directory with the name "configuration_files" This directory contains
database of keys sample xml file which needs to transferred.

Run built binary from step 9 with following command. sudo ./ble_host_app.out portname keys_file_path xml_file_path
sensor_data_file_path
eg: sudo ./ble_host_app.out /dev/ttyACM0 /home/yadhu/.br/keys.txt /home/yadhu/.br/sample.xml /home/yadhu/.br/data.dat

The need of different command line parameters is explained below.

port_name This is the first parameter this represents to which device node the ble dongle is connected this linux specific
this might vary from PC to PC, Hence check device node to which ble dongle is connected and pass this as an command line parameter.

keys_file_path This is the second parameter. This contains the keys database and their permission levels.

xml_file_path: This is the third parameter, Path to xml file which needs to be transferred to smart phone. In the current
implementation this file size is limited to 5KB.

sensor_data_file_path: This is the fourth parameter, path to sensor data file.

 After running the above command, device will start broadcasting the itself as camera.


Protocol between the BLE device and the smart phone app.
Steps by the application to connect to B and R BLE smart dongle.

First smart phone will scan for available devices.
Presently BLE Dongle is flashed with firmware which advertises itself as "camera" device.
Smart phone will connect to BLE smart dongle which advertises itself as a "camera"
After connecting the smart phone can see four services implemented. Following figure shows the list of services.


The first service named Generic Access(UUID: 0x1800) is the default service which is present in all the BLE profiles.

The next service(UUID:00431c4a-a7a4-428b-a96d-d92d43c8c7cf) is a read/write service which has the sensor data after
valid authentication. This service contains one characteristics data(UUID:f1b41cde-dbf5-4acf-8679-ecb8b4dca6fe) value
respectively. The characteristic is 241 byte wide(128-bit encrypted value(240 bytes) plus 1 byte checksum of unencrypted
sensor data) wide. These values contain the encrypted sensor data of 240 bytes wide and one byte of check sum of unencrypted
sensor data. Smart phone application will decrypt the first 240 bytes and then will calculate the checksum of the decrypted
data, compares the checksum to 241th byte to make sure the data is valid/decrypted correctly or not.

The next service(UUID:00431c4a-c7a4-428b-a96d-d92d43c8c7cf) is the very important special service which contains a
readable and writable characteristic(UUID:f1b41cdg-dbf5-4acf-8679-ecb8b4dca6fe) it is 1 byte wide. This characteristic
is used for sending the authentication key index from smart phone to the ble dongle. If this key index matches any of
the key in the Linux host then dongle starts sending encrypted sensor data. Smart phone will scan the QR code and will
write the key index(Key index is 1 byte) of the security key to this characteristic. This service contains two more
characteristics which are used for initial transfer of the zipped xml from ble device to smartphone(After zipped XML
transfer these characteristics are useless) According to the current implementation, It can handle zipped xml file of
5 KB(It doesn't make sense to transfer a large file via ble) In these two characteristics one is of type 600 bytes with
characteristic UUID 8c7c03bc-1bc8-43e3-8267-f5c0c0a6eac8 This characteristic is special type. Whenever gatt client tries
to read the value, ble dongle internally gets the call back and first 600 bytes of zipped xml gets written(This will happen
within 30 seconds of read request) then gatt client receives first 600 bytes of data. For the next 600 bytes client should
read this characteristic again then gatt server will send next 600 byte chunk to client. This process continues until all
xml data transferred, Once file transfer is complete, The BLE device will notify the gatt client by writing the value xml
file size to last characteristic of size 2 bytes. This last characteristic has UUID 79e40442-4e81-411e-aeec-8e0befc3d24e.
This notifiable characteristic. Before reading the xml data client should register for this notifiable last characteristic
for receiving the notification. Once the complete file is transferred smart phone receives a notification if it earlier
registered for this characteristic. This notification contains the value of the xml file size. There is no handshake and
error correction or checksum is needed since BLE stack(Specifically link layer) makes sures that correct data is transferred
between two communicating devices.

BLE device will verify the key index. If it belongs to any of the keys in the key database it will continue connection
or else it will automatically disconnect from the smart phone.

If the key index is valid then BLE smart device will determine the key  from a key database based on the key id sent from app.
Based on the authentication key, BLE device will expose the corresponding encrypted values of the sensors.

The decrypted value of the first 240 bytes of data characteristic of the characteristic represent the real sensor
value this can used plotting graph or analytics(Whatever it is, its apps job).
If the smart phone gets disconnected, Then again BLE smart dongle starts advertising its presence.
