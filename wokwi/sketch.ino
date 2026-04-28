/**
 * CardioIA — Fase 3 | Parte 1: Edge Computing
 * Sistema vestível de monitoramento cardíaco com ESP32
 *
 * Sensores utilizados:
 *   - DHT22: temperatura (°C) e umidade relativa (%)
 *   - Potenciômetro: simula batimentos cardíacos (BPM)
 *
 * Lógica de resiliência offline:
 *   - Quando sem conexão, dados são armazenados em fila local (máx. 20 registros)
 *   - Quando conexão volta, todos os dados são sincronizados via Serial e a fila é limpa
 *   - Conectividade é alternada a cada 30 segundos para demonstrar o ciclo
 */

#include <DHT.h>

// ─── Configuração dos pinos ───────────────────────────────────────────────────

#define PINO_DHT     4   // Pino de dados do sensor DHT22
#define TIPO_DHT     DHT22
#define PINO_POT     34  // Pino analógico do potenciômetro (GPIO34 = ADC1_CH6)

// ─── Parâmetros do sistema ────────────────────────────────────────────────────

#define INTERVALO_LEITURA   5000   // Intervalo entre leituras: 5 segundos (ms)
#define INTERVALO_WIFI      30000  // Intervalo para alternar conectividade: 30 segundos (ms)
#define MAX_REGISTROS       20     // Capacidade máxima da fila offline

// ─── Estrutura de um registro de leitura ─────────────────────────────────────

struct Leitura {
  unsigned long timestamp; // Tempo em ms desde o boot
  int    bpm;              // Batimentos por minuto
  float  temperatura;      // Temperatura em graus Celsius
  float  umidade;          // Umidade relativa em %
};

// ─── Variáveis globais ────────────────────────────────────────────────────────

DHT dht(PINO_DHT, TIPO_DHT);

// Fila circular para armazenamento offline
Leitura filaOffline[MAX_REGISTROS];
int totalArmazenado = 0; // Quantidade de registros atualmente na fila

// Controle de conectividade simulada
bool wifiConectado = false;          // Estado atual da conexão
unsigned long ultimoToggleWifi = 0;  // Último momento em que o estado foi alternado
unsigned long ultimaLeitura    = 0;  // Último momento em que os sensores foram lidos

// ─── Funções ──────────────────────────────────────────────────────────────────

/**
 * lerBPM()
 * Lê o valor analógico do potenciômetro (0–4095) e mapeia
 * para uma faixa realista de batimentos cardíacos: 40–180 BPM.
 * Retorna o valor inteiro de BPM.
 */
int lerBPM() {
  int valorADC = analogRead(PINO_POT); // Leitura bruta do ADC (0 a 4095)
  int bpm = map(valorADC, 0, 4095, 40, 180); // Mapeia para 40–180 BPM
  return bpm;
}

/**
 * lerSensores()
 * Lê todos os sensores e retorna uma struct Leitura preenchida.
 * Temperatura em °C, umidade em %, BPM mapeado do potenciômetro.
 */
Leitura lerSensores() {
  Leitura l;
  l.timestamp   = millis();           // Tempo em ms desde o boot do ESP32
  l.bpm         = lerBPM();           // BPM simulado pelo potenciômetro
  l.temperatura = dht.readTemperature(); // Temperatura em graus Celsius
  l.umidade     = dht.readHumidity();    // Umidade relativa do ar em %

  // Proteção contra falha de leitura do DHT22
  if (isnan(l.temperatura)) l.temperatura = -1.0;
  if (isnan(l.umidade))     l.umidade     = -1.0;

  return l;
}

/**
 * armazenarOffline()
 * Adiciona uma leitura na fila local.
 * Se a fila estiver cheia (20 registros), descarta o registro mais antigo
 * deslocando todos os elementos e inserindo o novo no final.
 */
void armazenarOffline(Leitura l) {
  if (totalArmazenado < MAX_REGISTROS) {
    // Ainda há espaço: insere normalmente
    filaOffline[totalArmazenado] = l;
    totalArmazenado++;
  } else {
    // Fila cheia: descarta o mais antigo (índice 0) e desloca os demais
    for (int i = 0; i < MAX_REGISTROS - 1; i++) {
      filaOffline[i] = filaOffline[i + 1];
    }
    filaOffline[MAX_REGISTROS - 1] = l; // Insere o novo no final
    Serial.println("[AVISO] Fila cheia — registro mais antigo descartado.");
  }
}

/**
 * imprimirLeitura()
 * Formata e imprime uma leitura no Monitor Serial no padrão:
 * [TIMESTAMP] BPM: X | Temp: X°C | Umidade: X% | Status: ONLINE/OFFLINE
 */
void imprimirLeitura(Leitura l, String status) {
  Serial.print("[");
  Serial.print(l.timestamp);
  Serial.print("ms] BPM: ");
  Serial.print(l.bpm);
  Serial.print(" | Temp: ");
  Serial.print(l.temperatura, 1); // 1 casa decimal
  Serial.print("C | Umidade: ");
  Serial.print(l.umidade, 1);     // 1 casa decimal
  Serial.print("% | Status: ");
  Serial.println(status);
}

/**
 * sincronizarFila()
 * Quando a conexão Wi-Fi volta, imprime todos os registros armazenados
 * na fila offline via Serial (simulando envio para a nuvem) e limpa a fila.
 */
void sincronizarFila() {
  if (totalArmazenado == 0) return; // Nada a sincronizar

  Serial.println("========================================");
  Serial.print("[SYNC] Sincronizando ");
  Serial.print(totalArmazenado);
  Serial.println(" registro(s) armazenado(s) offline...");
  Serial.println("========================================");

  for (int i = 0; i < totalArmazenado; i++) {
    imprimirLeitura(filaOffline[i], "SYNC→ONLINE");
  }

  // Limpa a fila após sincronização
  totalArmazenado = 0;

  Serial.println("========================================");
  Serial.println("[SYNC] Sincronização concluída. Fila limpa.");
  Serial.println("========================================");
}

/**
 * alternarWifi()
 * Simula a alternância de conectividade a cada 30 segundos.
 * Quando passa de offline para online, dispara a sincronização da fila.
 */
void alternarWifi() {
  if (millis() - ultimoToggleWifi >= INTERVALO_WIFI) {
    ultimoToggleWifi = millis();
    wifiConectado = !wifiConectado; // Inverte o estado

    if (wifiConectado) {
      Serial.println("\n>>> Wi-Fi CONECTADO — iniciando sincronização...");
      sincronizarFila(); // Envia dados acumulados offline
    } else {
      Serial.println("\n>>> Wi-Fi DESCONECTADO — modo offline ativado.");
    }
  }
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200); // Inicializa comunicação serial a 115200 baud
  dht.begin();          // Inicializa o sensor DHT22

  Serial.println("========================================");
  Serial.println("  CardioIA — Monitoramento Cardíaco");
  Serial.println("  Fase 3 | Parte 1: Edge Computing");
  Serial.println("========================================");
  Serial.println("Sistema iniciado. Wi-Fi começa DESCONECTADO.");
  Serial.println("Conectividade alterna a cada 30 segundos.");
  Serial.println("Leituras a cada 5 segundos.");
  Serial.println("========================================\n");

  ultimoToggleWifi = millis(); // Marca o início do ciclo de alternância
  ultimaLeitura    = millis();
}

// ─── Loop principal ───────────────────────────────────────────────────────────

void loop() {
  // 1. Verifica se é hora de alternar o estado do Wi-Fi
  alternarWifi();

  // 2. Verifica se é hora de fazer uma nova leitura (a cada 5 segundos)
  if (millis() - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = millis();

    Leitura leitura = lerSensores(); // Coleta dados de todos os sensores

    if (wifiConectado) {
      // Online: imprime diretamente (simula envio imediato para a nuvem)
      imprimirLeitura(leitura, "ONLINE");
    } else {
      // Offline: armazena na fila local e imprime com status OFFLINE
      armazenarOffline(leitura);
      imprimirLeitura(leitura, "OFFLINE");
      Serial.print("    [Fila local: ");
      Serial.print(totalArmazenado);
      Serial.print("/");
      Serial.print(MAX_REGISTROS);
      Serial.println(" registros]");
    }
  }
}
