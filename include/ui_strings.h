#define PWDER_VERSION "v0.6 \"Repainted\""
#define CREDITS_PWDER "PWDer 0.6 \"Repainted\" >"

// English
#ifdef lang_en
#define PASSWORD_MANAGER_TITLEBAR "Passwords"
#define LOGIN_TITLEBAR "Enter password"
#define HANDBOOK_TITLEBAR "Help"
#define OPTIONS_TITLEBAR "Options"
#define SYNC_TITLEBAR "Synchronization"
#define CREDITS_TITLEBAR "About"
#define PWIMPORT_TITLEBAR "New passwords found"
#define SEARCH_TITLEBAR "Search"

const String handbook[22] PROGMEM = {
    "ESC: Exit", "< Navigate >",
    "Sometimes you need to use", "FN with ESC and arrow keys",
    "H: Open this handbook", "OK: Input with default mode",
    "1: Enter login", "2: Enter password",
    "3: Login-TAB-Password-Enter", "4: Enter OTP if available",
    "T: Press TAB on computer", "R: Press ENTER on computer",
    "V: Preview credential", "Q: Search",
    "B: Connect Bluetooth", "N: Connect Wi-Fi",
    "L: Lock device", "M: Mute speaker",
    "O: Open options", "C: Open About page",
    "FN + Top button:", "Take a screenshot"
};

#define OPTIONS_BRIGHTNESS "Brightness:"
#define OPTIONS_BRIGHTNESS_PERCENTAGE "%"
#define OPTIONS_DEFAULT_USERNAME "Default input mode: username"
#define OPTIONS_DEFAULT_PASSWORD "Default input mode: password"
#define OPTIONS_DEFAULT_FULL "Default input mode: full"
#define OPTIONS_AUTOLOCK_ON "Inactivity lock: On"
#define OPTIONS_AUTOLOCK_OFF "Inactivity lock: Off"
#define OPTIONS_COLOR_SCHEME "Theme:"
#define OPTIONS_DEVICE_PASSWORD "Password:"
#define OPTIONS_RTC_TIME "RTC Time: "
#define OPTIONS_RTC_NTPSYNC "Press OK to sync with NTP"
#define OPTIONS_RTC_NTPSYNC_RTC_UNAVAILABLE "Sync unavailable: No RTC"
#define OPTIONS_SSID "Wi-Fi SSID:"
#define OPTIONS_WIFI_PASSWORD "Wi-Fi Key:"

#define SYNC_WIFI_CONNECTING_PHASE "Connecting to Wi-Fi"
#define SYNC_SERVER_CONNECTING_PHASE "Connecting to server"
#define SYNC_DOWNLOAD_PHASE "Downloading data"
#define SYNC_OK "Sync OK, disconnecting"
#define SYNC_RETURN "Returning in 3 seconds"
#define SYNC_ERR "Sync error:"
#define SYNC_ERR_DESCRIPTION_WIFI "Wi-Fi error"
#define SYNC_ERR_DESCRIPTION_SERVER "Server error"
#define SYNC_ERR_DESCRIPTION_FILE "File error"

#define CREDITS_WEBSITE "< Visit my website!"
#define CREDITS_PWDER_GITHUB "adamecki/PWDer on GitHub"
#define CREDITS_WEBSITE_LINK "floriano.uk"

#define PWIMPORT_REPLACE_PASSWORDS "Replace current passwords?"

#define SEARCH_NO_RESULTS "No results"

#define SDCARD_NOT_FOUND_TITLE "Error"
#define SDCARD_NOT_FOUND_DESCRIPTION "SD card not found!"
#define CONNECTION_LIMIT_TITLE "Can't establish this connection"
#define CONNECTION_LIMIT_DESCRIPTION "Restart PWDer to activate"

#define SAMPLE_ENTRY "Sample Entry"
#define SAMPLE_USERNAME "sample_user"
#define SAMPLE_PASSWORD "sample_password"
#endif

// Polish
#ifdef lang_pl
#define PASSWORD_MANAGER_TITLEBAR "Moje hasla"
#define LOGIN_TITLEBAR "Podaj haslo"
#define HANDBOOK_TITLEBAR "Pomoc"
#define OPTIONS_TITLEBAR "Opcje"
#define SYNC_TITLEBAR "Synchronizacja"
#define CREDITS_TITLEBAR "O programie"
#define PWIMPORT_TITLEBAR "Znaleziono nowe hasla"
#define SEARCH_TITLEBAR "Wyszukaj"

const String handbook[22] PROGMEM = {
    "ESC: Wyjdz", "< Nawiguj >",
    "Czasami z przyciskami strzalek", "oraz ESC nalezy uzyc FN",
    "H: Otworz tego pomocnika", "OK: Wprowadz zgodnie z ust. dom.",
    "1: Wprowadz login", "2: Wprowadz haslo",
    "3: Login-TAB-Haslo-Enter", "4: Wprowadz OTP jesli dostepne",
    "T: Wcisnij TAB na komputerze", "R: Wcisnij ENTER na komputerze",
    "V: Podglad wpisu", "Q: Wyszukaj",
    "B: Polacz Bluetooth", "N: Polacz Wi-Fi",
    "L: Zablokuj urzadzenie", "M: Wycisz dzwiek",
    "O: Otworz ustawienia", "C: Otworz \"O programie\"",
    "FN + Gorny przycisk:", "Zrzut ekranu"
};

#define OPTIONS_BRIGHTNESS "Jasnosc:"
#define OPTIONS_BRIGHTNESS_PERCENTAGE "%"
#define OPTIONS_DEFAULT_USERNAME "Domyslnie wprowadz: nazwe uzytkownika"
#define OPTIONS_DEFAULT_PASSWORD "Domyslnie wprowadz: haslo"
#define OPTIONS_DEFAULT_FULL "Domyslnie wprowadz: wszystko"
#define OPTIONS_AUTOLOCK_ON "Blokuj gdy nieaktywny: Tak"
#define OPTIONS_AUTOLOCK_OFF "Blokuj gdy nieaktywny: Nie"
#define OPTIONS_COLOR_SCHEME "Motyw:"
#define OPTIONS_DEVICE_PASSWORD "Haslo glowne:"
#define OPTIONS_RTC_TIME "Czas RTC: "
#define OPTIONS_RTC_NTPSYNC "OK - synchronizacja z NTP"
#define OPTIONS_RTC_NTPSYNC_RTC_UNAVAILABLE "Sync. niedostepna: Brak RTC"
#define OPTIONS_SSID "SSID Wi-Fi:"
#define OPTIONS_WIFI_PASSWORD "Haslo Wi-Fi:"

#define SYNC_WIFI_CONNECTING_PHASE "Laczenie z Wi-Fi"
#define SYNC_SERVER_CONNECTING_PHASE "Laczenie z serwerem"
#define SYNC_DOWNLOAD_PHASE "Pobieranie danych"
#define SYNC_OK "Synchr. OK, rozlaczanie"
#define SYNC_RETURN "Powrot za 3 sekundy"
#define SYNC_ERR "Blad synchronizacji:"
#define SYNC_ERR_DESCRIPTION_WIFI "Problem z Wi-Fi"
#define SYNC_ERR_DESCRIPTION_SERVER "Problem z serwerem"
#define SYNC_ERR_DESCRIPTION_FILE "Problem z plikiem"

#define CREDITS_WEBSITE "< Zobacz moja strone! >"
#define CREDITS_PWDER_GITHUB "adamecki/PWDer na GitHub"
#define CREDITS_WEBSITE_LINK "floriano.uk"

#define PWIMPORT_REPLACE_PASSWORDS "Zastapic obecne hasla?"

#define SEARCH_NO_RESULTS "Brak wynikow"

#define SDCARD_NOT_FOUND_TITLE "Blad"
#define SDCARD_NOT_FOUND_DESCRIPTION "Nie znaleziono karty SD!"
#define CONNECTION_LIMIT_TITLE "Nie mozna nawiazac polaczenia"
#define CONNECTION_LIMIT_DESCRIPTION "Zrestartuj PWDer, by aktywowac"

#define SAMPLE_ENTRY "Przykladowy wpis"
#define SAMPLE_USERNAME "przykladowa_nazwa_uzytkownika"
#define SAMPLE_PASSWORD "przykladowe_haslo"
#endif
