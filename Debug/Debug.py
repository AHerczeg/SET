import sys
import glob
import serial
import time


def serial_ports():
    print("Searching for ports")
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            if port != "COM5":
                print(port)
                ser = serial.Serial(port)
                ser.baudrate = 9600
                text = "scanning"
                ser.write(text.encode())
                time.sleep(1)
                out = ser.readline()
                ser.close()
                if(out.decode()):
                    result.append(port)
        except (OSError, serial.SerialException):
            pass


    return result

#if __name__ == '__main__':
#    print(serial_ports())
portList = serial_ports()
print(portList)
for port in portList:
    print(port)
    out = ''
    ser = serial.Serial(port)
    ser.baudrate = 9600
    text = "debug"
    ser.write(text.encode())
    time.sleep(1)
    while (out.strip() != "DEBUG_END"):
        out = ser.readline()
        out = out.decode()
        print(out)
    ser.close()