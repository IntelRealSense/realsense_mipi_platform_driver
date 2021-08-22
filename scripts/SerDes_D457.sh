
#!/bin/bash
 
sudo i2cset -f -y 2 0x72 0x7

# Deserializer
sudo i2cset -f -y 2 0x48 0x14 0x2858 w #PHY A Optimization
sudo i2cset -f -y 2 0x48 0x14 0x6859 w #PHY A Optimization
sudo i2cset -f -y 2 0x48 0x15 0x2858 w #PHY B Optimization
sudo i2cset -f -y 2 0x48 0x15 0x6859 w #PHY B Optimization
sudo i2cset -f -y 2 0x48 0x00 0x3110 w #OneShot Reset

sleep 1

# Serializer
#sudo i2cset -f -y 2 0x40 0x00 0xe201 w #Enable I2C_PT-1 and 2
sudo i2cset -f -y 2 0x40 0x00 0x7302 w #Enable all pipes
sudo i2cset -f -y 2 0x40 0x03 0x1131 w #2 data lanes on port A
sudo i2cset -f -y 2 0x40 0x03 0x6708 w #All pipes pull clock from port B
sudo i2cset -f -y 2 0x40 0x03 0x7011 w #All pipes pull data from port B
sudo i2cset -f -y 2 0x40 0x03 0x5E14 w #Pipe X pulls RGB (DT 0x1E)
sudo i2cset -f -y 2 0x40 0x03 0x0209 w #Pipe X pulls VC1
sudo i2cset -f -y 2 0x40 0x03 0x000A w #
sudo i2cset -f -y 2 0x40 0x03 0x5E16 w #Pipe Y pulls Depth (DT 0x1E)
sudo i2cset -f -y 2 0x40 0x03 0x010B w #Pipe Y pulls VC0
sudo i2cset -f -y 2 0x40 0x03 0x000C w #
sudo i2cset -f -y 2 0x40 0x03 0x6A18 w #Pipe Z pulls Y8 (DT 0x2A)
sudo i2cset -f -y 2 0x40 0x03 0x5219 w #Pipe Z pulls EMB8 (DT 0x12)
sudo i2cset -f -y 2 0x40 0x03 0x010D w #Pipe Z pulls VC0
sudo i2cset -f -y 2 0x40 0x03 0x000E w #
sudo i2cset -f -y 2 0x40 0x03 0x0412 w #Double RAW8 on pipe Z
sudo i2cset -f -y 2 0x40 0x03 0x301E w #BPP = 16 in pipe Z
sudo i2cset -f -y 2 0x40 0x03 0x641A w #Pipe U pulls Y12I (DT 0x24)
sudo i2cset -f -y 2 0x40 0x03 0x010F w #Pipe U pulls VC0
sudo i2cset -f -y 2 0x40 0x03 0x0010 w #
sudo i2cset -f -y 2 0x40 0x03 0x8015 w #Enable independent VC's

# Deserializer
#sudo i2cset -f -y 2 0x48 0x00 0xc201 w #Enable I2C_PT-1 and 2
sudo i2cset -f -y 2 0x48 0x04 0x504A w # 2 lanes on port A
sudo i2cset -f -y 2 0x48 0x03 0x2F20 w #1.5Gbps on Port A
sudo i2cset -f -y 2 0x48 0x03 0x401C w #Un-double RAW8 in pipe Z
sudo i2cset -f -y 2 0x48 0x03 0x401F w #
sudo i2cset -f -y 2 0x48 0x04 0x0273 w #
sudo i2cset -f -y 2 0x48 0x04 0x070B w #Enable 3 mappings for Pipe X
sudo i2cset -f -y 2 0x48 0x04 0x5E0D w #Map RGB, VC1
sudo i2cset -f -y 2 0x48 0x04 0x5E0E w #	
sudo i2cset -f -y 2 0x48 0x04 0x400F w #Map frame start, VC1
sudo i2cset -f -y 2 0x48 0x04 0x4010 w #	
sudo i2cset -f -y 2 0x48 0x04 0x4111 w #Map frame end, VC1
sudo i2cset -f -y 2 0x48 0x04 0x4112 w # 	
sudo i2cset -f -y 2 0x48 0x04 0x152D w #All mappings to PHY1 (master for port A)
sudo i2cset -f -y 2 0x48 0x04 0x038B w # Enable 2 mappings for Pipe Z  
sudo i2cset -f -y 2 0x48 0x04 0x2A8D w # Map Y8, VC0                   
sudo i2cset -f -y 2 0x48 0x04 0x2A8E w #                               
sudo i2cset -f -y 2 0x48 0x04 0x128F w #Map EMB8, VC0                  
sudo i2cset -f -y 2 0x48 0x04 0x1290 w #                               
sudo i2cset -f -y 2 0x48 0x04 0x05AD w #Map to PHY1 (master for port A)
sudo i2cset -f -y 2 0x48 0x04 0x07CB w # Enable 3 mappings for Pipe U  
sudo i2cset -f -y 2 0x48 0x04 0x24CD w # Map Y12I, VC0                 
sudo i2cset -f -y 2 0x48 0x04 0x24CE w #                               
sudo i2cset -f -y 2 0x48 0x04 0x00CF w #Map frame start, VC0           
sudo i2cset -f -y 2 0x48 0x04 0x00D0 w #                               
sudo i2cset -f -y 2 0x48 0x04 0x01D1 w #Map frame end, VC0             
sudo i2cset -f -y 2 0x48 0x04 0x01D2 w #                               
sudo i2cset -f -y 2 0x48 0x04 0x15ED w #Map to PHY1 (master for port A)

### HW-SYNC  ###
# SerDes Depth Trigger Path MFP7 > MFP1 
sudo i2cset -f -y 2 0x48 0x02 0x82c5 w #MFP7
sudo i2cset -f -y 2 0x48 0x02 0x1fc6 w
sudo i2cset -f -y 2 0x40 0x02 0x84c1 w #MFP1
sudo i2cset -f -y 2 0x40 0x02 0x60c2 w #OUT_TYPE bit to 1
sudo i2cset -f -y 2 0x40 0x02 0x1fc3 w

# SerDes RGB Trigger Path MFP9 > MFP0
sudo i2cset -f -y 2 0x48 0x02 0x82cb w #MFP9
sudo i2cset -f -y 2 0x48 0x02 0x1bcc w
sudo i2cset -f -y 2 0x40 0x02 0x84be w #MFP0
sudo i2cset -f -y 2 0x40 0x02 0x60bf w #OUT_TYPE bit to 1
sudo i2cset -f -y 2 0x40 0x02 0x1bc0 w
