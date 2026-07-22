# cryptography libs
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import hashes

# KeePass database support
from pykeepass import PyKeePass
from urllib.parse import unquote

# secure password input
from getpass import getpass

# data structure support
from ctypes import *
import struct

# communication
import serial
import serial.tools.list_ports
import time
import sys
import os

# definitions
MAX_ENTRIES = 101
MAX_LENGTH = 128

SALT_SIZE = 16
ITER_SIZE = 4

if sys.platform == "darwin":
    PWROTOCOL_SERIAL_PREFIX = "/dev/cu"
elif sys.platform == "win32":
    PWROTOCOL_SERIAL_PREFIX = "COM"
else:
    PWROTOCOL_SERIAL_PREFIX = "/dev/ttyACM"

PWROTOCOL_IMPORT_MAGIC      = "PWIMPORT"
PWROTOCOL_HANDSHAKE_SUCCESS = b"OK"

PWROTOCOL_PING              = b'\x01'
PWROTOCOL_GET_SALT          = b'\x02'
PWROTOCOL_GET_ITERATIONS    = b'\x03'
PWROTOCOL_FILE_IMPORT       = b'\x04'
PWROTOCOL_REQUEST_CONF      = b'\x05'

PWROTOCOL_TIMEOUT_S         = 10
PWROTOCOL_BAUDRATE          = 115200
PWROTOCOL_CHUNK_SIZE        = 64

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class Credential(Structure):
    _pack_ = 1
    _layout_ = "ms"
    _fields_ = [
        ("title", c_char * MAX_LENGTH),
        ("username", c_char * MAX_LENGTH),
        ("password", c_char * MAX_LENGTH),
        ("totp_secret", c_char * MAX_LENGTH),
    ]

class Vault(Structure):
    _pack_ = 1
    _layout_ = "ms"
    _fields_ = [
        ("credential_count", c_uint16),
        ("data", Credential * MAX_ENTRIES)
    ]

pwdstring = ''
vault = Vault()

vault.credential_count = 0

def smartSubstring(prefix, suffix, value):
    if not value:
        return None

    start = value.find(prefix)
    if start == -1:
        return None
    start += len(prefix)
    end = value.find(suffix, start)

    return value[start:] if end == -1 else value[start:end]

def find_pwder():
    print(bcolors.HEADER + "Looking for PWDer..." + bcolors.ENDC)
    for port in serial.tools.list_ports.comports():
        if PWROTOCOL_SERIAL_PREFIX not in port.device:
            continue
        try:
            print(f'Trying {port.device}')
            pwder = serial.Serial(
                port.device,
                baudrate=PWROTOCOL_BAUDRATE,
                timeout=PWROTOCOL_TIMEOUT_S
            )

            pwder.reset_input_buffer()
            pwder.reset_output_buffer()

            pwder.write(PWROTOCOL_PING)
            reply = pwder.read(len(PWROTOCOL_HANDSHAKE_SUCCESS))

            pwder.reset_input_buffer()
            pwder.reset_output_buffer()

            if reply == PWROTOCOL_HANDSHAKE_SUCCESS:
                print(bcolors.OKGREEN + f'Found PWDer on {port.device}' + bcolors.ENDC)
                return pwder

            pwder.close()

        except serial.SerialException:
            pass

    return None

def send_chunked(ser: serial.Serial, data: bytes):
    to_send = len(data)
    sent = 0

    while sent < to_send:
        chunk = data[sent:sent + PWROTOCOL_CHUNK_SIZE]
        ser.write(chunk)
        ser.flush()

        response = ser.read(len(PWROTOCOL_HANDSHAKE_SUCCESS))

        if len(response) != len(PWROTOCOL_HANDSHAKE_SUCCESS):
            exit(1)

        if response != PWROTOCOL_HANDSHAKE_SUCCESS:
            exit(1)

        sent += PWROTOCOL_CHUNK_SIZE

###
### Title screen
###

print(bcolors.HEADER + bcolors.BOLD + "=== PWDer KDBX Synchronization Tool ===" + bcolors.ENDC)
print("| Version 0.4")
print("| by @adamecki")
print()

### Input validation
if len(sys.argv) < 2:
    print(bcolors.FAIL + "E: KDBX file path not specified!" + bcolors.ENDC)
    exit(1)
if os.path.exists(sys.argv[1]) == False:
    print(bcolors.FAIL + "E: KDBX file specified does not exist!" + bcolors.ENDC)
    exit(1)

### Open kdbx
print(bcolors.HEADER + f'Opening KDBX file {sys.argv[1]}...' + bcolors.ENDC)
keepass_password = getpass()

try:
    db = PyKeePass(sys.argv[1], password=keepass_password)
except:
    print(bcolors.FAIL + "E: Error opening KDBX file. The password is incorrect or the specified file might be corrupt." + bcolors.ENDC)
    exit(1)

print(bcolors.OKGREEN + "KDBX file opened successfully." + bcolors.ENDC)
print()

### Find PWDer
print(bcolors.HEADER + "Attempting serial connection with PWDer." + bcolors.ENDC)
print("Please connect PWDer to your computer and then press Enter...", end='')
input()
print()

pwder = find_pwder()
if pwder is None:
    print(bcolors.FAIL + "E: No PWDer found!" + bcolors.ENDC)
    exit(1)

### Get config
pwder.write(PWROTOCOL_REQUEST_CONF)
credential_bytes = pwder.read(sizeof(Credential))
pwder.reset_input_buffer()
pwder.reset_output_buffer()
credential = Credential.from_buffer_copy(credential_bytes)
vault.data[0].title = credential.title
vault.data[0].username = credential.username
vault.data[0].password = credential.password
vault.data[0].totp_secret = credential.totp_secret

### Get iteration number
pwder.write(PWROTOCOL_GET_ITERATIONS)
iterations_bytes = pwder.read(ITER_SIZE)
pwder.reset_input_buffer()
pwder.reset_output_buffer()
iterations = struct.unpack("<I", iterations_bytes)[0]

### Get salt
pwder.write(PWROTOCOL_GET_SALT)
salt = pwder.read(SALT_SIZE)
pwder.reset_input_buffer()
pwder.reset_output_buffer()

print()

### Derive key
print(bcolors.HEADER + "Please enter your PWDer password." + bcolors.ENDC)
print(bcolors.WARNING + "The password is not checked. Entering an incorrect password will result in an import failure." + bcolors.ENDC)

pwder_password = getpass().encode("utf-8")
print()

print(bcolors.HEADER + "Deriving AES128 key... " + bcolors.ENDC, end='')

kdf = PBKDF2HMAC(
    algorithm=hashes.SHA256(),
    length=16,
    salt=salt,
    iterations=iterations,
)

key = kdf.derive(pwder_password)

print(bcolors.OKGREEN + "OK" + bcolors.ENDC)
print()

### Fill vault with data
print(bcolors.HEADER + "Reading KDBX data... " + bcolors.ENDC, end='')

i = 0
for entry in db.entries:
    if i < 100:
        vault.credential_count += 1

        if not entry.title:
            vault.data[i+1].title = bytes("", 'utf-8')
        else:
            vault.data[i+1].title = bytes(entry.title, 'utf-8')

        if not entry.username:
            vault.data[i+1].username = bytes("", 'utf-8')
        else:
            vault.data[i+1].username = bytes(entry.username, 'utf-8')

        if not entry.password:
            vault.data[i+1].password = bytes("", 'utf-8')
        else:
            vault.data[i+1].password = bytes(entry.password, 'utf-8')

        otp = smartSubstring("secret=", "&", f'{entry.otp}')
        if otp:
            otp = unquote(otp)
            vault.data[i+1].totp_secret = bytes(otp, 'utf-8')
        else:
            vault.data[i+1].totp_secret = bytes("", 'utf-8')
    else:
        break
    i += 1

print(bcolors.OKGREEN + "OK" + bcolors.ENDC)
print()

### Encrypt vault data
print(bcolors.HEADER + "Encrypting data in PWDer-friendly format... " + bcolors.ENDC, end='')

plaintext = bytes(vault)
nonce = os.urandom(12)

aes = AESGCM(key)
encrypted = aes.encrypt(
    nonce,
    plaintext,
    None
)
ciphertext = encrypted[:-16]
tag = encrypted[-16:]

print(bcolors.OKGREEN + "OK" + bcolors.ENDC)
print()

### Send to PWDer
print(bcolors.HEADER + "Sending data to PWDer... " + bcolors.ENDC, end='')

pwder.write(PWROTOCOL_FILE_IMPORT)
pwder.read(2)

pwder.write(b"PWIMPORT")
pwder.read(2)

pwder.write(nonce)
pwder.read(2)

pwder.write(struct.pack("<I", len(ciphertext)))
pwder.read(2)

send_chunked(pwder, ciphertext)

pwder.write(tag)
pwder.read(2)

print(bcolors.OKGREEN + "OK" + bcolors.ENDC)
print()
print(bcolors.OKGREEN + "Vault sent to PWDer. Follow the instructions on PWDer's screen. Goodbye." + bcolors.ENDC)
