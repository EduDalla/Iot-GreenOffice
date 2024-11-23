#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configuração do DHT22
#define DHTPIN 12       // Pino do sensor DHT22
#define DHTTYPE DHT22  // Tipo do sensor
DHT dht(DHTPIN, DHTTYPE);

// Configuração do sensor PIR
#define PIRPIN 4       // Pino do sensor PIR
int pirState = LOW;

// Configuração do potenciômetro simulando o MQ135
#define POTPIN 32      // Pino analógico para o potenciômetro
float qualidadeAr;     // Variável para armazenar a qualidade do ar

// Configuração do Wi-Fi e MQTT
const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";
const char* BROKER_MQTT = "broker.hivemq.com";
const int BROKER_PORT = 1883;
const char* ID_MQTT = "greenoffice";
const char* TOPIC_DADOS = "/greenoffice_dados";

WiFiClient espClient;
PubSubClient client(espClient);

// Variáveis do sistema
float temperatura;
float umidade;
bool arCondicionadoLigado = false;

// Função para conectar ao Wi-Fi
void setup_wifi() {
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("Endereço de IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("A conexão WiFi falhou. Continuando o procedimento sem o WiFi.");
  }
}

// Função para conectar ao MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Conectando ao MQTT...");
    if (client.connect("ESP32GreenOffice")) {
      Serial.println("Conectado ao MQTT.");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Configuração inicial
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PIRPIN, INPUT);

  setup_wifi();
  client.setServer(BROKER_MQTT, BROKER_PORT);
  client.setCallback(callback);
}

// Loop principal
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leitura do sensor DHT22
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();

  // Leitura do sensor PIR
  int pirLeitura = digitalRead(PIRPIN);
  pirState = pirLeitura == HIGH ? HIGH : LOW;

  // Lógica para ligar/desligar o ar-condicionado com base na presença
  arCondicionadoLigado = (pirState == HIGH);

  // Leitura do potenciômetro simulando o MQ135
  int leituraPot = analogRead(POTPIN);
  qualidadeAr = map(leituraPot, 0, 1023, 300, 500); // Base inicial de qualidade

  // Ajuste da qualidade do ar baseado no movimento
  if (pirState == HIGH) {
    qualidadeAr -= 50; // Reduz a qualidade do ar na presença de movimento
  } else {
    qualidadeAr += 10; // Melhora a qualidade do ar gradativamente sem movimento
  }

  // Garante que o valor fique dentro de um intervalo lógico
  qualidadeAr = constrain(qualidadeAr, 0, 500);

  // Monta o payload JSON
  String payload = "{";
  payload += "\"temperatura\": " + String(temperatura, 1) + ",";
  payload += "\"umidade\": " + String(umidade, 1) + ",";
  payload += "\"qualidade_ar\": " + String(qualidadeAr, 1) + ",";
  payload += "\"status\": \"" + String(arCondicionadoLigado ? "ON" : "OFF") + "\"";
  payload += "}";

  // Publica no tópico MQTT
  client.publish(TOPIC_DADOS, payload.c_str());
  Serial.println("Dados enviados: " + payload);

  delay(2000); // Aguarda 2 segundos antes da próxima leitura
}
