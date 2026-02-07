import serial
import serial.tools.list_ports
import time
from datetime import datetime

def find_device():
    """Automatically find the microcontroller port"""
    ports = serial.tools.list_ports.comports()
    
    # Common identifiers for different microcontrollers
    identifiers = [
        'CH340',    # Common USB-serial chip
        'CP210',    # Silicon Labs
        'FTDI',     # FTDI chips
        'USB Serial',
        'Arduino',
        'RP2040',   # Raspberry Pi Pico
        'STM32',
        'ESP32',
        'Teensy',
        'ACM'       # Common on Linux
    ]
    
    for port in ports:
        port_info = f"{port.description} {port.manufacturer} {port.hwid}".upper()
        
        for identifier in identifiers:
            if identifier.upper() in port_info:
                return port.device
    
    return None

def connect_to_device(baud=115200, retry_interval=2):
    """Keep trying to connect to the device"""
    ser = None
    
    while True:
        port = find_device()
        
        if port is None:
            print("No device found. Waiting for device...", end='\r')
            time.sleep(retry_interval)
            continue
        
        try:
            print(f"\nFound device on {port}. Connecting...")
            ser = serial.Serial(port, baud, timeout=1)
            time.sleep(2)  # Wait for connection to establish
            print(f"Connected successfully to {port}")
            return ser, port
            
        except serial.SerialException as e:
            print(f"Failed to connect to {port}: {e}")
            print(f"Retrying in {retry_interval} seconds...")
            time.sleep(retry_interval)

def main():
    BAUD = 115200
    RETRY_INTERVAL = 2  # seconds between retry attempts
    
    print("Time Sync Service")
    print("Searching for device...")
    print("Press Ctrl+C to stop\n")
    
    ser = None
    port = None
    
    try:
        # Initial connection
        ser, port = connect_to_device(BAUD, RETRY_INTERVAL)
        print("Sending time to microcontroller...\n")
        
        while True:
            try:
                current_time = datetime.now().strftime("%H:%M:%S")
                ser.write((current_time + '\n').encode())
                print(f"Sent: {current_time}", end='\r')
                time.sleep(1)
                
            except serial.SerialException:
                # Connection lost
                print("\n\nConnection lost! Attempting to reconnect...")
                if ser and ser.is_open:
                    ser.close()
                
                # Try to reconnect
                ser, port = connect_to_device(BAUD, RETRY_INTERVAL)
                print("Reconnected! Resuming time sync...\n")
                
    except KeyboardInterrupt:
        print("\n\nStopped by user")
    finally:
        if ser and ser.is_open:
            ser.close()
            print(f"Closed connection to {port}")

if __name__ == "__main__":
    main()