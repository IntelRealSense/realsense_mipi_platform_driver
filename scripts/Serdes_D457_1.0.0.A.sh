#D457 SerDes script: 
# D457_MAX9295A_MAX9296A_Intel.cpp
# Version: 1.0.0.A
# Compatible Driver Version: 1.0.1.0
# Compatible FW Version: 5.13.0.150
# The script shall be used for the following configuration: 
#       Depth + Metadata  +   RGB +  Metadata
#D457 SerDes script: 
# D457_MAX9295A_MAX9296A_Intel.cpp

# Written by Oliver Jakobi, Edited by Eddie De Reza
# Analog Devices
# Feb 1, 2022

# This script sets up MAX9295A and MAX9296A.
# EMB8 will be carried with Depth, RGB , Y8 (Not Y12I)

# MAX9295A
# CSI Port A:	2 lanes
# Pipe X:	Depth 	DT 0x1E VC0
#		EMB8	DT 0x12 VC0
# Pipe Y:	RGB 	DT 0x1E VC1
#		EMB8	DT 0x12 VC1
# Pipe Z:	Y8 	DT 0x2A VC2
#		EMB8	DT 0x12 VC2
# Pipe U:	Y12I 	DT 0x24 VC2

# MAX9296A
# CSI Port A:	2 lanes, 1500Mbps/lane
# Pipe X:	Depth 	DT 0x1E VC0
#		EMB8	DT 0x12 VC0
# Pipe Y:	RGB 	DT 0x1E VC1
#		EMB8	DT 0x12 VC1
# Pipe Z:	Y8 	DT 0x2A VC2
#		EMB8	DT 0x12 VC2
# Pipe U:	Y12I 	DT 0x24 VC2

# Default Power Up States
# MAX9295A
# CFG0 : I2C adr 0x80
# CFG1 : GMSL2, 6Gbps, Coax

# MAX9296A
# CFG0 : I2C adr 0x90
# CFG1 : GMSL2, 6Gbps, Coax

# I2C Addresses (8-bit)
# MAX9295A : 0x80
# MAX9296A : 0x90

# Script format
# [Number of bytes],[Slave adr],[Reg adr MSB],[Reg adr LSB],[Data],
# OR
# 0x00,[delay in ms],

#!/bin/bash
 
sudo i2cset -f -y 2 0x72 0x7

# Init

sudo i2cset -f -y 2 0x40 0x03 0x1002 w # Increase CMU regulator voltage
sudo i2cset -f -y 2 0x48 0x03 0x1002 w # Increase CMU regulator voltage
                                
sudo i2cset -f -y 2 0x48 0x14 0x2858 w # PHY A Optimization
sudo i2cset -f -y 2 0x48 0x14 0x6859 w # PHY A Optimization
sudo i2cset -f -y 2 0x48 0x15 0x2858 w # PHY B Optimization
sudo i2cset -f -y 2 0x48 0x15 0x6859 w # PHY B Optimization
                                
sudo i2cset -f -y 2 0x48 0x00 0x3110 w # One-shot reset  enable auto-link
# what is this? - 0x00,0x64,

sleep 1

# MAX9295A Setup **********************************************************

sudo i2cset -f -y 2 0x40 0x00 0xF302 w # Enable all pipes -- reg 0x0002 data - 0xF3
                                
sudo i2cset -f -y 2 0x40 0x03 0x1131 w # Write 0x33 for 4 lanes
                                
sudo i2cset -f -y 2 0x40 0x03 0x6F08 w # All pipes pull clock from port B
                                
sudo i2cset -f -y 2 0x40 0x03 0xF011 w # All pipes pull data from port B
                                
sudo i2cset -f -y 2 0x40 0x03 0x5E14 w # Pipe X pulls Depth (DT 0x1E)
sudo i2cset -f -y 2 0x40 0x03 0x5215 w # Pipe X pulls EMB8 (DT 0x12)
sudo i2cset -f -y 2 0x40 0x03 0x0109 w # Pipe X pulls VC0
sudo i2cset -f -y 2 0x40 0x03 0x000A w

sudo i2cset -f -y 2 0x40 0x03 0x0312 w # Double 8-bit data on pipe X, Y  
sudo i2cset -f -y 2 0x40 0x03 0x301C w # BPP = 16 in pipe X 
                                
sudo i2cset -f -y 2 0x40 0x03 0x5E16 w # Pipe Y pulls RGB (DT 0x1E)
sudo i2cset -f -y 2 0x40 0x03 0x5217 w # Pipe Y pulls EMB8 (DT 0x12)
sudo i2cset -f -y 2 0x40 0x03 0x020B w # Pipe Y pulls VC1
sudo i2cset -f -y 2 0x40 0x03 0x000C w
sudo i2cset -f -y 2 0x40 0x03 0x301D w # BPP = 16 in pipe Y                         

sudo i2cset -f -y 2 0x40 0x03 0x6A18 w # Pipe Z pulls Y8 (DT 0x2A) -- reg 0x0318 data - 0x6A
sudo i2cset -f -y 2 0x40 0x03 0x5219 w # Pipe Z pulls EMB8 (DT 0x12) -- reg 0x0319 data - 0x52
sudo i2cset -f -y 2 0x40 0x03 0x040D w # Pipe Z pulls VC2
sudo i2cset -f -y 2 0x40 0x03 0x000E w
# sudo i2cset -f -y 2 0x40 0x03 0x301E w # BPP = 16 in pipe Z -- reg 0x031E data - 0x30
                                
sudo i2cset -f -y 2 0x40 0x03 0x641A w # Pipe U pulls Y12I (DT 0x24)
sudo i2cset -f -y 2 0x40 0x03 0x040F w # Pipe U pulls VC2
sudo i2cset -f -y 2 0x40 0x03 0x0010 w
                                
sudo i2cset -f -y 2 0x40 0x03 0xD215 w # Enable independent VC's

sudo i2cset -f -y 2 0x40 0x01 0x0E02 w # LIM_HEART Pipe X: Disabled
sudo i2cset -f -y 2 0x40 0x01 0x0E0A w # LIM_HEART Pipe Y: Disabled                                 
sudo i2cset -f -y 2 0x40 0x01 0x0E12 w # LIM_HEART Pipe Z: Disabled
sudo i2cset -f -y 2 0x40 0x01 0x0E1A w # LIM_HEART Pipe U: Disabled



# MAX9296A Setup **********************************************************

sudo i2cset -f -y 2 0x48 0x04 0x504A w # 4 lanes on port A // Write 0x50 for 2 lanes
                                
sudo i2cset -f -y 2 0x48 0x03 0x2F20 w # 1500Mbps/lane on port A
                                
sudo i2cset -f -y 2 0x48 0x03 0x001C w # Un-double 8-bit data in , do not un-double 8bpp
sudo i2cset -f -y 2 0x48 0x03 0x001F w # Un-double 8-bit data in , do not un-double 8bpp
sudo i2cset -f -y 2 0x48 0x04 0x1073 w # Un-double 8-bit data, No:ALT_MEM_MAP8 & Yes:ALT2_MEM_MAP8
                                						    
sudo i2cset -f -y 2 0x48 0x02 0x3939 w # Force VS low in pipe U (VC2 VS will come from pipe Z, not needed for pipe U)

sudo i2cset -f -y 2 0x48 0x04 0x0F0B w # Enable 4 mappings for Pipe X
sudo i2cset -f -y 2 0x48 0x04 0x1E0D w # Map Depth  VC0
sudo i2cset -f -y 2 0x48 0x04 0x1E0E w	
sudo i2cset -f -y 2 0x48 0x04 0x000F w # Map frame start  VC0
sudo i2cset -f -y 2 0x48 0x04 0x0010 w	
sudo i2cset -f -y 2 0x48 0x04 0x0111 w # Map frame end  VC0
sudo i2cset -f -y 2 0x48 0x04 0x0112 w
sudo i2cset -f -y 2 0x48 0x04 0x1213 w # Map EMB8, VC0
sudo i2cset -f -y 2 0x48 0x04 0x1214 w 	
sudo i2cset -f -y 2 0x48 0x04 0x552D w # All mappings to PHY1 (master for port A)
                                
sudo i2cset -f -y 2 0x48 0x04 0x0F4B w # Enable 4 mappings for Pipe Y
sudo i2cset -f -y 2 0x48 0x04 0x5E4D w # Map RGB  VC1
sudo i2cset -f -y 2 0x48 0x04 0x5E4E w	
sudo i2cset -f -y 2 0x48 0x04 0x404F w # Map frame start  VC1
sudo i2cset -f -y 2 0x48 0x04 0x4050 w	
sudo i2cset -f -y 2 0x48 0x04 0x4151 w # Map frame end  VC1
sudo i2cset -f -y 2 0x48 0x04 0x4152 w 	
sudo i2cset -f -y 2 0x48 0x04 0x5253 w # Map EMB8, VC1
sudo i2cset -f -y 2 0x48 0x04 0x5254 w 	
sudo i2cset -f -y 2 0x48 0x04 0x556D w # All mappings to PHY1 (master for port A)
                        
sudo i2cset -f -y 2 0x48 0x04 0x0F8B w # Enable 4 mappings for Pipe Z
sudo i2cset -f -y 2 0x48 0x04 0xAA8D w # Map Y8  VC2
sudo i2cset -f -y 2 0x48 0x04 0xAA8E w
sudo i2cset -f -y 2 0x48 0x04 0x808F w # Map frame start  VC2
sudo i2cset -f -y 2 0x48 0x04 0x8090 w
sudo i2cset -f -y 2 0x48 0x04 0x8191 w # Map frame end  VC2
sudo i2cset -f -y 2 0x48 0x04 0x8192 w
sudo i2cset -f -y 2 0x48 0x04 0x9293 w # Map EMB8, VC2
sudo i2cset -f -y 2 0x48 0x04 0x9294 w
sudo i2cset -f -y 2 0x48 0x04 0x55AD w # Map to PHY1 (master for port A)
                                
sudo i2cset -f -y 2 0x48 0x04 0x01CB w # Enable 1 mappings for Pipe U (VC2 FS/FE will come from pipe Z, not needed in pipe U)
sudo i2cset -f -y 2 0x48 0x04 0xA4CD w # Map Y12I  VC2
sudo i2cset -f -y 2 0x48 0x04 0xA4CE w
sudo i2cset -f -y 2 0x48 0x04 0x01ED w # Map to PHY1 (master for port A)

sudo i2cset -f -y 2 0x48 0x01 0x2300 w # SEQ_MISS_EN Pipe X: Disabled / DIS_PKT_DET Pipe X: Disabled
sudo i2cset -f -y 2 0x48 0x01 0x2312 w # SEQ_MISS_EN Pipe Y: Disabled / DIS_PKT_DET Pipe Y: Disabled
sudo i2cset -f -y 2 0x48 0x01 0x2324 w # SEQ_MISS_EN Pipe Z: Disabled / DIS_PKT_DET Pipe Z: Disabled
sudo i2cset -f -y 2 0x48 0x01 0x2336 w # SEQ_MISS_EN Pipe U: Disabled / DIS_PKT_DET Pipe U: Disabled

### HW-SYNC  ###
# SerDes Depth Trigger Path MFP7 > MFP8 
sudo i2cset -f -y 2 0x48 0x02 0x82c5 w #MFP7
sudo i2cset -f -y 2 0x48 0x02 0x1fc6 w
sudo i2cset -f -y 2 0x40 0x02 0x84d6 w #MFP8
sudo i2cset -f -y 2 0x40 0x02 0x60d7 w #OUT_TYPE bit to 1
sudo i2cset -f -y 2 0x40 0x02 0x1fd8 w

# SerDes RGB Trigger Path MFP9 > MFP0
sudo i2cset -f -y 2 0x48 0x02 0x82cb w #MFP9
sudo i2cset -f -y 2 0x48 0x02 0x1bcc w
sudo i2cset -f -y 2 0x40 0x02 0x84be w #MFP0
sudo i2cset -f -y 2 0x40 0x02 0x60bf w #OUT_TYPE bit to 1
sudo i2cset -f -y 2 0x40 0x02 0x1bc0 w
