# WIZ750SR
WIZnet Serial to Ethernet(S2E) module based on W7500 chip, WIZ107/108SR S2E compatible device

- 48(W) x 30mm(L) x 18mm(H)
<!-- WIZ750SR pic -->
<p align="center">
## WIZ750SR EVB (Separate purchases)

- WIZ750SR Developer Board
- USB to UART chip, CP2104
- RESET Tact Switch
- BOOT0 Slide Switch
- H/W Trig Slide Switch
- Buttons/LEDs for Expansion GPIO TEST (Digital / Analog)
- Micro USB Connector
- WIZ750SR-EVB (TTL/RS232)
 - RS-232C Connector, D-SUB9-MALE.
- WIZ750SR-EVB (RS422/485)
 - RS-422/485 Connector, ECH381R.

<!-- WIZ750SR EVB pic -->
<p align="center">
  <img width="60%" src="http://www.wiznet.io/wp-content/uploads/2016/11/1WIZ750SR-TTL-EVB_.png" />
</p>
 
For more details, please refer to [WIZ750SR Wiki page](https://wizwiki.net/wiki/doku.php?id=products:wiz750sr:start) in [WIZnet Wiki](https://wizwiki.net).
 
 
## Features
- WIZnet W7500P Hardwired TCP/IP SoC chip
  - Hardwired TCP/IP Core
  - The one-chip solution which integrates an ARM Cortex-M0
  - Embedded PHY(IC plus IP101G)
  - Hardwired TCP/IP stack supports TCP, UDP, IPv4, ICMP, ARP, IGMP, and PPPoE protocols
  - Easy to implement the other network protocols
- Software and Hardware compatible with WIZ107/108SR S2E Module
- The modules is available in three versions:
  - WIZ750SR-TTL: TTL Version
  - WIZ750SR-RS232: RS-232 Version
  - WIZ750SR-RS485: RS-485/422 Version
- For more details, please refer to the [WIZ750SR Wiki page](http://wizwiki.net/wiki/doku.php?id=products:wiz750sr:start)
 
 
## Hardware material, Documents and Others
Various materials are could be found at [WIZ750SR Wiki page](https://wizwiki.net/wiki/doku.php?id=products:wiz750sr:start) in [WIZnet Wiki](https://wizwiki.net).
- Documents
  - Overview
  - Getting Started Guide
  - User's Manual
  - Configuration Tool Manual
  - Command Manual
  - Trouble Shooting
- Technical Reference (Datasheet)
  - Hardware Specification
  - Electrical Characteristics
  - Reference Schematic & Parts
  - Dimension
 
 
## Software
These are Firmware projects (source code) based on Keil IDE for ARM (version 5)
- Firmware source code
  - Application (App)
  - Boot
- WIZ750SR operation manual
  - [WIZ750SR Command Manual](http://wizwiki.net/wiki/doku.php?id=products:wiz750sr:commandmanual:start)
  - [WIZ750SR Configuration Tool Manual](http://wizwiki.net/wiki/doku.php?id=products:wiz750sr:configtoolmanual:start)
 
 
## Tool
- [ISP Download Tool](https://wizwiki.net/wiki/lib/exe/fetch.php?media=products:wiz750sr:download:w7500_isp_20150401.zip)
- [Configuration Tool (GUI)](https://github.com/Wiznet/WIZnet-S2E-Tool-GUI) (New!)
- [Configuration Tool (CLI)](https://github.com/Wiznet/WIZnet-S2E-Tool)
- [WIZVSP](https://wizwiki.net/wiki/doku.php?id=products:wiz750sr:download:start) 
 
 
## Update History
**The Boot of V1.3.0 and less than App of V1.2.6 version can't be used together.**
**But the Boot of less than V1.2.6 and APP of V1.3.0 version can use together.**
<p align="center">
  <img width="40%" src="https://user-images.githubusercontent.com/9648281/69318883-6e5b1980-0c81-11ea-8f7b-b2b866b773da.JPG" />
</p>
v1.3.1 Stable
- Bug fixes:
 - When exception situation of TCP Connection - Class C private IP
When the module IP and Remote IP is 192.168.xx.xx and third ip is different for each, ARP request operation is not transmitted.(This part had deleted)


v1.3.0 Stable
- Bug fixes:
  - When APP firmware is uploading using the configuration Tool and Board is power off, the product not operate.
  - DHCP Lease Time :  when _DHCP_DEBUG_ enable, DHCP Lease Time is fixed as 10 second. 
  - The Status LED(PA_07) of  WIZ750SR_1xx doesn't operate.
 
- Improvements:
  
  - The ISR adress have changed to fixed address. The ISR address of APP didn't copy and use at Boot.
  -  DHCP lease time is modified to use it that information of received packet from DHCP Server.
  - Compare DHCP Server IP :  When DHCP OFFER Packet receive from DHCP Server,the DHCP server ip in packet compare with previous DHCP Server IP.
  - The Status LED(PA_07) of  WIZ750SR_1xx is modified. Turning on an LED and making it blink once per second.

=====================================================
v1.2.7 Stable
- Bug fixes:
 - When exception situation of TCP Connection - Class C private IP
When the module IP and Remote IP is 192.168.xx.xx and third ip is different for each, ARP request operation is not transmitted.(This part had deleted)

v1.2.6 Stable
- Bug fixes:
  - If serial data packing 'time' option  is less than 500ms, the first serial data(+++) ignored in GW mode.

- Improvements:
  - While IP address allocate using DHCP,
    the device can be made searchable using the Configuration tool and it can change the mode(AT mode <-> GW mode).
    - While IP address is allocating using DHCP , it can change the AT mode. It can show that the status of channel change from ATmode to OPEN in Configuration tool. So AT command didn't operate.    
 

v1.2.5 Stable
- Bug fixes:
  - Modyfied the setSn_DHAR() Address offset.
  - Modyfied the WIZ750SR-110 HW_Trigger pin.

- Improvements:
  - If remote ip address is multicast address, enable multicating and set Sn_DHAR, Sn_DIPR, Sn_DPORTR before open socket 
  
v1.2.4 Stable
- Bug fixes:
  - Serial data packing 'time' option does not work if serial command mode switching code is disabled.
  - Fixed typo of debug message

- Improvements
  - Added the PHY init stabilization code
  - Restore TCP connection status function to Status Pin (WIZ750SR-100/105/110 only)
    - The Status pin now acts as an 'HW_Trigger' pin(Input at booting) and a TCP connection status pin(output). 

v1.2.3 Stable
- Bug Fixes:
  - Fixed an issue where the pin states could not be saved correctly if the user's GPIO is set to 'Digital Output'

- Improvements:
  - From this version, AppBoot mode supports TCP unicast search function
  	- Until the previous version, AppBoot mode only supported the UDP search function
  	- This is an enhancement to enable firmware update even in device control using TCP unicast search function in new configuration tool

- Changes:
  - The function of the STATUS pin that was responsible for the TCP connection status indicator function has been changed to Command mode switch trigger pin (HW_Trigger).
    - This changes only apply to the WIZ750SR-1xx product family (WIZ750SR-100/105/110, change to maintain compatibility with WIZ100SR)
    - The HW_Trigger pin up to v1.2.2 will no longer work with v1.2.3. The STATUS pin acts as the HW_Trigger.
    
v1.2.2 Stable
- Bug Fixes:
  - Stabilize the network operation of the W7500P devices 
  - Error in setting function in App_Boot mode due to 'TR', 'SC' command processing failure
  - Applied the missing TCP retransmission timeout-value change

v1.2.2 Pre-release
- Bug Fixes:
  - Internal bug fixing (PHY init)
  - Char serial packing option(delimiter) setting does not correctly apply some hex codes
    
- Improvements:
  - Changed the location of the flash erase operation function during network firmware update
    - Now the application area flash memory is erased after socket establishment of TCP connection for the firmware update
  - Extended serial debug message added: 
    - If this option is selected, users can check the sent and received data via the serial debug port. (WIZnet S2E tool GUI version 0.4.0 or later)

- Changes:
  - Changed the TCP retransmission timeout-value from 250ms to 200ms(return to default value)
      - The TCP timeout can be adjusted by the TCP retransmission retry count added in firmware version 1.2.0.

v1.2.1
- Improvements:
  - Enhanced operational stability when performing configuration data save and factory reset functions 

v1.2.1 Pre-release
- Bug Fixes:
  - 'Character' of serial data packing option is not set correctly 

v1.2.0
- Bug Fixes:
  - Data loss problem due to unintended delay in RS-485 mode (WIZ750SR-RS485 only)
  - Incorrect command processing problem: If the processing of a command set containing unsupported commands via Ethernet is sent to WIZ750SR, the device only responds a result until the error occurs
- Improvements:
  - Command added:
    - TR: TCP retransmission retry count adjustment function(WR, param range: 1 ~ 255)
    - BU: AppBoot update function (WO)
  - Changed the firmware update mechanism:
    - The app-backup area is no longer used and flash update operation is only performed in AppBoot mode
  - Now an error message about command processing will be output only during serial command mode operation  
- Known Issues:
  - 'Character' of serial data packing option is not set correctly
    - This issue will be fixed in firmware version 1.2.1

v1.1.2
- Bug Fixes:
  - MAC address changed problem when device settings are saved (very occasionally)
  - Delayed response to search in config-tool when connecting to server in TCP client mode
  - Force socket close when fw update fails occurred
- Improvements:
  - Supports serial baudrate up to 460.8kbps
  - Added device profiles of WIZ750SR-1xx series to project source code

v1.1.1
- Bug Fixes:
  - Firmware update timeout function error in AppBoot mode
  - 10M Ethernet fixed error (W7500P only)
  - Invalid parameter of FW command (firmware update)
- Improvements:
  - Command added: AppBoot mode switching function(AB)
  - Initial boot latency(1500ms) has been removed

v1.1.0
- First release : 2016
