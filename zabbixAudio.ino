
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include "config.hpp"
#include "EmonLib.h" //INCLUSÃO DE BIBLIOTECA Sensores de Tensão
#include "DHT.h" //INCLUSÃO DE BIBLIOTECA DE TEMPERATURA DHT11

//Pinos Utilizados
const int pintensao0 = 0; //P





























































INO ANALÓGICO UTILIZADO PELO SENSOR
const int pintensao1 = 2; //PINO ANALÓGICO UTILIZADO PELO SENSOR
const int pintensao2 = 1; //PINO ANALÓGICO UTILIZADO PELO SENSOR
const int pintemp    = A4; //PINO ANALÓGICO UTILIZADO PELO SENSOR
const int pinaudio1  = A5; //PINO ANALÓGICO UTILIZADO PELO SENSOR
const int pinaudio2  = A3; //PINO ANALÓGICO UTILIZADO PELO SENSOR

#define VOLT_CAL 215.3   //VALOR DE CALIBRAÇÃO (DEVE SER AJUSTADO EM PARALELO COM UM MULTÍMETRO)

#define DHTTYPE    DHT11     // DHT 22 (AM2302)
EnergyMonitor emon1; //CRIA UMA INSTÂNCIA
EnergyMonitor emon2; //CRIA UMA INSTÂNCIA
//dht DHT; //VARIÁVEL DO TIPO DHT
DHT dht(pintemp, DHTTYPE);
 
    int counter;
    float valor;
    float  t0;
    float  t1;
    float temperatura;
    float umidade;
    int audio1;
    int audio2;
byte mac[] = { MAC1, MAC2, MAC3, MAC4, MAC5, MAC6 };
IPAddress zabbix(ZABBIX_IP1, ZABBIX_IP2, ZABBIX_IP3, ZABBIX_IP4);

int zabbixPort = 10051;

int numeroDeVariaveis = 6; //COLOQUE AQUI A QUANTIDADE DE VARIÁVEIS QUE SERÃO ENVIADAS PARA O ZABBIX
 
void zabbix_send_trap(void);
 
void setup() {
    Serial.begin(9600);
     dht.begin();
    Serial.println("Obtaining IP address using DHCP...");
    while (Ethernet.begin(mac) != 1) {
        delay(10);
    }


    Serial.println("setup done.");

  emon1.voltage(pintensao0, VOLT_CAL, 1.7); //PASSA PARA A FUNÇÃO OS PARÂMETROS (PINO ANALÓGIO / VALOR DE CALIBRAÇÃO / MUDANÇA DE FASE)
  emon2.voltage(pintensao1, VOLT_CAL, 1.7); //PASSA PARA A FUNÇÃO OS PARÂMETROS (PINO ANALÓGIO / VALOR DE CALIBRAÇÃO / MUDANÇA DE FASE)
}
 
void loop() {
 
     zabbix_send_trap();
           
     delay(2000);
     
}
const char PROGMEM content_header[] = "{\"request\":\"sender data\",\"data\":[";
const char PROGMEM content_item[] = "{\"host\":\"%s\",\"key\":\"%s\",\"value\":\"%d\"}";
const char PROGMEM content_footer[] = "]}";
 
char host[] = "arduino2";     // hostname zabbix
char key[][15] = {"temperatura","umidade","tensao1", "tensao2", "audio1", "audio2"};            // DEFINA AQUI O NOME DAS VARIAVEIS (tamanho máximo definido pelo 11)
//char counterAtual[] = {temperatura,umidade,t0,t1};

unsigned int prepare_content(char *dst, char keyAtual[]) {
    unsigned int len = 0;
 
    memcpy(&dst[len], content_header, sizeof(content_header));
    len += sizeof(content_header) - 1;
    len += sprintf_P(&dst[len], content_item, host, keyAtual, counter);
    memcpy(&dst[len], content_footer, sizeof(content_footer));
    len += sizeof(content_footer) - 1;
    return len;
}
 
EthernetClient zbx_client;
char packet_header[] = "ZBXD\1"; //followed by unsigned long long content_len
unsigned long long content_len;
unsigned int payload_len;
char packet_content[256];
 
void zabbix_send_trap() {

   //sensores de tensão
  emon1.calcVI(17,500); //FUNÇÃO DE CÁLCULO (17 SEMICICLOS, TEMPO LIMITE PARA FAZER A MEDIÇÃO)  
  emon2.calcVI(17,500); //FUNÇÃO DE CÁLCULO (17 SEMICICLOS, TEMPO LIMITE PARA FAZER A MEDIÇÃO)  
  float t0  = emon1.Vrms; //VARIÁVEL RECEBE O VALOR DE TENSÃO RMS OBTIDO
  float t1  = emon2.Vrms; //VARIÁVEL RECEBE O VALOR DE TENSÃO RMS OBTIDO
  
  //sensor de temperatura
//  DHT.read11(pintemp); //LÊ AS INFORMAÇÕES DO SENSOR
  float  temperatura = dht.readTemperature(); //VARIÁVEL RECEBE A TEMPERATURA MEDIDA
  float  umidade = dht.readHumidity(); //VARIÁVEL RECEBE A UMIDADE MEDIDA
  audio1 = analogRead(pinaudio1);
  audio2 = analogRead(pinaudio2); 
       
    for(int i=0; i < sizeof(key)/sizeof(key[i]);i++){
       String item = String(key[i]);
       if (item == "temperatura")
            valor = temperatura;
       
       if (item == "umidade")
            valor = umidade;      
          
       if (item == "tensao1")
           valor = t0;

       if (item == "tensao2")
            valor = t1;

       if (item == "audio1")
            valor = audio1;
       
       if (item == "audio2")
            valor = audio2;
   

      counter = static_cast<int>(valor);
          Serial.println(key[i]);
          Serial.println(counter);
          Serial.println(valor);
                        
      content_len = prepare_content(packet_content, key[i]);
      payload_len = sizeof(content_header) + content_len + sizeof(content_footer);
      if (zbx_client.connect(zabbix, zabbixPort)) {
          Serial.println("connected to zabbix");
          Serial.println("sending data");
          zbx_client.write(packet_header, sizeof(packet_header) - 1);
          zbx_client.write((char *)&content_len, sizeof(content_len));
          zbx_client.write(packet_content, content_len);
          delay(1);
          zbx_client.stop();
          Serial.println("disconnected");
      }
    }
}
