/*
######################################################################
##      Integração das tecnologias da REMOTE IO com Node IOT        ##
##                          Version 1.0                             ##
##   Código base para implementação de projetos de digitalização de ##
##   processos, automação, coleta de dados e envio de comandos com  ##
##   controle embarcado e na nuvem.                                 ##
##                                                                  ##
######################################################################
*/

#include <ESP32RemoteIO.h>

RemoteIO device1;

void myCallback(String ref, String value)
{
/* Example:

  if (ref == "turnOnLight")
  {
    device1.updatePinOutput(ref);        // This function will update the IO pin linked to 'turnOnLight' variable on previously done NodeIoT device configuration
  }
  
*/

  //Serial.printf("ref: %s, value: %s\n", ref, value);
}

void setup() 
{
  device1.begin(myCallback);
}

void loop() 
{
  device1.loop();
}
