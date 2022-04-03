/*   DISPOSICION DE LA PANTALLA (320x240):
                                              EJE X

                                0,0 -------------------------> 320
                                  ¡  _______________________
                             _____¡_|                       |
                    CABLE -> _____¡_|                       |
                                  ¡ |       PANTALLA        |
                                  ¡ |                       |
                         EJY Y 240¡ |_______________________| 320,240

 */

/*LIBRERIAS*/
#include <Arduino_FreeRTOS.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <UTFT.h>
#include <URTouch.h>
#include <UTFT_Buttons.h>
#include <TouchScreen.h>
#include <UTFTGLUE.h>     
#include <semphr.h>

float temperatura = 0;
float tempAnt = 0;
float t=31;     //Temperatura de referencia
int out = 0, aux = 0, inc = 0, modo = 0, estado_caldera = 0;
const int STACK_SIZE = 256;     //Stack size in words, not bytes
const int pinDatosDQ = 53;      //Pin donde se conecta el bus 1-Wire
const int XP = 6, XM = A2, YP = A1, YM = 7;     //ID=0x9341
const int TS_LEFT = 180, TS_RT = 956, TS_TOP = 898, TS_BOT = 177;     //Calibración de la pantalla

UTFTGLUE myGLCD(0,A2,A1,A3,A4,A0);
Adafruit_GFX_Button on_btn, off_btn, modo_btn, made_btn, out_btn;     //Botones
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

MCUFRIEND_kbv tft;
OneWire oneWireObjeto(pinDatosDQ);                    //Instancia a OneWire
DallasTemperature sensorDS18B20(&oneWireObjeto);      //Instancia a DallasTemperature

SemaphoreHandle_t xSem_Tft = NULL;

/*PRESIONES*/
#define MINPRESSURE 200
#define MAXPRESSURE 5000

/*COLORES*/
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define FONDO   0x8802      //Color del fondo

/*LOGO UHU*/
#define LCD_CS A3     //Chip Select goes to Analog 3
#define LCD_CD A2     //Command/Data goes to Analog 2
#define LCD_WR A1     //LCD Write goes to Analog 1
#define LCD_RD A0     //LCD Read goes to Analog 0
#define LCD_RESET A4  //Can alternately just connect to Arduino's reset pin
int direccion = 0;
uint16_t wid, i = 190;

/*FUNCION DE LA PANTALLA*/
/*Permite saber sonde se ha pulsado en la pantalla*/
int pixel_x, pixel_y;         //Touch_getXY() updates global vars
bool Touch_getXY(void){
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);        //Restore shared pins
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);     //Because TFT control pins
  digitalWrite(XM, HIGH);
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  if (pressed){
    pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());     //.kbv makes sense to me
    pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
  }
  return pressed;
}


/*PANTALLA PRINCIPAL*/
void redibuja_tft(void){
  myGLCD.setColor(FONDO);                   //Resetea la parte central de la pantalla con el color del fondo
  myGLCD.fillRect(0, 13, 319, 226);

  /*Tubería*/
  myGLCD.setColor(114, 119, 90);
  myGLCD.fillRect(270, 30, 320, 50);        //Tubería estrecha
  myGLCD.fillRect(230, 27, 260, 53);        //Tubería ancha (parte 1)
  myGLCD.fillRect(230, 53, 250, 60);        //Tubería ancha (parte 2)
       
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(260, 25, 270, 55);        //Elemento para reducir el diámetro de la tubería
  myGLCD.drawRect(270, 30, 320, 50);        //Borde de la tubería estrecha
  myGLCD.drawRect(260, 25, 270, 55);        //Borde del elemento para reducir el diámetro de la tubería
  myGLCD.drawLine(230, 27, 260, 27);        //Borde de la tubería ancha
  myGLCD.drawLine(250, 53, 260, 53);        //Borde de la tubería ancha
  myGLCD.drawLine(250, 53, 250, 60);        //Borde de la tubería ancha
  myGLCD.drawLine(230, 27, 230, 60);        //Borde de la tubería ancha  
  
  /*Caldera*/
  myGLCD.setColor(114, 119, 90);
  myGLCD.fillRect(200, 60, 280, 180);       
  myGLCD.setColor(0, 0, 0);
  myGLCD.drawRect(200, 60, 280, 180);       //Borde de la caldera
  myGLCD.drawRect(205, 65, 275, 175);       //Línea negra dentro de la caldera
  myGLCD.drawCircle(260, 120, 6);           //Botón de encendido de la caldera || (Centro x, centro y, radio)

  /*Caja amarilla donde se muestra el valor de la temperatura*/
  tft.fillRect(20, 95, 170, 70, YELLOW);    //Borde
  tft.fillRect(25, 100, 160, 60, BLACK);    //Relleno
  
  /*Escribe "TEMPERATURA" encima de la caja amarilla, "CALDERA" en la caldera*/
  tft.setTextColor(BLACK, FONDO);
  tft.setTextSize(2);
  tft.setCursor(40, 75);      //Coloca el cursor para ver donde empezar a escribir
  tft.setRotation(1);         //La rotacion debe ser 1 para que no se ponga como la pantalla vertical     
  tft.println("TEMPERATURA");
  
  myGLCD.setBackColor(114, 119, 90);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("CALDERA",214,82);
  
  /*Botones y pantalla de la temperatura || (&tft, Origen X, origen Y, ancho, alto, color borde, color interior, color letra, "texto", tamaño letra)*/
  tft.setRotation(1);
  on_btn.initButton(&tft,  60, 200, 80, 30, BLACK, GREEN, BLACK, "ON", 2);    //Botón ON
  off_btn.initButton(&tft, 150, 200, 80, 30, BLACK, RED, BLACK, "OFF", 2);    //Botón OFF
  modo_btn.initButton(&tft, 240, 200, 80, 30, BLACK, BLUE, BLACK, "", 2);     //Botón AUTO/MANU
  made_btn.initButton(&tft, 150, 40, 32, 30, FONDO, FONDO, FONDO, "", 1);     //Botón AUTORES
  on_btn.drawButton(false);   //El false y el true hacen la animación (invertir colores) cuando se pulsa*/
  off_btn.drawButton(false);
  modo_btn.drawButton(false);
  made_btn.drawButton();
  
  tft.fillRect(215, 160, 20, 10, GREEN);    //Rectángulo verde de la caldera
  tft.fillRect(245, 160, 20, 10, RED);      //Rectángulo rojo de la caldera
  
  /*Figura del botón de autores*/
  myGLCD.setColor(RED);
  myGLCD.fillCircle(160,40,15);             //Circulo rojo
  myGLCD.setColor(255,123,0);
  myGLCD.fillCircle(160,32,5);              //Cabeza 
  tft.fillRect(154.9, 38, 13, 15, CYAN);    //Tronco
  
  /*Muestra la temperatura*/
  if((temperatura < t)) myGLCD.setColor(0,255,0);  //Temperatura en verde
  else myGLCD.setColor(255,0,0);                   //Temperatura en rojo
  
  myGLCD.setBackColor(BLACK);
  myGLCD.setFont(BigFont);    // Selecciona la fuente para la temperatura del motor
  myGLCD.print("C",144,123);
  myGLCD.setFont(SmallFont);
  myGLCD.print("o",140,118);
  
  /*Escribe el modo actual en el botón correspondiente y, si está en modo manual, deja la caldera en el estado guardado*/
  if (modo==0){                   //Modo automático
    modo_btn.drawButton();    
    
    tft.setTextColor(BLACK, BLUE);
    tft.setTextSize(2);
    tft.setCursor(216, 192);      //Coloca el cursor para ver donde empezar a escribir
    tft.setRotation(1);           //La rotacion debe ser 1 para que no se ponga como la pantalla vertical     
    tft.println("AUTO");
  }
  else{                               //Modo manual
    modo_btn.drawButton(true);
  
    if(estado_caldera==0){
      myGLCD.setColor(0,0,0);
      myGLCD.fillCircle(260,120,5);   //Caldera apagada
    }
    else{
      myGLCD.setColor(255,123,0);
      myGLCD.fillCircle(260,120,5);   //Caldera encendida      
    }
    
    tft.setTextColor(BLUE, BLACK);
    tft.setTextSize(2);
    tft.setCursor(216, 192);      //Coloca el cursor para ver donde empezar a escribir
    tft.setRotation(1);           //La rotacion debe ser 1 para que no se ponga como la pantalla vertical     
    tft.println("MANU");
  }  
}

/*TAREA 1 - Actualiza la pantalla*/
void vTaskUpdateTFT( void * pvParameters ){
  for( ;; ){
    /*Este if lo uso para dibujar la pantalla inicial.
    Uso inc==0 para que la primera vez dibuje la pantalla inicial, luego sumo inc y ya nunca entrará más por esto. 
    Cuando vuelva a entrar,será porque aux ==1, que será cuando necesite volver a dibujarlo (cuando pulse salir de la pantalla de nuestros nombres)*/
    xSemaphoreTake( xSem_Tft, portMAX_DELAY );    //Toma el semáforo
    
    if (aux==1 || inc ==0){
     redibuja_tft(); 
     inc=1; //Al aumentar inc, solo entra la primera vez por este motivo, ya no usare mas inc
     aux=0;
    }
  
    /*Comprobación y localización de la pulsación en la pantalla*/
    bool down = Touch_getXY();
    on_btn.press(down && on_btn.contains(pixel_x, pixel_y));
    off_btn.press(down && off_btn.contains(pixel_x, pixel_y));
    modo_btn.press(down && modo_btn.contains(pixel_x, pixel_y));
    made_btn.press(down && made_btn.contains(pixel_x, pixel_y));
  
    /*Invierte colores cuando se suelta el botón correspodiente*/
    if (on_btn.justReleased()) on_btn.drawButton();
    if (off_btn.justReleased()) off_btn.drawButton();
    
    /*Invierte colores cuando se pulsa el botón y enciende o apaga la caldera si se está en modo manual*/
    if (on_btn.justPressed()) {
      if(estado_caldera==0 && modo==1){
        estado_caldera=1;               //Estado 1: caldera encendida
        myGLCD.setColor(255,123,0);
        myGLCD.fillCircle(260,120,5);   //Enciende la caldera
      }
      on_btn.drawButton(true);
    }
    if (off_btn.justPressed()){
      if(estado_caldera==1 && modo==1){
        estado_caldera=0;               //Estado 0: caldera apagada
        myGLCD.setColor(0,0,0);
        myGLCD.fillCircle(260,120,5);   //Apaga la caldera
      }
      off_btn.drawButton(true);
    }

    /*CAMBIO DE MODO DE FUNCIONAMIENTO (AUTOMÁTICO/MANUAL)*/
    if (modo_btn.justPressed()){
      if (modo==0){
        modo=1;                         //Modo manual
        modo_btn.drawButton(true);    
        myGLCD.setColor(0,0,0);
        myGLCD.fillCircle(260,120,5);   //Apaga la caldera
        
        tft.setTextColor(BLUE, BLACK);
        tft.setTextSize(2);
        tft.setCursor(216, 192);        //Coloca el cursor para ver donde empezar a escribir
        tft.setRotation(1);             //La rotacion debe ser 1 para que no se ponga como la pantalla vertical     
        tft.println("MANU");            //Muestra el estado actual del sistema
      }
      else
      {
        modo=0;                         //Modo automático
        estado_caldera=0;               //Estado 0: caldera apagada (para que esté apagada cuando se vuelva a modo manual)
        modo_btn.drawButton();
        
        tft.setTextColor(BLACK, BLUE);
        tft.setTextSize(2);
        tft.setCursor(216, 192);        //Coloca el cursor para ver donde empezar a escribir
        tft.setRotation(1);             //La rotacion debe ser 1 para que no se ponga como la pantalla vertical     
        tft.println("AUTO");            //Muestra el estado actual del sistema
      }
    } 

    /*CAMBIO A PANTALLA DE AUTORES*/
    if (made_btn.justPressed()){
      myGLCD.setColor(FONDO);          //Resetea la parte central de la pantalla con el fondo blanco
      myGLCD.fillRect(0, 13, 319, 226);
      
      myGLCD.setFont(BigFont);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setBackColor(0, 0, 0);   
      myGLCD.print(" AUTORES ", CENTER, 30);
      
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(0, 0, 0);
      myGLCD.setBackColor(FONDO);
      myGLCD.print("ALEJANDRO GARROCHO CRUZ", CENTER, 60);
      myGLCD.print("MACARENA MARTINEZ LUNA", CENTER, 90);
      myGLCD.print("ADRIAN RODRIGUEZ VAZQUEZ", CENTER, 120);
      myGLCD.print("MARIA SACRISTAN GARCIA", CENTER, 150);
  
      tft.setRotation(1);            //PORTRAIT
      out_btn.initButton(&tft,  160, 200, 80, 30, BLACK, BLACK, WHITE, "SALIR", 1); //Botón SALIR
      out_btn.drawButton(false);
      
      /*En este bucle, entrare siempre que se pulse en el boton de autores, compruebo en esta pantalla donde se ha pulsado, 
      y si he presionado salir, cambio out = 1 para salir del bucle, y cambio tambien aux = 1 para entrar en el primer if 
      del loop y dibujar la pantalla inicial*/
      while (out == 0){
        bool down = Touch_getXY();
        out_btn.press(down && out_btn.contains(pixel_x, pixel_y));
        if (out_btn.justPressed()){
          out_btn.drawButton(true);
          out = 1;
          aux=1;
        }
  
        if (out_btn.justReleased()){
          out_btn.drawButton();
        }                                
      }       
    }    
    
    xSemaphoreGive( xSem_Tft );                   //Devuelve el semáforo
    vTaskDelay(100 / portTICK_PERIOD_MS);         //Se suspende la tarea por 200 ms para no sobrecargar
  }
}

/*TAREA 2 - Actualiza el valor de temperatura y el estado de la caldera en modo automático*/
void vTaskSensorRead( void * pvParameters ){
  for( ;; ){
    xSemaphoreTake( xSem_Tft, portMAX_DELAY );    //Toma el semáforo
    
    valortemperatura();
    led_modoAUTO();
    out=0; 
                 
    xSemaphoreGive( xSem_Tft );                   //Devuelve el semáforo
    vTaskDelay(200 / portTICK_PERIOD_MS);         //Se suspende la tarea por 200 ms para no sobrecargar
  }
}

/*TAREA 3 - Mueve el logo de la UHU*/
void vTaskLogoTft( void * pvParameters ){
  for( ;; ){
    xSemaphoreTake( xSem_Tft, portMAX_DELAY );    //Toma el semáforo
    
    /*Movimiento del logo de la UHU*/  
    wid = tft.width();
    if(direccion==0){
      if (i<250){
        #if defined(MCUFRIEND_KBV_H_)
          if(1){
            extern const uint8_t uhu[];
            tft.setAddrWindow(wid - 40 - i, 22 + 0, wid - 1 - i, 22 + 39);
            tft.pushColors(uhu, 1600, 1);
          }
        #endif
        i++;
      }
      else direccion=1;
    }
    else{
      if (i>190){
        #if defined(MCUFRIEND_KBV_H_)
          if(1){
            extern const uint8_t uhu[];
            tft.setAddrWindow(wid - 40 - i, 22 + 0, wid - 1 - i, 22 + 39);
            tft.pushColors(uhu, 1600, 1);
          }
        #endif
        i--;
      }
      else direccion=0;
    } 
    
    xSemaphoreGive( xSem_Tft );                   //Devuelve el semáforo
    vTaskDelay(100 / portTICK_PERIOD_MS);         //Se suspende la tarea por 100 ms para no sobrecargar
  }
}

/*CONFIGURACIÓN*/
void setup(){
  BaseType_t xReturned;
  
  Serial.begin(9600);
  tft.reset();
  
  uint16_t identifier = tft.readID();
  uint16_t ID = tft.readID();
  
  if (identifier == 0xEFEF) identifier = 0x9486;
  
  tft.begin(identifier);  
  tft.begin(ID);            //Logo UHU

  randomSeed(analogRead(0));
  
  //Inicia la pantalla
  myGLCD.InitLCD();

  /*SENSOR TEMPERATURA*/
  sensorDS18B20.begin();    //Iniciamos el bus 1-Wire llamando a la función sensorDS18B20.begin()que no admite ningún parámetro
  pinMode(51, OUTPUT);      //Alimentamos el sensor

  /*FONDO DE PANTALLA*/
  myGLCD.clrScr();                        //Limpia la pantalla
  myGLCD.setColor(255, 255, 255);         //El orden es (rojo, verde, azul)
  myGLCD.fillRect(0, 0, 319, 239);        //Fondo blanco
  myGLCD.setColor(255, 255, 255); 
  myGLCD.fillRect(0, 0, 319, 13);         //Borde superior blanco
  myGLCD.fillRect(0, 226, 319, 239);      //Borde inferior blanco
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.print("* Master Ingenieria Industrial, UHU *", CENTER, 1);     //Texto de la parte superior de la pantalla//
  myGLCD.setColor(0,0,0);
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.print("Diseno electronico, curso 18-19", CENTER, 227);         //Texto de la parte inferior de la pantalla//

  /*SEMÁFORO*/
  vSemaphoreCreateBinary( xSem_Tft );   //Creación deL semáforo

  /*TESTEO DE TAREAS*/
  xReturned = xTaskCreate(
                  vTaskUpdateTFT,    
                  "UpdateTFT",
                  STACK_SIZE, 
                  NULL,  
                  tskIDLE_PRIORITY,     //Prioridad
                  NULL );
  if(xReturned != pdPASS) Serial.println("Error creando tarea 1");     //Testea la tarea 1
    
  xReturned = xTaskCreate(
                  vTaskSensorRead,  
                  "UpdateTFT",     
                  STACK_SIZE,
                  NULL,  
                  tskIDLE_PRIORITY,     //Prioridad
                  NULL ); 
  if (xReturned != pdPASS) Serial.println("Error creando tarea 2");     //Testea la tarea 2
  
  xReturned = xTaskCreate(
                  vTaskLogoTft,
                  "UpdateTFT",
                  STACK_SIZE, 
                  NULL, 
                  tskIDLE_PRIORITY,     //Prioridad
                  NULL );
  if (xReturned != pdPASS) Serial.println("Error creando tarea 3");     //Testea la tarea 3
}

void loop(){}

/*FUNCIÓN DE LECTURA DE LA TEMPERATURA
  En esta función accederemos a los sensores a través del bus 1-Wire*/
void valortemperatura(){
  //Alimentamos el sensor
  digitalWrite(51, HIGH);
  sensorDS18B20.requestTemperatures();
  temperatura = sensorDS18B20.getTempCByIndex(0);
  
  if((temperatura < t)){   // identifica el color a ser          
    myGLCD.setColor(0,255,0);  
  } 
  else{
    myGLCD.setColor(255,0,0);
  }

  //Solo se actualiza el valor de temperatura mostrado en pantalla si cambia respecto del anterior
  if (temperatura!=tempAnt){
    myGLCD.setBackColor(BLACK);
    myGLCD.setFont(BigFont);                        //Selecciona la fuente para la temperatura
    myGLCD.printNumF(temperatura,2,48,123,46,5);
    myGLCD.print("C",144,123);
    myGLCD.setFont(SmallFont);
    myGLCD.print("o",140,118);
           
    tempAnt=temperatura;                            //Guarda la temperatura actual como la anterior
  }
  return;
}

/*FUNCIÓN DE ENCENDIDO DE LA CALDERA EN MODO AUTOMÁTICO*/
void led_modoAUTO(){
  if (modo==0){
    if (temperatura>=t){    
      myGLCD.setColor(0,0,0);
      myGLCD.fillCircle(260,120,5);     //Apaga la caldera
    }
    else{
      myGLCD.setColor(255,123,0);
      myGLCD.fillCircle(260,120,5);     //Enciende la caldera
    }
  }
}
