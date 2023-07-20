# Würfel 2.0

Projekt ESP8266 - D1 mini Matrix-Würfel mit Webserver

Umfasst eine LED Matrix, 3 Taster (davon 2 frei programmierbar) und eine einfache Website, welche leicht mit Funktionen erweitert werden kann.

## Dokumentation

### Einschalten

Das Einschalten des Geräts erfolgt mit dem linken Taster.

Der Controller versorgt sich über eine [Selbsthaltungs-Schaltung](https://github.com/sixteenlines/dice-2/blob/production/self-retain.jpg?raw=true) und schaltet so nach einiger Zeit (einstellbar über Webinterface) automatisch ab.

Ein manueller Reset ist über den Reset pin auf der Unterseite des Gehäuses möglich.

### AP-Mode

Beim einschalten versucht der Controller sich mit den bekannten Daten in einem bestehendem WLAN als Client anzumelden. Misslingt dies nach 3 Versuchen leuchtet ein roter Punkt in der Matrix auf und der Controller schaltet wieder ab.

Um die Anmeldedaten zu bearbeiten, kann das Gerät im Wifi-Manager-Modus gestartet werden. Hierzu wird beim Einschalten der mittlere Taster 5 Sekunden lang gedrückt gehalten. Der ESP hostet nun einen Access-Point und wird sichtbar als "MAGIC-DICE-SETUP".

Bei der Verbindung versucht der Controller DNS Anfragen auf ein Captive Portal umzuleiten und die Dateneingabe Maske öffnet sich automatisch. Gelingt dies nicht, kann der Wifi-Manager über die [8.8.8.8](8.8.8.8) erreicht werden.

Nach absenden des Formulars startet der Controller neu, bei erfolgreicher Verbindung im WLAN leuchtet ein grüner Punkt in der Matrix auf.

### Client-Mode

Wurde im Formular eine statische IP vergeben, kann man das Webinterface unter der eingestellten IP erreichen.

Wird DHCP verwendet kann die erhaltene IP über die [serielle Schnittstelle](#serielle-schnitstelle) ausgelesen werden.

Es kann nun über betätigen des linken Tasters oder den Button im Webinterface gewürfelt werden.

Ebenso is im Webinterface die Matrix als <div> Container widergespiegelt. Unter der Matrix kann die Farbe zum Würfeln eingestellt werden. Außerdem können individuelle LEDs angeklickt werden um sie in der gewählten Farbe aufleuchten zu lassen. Wiederholtes klicken schaltet sie wieder aus.

Über das Einstellungsmenü kann die Dauer der Beleuchtung und die Zeit bis der Controller abschaltet, konfiguriert werden.

Ebenso findet sich hier der WiFi-Manager wieder, falls in ein anderes Netzwerk gewechselt werden soll. Nach Eingabe muss jedoch der Controller erst [neu gestartet](#einschalten) werden.

### Serielle Schnitstelle

Verbindet man das Gerät über USB, kann mithilfe eines Terminalprogramms Debugging Informations und von DHCP erhaltene IP-Adressen ausgelesen werden.

Der Controller sendet standardmäßig mit einer Baudrate von 115200.

Zur Fehlerdiagnose ist Empfohlen den Controller zu verbinden, die entsprechende serielle Schnittstelle zu öffnen und zunächst einen [reset](#einschalten) durchzuführen.

Es werden nun sämtliche Schritte des Bootvorgangs gespiegelt. Außerdem werden Anfragen an den Webserver gelogt.

## Repo Übersicht

### cad

Beinhaltet KiCad Projektdaten - Schaltplan und Platinenlayout.

### data

Beinhaltet Files für das Dateisystem

- HTML Seiten
- CSS Stylesheets
- Icons und Bilder
- Textdateien
  - creds (SSID, Passwort, etc)
  - settings (Timeoutzeiten)

### src

Quellcode

- header_main.hpp - Macros und Header
- main.cpp - Microcontroller Programmroutinen

## IDE & Framework

Das Projekt wurde in der [PlatformIO](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) Entwicklungsumgebung und mithilfe des Arduino Frameworks in Visual Studio Code realisiert. Die Erweiterung bezieht automatisch alle benötigten Libraries, welche in der [platformio.ini](https://github.com/sixteenlines/dice-2/blob/production/platformio.ini) hinterlegt sind.

In dieser werden auch die Einstellungen für den seriellen Monitor oder die entsprechenden Flags für den Programmer gesetzt.