import socket
import logging
import struct
from time import sleep

#Reflow oven Arduino controller emulating class:
class RelfowEmulator:
    #command opcodes:
    HEATER_CONTROL_CMD = 0x01
    OVEN_CONTROL_RESERVE_CMD = 0x02
    OVEN_CONTROL_RELEASE_CMD = 0x03
    
    #constructor:
    def __init__(self, if_ip, if_port, cntrl_port):
        logging.info('Reflow emulator started.')
        #network config:
        self.if_ip = if_ip
        self.if_port = if_port
        self.cntrl_ip = '' #default any network
        self.cntrl_port = cntrl_port
        
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.if_ip, self.if_port))
        self.sock.setblocking(False) # Set socket to non-blocking mode

        #oven state:
        self.heater_state = False #default off
        self.current_temp = 21
        self.reserved = False #default oven not reserved

    #destructor:
    def __del__(self):
        logging.info('Reflow emulator stopped.')
        self.sock.close()

    #parse controller command:
    def parse_cmd(self, cmd, arg, addr):
        if (cmd == HEATER_CONTROL_CMD):
            logging.info('Heater control command received.')
            if (arg):
                self.heater_state = True
            else:
                self.heater_state = False
        elif (cmd == OVEN_CONTROL_RESERVE_CMD):
            logging.info('Oven reserve command received.')
            self.reserved = True
            self.cntrl_ip = addr[0]
        elif (cmd == OVEN_CONTROL_RELEASE_CMD):
            logging.info('Oven release command received.')
            self.reserved = False
        else:
            logging.error('Unknown command received.')

    #create status message:
    def create_status_msg(self):
        status_msg = struct.pack(">H", self.current_temp) + struct.pack("B", self.reserved)
        return status_msg
        
    
    #run:
    def run(self):
        while True:
            #check if packet received:
            try:
                data, addr = self.sock.recvfrom(1024)

                if data:
                    logging.info('Packet received, IP address: %s', addr[0])
                    self.parse_cmd(data[0], data[1], addr)
            except:
                pass

            #send status packet:
            if (self.reserved):
                msg = self.create_status_msg()
                sock.sendto(msg,(self.cntrl_ip, self.cntrl_port))

            #sleep for some time:
            sleep(0.01) #10ms
                
if __name__ == "__main__":
    # execute only if run as a script
    inst = RelfowEmulator('127.0.0.1', 9000, 9001)
    inst.run()
