#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 8, 7, 6, 5);

#include <DS1302.h>
// Init the DS1302
DS1302 rtc(2, 3, 4);

const int brightnessPin = 10;     //pin to control brightness
//int brightness = 0;               //default brightness
int previousHour = 25;              //default hour

int currentMinute=0;
#define INTERVAL_SECONDS 2        //10 secondi
#define INTERVAL_MINUTES 10       //10 minuti

#define INTERVAL_WETS_TIMES 5     //numero di volte massimo che posso innaffiare la pianta in un determinato tempo

const int moisturePin=A0;         //pin per il lettore capacitivo
const int airValue=445;           //valore massimo per il sensore
const int waterValue=196;         //valore minimo per il sensore

int soilMoisture=0;               //valore inizializzato del sensore
int soilMoisturePercent=0;        //valore inizializzato per la percentuale

#define WET_VALUE 30              //Under 30% the plants gets water

const int relayPin=9;             //pin per il relay

int wetTimes = 0;                 //numero di volte che ho innaffiato

#define READINGS 100              //100 letture per la media
int lastReadings[READINGS];
int currentReading = 0;            //inizializzo la lettura corrente
int average = 0;

const int buttonPin = 13;

unsigned long currentMillis = millis();
unsigned long DELAY_IN_MILLISECONDS = 600000;   //impostato: 10 minuti   //un minuto: 60000

int buttonState = 0;              //stato del bottone

unsigned long  previousMillisAverage = 0;        //tempo passato per ogni media effettuata.
void setup() {
  
  lcd.begin(16,2);                               //inizializzo lo schermo
  Serial.begin(9600);                            //inizializzo il serial monitor

  //inizializzo il real time clock
  rtc.halt(false);    

  //inizializzo il pin per il relay
  pinMode(relayPin,OUTPUT);
  digitalWrite(relayPin,HIGH);                   

  pinMode(buttonPin, INPUT);                     //inizializzo il pin per il button


  pinMode(brightnessPin, OUTPUT);                //inizializzo il pin per la luminosità dell' LCD
  //digitalWrite(brightnessPin, HIGH);
  analogWrite(brightnessPin, 0);                 //all'inizio l'LCD ha luminosità minima

  setTime();                                     //aggiorno l'orario dell' RTC
    
  previousMillisAverage = millis();              //inizializzo i millisecondi passati 

  
}

void loop() {
  readMoisturePercent();                         //leggo dal sensore l'umidità del terreno

  int temp = getHours();                         //leggo l'ora corrente
  if(previousHour != temp){                      //controllo ogni ora
    setBrightness(whatTimeItIs());               //imposto la luminosità corrispondente
    previousHour = temp;                         //aggiorno l'ora precedente per controllare in futuro se sarà cambiata
    hourView();                                  //schermata LCD dell'ora corrente
    delay(2000);                                 //un po' di delay, so che a voi piace (-cit)
  }
  
  if(currentReading < READINGS){
    if(intervalSeconds(2)){
      addReading();
      delay(500);
    }
  }else{
    getAverage();
    currentReading = 0;
    previousMillisAverage = millis();
  }

  checkTimes();                                   //funzione che serve a controllare se in un determinato intervallo ho innaffiato la pianta piu di n volte.
  //Serial.println(currentReading);
  
  if(checkButton()){
    displayWatering();
    annaffia();
    delay(2000);
  }
  
  
  if(intervalSeconds()){
    
    displayDefault();

    //########################### BIG BRAIN STUFF ###########################
    if(soilMoisturePercent < WET_VALUE){
      if(wetTimes < INTERVAL_WETS_TIMES){         //se nell'ultimo intervallo ho innaffiato la pianta meno di n volte.
        wetTimes +=1;                             //incremento le volte che ho innaffiato la pianta
        annaffia();
      }
    }
    //########################### BIG BRAIN STUFF ###########################

    //delay(1000);
    delay(250);
  }
}
/*
brightness: 
low: 25, 50
medium: 75,100
High: 100>
*/

/*
* Riceve un valore da 0 a 4 dipendente dal momento della giornata
* oppure in modalità override
*/
void setBrightness(int dayTime){
  switch(dayTime){
    //notte
    case 0:
      analogWrite(brightnessPin,10);
      break;
    //mattina
    case 1:
      analogWrite(brightnessPin,50);
      break;
    //mezzogiorno
    case 2:
      analogWrite(brightnessPin,160);
      break;
    //pomeriggio
    case 3:
      analogWrite(brightnessPin,100);
      break;
    //sera
    case 4:
      analogWrite(brightnessPin,50);
      break;
    //qualcosa è andato storto
    default:
      analogWrite(brightnessPin,50);    //nel caso la luminosità è quella della mattina (media)
      break;
  }
}

/**
* Ritorna:
* 0 se è notte
* 1 se è mattina
* 2 se è mezzogiorno
* 3 se è pomeriggio
* 4 se è sera
* 
* Periodi:
* 23 - 8 notte
* 8 - 12 mattina
* 12 - 15 mezzogiorno
* 15 - 19 pomeriggio 
* 19 - 23 sera
*/
int whatTimeItIs(){
  int hour = getHours();
  //mattina
  if(hour == 8 || hour == 9 ||hour ==  10 ||hour ==  11){
    return 1;
  }
  if(hour == 12 || hour == 13 || hour == 14){
    return 2;
  }
  if(hour == 15 || hour == 16 || hour == 17 || hour == 18){
    return 3;
  }
  if(hour == 19 || hour == 20 || hour == 21 || hour == 22 || hour == 23){
    return 4;
  }
  if(hour == 24 || hour == 1 || hour == 2 || hour == 3 || hour == 4 || hour == 5 || hour == 6 || hour == 7){
    return 0;
  }
  
  return 1;     //non necessario
}

void printLCD(int value){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(value);
}







//########################### SCHERMATA DI DEFAULT ###########################
void displayDefault(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Hum attuale: ");
  lcd.print(soilMoisturePercent);
  lcd.print("%");

  lcd.setCursor(0,1);
  lcd.print("Media: ");
  lcd.print(average);
  lcd.print("%");
  lcd.print("  ");
  unsigned long tempMS = millis() - previousMillisAverage;


  //se i millisecondi passati dall'ultima media sono inferiori al minuto, vengono mostrati i secondi, 
  //senò vengono mostrati i minuti.
  if(tempMS < 60000){
    lcd.print((millis() - previousMillisAverage)/1000);
    lcd.print("s");
  }else{
    lcd.print((millis() - previousMillisAverage)/60000);
    lcd.print("m");
  }
  
}

//########################### SCHERMATA DI DEFAULT ###########################

//########################### SCHERMATA PER INNAFFIARE ###########################
void displayWatering(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("!! OVERRIDE !!");
  lcd.setCursor(0,1);
  lcd.print("Innaffio");
}
//########################### SCHERMATA CHE MOSTRA L'ORA CORRENTE ###########################
/*
*Mostra sull'lcd che momento della giornata è 
* e l'ora corrente.
*/
void hourView(){
  lcd.clear();
  lcd.setCursor(0,0);

  switch(whatTimeItIs()){
    //notte
    case 0:
      lcd.print("E' notte!");
      break;
    //mattina
    case 1:
      lcd.print("E' mattina!");
      break;
    //mezzogiorno
    case 2:
      lcd.print("E' mezzogiorno!");
      break;
    //pomeriggio
    case 3:
      lcd.print("E' pomeriggio!");
      break;
    //sera
    case 4:
      lcd.print("E' sera!");
      break;
    //qualcosa è andato storto
    default:
      lcd.print("!! ERRORE !!");
      break;
  }

  lcd.setCursor(0,1);
  lcd.print("Ora: ");
  lcd.print(rtc.getTimeStr());
}






bool checkButton(){
  // read the state of the pushbutton value
  buttonState = digitalRead(buttonPin);
  // check if pushbutton is pressed.  if it is, the
  // buttonState is HIGH
  if (buttonState == HIGH) {
    return true;
  } else {
    return false;
  }
}

void addReading(){
  lastReadings[currentReading] = soilMoisturePercent;
  Serial.print("Current read: ");
  Serial.println(soilMoisturePercent);
  
  currentReading+=1;
}

void getAverage(){
  int temp=0;

  for(int i = 0; i<READINGS; i++){
    temp += lastReadings[i];
  }
  average=temp/READINGS;
}

void checkTimes(){
  unsigned long mills=millis();
  if(mills-currentMillis >= DELAY_IN_MILLISECONDS){
    currentMillis=mills;
    wetTimes=0;                                   //inizializzo le volte che ho innaffiato dato che il periodo è passato
  }
}

int getMinutes(){
  String timeString = rtc.getTimeStr(); //returns 50
  return (timeString.substring(3,5)).toInt();
}
int getHours(){
  String timeString = rtc.getTimeStr(); //returns 11
  return (timeString.substring(0,2)).toInt();
}
int getSeconds(){
  String timeString = rtc.getTimeStr(); //returns 11:50:55
  return (timeString.substring(6,8)).toInt();
}
/*The function returns true if the time interval has passed
* returns false if it didn't pass
* There must be a delay after this (if it returns true)*/

bool intervalSeconds(){
  if(getSeconds()%INTERVAL_SECONDS == 0){
    //delay(1000);
    return true;
  }
  return false;
}

bool intervalSeconds(int seconds){
  if(getSeconds()%seconds == 0){
    //delay(1000);
    return true;
  }
  return false;
}

/*The function returns true if the time interval has passed
* returns false if it didn't pass
* There must be a delay after this (if it returns true)*/

bool intervalMinutes(){
  if(getMinutes()%INTERVAL_MINUTES == 0){
    //delay(1000);
    return true;
  }
  return false;
}

void readMoisturePercent(){
  soilMoisture=analogRead(moisturePin);
  soilMoisturePercent=map(soilMoisture,airValue,waterValue,0,100);

  if(soilMoisturePercent>100){
    soilMoisturePercent=100;
  }else if(soilMoisturePercent<0){
    soilMoisturePercent=0;
  }
}
//Waters the plant
void annaffia(){
  digitalWrite(relayPin,LOW);
  delay(1000);
  digitalWrite(relayPin,HIGH);
  delay(1000);
}

//Sets the time for the real time clock
void setTime(){
  String timeString = __TIME__ ;
  int hourInt = (timeString.substring(0,2)).toInt();
  int minuteInt = (timeString.substring(3,5)).toInt();
  int secondInt = (timeString.substring(6,8)).toInt();

  String dayString = __DATE__;
  String monthString = (dayString.substring(0,3));
  int dayInt = (dayString.substring(4,6).toInt());
  int yearInt = (dayString.substring(7,11).toInt());
  
  rtc.setTime(hourInt, minuteInt, secondInt);
  rtc.setDate(dayInt, getMonthIndex(monthString), yearInt);
  
}

//JanFebMarAprMayJunJulAugSepOctNovDec
int getMonthIndex(String month){
  if(month.equals("Jan")){
    return 1;
  }
  if(month.equals("Feb")){
    return 2;
  }
  if(month.equals("Mar")){
    return 3;
  }
  if(month.equals("Apr")){
    return 4;
  }
  if(month.equals("May")){
    return 5;
  }
  if(month.equals("Jun")){
    return 6;
  }
  if(month.equals("Jul")){
    return 7;
  }
  if(month.equals("Aug")){
    return 8;
  }
  if(month.equals("Sep")){
    return 9;
  }
  if(month.equals("Oct")){
    return 10;
  }
  if(month.equals("Nov")){
    return 11;
  }
  if(month.equals("Dec")){
    return 12;
  }
  return 1;
}
