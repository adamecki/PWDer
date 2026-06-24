# PWDer
PWDer is a simple password manager for the M5Cardputer. Its name is a combination of a few words:
- PWD (password)
- Manager
- Cardputer

and in order to not break your tongue, you can pronounce it as "Powder".

<img src="./photos/main.webp" alt="main" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

It started as an idea to solve the problem of the meticulous process of logging into personal accounts on different public computers (like school or work), where you don't have your password manager. It simulates a USB keyboard, entering the passwords for you into every computer you plug it in to. I thought of portable / cloud password managers only when I was halfway through doing this project, so I decided to continue it.
# Key functionalities
- Storing up to 100 passwords using AES128 encryption
- Protection with master password
- Searching for passwords by entry name
- Automatic password input to your computer using USB cable
- Synchronizing with .kdbx (KeePass) files over network\* or via SD card
- Support for TOTP two-factor authentication
- Fun, simple design

\* Network transfer is not yet encrypted - its usage is highly discouraged unless you're the only user of your local network. Synchronize at your own risk.
# Setting it up
## Prerequisites
- I highly recommend using [Launcher](https://github.com/bmorcelli/Launcher) for running the program. However, it can be flashed and used directly too.
- A .kdbx database is required to import passwords into the Cardputer. Your computer also has to be able to run Python scripts. (This requirement will disappear once I add manual password entering)
## Step 1: Preparing the SD card
- Format an SD card using the **FAT32** file system
## Step 2: Installing PWDer on the Cardputer
- Set up an Arduino IDE for [programming the Cardputer](https://docs.m5stack.com/en/arduino/m5cardputer/program)
- Download the source code and open the `PWDer.ino` file with Arduino IDE
- Open up `enckey.cpp` and set the value of `char* enckey` to a set of 16 random lowercase letters
- Flash the program onto the Cardputer or Export Compiled Program (if you're going to use Launcher) by pressing Ctrl+Alt+S
- *Optional: If you chose the second option, move the exported .bin file to the SD card*
- Make sure the SD card is in, then turn on the Cardputer
## Step 3: First run
- If you're using launcher, install the .bin file. Otherwise, wait until the program asks for the password.
- The first run password is "default". Enter it and press OK, you'll see the "Nothing" screen - the default and only password entry if the passwords file was nonexistent.
## Step 4: Importing the passwords for the first time
- Prepare your .kdbx database
- Open your terminal in `pwder_keepass_sync` directory
- Create a Python virtual environment and install dependencies
```bash
python3 -m venv .
source ./bin/activate
pip install pykeepass
```
(next time you try to use the script, just enter `source ./bin/activate`)
- Run the script
```bash
python3 main.py /your/database.kdbx --genfile
```
- A `pwimport` file will appear. Move it to the root directory of your SD card and insert the card into the Cardputer.
- Turn on or reset the Cardputer. It will prompt you to import the passwords. Press Y.
- Otherwise, you can just create the `pwimport` file manually with following syntax
```
title_1
username_1
password_1
otp_secret_1 (or empty if you don't have / don't want to use OTP)
title_2
username_2
password_2
otp_secret_2 (or empty)
title_n
username_n
password_n
otp_secret_n (or empty)
```
- This won't be possible once encrypted password importing will be implemented. Make sure `\n` (LF) is being used (and not `\r\n` (CR LF)).
# Usage
## Main screen
This is where you can select a password to enter.

<img src="./photos/main.webp" alt="main" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

Press the arrow keys to navigate, and hold down V to preview the username and password you're about to enter. Connect the Cardputer to your computer and do one of these things:
- `1` - enter the username for currently selected entry
- `2` - enter the password
- `3` - enter the username, then press TAB, then enter the password, and then press Enter
- `4` - enter current TOTP
- `Enter` - perform one of these three actions, depending on the selection made in Options.
- `V` - hold to preview username, password and TOTP
- `T` - simulate pressing TAB on the computer.
- `R` - simulate pressing Enter on the computer.
- `M` - mute or unmute the speaker. The default state for the speaker is muted.
- `L` - lock the device. Alternatively, you can just reset the device.
- `S` - attempt to synchronize the passwords over the network.
- `O` - open Options.
- `C` - open Credits.
- `Q` - open Search menu (press Fn+Esc to exit)

## TOTP
### Introduction
PWDer now supports two-factor authentication using six-digit one time password.

Neither ESP32, nor Cardputer do have an RTC backup battery to keep the time correct even when the device is powered off. Currently, there are two options to use time-based one-time passwords with PWDer.

### First solution: M5Unit RTC
If you have an M5Unit RTC, connect it to Cardputer's Grove connector before starting PWDer. It will detect the unit and display a small clock icon in the top right corner. It means that Cardputer is using RTC for generating TOTP.
<img src="./photos/totpishere.webp" alt="TOTP is here" width="40%">

Currently, there is no way to set time of the RTC directly from PWDer. However, in the future, I will introduce both manual time adjustment and setting the RTC time via NTP.

*Notice: Don't connect any other units to the Grove connector while PWDer is running. It hasn't been tested, but Cardputer might read it incorrectly as an M5Unit RTC and give incorrect time-based one-time passwords.

### Second solution: NTP network time
The Cardputer can to connect to an NTP server via Wi-Fi in order to synchronize time, so it can generate a TOTP. For now, the default pool.ntp.org NTP server for the NTP client library is used, but in the future there will be a way to change the NTP server directly from PWDer's UI. For now the only way to change an NTP server is editing the code.
<img src="./photos/m5unitrtc.webp" alt="M5Unit RTC attached to a Cardputer" width="40%">

With that said, even if you have a TOTP secret in your database, PWDer won't show the TOTP if it doesn't have a time provider. If both RTC and NTP are available, Cardputer uses RTC as the primary time provider, as it is more reliable (works offline).

### How to add a TOTP secret to my entry?
<img src="./photos/kptotp.webp" alt="KeePass TOTP settings for an entry" width="40%">
KeePassXC supports TOTP. If a secret key for your entry is set, the Python vault extraction script provided in the `pwder_keepass_sync` directory will find it and place it in your import file for PWDer. Just like passwords, it is stored on the SD card in an encrypted form (excluding the import phase for now).

Otherwise, if you're writing an import file manually, you can add the secret key as the fourth line of each entry, as described earlier.

### Where do I find the TOTP in PWDer's UI?
<img src="./photos/totpishere.webp" alt="TOTP is here" width="40%">
If the NTP requirements are met and your entry has a TOTP secret set, the entry title will be blue. After that, you can hold `v` to display the one time password: it will show on the right side of your username, or press `4` to enter it to your computer.

## Lock screen
Here you have to enter the correct password (then press OK) to access the device. If you've locked yourself out, you can remove the `pwder/config` file from the SD card (the password will be "default" again), but keep in mind that all your saved passwords and configuration will disappear!

<img src="./photos/lock.webp" alt="lock" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

## Help screen
A help screen is a quick and complex guide to all the keybindings. Press the arrow keys / esc to navigate.

<img src="./photos/help.webp" alt="help" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

## Options screen
Press O to open the options screen.

<img src="./photos/opts1.webp" alt="opts2" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

Press enter to switch between default password input modes.

<img src="./photos/opts2.webp" alt="opts2" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

On pages that require complex typing (so basically anything else than switching the default password input mode), you need to hold FN when pressing arrow keys / esc to navigate.

<img src="./photos/opts3.webp" alt="opts3" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

<img src="./photos/opts4.webp" alt="opts4" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

<img src="./photos/opts5.webp" alt="opts5" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

After changing a property, press Enter to save it. Additionally, after changing the device password, you'll be prompred to enter the current password. Press FN+Esc or enter an incorrect password to cancel the procedure.

<img src="./photos/opts6.webp" alt="opts6" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

## Network synchronization screen
Before accessing this screen, make sure the Wi-Fi settings in Options are set correctly and that the Python server is running, otherwise the procedure will fail. It will attempt to download the password data from the network and import it. Proceed with caution as it is not encrypted!

<img src="./photos/pyscript.webp" alt="pyscript" width="40%">

Hosting an unencrypted synchronization over network.

<img src="./photos/sync1.webp" alt="sync1" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

<img src="./photos/sync2.webp" alt="sync2" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

<img src="./photos/sync3.webp" alt="sync3" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

## Credits screen
Well, credits are credits. Press the arrow keys / esc to navigate. Pressing Enter will input the link to the selected thing (preferably to the browser).

<img src="./photos/credits.webp" alt="credits" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

## Password import screen
Once Cardputer finds `/pwimport` file during startup on the SD card, this screen will be shown after entering the password. Press Y to import the passwords or N to delete the file and keep current passwords. Keep in mind that there is no appending passwords yet - when importing new passwords, everything will be overwritten with the new ones!

<img src="./photos/import.webp" alt="import" width="40%">
*This screenshot comes from an earlier version of PWDer. The user interface might look slightly different from what is shown in the picture.*

# Other languages
- Polish is available. To change a language, recompile PWDer with `#define lang_pl` instead of default `#define lang_en` in line 18 of the file `src/PWDer.ino`.

# Be careful!
While data stored on the device is encrypted, the import file and network synchronization aren't! I highly discourage using the network synchronization feature at all.
# Roadmap
In future versions, I plan to include these features:
- Manual password adding
- Exporting passwords from the device
- Encrypted password importing and synchronizing over network
- Encryption key stored in Cardputer's non-volatile memory instead of the program, and the ability to randomize/change it from options screen
# Credits
- [ESP32-Encrypt](https://github.com/josephpal/esp32-Encrypt) - a library for AES128 encryption
- [TOTP-Arduino](https://github.com/lucadentella/TOTP-Arduino) - a library for generating time-based one time passwords for 2FA
- [Arduino-Base32-Decode](https://github.com/dirkx/Arduino-Base32-Decode) - a library for handling Base32
