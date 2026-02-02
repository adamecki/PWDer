from pykeepass import PyKeePass
from getpass import getpass
from http.server import BaseHTTPRequestHandler, HTTPServer
import time
import sys
import os
import socket

def getLocalIP():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("1.1.1.1", 80))
    localIP = s.getsockname()[0]
    s.close()
    return localIP

hostname = getLocalIP()
http_port = 2137
pwdstring = ''

class PwderServer(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()
        self.wfile.write(bytes(pwdstring, "utf-8"))

if(len(sys.argv) < 2):
    print('No database file specified!')
else:
    if(sys.argv[1] == '--help'):
        print(f'Syntax: python3 {sys.argv[0]} <database_file|--help> [port HTTP|--genfile]')
        print('Options:')
        print("\t--help - shows this page")
        print("\t--genfile - creates a password file instead of network sharing")
    else:
        if(os.path.exists(sys.argv[1]) == False):
            print('Specified file does not exist!')
        else:
            print(f'Opening file {sys.argv[1]}...')
            pwd = getpass()
            try:
                db = PyKeePass(sys.argv[1], password=pwd)
                i = 0
                for entry in db.entries:
                    if i != 0:
                        pwdstring += '\n'
                    # Max 100 passwords
                    if i < 100:
                        pwdstring += entry.title + '\n'
                        pwdstring += entry.username + '\n'
                        pwdstring += entry.password
                    i += 1
                if(len(sys.argv) > 2 and sys.argv[2] == '--genfile'):
                    print('Creating a file...')

                    with open("pwimport", "w") as pwimport:
                        pwimport.write(pwdstring)

                    print('File is available in the script directory under the name pwimport.')
                    print('Copy it to the SD card root directory and start up the Cardputer to import data.')

                    print('Program finished successfully.')
                else:
                    print('Starting server...')

                    # Check for custom port
                    if(len(sys.argv) > 2):
                        http_port = int(sys.argv[2])

                    webServer = HTTPServer((hostname, http_port), PwderServer)
                    print(f'Server is available under the address http://{hostname}:{http_port}.')
                    print('Data is available for synchronization. To stop, press Ctrl+C.')

                    try:
                        webServer.serve_forever()
                    except KeyboardInterrupt:
                        pass
                    
                    webServer.server_close()
                    print('Program finished successfully.')
            except:
                print('An error occured! Possible causes include: incorrect password, incorrect format of the database or wrong HTTP port.')
