import socket
import logging
import struct
import thread

#command opcodes:
HEATER_CONTROL_CMD = 1
OVEN_CONTROL_RESERVE_CMD = 2
OVEN_CONTROL_RELEASE_CMD = 3
STATUS_REQUEST_CMD = 4
STOP_CMD = 5

#Reflow oven Arduino controller emulating class:
class RelfowEmulator:
    #constructor:
    def __init__(self, if_ip, if_port, cntrl_port):
        print('Reflow emulator started.')
        #network config:
        self.if_ip = if_ip
        self.if_port = if_port
        self.cntrl_ip = '' #default any network
        self.cntrl_port = cntrl_port
        
        self.sock_in = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock_in.bind((self.if_ip, self.if_port))

        self.sock_out = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        #oven state:
        self.heater_state = False #default off
        self.current_temp = 21
        self.reserved = False #default oven not reserved

        #run flag:
        self.run_flag = True
        
    #destructor:
    def __del__(self):
        print('Reflow emulator stopped.')
        self.sock.close()

    #parse controller command:
    def parse_cmd(self, cmd, arg, addr):
        if (cmd == HEATER_CONTROL_CMD):
            print('Heater control command received.')
            if (arg):
                self.heater_state = True
            else:
                self.heater_state = False
        elif (cmd == OVEN_CONTROL_RESERVE_CMD):
            print('Oven reserve command received.')
            self.reserved = True
            self.cntrl_ip = addr[0]
        elif (cmd == OVEN_CONTROL_RELEASE_CMD):
            print('Oven release command received.')
            self.reserved = False
        elif (cmd == STATUS_REQUEST_CMD):
            print('Status request command received.')
            msg = self.create_status_msg()
            self.sock_out.sendto(msg,addr)
        elif (cmd == STOP_CMD):
            print('Stop command received.')
            self.run_flag = False
        else:
            print('Unknown command received.')

    #create status message:
    def create_status_msg(self):
        status_msg = struct.pack(">H", self.current_temp) + struct.pack("B", self.heater_state)
        return status_msg
        
    
    #run:
    def run(self):
        while True:
            #check if packet received:
            data, addr = self.sock_in.recvfrom(1024)

            #parse data if present:
            if data:
                print('Packet received, IP address: %s', addr[0])
                self.parse_cmd(ord(data[0]), ord(data[1]), addr)

            if (self.run_flag == False): #if false stop execution
                break
                
if __name__ == "__main__":
    # execute only if run as a script:
    inst = RelfowEmulator('127.0.0.1', 9000, 9001)
    inst.run()
