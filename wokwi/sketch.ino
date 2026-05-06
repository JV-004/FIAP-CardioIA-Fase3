/**
 * CardioIA - Fase 3: Sistema Vestivel de Monitoramento Cardiaco
 * Edge Computing com ESP32 + Resiliencia Offline
 *
 * Descricao: Sistema que monitora sinais vitais (BPM, temperatura, umidade)
 * com capacidade de armazenamento local quando offline e sincronizacao
 * automatica quando a conectividade retorna.
 *
 * Hardware:
 *   - ESP32 DevKit v1
 *   - DHT22 (GPIO4)   - Temperatura e umidade (com resistor pull-up 10k entre VCC e SDA)
 *   - Potenciometro (GPIO34) - Simula sensor de BPM
 *   - LED Verde (GPIO2)  - Indica status WiFi
 *   - LED Vermelho (GPIO15) - Indica alertas medicos
 *
 * Autor: Equipe CardioIA
 */

#include <DHT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ─── Pinos ────────────────────────────────────────────────────────────────────

#define PINO_DHT22        4    // Pino de dados do sensor DHT22
#define TIPO_DHT          DHT22
#define PINO_BPM          32   // Pino analogico do potenciometro (simula BPM)
#define PINO_LED_WIFI     2    // LED verde  - status da conexao WiFi
#define PINO_LED_ALERTA   15   // LED vermelho - alertas medicos

// ─── Parametros do sistema ────────────────────────────────────────────────────

#define INTERVALO_LEITURA     5000   // Intervalo entre leituras: 5 segundos (ms)
#define INTERVALO_WIFI        30000  // Intervalo para alternar WiFi: 30 segundos (ms)
#define MAX_REGISTROS_OFFLINE 20     // Capacidade maxima da fila offline

// ─── Limites de alerta medico ─────────────────────────────────────────────────

#define BPM_LIMITE_ALTO      120    // Taquicardia: > 120 BPM
#define TEMP_LIMITE_ALTO     38.0   // Febre: > 38 graus C
#define UMIDADE_LIMITE_BAIXO 40.0   // Ambiente seco: < 40%
#define UMIDADE_LIMITE_ALTO  80.0   // Ambiente umido: > 80%

// ─── Estrutura de dados ───────────────────────────────────────────────────────

/**
 * LeituraVital
 * Representa uma leitura completa dos sensores com timestamp e status de alerta.
 */
struct LeituraVital {
  unsigned long timestamp; // Tempo em ms desde o boot do ESP32
  int   bpm;               // Batimentos por minuto (40-180 BPM)
  float temperatura;       // Temperatura corporal em graus Celsius
  float umidade;           // Umidade relativa do ar em porcentagem
  bool  alertaAtivo;       // true se algum parametro esta em estado de alerta
};

// ─── Variaveis globais ────────────────────────────────────────────────────────

DHT sensorDHT(PINO_DHT22, TIPO_DHT);

LeituraVital filaOffline[MAX_REGISTROS_OFFLINE]; // Fila de armazenamento offline
int totalRegistrosArmazenados = 0;               // Quantidade atual na fila

bool          wifiConectado    = false; // Estado simulado da conexao WiFi
unsigned long ultimoToggleWifi = 0;    // Ultimo momento de alternancia do WiFi
unsigned long ultimaLeitura    = 0;    // Ultimo momento de leitura dos sensores

unsigned long totalLeituras       = 0; // Contador de leituras realizadas
unsigned long totalAlertasGerados = 0; // Contador de alertas gerados

// ─── Configuracao WiFi e MQTT ─────────────────────────────────────────────────

const char* ssid = "Wokwi-GUEST";
const char* senhaWifi = "";

const char* mqttServer = "de51ea6bec7c4a3a99b79d3346186628.s1.eu.hivemq.cloud";
const int mqttPort = 8883;

const char* mqttUser = "cardio";
const char* mqttPassword = "Cardio123";

const char* topicoDados = "cardio/dados";
const char* topicoAlerta = "cardio/alerta";

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// ─── Prototipo necessario (verificarAlertas usa LeituraVital) ─────────────────

bool verificarAlertas(LeituraVital leitura);
void conectarWiFiReal();
void conectarMQTT();
void publicarMQTT(LeituraVital leitura, String origem);

// ─── Conexao WiFi real e MQTT ─────────────────────────────────────────────────

void conectarWiFiReal() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("[WIFI REAL] Conectando ao Wokwi-GUEST");

  WiFi.begin(ssid, senhaWifi);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI REAL] Conectado!");
  } else {
    Serial.println("\n[WIFI REAL] Falha na conexao.");
  }
}

void conectarMQTT() {
  if (mqttClient.connected()) return;

  wifiClient.setInsecure();
  mqttClient.setServer(mqttServer, mqttPort);

  Serial.print("[MQTT] Conectando ao HiveMQ");

  int tentativas = 0;
  while (!mqttClient.connected() && tentativas < 5) {
    String clientId = "CardioIA-ESP32-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("\n[MQTT] Conectado!");
    } else {
      Serial.print(".");
      Serial.print(" erro=");
      Serial.print(mqttClient.state());
      delay(2000);
      tentativas++;
    }
  }
}

void publicarMQTT(LeituraVital leitura, String origem) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[MQTT] WiFi real indisponivel.");
    return;
  }

  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Broker indisponivel.");
    return;
  }

  String json = "{";
  json += "\"timestamp\":" + String(leitura.timestamp) + ",";
  json += "\"bpm\":" + String(leitura.bpm) + ",";
  json += "\"temperatura\":" + String(leitura.temperatura, 1) + ",";
  json += "\"umidade\":" + String(leitura.umidade, 1) + ",";
  json += "\"alerta\":" + String(leitura.alertaAtivo ? "true" : "false") + ",";
  json += "\"origem\":\"" + origem + "\"";
  json += "}";

  mqttClient.publish(topicoDados, json.c_str());

  Serial.print("[MQTT] Publicado em cardio/dados: ");
  Serial.println(json);

  if (leitura.alertaAtivo) {
    mqttClient.publish(topicoAlerta, json.c_str());
    Serial.println("[MQTT] Alerta publicado em cardio/alerta");
  }
}

// ─── Funcoes de leitura dos sensores ─────────────────────────────────────────

/**
 * lerBPM()
 * Simula leitura de sensor de BPM com valores aleatorios realistas (60-115 BPM).
 * O potenciometro no simulador Wokwi com board-esp32-devkit-c-v4 sempre retorna
 * o valor maximo do ADC, por isso usamos geracao aleatoria para simular variacao
 * natural dos batimentos cardiacos em repouso.
 * Em hardware real, substituir por: return map(analogRead(PINO_BPM), 0, 4095, 40, 180);
 */
int lerBPM() {
  randomSeed(millis() ^ analogRead(PINO_BPM)); // Semente mista para maior variacao
  return random(60, 116); // Faixa normal de repouso: 60-115 BPM
}

/**
 * lerTemperaturaUmidade()
 * Le o sensor DHT22 e preenche os ponteiros com temperatura (Celsius) e umidade (%).
 * Retorna true se a leitura foi valida, false se o sensor retornou NaN.
 */
bool lerTemperaturaUmidade(float* temperatura, float* umidade) {
  *temperatura = sensorDHT.readTemperature(); // Temperatura em Celsius
  *umidade     = sensorDHT.readHumidity();    // Umidade relativa em %

  if (isnan(*temperatura) || isnan(*umidade)) {
    Serial.println("[ERRO] Falha na leitura do DHT22");
    *temperatura = -999.0; // Valor sentinela para erro
    *umidade     = -999.0;
    return false;
  }
  return true;
}

/**
 * coletarDadosVitais()
 * Le todos os sensores, monta e retorna uma LeituraVital completa.
 */
LeituraVital coletarDadosVitais() {
  LeituraVital leitura;

  leitura.timestamp = millis(); // Registra o momento da leitura
  leitura.bpm       = lerBPM();
  lerTemperaturaUmidade(&leitura.temperatura, &leitura.umidade);
  leitura.alertaAtivo = verificarAlertas(leitura);

  totalLeituras++;
  if (leitura.alertaAtivo) totalAlertasGerados++;

  return leitura;
}

// ─── Sistema de alertas medicos ───────────────────────────────────────────────

/**
 * verificarAlertas()
 * Analisa os parametros vitais e retorna true se algum limite foi ultrapassado.
 * Condicoes verificadas: taquicardia, febre, umidade inadequada.
 */
bool verificarAlertas(LeituraVital leitura) {
  bool alerta = false;

  if (leitura.bpm > BPM_LIMITE_ALTO) {
    Serial.print("[ALERTA] Taquicardia: ");
    Serial.print(leitura.bpm);
    Serial.println(" BPM");
    alerta = true;
  }

  if (leitura.temperatura > TEMP_LIMITE_ALTO && leitura.temperatura != -999.0) {
    Serial.print("[ALERTA] Febre: ");
    Serial.print(leitura.temperatura, 1);
    Serial.println(" C");
    alerta = true;
  }

  if (leitura.umidade != -999.0 &&
      (leitura.umidade < UMIDADE_LIMITE_BAIXO || leitura.umidade > UMIDADE_LIMITE_ALTO)) {
    Serial.print("[ALERTA] Umidade inadequada: ");
    Serial.print(leitura.umidade, 1);
    Serial.println("%");
    alerta = true;
  }

  return alerta;
}

/**
 * atualizarLEDs()
 * LED verde aceso = WiFi conectado.
 * LED vermelho aceso = alerta medico ativo.
 */
void atualizarLEDs(bool alertaAtivo) {
  digitalWrite(PINO_LED_WIFI,   wifiConectado ? HIGH : LOW);
  digitalWrite(PINO_LED_ALERTA, alertaAtivo   ? HIGH : LOW);
}

// ─── Armazenamento offline (Edge Computing) ───────────────────────────────────

/**
 * armazenarOffline()
 * Insere uma leitura na fila local.
 * Quando a fila esta cheia (20 registros), descarta o mais antigo (FIFO).
 */
void armazenarOffline(LeituraVital leitura) {
  if (totalRegistrosArmazenados < MAX_REGISTROS_OFFLINE) {
    filaOffline[totalRegistrosArmazenados] = leitura;
    totalRegistrosArmazenados++;
  } else {
    // Desloca todos os elementos para liberar espaco no final (politica FIFO)
    for (int i = 0; i < MAX_REGISTROS_OFFLINE - 1; i++) {
      filaOffline[i] = filaOffline[i + 1];
    }
    filaOffline[MAX_REGISTROS_OFFLINE - 1] = leitura;
    Serial.println("[EDGE] Fila cheia - registro mais antigo descartado");
  }
}

/**
 * imprimirLeitura()
 * Imprime uma leitura no formato:
 * [TIMESTAMPms] BPM: X | Temp: X.XC | Umidade: X.X% | Status: OFFLINE/ONLINE
 */
void imprimirLeitura(LeituraVital leitura, String status) {
  Serial.print("[");
  Serial.print(leitura.timestamp);
  Serial.print("ms] BPM: ");
  Serial.print(leitura.bpm);
  Serial.print(" | Temp: ");
  Serial.print(leitura.temperatura, 1); // 1 casa decimal
  Serial.print("C | Umidade: ");
  Serial.print(leitura.umidade, 1);     // 1 casa decimal
  Serial.print("% | Status: ");
  Serial.print(status);
  if (leitura.alertaAtivo) Serial.print(" | ALERTA");
  Serial.println();
}

/**
 * sincronizarDadosOffline()
 * Envia todos os registros da fila via Serial (simula envio para nuvem via MQTT)
 * e limpa a fila apos a sincronizacao.
 */
void sincronizarDadosOffline() {
  if (totalRegistrosArmazenados == 0) {
    Serial.println("[SYNC] Nenhum dado offline para sincronizar");
    return;
  }

  Serial.println("========================================");
  Serial.print("[SYNC] Sincronizando ");
  Serial.print(totalRegistrosArmazenados);
  Serial.println(" registro(s) offline...");
  Serial.println("========================================");

  for (int i = 0; i < totalRegistrosArmazenados; i++) {
    imprimirLeitura(filaOffline[i], "SYNC->CLOUD");

    publicarMQTT(filaOffline[i], "fila_offline");  // Envia o dado salvo para a nuvem via MQTT
    delay(300);                                    // Pequeno intervalo para evitar envio muito rápido
  }

  totalRegistrosArmazenados = 0; // Limpa a fila apos sincronizacao

  Serial.println("========================================");
  Serial.println("[SYNC] Concluido. Fila limpa.");
  Serial.println("========================================");
}

// ─── Simulacao de conectividade WiFi ─────────────────────────────────────────

/**
 * simularConectividadeWiFi()
 * Alterna o estado do WiFi a cada 30 segundos.
 * Ao reconectar, dispara a sincronizacao automatica dos dados offline.
 */
void simularConectividadeWiFi() {
  if (millis() - ultimoToggleWifi >= INTERVALO_WIFI) {
    ultimoToggleWifi = millis();
    wifiConectado = !wifiConectado;

    if (wifiConectado) {
      Serial.println("\n[WIFI] CONECTADO - Modo online ativado");
      Serial.println("   Iniciando sincronizacao automatica...");
      conectarWiFiReal();
      conectarMQTT();
      sincronizarDadosOffline();
    } else {
      Serial.println("\n[WIFI] DESCONECTADO - Modo offline ativado");
      Serial.println("   Armazenamento local (Edge Computing) ativo");
    }
  }
}

// ─── Setup ────────────────────────────────────────────────────────────────────

/**
 * setup()
 * Inicializa Serial, pinos, sensor DHT22 e exibe informacoes do sistema.
 * CORRECAO: delays reduzidos para 500ms — valores maiores causavam perda
 * das primeiras mensagens no Monitor Serial do Wokwi.
 */
void setup() {
  Serial.begin(115200);
  delay(500); // CORRIGIDO: era 3000ms — reduzido para nao travar o Monitor Serial

  pinMode(PINO_LED_WIFI,   OUTPUT);
  pinMode(PINO_LED_ALERTA, OUTPUT);

  // Garante LEDs apagados na inicializacao
  digitalWrite(PINO_LED_WIFI,   LOW);
  digitalWrite(PINO_LED_ALERTA, LOW);

  sensorDHT.begin();
  delay(2000); // DHT22 precisa de tempo para estabilizar no Wokwi

  Serial.println("========================================");
  Serial.println("  CardioIA - Monitoramento Cardiaco");
  Serial.println("  Fase 3: Edge Computing + IoT Medico");
  Serial.println("========================================");
  Serial.println("Hardware:");
  Serial.println("  - ESP32 DevKit v1");
  Serial.println("  - DHT22 (Temp + Umidade) - GPIO4");
  Serial.println("  - Potenciometro BPM      - GPIO32");
  Serial.println("  - LED WiFi (verde)        - GPIO2");
  Serial.println("  - LED Alerta (vermelho)   - GPIO15");
  Serial.println("Configuracoes:");
  Serial.print("  - Leitura a cada: ");
  Serial.print(INTERVALO_LEITURA / 1000);
  Serial.println("s");
  Serial.print("  - Fila offline max: ");
  Serial.print(MAX_REGISTROS_OFFLINE);
  Serial.println(" registros");
  Serial.print("  - Alternancia WiFi: ");
  Serial.print(INTERVALO_WIFI / 1000);
  Serial.println("s");
  Serial.println("Limites de alerta:");
  Serial.print("  - Taquicardia: BPM > ");
  Serial.println(BPM_LIMITE_ALTO);
  Serial.print("  - Febre: Temp > ");
  Serial.print(TEMP_LIMITE_ALTO, 1);
  Serial.println(" C");
  Serial.print("  - Umidade: < ");
  Serial.print(UMIDADE_LIMITE_BAIXO, 0);
  Serial.print("% ou > ");
  Serial.print(UMIDADE_LIMITE_ALTO, 0);
  Serial.println("%");
  Serial.println("========================================");

  // Teste inicial dos sensores
  Serial.println("[TESTE] Verificando sensores...");

  int bpmTeste = lerBPM();
  Serial.print("  - BPM inicial: ");
  Serial.print(bpmTeste);
  Serial.println(" BPM");

  float tempTeste, umidTeste;
  if (lerTemperaturaUmidade(&tempTeste, &umidTeste)) {
    Serial.print("  - Temperatura: ");
    Serial.print(tempTeste, 1);
    Serial.println(" C");
    Serial.print("  - Umidade: ");
    Serial.print(umidTeste, 1);
    Serial.println("%");
  }

  Serial.println("[OK] Sistema pronto. WiFi inicia DESCONECTADO.");
  Serial.println("========================================\n");
  Serial.flush(); // ADICIONADO: garante que todo o buffer foi enviado ao Monitor Serial

  // Inicializa os timers apos o setup completo
  ultimoToggleWifi = millis();
  ultimaLeitura    = millis();
  wifiConectado    = false;
  atualizarLEDs(false);
}

// ─── Loop principal ───────────────────────────────────────────────────────────

/**
 * loop()
 * Gerencia a coleta de dados, conectividade simulada e armazenamento.
 */
void loop() {
  // 1. Verifica alternancia de WiFi (a cada 30s)
  simularConectividadeWiFi();

  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  // 2. Verifica se e hora de nova leitura (a cada 5s)
  if (millis() - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = millis();

    LeituraVital leitura = coletarDadosVitais();
    atualizarLEDs(leitura.alertaAtivo);

    if (wifiConectado) {
      // Online: imprime diretamente (simula envio via MQTT)
      imprimirLeitura(leitura, "ONLINE");
      publicarMQTT(leitura, "tempo_real");
      Serial.println("    -> Enviado para nuvem via MQTT");
    } else {
      // Offline: armazena na fila local (Edge Computing)
      armazenarOffline(leitura);
      imprimirLeitura(leitura, "OFFLINE");
      Serial.print("    -> Fila local: [");
      Serial.print(totalRegistrosArmazenados);
      Serial.print("/");
      Serial.print(MAX_REGISTROS_OFFLINE);
      Serial.println("]");
    }

    // Exibe estatisticas a cada 10 leituras
    if (totalLeituras % 10 == 0) {
      Serial.print("\n[STATS] Leituras: ");
      Serial.print(totalLeituras);
      Serial.print(" | Alertas: ");
      Serial.print(totalAlertasGerados);
      Serial.println("\n");
    }
  }

  delay(100); // Evita sobrecarga do processador
}
