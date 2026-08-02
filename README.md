Regulowany tłumik RF na module PE4302 z obsługą przez WWW.

<img width="1403" height="812" alt="IMG_20260802_221311979_HDR" src="https://github.com/user-attachments/assets/17080766-6ba0-4c7e-843c-c470c9f32086" />


Przy pomiarach torów w.cz bardzo często operujemy poziomami sygnałów które są zbyt wysokie. Najprostszym sposobem aby obniżyć poziom jest użycie tłumika. O ile stałe tłumiki są  dobrym rozwiązaniem ale nieraz podczas pomiaru trzeba zmienić poziom tłumienia. Przełączanie ręczne sprawia dużo problemów i w takim przypadku najlepszym rozwiązaniem jest tłumik regulowany. Budowanie tłumików na rezystorach jest możliwe ale zakup dobrych rezystorów 1% jest utrudnione i może okazać się, ze zbudowanie przełączanego  tłumika będzie kosztowne. I tu pomocny okazuje się układ tłumika regulowanego PE4302 firmy Peregrine Semiconductor Corp. Charakteryzuje się dobrymi parametrami, pracą do 4GHz i dwoma sposobami sterowania: szeregowy i równoległy. Postanowiłem wykorzystać ten moduł to zbudowania regulowanego tłumika. Jako platformę sterującą wykorzystałem moduł ESP32 a do wizualizacji i sterowania moduł zawierający wyświetlacz OLED 1.3” i enkoder obrotowy z przyciskiem. Jako, że zakupiony moduł ma wyprowadzony tylko interfejs równoległy wymusiło to na mnie taki rodzaj sterowania.

Tłumik regulowany PE4302 firmy Peregrine Semiconductor Corp

<img width="307" height="180" alt="pe4302" src="https://github.com/user-attachments/assets/778e801e-5b88-4718-82ba-583aed862139" />

PE4302 to 6-bitowy cyfrowy tłumik skokowy RF (DSA) o wysokiej liniowości, obejmujący zakres tłumienia 31,5 dB w krokach co 0,5 dB. Zapewnia zarówno równoległy, jak i szeregowy interfejs sterowania CMOS, działa na pojedynczym zasilaniu 3 V i utrzymuje wysoką dokładność tłumienia w funkcji częstotliwości i temperatury. Posiada również unikalny interfejs sterowania, który pozwala użytkownikowi wybrać początkowy stan tłumienia po włączeniu zasilania. PE4302 charakteryzuje się bardzo niskim tłumieniem wtrąceniowym i niskim poborem mocy. Cały układ jest umieszczony w obudowie QFN o wymiarach 4x4 mm. PE4302 jest produkowany w technologii UltraCMOS firmy Peregrine, opatentowanej odmianie technologii krzemu na izolatorze (SOI) na podłożu szafirowym, oferującej wydajność GaAs z ekonomią i integracją konwencjonalnych CMOS.



Podstawowe parametry:

•	Tłumienie: co 0,5 dB do 31,5 dB

•	Dokładność: do 1GHz +/-0.1dB +3%, powyżej 1GHz +/-0.15dB +5% 

•	Pasmo przenoszenia: od DC do 4 GHz

•	IP3 wejściowe: 52dB

•	Maksymalna moc wchodząca: +24dBm

•	Strata wtrąceniowa: 1.5dB

•	Szybkość przełączania: 1uS

•	Napięcie robocze: 3V

•	Pobór prądu: 100uA


Schemat wewnętrzny:
<img width="360" height="146" alt="pe_inside" src="https://github.com/user-attachments/assets/4faffbb2-3f5d-450c-a911-4561fbcb173c" />

Schemat podstawowy układu PE4302:
<img width="394" height="296" alt="pe_sch" src="https://github.com/user-attachments/assets/7f5085e1-85b1-495d-aad9-5003352417be" />

Charakterystyka tłumienia w paśmie:
<img width="394" height="346" alt="pe_att" src="https://github.com/user-attachments/assets/0d16fabf-d45c-4ff1-bd80-44fda46f370d" />


Elementy wykorzystane do budowy tłumika:

<img width="307" height="180" alt="pe4302" src="https://github.com/user-attachments/assets/ba0e1183-cd93-457a-9bd9-2211fca3a28a" />
<img width="198" height="143" alt="esp32" src="https://github.com/user-attachments/assets/b633d68c-8b7a-43be-9c04-175e9d65ac14" />
<img width="188" height="130" alt="oled" src="https://github.com/user-attachments/assets/8ff19fd3-33d3-41ef-8a28-c8bf03989caf" />


Opis układu tłumika.
Układ jest prosty. Sterowanie tłumika połączone jest z ESP32 ośmioma przewodami: zasilanie, masa i 6 sygnałów sterujących. Wyświetlacz OLED pracuje na magistrali i2c więc oprócz zasilania i masy wymaga tylko dwóch przewodów. Wraz z wyświetlaczem na płytce jest zainstalowany enkoder, on podłączony jest trzema przewodami: dwa do impulsów enkodera i jeden do przycisku, masa jest wykorzystana z masy wyświetlacza.
PE4302:

•	GPIO25 -> C0.5

•	GPIO26 -> C1

•	GPIO27 -> C2

•	GPIO32 -> C4

•	GPIO33 -> C8

•	GPIO23 -> C16

OLED:

•	GPIO21 -> SDA

•	GPIO22 -> SCL

Enkoder:

•	GPIO19 -> A 

•	GPIO18 -> B

•	GPIO5 -> SW 

Do wyświetlacza OLED podłączyłem napięcie 5V a do modułu PE4302 napięcie 3V z modułu ESP32.


Przed kompilacją należy ustawić login i hasło do sieci wifi, linia 42 i 43:

String wifiSSID     = "SSID";

String wifiPassword = "password";

Parametry te można po uruchomieniu z poziomu strony WWW zmienić.

Obsługa tłumika.
Po włączeniu zasilania na wyświetlaczu pojawia się poziom tłumienia. Obracanie pokrętłem enkodera powoduje zmianę tłumienia o 0,5dB. Naciśnięcie przycisku w enkoderze powoduje wyzerowanie tłumika. W dolnej linii na wyświetlaczu widać adres IP pod którym dostępna jest strona WWW do obsługi tłumika.

<img width="607" height="343" alt="www" src="https://github.com/user-attachments/assets/6d0c2d6f-6ffc-45e4-971c-0f7cea69d0fa" />


Strona WWW wyświetla poziom tłumienia, umożliwia wpisanie ręczne wartości tłumienia, zmianę z krokiem co 0,5dB jak i wyzerowanie tłumienia.  Dodatkowo mamy 64 przyciski wpisania „szybkiej wartości” tłumienia. Kliknięcie w przycisk od razu wpisze odpowiednią wartość tłumienia.  
W dolnej części znajduje się okno z możliwością wpisania innej sieci wifi i hasła do niej.


Całość została umieszczona w wydrukowanej na drukarce 3d obudowie z materiału PET-G. Z boku obudowy znajdują się dwa gniazda BNC podłączone do płytki tłumika. Całość zasilana jest z zasilacza 5V poprzez gniazdo USB-C z tyłu obudowy. Cały układ pobiera poniżej 1W mocy.



Opracowanie:

Artur SP3VSS

http://sp3vss.eu
