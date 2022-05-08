// Reveil fait avec:
// Arduino Nano Every -> il faut le renseigner ds l'IDE sous "outils" > "type de carte" (et l'installer au besoin depuis "gérer les bibliothèques"
// Afficheur LED Alpha-numérique en I2C (https://learn.adafruit.com/adafruit-led-backpack/0-54-alphanumeric)
// Module Real Time Clock DS1307 RTC connected via I2C (http://adafru.it/3296)
// Module TTP223 Capteur capacitif, un bouton poussoir, ainsi qu'un buzzer
// Pour info SDA: c'est la pin A4 et SCL la pin A5

// ### AJOUTER DE POUVOIR REGLER LA LUMINOSITE DU DIGIT ###

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "RTClib.h"
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// on initialise les variables:
int capteur_capaci; // lecture de la broche du capteur capacitif
int bouton_poussoir; // lecture de la broche du bouton poussoir
const int capteur_pin = 10; // le capteur TTP223 est sur la pin 5v, GND et la pin numérique 5 (sur une carte Uno)
const int bouton_pin = 9; // le bouton poussoir est sur la pin 9
const int buzzer_pin = 7; // le buzzer sur la 7
int heure = 0; // contient l'heure sur 2 chiffres (ex: 23)
int heure1 = 0; // le premier chiffre (ex: 2)
int heure2 = 0; // le 2ème (ex: 3)
int minut = 0; // contient les minutes sur 2 chiffres (ex: 59)
int minut1 = 0;
int minut2 = 0;
int reglage = 0; // si 3 -> réglage des heures de l'heure actuelle (avec le bouton setup) Si 4 -> réglage des minutes de l'heure actuelle
                //si 1 -> réglage des heures de l'alarme (avec le chamignon + le bouton setup) Si 2 -> réglage des minutes de l'alarme
int alarme = 1; // si 1 -> alarme activée et Si 0 -> alarme est désactivée (par défaut elle est activé pour le cas d'un reboot suite à une coupure de courant)
int heure_alarme = 0630; // contient l'heure de l'alarme sur 4 chiffres (ex: 2359)
int enfonce_tmp = 0; // utilisé lorsque l'alarme sonne, quand on enfonce le champignon il passe à 1 (le champignon a été remplacé par un capteur capacitif)
int nb_buzz = 0; // compte le nombre de bip pendant l'alarme (s'arrete à 10)
int temps = 0; // utilisé dans réglage pour compter le temps d'attente entre les appuis sur le bouton

void setup () {
  Serial.begin(57600);
  Serial.println("Test du système:");
  pinMode(capteur_pin, INPUT); // initialisons la broche du capteur en entrée
  pinMode(bouton_pin, INPUT_PULLUP); // initialisons la broche du bouton en entrée avec une résistance de pullup
  pinMode(buzzer_pin, OUTPUT); // initialisons la broche du bouton en sortie
  alpha4.begin(0x70);  // adresse I2C de l'affichage
  //alpha4.setBrightness(1); // luminosité de 0 à 15
  alpha4.clear();
  alpha4.writeDisplay(); // toujours rafraichir pour afficher

  //affichage de l'état du capteur capacitif
  capteur_capaci = digitalRead(capteur_pin);
  Serial.print("Etat capteur: ");
  Serial.println(capteur_capaci);

  //affichage de l'état du bouton
  bouton_poussoir = digitalRead(bouton_pin);
  Serial.print("Etat bouton: ");
  Serial.println(bouton_poussoir);

  //test affichage
  alpha4.writeDigitAscii(0, 'C');
  alpha4.writeDigitAscii(1, 'I');
  alpha4.writeDigitAscii(2, 'A');
  alpha4.writeDigitAscii(3, 'O');
  alpha4.writeDisplay(); // toujours rafraichir pour afficher
  
  //test du buzzer
  tone(buzzer_pin, 1000); // tone permet de générer un signal en KHz (ici 1KHz)
  delay(100);
  tone(buzzer_pin, 1500);
  delay(50);
  noTone(buzzer_pin); // Stop sound
  delay(800);

  alpha4.clear(); // on efface le digit
  alpha4.writeDisplay(); // on rafraichi pour afficher

  if (! rtc.begin()) {
    Serial.println("Module RTC ne fonctionne pas!");
    Serial.flush();
    while (1) delay(10);
  }
  if (! rtc.isrunning()) {
    Serial.println("Module RTC ne fonctionne pas! On le remet a l'heure");
    rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  Serial.println("Système prêt!");
  Serial.println();
}


void loop () {
  DateTime now = rtc.now();
  heure = now.hour();
  minut = now.minute();

  // On vérifie si l'heure actuelle correspond à l'heure de l'alarme (et que l'alarme est activée)
  if (alarme == 1 and heure_alarme == ((heure * 100) + minut)){
    Serial.println("Driiiiing..");
    Serial.println();
    enfonce_tmp = 1; // si le capteur capacitif est activé, on met $enfonce_tmp=0 et ca arrete la sonnerie
    nb_buzz = 1; // variable qui sert à compter le nombre de fois que le reveil a sonné
    // la première sonnerie est un peu + courte que les autres:
    digitalWrite(buzzer_pin,HIGH); // biiip
    delay(800);
    for(int i = 1; i < 10; i++) { // boucle de 9
      digitalWrite(buzzer_pin,LOW); // on coupe le buzzer et on attend 1 min
      for(int ii = 1; ii < 200; ii++) { // boucle de 200 pour le sleep 200x0.3sec donc 60sec
        if (digitalRead(capteur_pin) == HIGH) { // si le capteur capacitif est activé -> arret de la sonnerie
          Serial.println("Le capteur capacitif est activé -> Arret de la sonnerie..");
          enfonce_tmp = 0;
          tone(buzzer_pin, 1800); // on joue un mini son de confirmation
          delay(10);
          noTone(buzzer_pin); // Stop sound...
        } // fin de si le capteur capacitif est activé
      delay(300);
      } // fin for 200
      if (enfonce_tmp != 0){ // si $enfonce_tmp = 0 on laisse le buzzer couper
        digitalWrite(buzzer_pin,HIGH); // on lance le buzzer durant 2sec
        nb_buzz = (nb_buzz + 1); // on ajoute 1 au compteur de sonnerie
        for(int i = 0; i < 6; i++) { // boucle de 6 pour le sleep 6x0.3sec donc 1.8sec
          if (digitalRead(capteur_pin) == HIGH) { // check capteur capacitif pour arreter la sonnerie
            Serial.println("Le capteur capacitif est activé -> Arret de la sonnerie..");
            enfonce_tmp = 0;
            tone(buzzer_pin, 1800); // on joue un mini son de confirmation
            delay(10);
            noTone(buzzer_pin); // Stop sound...
          } // fin check du capteur capacitif
          delay(300);
        } // fin for 6
      } // fin de si $enfonce_tmp = 0
    } // fin de la boucle for 9x
    digitalWrite(buzzer_pin,LOW); // on coupe le buzzer si ce n'est deja fait
  } // fin de si l'heure actuelle correspond à l'heure de l'alarme



  // Sonnerie de secours: on verifie si l'heure actuelle correspond a l'heure d'alarme + 20min ET nb_buzz=11
  if(nb_buzz >= 10){
    enfonce_tmp = ((heure_alarme % 100) + 20); // on reutilise enfonce_tmp pour récupérer les minutes de heure_alarme et mettre + 20 min
    if(enfonce_tmp >= 60){ // si egal ou sup à 60
      enfonce_tmp = (enfonce_tmp - 60);
    }
    if (alarme == 1 and minut == enfonce_tmp){ // si les minutes actuelles = minutes de l'alarme + 20min
      Serial.println("L'heure actuelle correspond a celle de l'alarme + 20min, on active la sonnerie de secours");
      for(int i = 2; i < 10; i++) { // boucle de 8 (4.8sec)
        digitalWrite(buzzer_pin,HIGH); // on lance le buzzer
        delay(400);
        digitalWrite(buzzer_pin,LOW); // on coupe le buzzer
        delay(200);
      }
      nb_buzz = 0;
      Serial.println("Fin de l'alarme de secours.");
    }
  }



  // Si on appuie sur le capteur capacitif
  if (digitalRead(capteur_pin) == HIGH) { // si capteur est activé
    // J'ai eu bcp de mal pour l'affichage à cause du formatage, du coup mes variables heure1 s'afficheront en hexadécimal
    // on récupère le 1er chiffre dans $heure
    heure1 = 0;
    if (heure > 9 and heure < 20) {
      heure1 = 1;
    }
    if (heure > 19) {
      heure1 = 2;
    }
    
    // on récupère le 2e chiffre dans $heure
    heure2 = heure;
    while (heure2 > 9) { // boucle tant que > 9
      heure2 = (heure2 - 10);
    }
    
    
    // on récupère le 1er chiffre dans $minut
    minut1 = 0;
    if (minut > 9 and minut < 20) {
      minut1 = 1;
    }
    if (minut > 19 and minut < 30) {
      minut1 = 2;
    }
    if (minut > 29 and minut < 40) {
      minut1 = 3;
    }
    if (minut > 39 and minut < 50) {
      minut1 = 4;
    }
    if (minut > 49 and minut < 60) {
      minut1 = 5;
    }
    
    // on récupère le 2e chiffre dans $minut
    minut2 = minut;
    while (minut2 > 9) {
      minut2 = (minut2 - 10);
    }

    
    Serial.println("Capteur activé -> Affichage de l'heure:");
    Serial.print("Il est ");
    Serial.print(heure1);
    Serial.print(heure2);
    Serial.print(minut1);
    Serial.println(minut2);
    Serial.println();
  
    // si 12:00 -> on écrit "MIDI" sur le digit Sinon on affiche l'heure
    if (heure1 == 1 and heure2 == 2 and minut1 == 0 and minut2 == 0) {
      alpha4.writeDigitAscii(0, 'M');
      alpha4.writeDigitAscii(1, 'I');
      alpha4.writeDigitAscii(2, 'D');
      alpha4.writeDigitAscii(3, 'I');
      alpha4.writeDisplay();
    } else {
      // on affiche l'heure sur l'affichage
      if (heure1 != 0) { // si le premier chiffre est 0 (01 à 09h on affiche 1 à 9)
        alpha4.writeDigitAscii(0, (heure1 + 48)); // on ajoute 48 car il est affiché en hexadécimal
      } else {
        alpha4.writeDigitAscii(0, ' '); // si 0 -> on affiche rien
      }
      alpha4.writeDigitAscii(1, (heure2 + 48));
      alpha4.writeDigitAscii(2, (minut1 + 48));
      alpha4.writeDigitAscii(3, (minut2 + 48));
      alpha4.writeDisplay();
    }
    delay(2000); // on affiche l'heure durant 2 sec avant d'effacer le digit
    alpha4.writeDigitAscii(0, ' ');
    alpha4.writeDigitAscii(1, ' ');
    alpha4.writeDigitAscii(2, ' ');
    alpha4.writeDigitAscii(3, ' ');
    alpha4.writeDisplay();

    //si le capteur capacitif est tjs activé ET aussi le bouton -> on règle l'heure actuelle
    if (digitalRead(capteur_pin) == HIGH and digitalRead(bouton_pin) == LOW) {
      Serial.println("Réglage de l'heure actuelle");
      heure1 = 0; // remise à zéro des heure et minute
      heure2 = 0;
      minut1 = 0;
      minut2 = 0;
      temps = 0; // servira à compter le temps de relache du bouton
      // on affiche les heures à 0 et on n'affiche pas les minutes
      alpha4.writeDigitAscii(0, '0');
      alpha4.writeDigitAscii(1, '0');
      alpha4.writeDigitAscii(2, ' ');
      alpha4.writeDigitAscii(3, ' ');
      alpha4.writeDisplay();
      reglage = 3; // si 3 -> réglage des HH, de l'heure actuelle
    } // fin de si capteur est activé ET le bouton
  } // fin de si capteur est activé


  while (reglage == 3) { // si 1 -> réglage des HH, de l'heure actuelle
    boolean etatBouton = digitalRead(bouton_pin);
    if (etatBouton == LOW) { // si bouton est appuyé
      //Serial.println("Pour ctrl reglage des heures de l'alarme"); // controle qu'on est bien en mode réglage de l'heure de l'alarme
      heure2 = (heure2 + 1); // on fait défiler les heures
      if (heure2 > 9) {
        heure1 = (heure1 + 1);
        heure2 = 0;
      }
      if (heure1 == 2 and heure2 > 3) { // mais évidemment pas au-delà de 23h..
        heure1 = 0;
        heure2 = 0;
      }
      temps = 0; // on initialise le temps de comptage (si pas d'appui sur le bouton durant un temps)
      alpha4.writeDigitAscii(0, (heure1 + 48)); // on affiche les heures sur le digit
      alpha4.writeDigitAscii(1, (heure2 + 48));
      alpha4.writeDisplay();
      delay(450); // délai entre les chiffres
    }
    temps = (temps + 1); // si pas d'appui sur le bouton -> on ajoute 1
    if (temps > 5000) { // si on attends trop, on passe au réglage des minutes
      temps = 0;
      // on affiche les heure à 0 et on n'affiche pas les minutes
      alpha4.writeDigitAscii(0, ' ');
      alpha4.writeDigitAscii(1, ' ');
      alpha4.writeDigitAscii(2, '0');
      alpha4.writeDigitAscii(3, '0');
      alpha4.writeDisplay();
      delay(600); // attente avec les 00 minutes d'affichées
      reglage = 4; // on sort de la boucle et on ira dans la suivante
    }
  } // fin du réglage des heures actuelles


  while (reglage == 4) { // si 4 -> réglage des minutes de l'heure actuelle
    boolean etatBouton = digitalRead(bouton_pin);
    //Serial.print('x');
    if (etatBouton == LOW) { // si bouton est appuyé
      //Serial.println("Pour ctrl reglage des minutes de l'alarme"); // controler qu'on est bien en mode réglage des minutes de l'alarme
      minut2 = (minut2 + 1);
      if (minut2 > 9) {
        minut1 = (minut1 + 1);
        minut2 = 0;
      }
      if (minut1 >= 6) {
        minut1 = 0;
        minut2 = 0;
      }
      temps = 0;
      alpha4.writeDigitAscii(2, (minut1 + 48));
      alpha4.writeDigitAscii(3, (minut2 + 48));
      alpha4.writeDisplay();
      delay(450); // délai entre les chiffres
    }
    temps = (temps + 1);
    if (temps > 5000) { // si on attends trop, on sort du mode réglage
      temps = 0;
      heure = ((heure1 * 10) + (heure2));
      minut = ((minut1 * 10) + (minut2));
      rtc.adjust(DateTime(2018, 12, 31, heure, minut, 0)); // mise à jour des heures et minutes dans le module RTC
      reglage = 0; // on sort de la boucle et donc du mode réglage
      alpha4.writeDigitAscii(0, ' ');
      alpha4.writeDigitAscii(1, ' ');
      alpha4.writeDigitAscii(2, ' ');
      alpha4.writeDigitAscii(3, ' ');
      alpha4.writeDisplay();
    }
  } // fin du réglage des minutes de l'heure actuelle



  /* Ce deuxième bouton n'existe plus..
  // Si on appuie sur le bouton alarme ->
  if (digitalRead(bouton_pin) == HIGH) { // si bouton alarme est appuyé
    Serial.println("On appuie sur le bouton de l'alarme -> Affichage de l'heure d'alarme:");
    Serial.print("L'alarme est prévue pour ");
    if (alarme == 1) { // si l'alarme est activée
      Serial.println(heure_alarme);
      Serial.println();
  
      minut = (heure_alarme % 100); // on reprend les 2 premiers chiffres de heure_alarme
      heure = (heure_alarme / 100); // on reprend les 2 derniers chiffres de heure_alarme
      // J'ai eu bcp de mal pour l'affichage à cause du formatage, du coup mes variables heure1 s'afficheront en hexadécimal
      // on récupère le 1er chiffre dans $heure
      heure1 = 0;
      if (heure > 9 and heure < 20) {
        heure1 = 1;
      }
      if (heure > 19) {
        heure1 = 2;
      }
      
      // on récupère le 2e chiffre dans $heure
      heure2 = heure;
      while (heure2 > 9) { // boucle tant que > 9
        heure2 = (heure2 - 10);
      }
      
      
      // on récupère le 1er chiffre dans $minut
      minut1 = 0;
      if (minut > 9 and minut < 20) {
        minut1 = 1;
      }
      if (minut > 19 and minut < 30) {
        minut1 = 2;
      }
      if (minut > 29 and minut < 40) {
        minut1 = 3;
      }
      if (minut > 39 and minut < 50) {
        minut1 = 4;
      }
      if (minut > 49 and minut < 60) {
        minut1 = 5;
      }
      
      // on récupère le 2e chiffre dans $minut
      minut2 = minut;
      while (minut2 > 9) {
        minut2 = (minut2 - 10);
      }
  
      // on affiche l'heure de l'alarme sur l'affichage
      if (heure1 != 0) { // si le premier chiffre est 0 (01 à 09h on affiche 1 à 9)
        alpha4.writeDigitAscii(0, (heure1 + 48)); // on ajoute 48 car il est affiché en hexadécimal
      } else {
        alpha4.writeDigitAscii(0, ' '); // si 0 -> on affiche rien
      }
      alpha4.writeDigitAscii(1, (heure2 + 48));
      alpha4.writeDigitAscii(2, (minut1 + 48));
      alpha4.writeDigitAscii(3, (minut2 + 48));
      alpha4.writeDisplay();
    }else{ // si l'alarme est déactivée
      Serial.print("- STOP -");
      alpha4.writeDigitAscii(0, 'S'); // et on écrit "STOP" sur le digit
      alpha4.writeDigitAscii(1, 'T');
      alpha4.writeDigitAscii(2, 'O');
      alpha4.writeDigitAscii(3, 'P');
      alpha4.writeDisplay();
    }
    delay(2000); // on affiche l'heure durant 2 sec avant d'effacer le digit
    alpha4.writeDigitAscii(0, ' ');
    alpha4.writeDigitAscii(1, ' ');
    alpha4.writeDigitAscii(2, ' ');
    alpha4.writeDigitAscii(3, ' ');
    alpha4.writeDisplay();


    //si le bouton d'alarme est tjs enfoncé -> on active / désactive l'alarme
    if (digitalRead(bouton_pin) == HIGH) { // si bouton de l'alarme est appuyé
      digitalWrite(buzzer_pin,HIGH); // biiip
      delay(300);
      digitalWrite(buzzer_pin,LOW);
      if (alarme == 1) { // si l'alarme est activée
        Serial.println("On desactive l'alarme");
        Serial.println();
        alarme = 0; // on la désactive
        alpha4.writeDigitAscii(0, 'S'); // et on écrit "STOP" sur le digit
        alpha4.writeDigitAscii(1, 'T');
        alpha4.writeDigitAscii(2, 'O');
        alpha4.writeDigitAscii(3, 'P');
        alpha4.writeDisplay();
        delay(400);
      }else{
        Serial.println("On active l'alarme");
        Serial.println(" ");
        alarme = 1;
        alpha4.writeDigitAscii(0, 'A'); // ### MODIFIER POUR AFFICHER L'HEURE DE L'ALARME ###
        alpha4.writeDigitAscii(1, 'R');
        alpha4.writeDigitAscii(2, 'M');
        alpha4.writeDigitAscii(3, 'E');
        alpha4.writeDisplay();
        delay(400);
      }
      alpha4.writeDigitAscii(0, ' '); // ### MODIFIER POUR AFFICHER L'HEURE DE L'ALARME ###
      alpha4.writeDigitAscii(1, ' ');
      alpha4.writeDigitAscii(2, ' ');
      alpha4.writeDigitAscii(3, ' ');
      alpha4.writeDisplay();
    }

  }
  */


  
  boolean etatBouton = digitalRead(bouton_pin);
  if (etatBouton == LOW) { // si bouton est appuyé
    Serial.println("Réglage de l'alarme");
    temps = 0; // servira à compter le temps de relache du bouton
    if (alarme == 1) { // si l'alarme est activée, on propose le 'STOP'
      alpha4.writeDigitAscii(0, 'S'); // et on écrit "STOP" sur le digit
      alpha4.writeDigitAscii(1, 'T');
      alpha4.writeDigitAscii(2, 'O');
      alpha4.writeDigitAscii(3, 'P');
      alpha4.writeDisplay();
      delay(450); // délai entre les chiffres
      alpha4.clear(); // on efface le digit
      alpha4.writeDisplay(); // on rafraichi pour afficher
      if (etatBouton == LOW) { // si bouton est tjs appuyé -> on veut désactiver l'alarme
        alarme = 0; // on déactive l'alarme
        Serial.println("L'alarme a été désactivée");
        // on affiche une confirmation
        tone(buzzer_pin, 1800); // on joue un mini son de confirmation
        delay(10);
        noTone(buzzer_pin); // Stop sound...
        for(int i = 1; i < 4; i++) { // boucle de 3
          alpha4.writeDigitAscii(0, 'S'); // et on écrit "STOP" sur le digit
          alpha4.writeDigitAscii(1, 'T');
          alpha4.writeDigitAscii(2, 'O');
          alpha4.writeDigitAscii(3, 'P');
          alpha4.writeDisplay(); // rafraichir pour afficher
          delay(200);
          alpha4.clear();
          alpha4.writeDisplay(); // toujours rafraichir pour afficher
          delay(200);
        }
        goto finsetup; // on va après le réglage de l'alarme (on saute le reglage quoi)
      }
    }
    // Réglage de l'heure d'alarme
    alarme = 1; // on active l'alarme
    Serial.println("L'alarme a été activée");
    heure1 = 0; // remise à zéro des heure et minute
    heure2 = 0;
    minut1 = 0;
    minut2 = 0;
    // on affiche les heures à 0 et on n'affiche pas les minutes
    alpha4.writeDigitAscii(0, '0');
    alpha4.writeDigitAscii(1, '0');
    alpha4.writeDigitAscii(2, ' ');
    alpha4.writeDigitAscii(3, ' ');
    alpha4.writeDisplay();
    reglage = 1; // si 1 -> réglage des sheure de l'alarme
  }

  while (reglage == 1) { // si 1 -> réglage des heures de l'alarme
    boolean etatBouton = digitalRead(bouton_pin);
    if (etatBouton == LOW) { // si bouton est appuyé
      heure2 = (heure2 + 1); // on fait défiler les heures
      if (heure2 > 9) {
        heure1 = (heure1 + 1);
        heure2 = 0;
      }
      if (heure1 == 2 and heure2 > 3) { // mais évidemment pas au-delà de 23h..
        heure1 = 0;
        heure2 = 0;
      }
      temps = 0; // on initialise le temps de comptage (si pas d'appui sur le bouton durant un temps)
      alpha4.writeDigitAscii(0, (heure1 + 48)); // on affiche les heures sur le digit
      alpha4.writeDigitAscii(1, (heure2 + 48));
      alpha4.writeDisplay();
      delay(450); // délai entre les chiffres
    }
    temps = (temps + 1); // si pas d'appui sur le bouton -> on ajoute 1
    if (temps > 5000) { // si on attends trop, on passe au réglage des minutes
      temps = 0;
      // on affiche les heure à 0 et on n'affiche pas les minutes
      alpha4.writeDigitAscii(0, ' ');
      alpha4.writeDigitAscii(1, ' ');
      alpha4.writeDigitAscii(2, '0');
      alpha4.writeDigitAscii(3, '0');
      alpha4.writeDisplay();
      delay(600); // attente avec les 00 minutes d'affichées
      reglage = 2; // on sort de la boucle et on ira dans la suivante
    }
  } // fin du réglage des heures de l'alarme


  while (reglage == 2) { // si 2 -> réglage des minutes de l'alarme
    boolean etatBouton = digitalRead(bouton_pin);
    //Serial.print('x');
    if (etatBouton == LOW) { // si bouton est appuyé
      //Serial.println('M'); // Affiche M dans la console pour controler qu'on est bien en mode réglage des minutes
      minut2 = (minut2 + 5);
      if (minut2 > 9) {
        minut1 = (minut1 + 1);
        minut2 = 0;
      }
      if (minut1 >= 6) {
        minut1 = 0;
        minut2 = 0;
      }
      alpha4.writeDigitAscii(2, (minut1 + 48));
      alpha4.writeDigitAscii(3, (minut2 + 48));
      alpha4.writeDisplay();
      delay(450); // délai entre les chiffres
    }
    temps = (temps + 1);
    if (temps > 5000) { // si on attends trop, on sort du mode réglage
      temps = 0;
      heure_alarme = ((heure1 * 1000)+(heure2 * 100) + (minut1 * 10) + minut2); // ex: 2359
      reglage = 0; // on sort de la boucle et donc du mode réglage
      alpha4.writeDigitAscii(0, ' ');
      alpha4.writeDigitAscii(1, ' ');
      alpha4.writeDigitAscii(2, ' ');
      alpha4.writeDigitAscii(3, ' ');
      alpha4.writeDisplay();
      Serial.print("Alarme réglée sur: ");
      Serial.println(heure_alarme);
      Serial.println();

      // Affichage de confirmation
      tone(buzzer_pin, 1800); // on joue un mini son de confirmation
      delay(10);
      noTone(buzzer_pin); // Stop sound...
      for(int i = 1; i < 4; i++) { // boucle de 3
        alpha4.writeDigitAscii(0, (heure1 + 48)); // on affiche heure d'alarme sur le digit
        alpha4.writeDigitAscii(1, (heure2 + 48));
        alpha4.writeDigitAscii(2, (minut1 + 48));
        alpha4.writeDigitAscii(3, (minut2 + 48));
        alpha4.writeDisplay(); // rafraichir pour afficher
        delay(200);
        alpha4.clear();
        alpha4.writeDisplay(); // toujours rafraichir pour afficher
        delay(200);
      }
    }
  } // fin du réglage des minutes de l'alarme
  finsetup: // Etiquette pour si on a sauté le reglage de l'alarme (Quand on l'a désactivée en affichant 'STOP')

  //si on veut maintenir un témoin si l'alarme est activée (le point)
  //if (alarme == 1) {//si l'alarme est activée -> on affiche juste le point
  //  alpha4.writeDigitAscii(3, 14); // le point pour le temoin d'alarme
  //}
  //alpha4.writeDisplay();

  delay(10); // pause
}
