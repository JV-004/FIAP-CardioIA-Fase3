/**
 * CardioIA - Fase 3: Sistema Vestivel de Monitoramento Cardiaco
 * Edge Computing com ESP32 + Resiliencia Offline
 * Versao PlatformIO (identica ao sketch.ino com #include <Arduino.h>)
 */

#include <Arduino.h>
#include <DHT.h>

// ─── Pinos ────────────────────────────────────────────────────────────────────

#define PINO_DHT22        4
#define TIPO_DHT          DHT22
#define PINO_BPM          32
#define PINO_LED_WIFI     2
#define PINO_LED_ALERTA   15

// ─── Parametros do sistema ────────────────────────────────────────────────────

#define INTERVALO_LEITURA     5000
#define INTERVALO_WIFI        30000
#define MAX_REGISTROS_OFFLINE 20

// ─── Limites de alerta medico ─────────────────────────────────────────────────

#define BPM_LIMITE_ALTO      120
#define TEMP_LIMITE_ALTO     38.0
#define UMIDADE_LIMITE_BAIXO 40.0
#define UMIDADE_LIMITE_ALTO  80.0

// ─── Estrutura de dados ───────────────────────────────────────────────────────

struct LeituraVital {
  unsigned long timestamp;
  int   bpm;
  float temperatura;
  float umidade;
  bool  alertaAtivo;
};

// ─── Variaveis globais ────────────────────────────────────────────────────────

DHT sensorDHT(PINO_DHT22, TIPO_DHT);

LeituraVital filaOffline[MAX_REGISTROS_OFFLINE];
int totalRegistrosArmazenados = 0;

bool          wifiConectado    = false;
unsigned long ultimoToggleWifi = 0;
unsigned long ultimaLeitura    = 0;

unsigned long totalLeituras       = 0;
unsigned long totalAlertasGerados = 0;

// ─── Prototipo ────────────────────────────────────────────────────────────────

bool verificarAlertas(LeituraVital leitura);

// ─── Funcoes ─────────────────────────────────────────────────────────────────

int lerBPM() {
  randomSeed(millis() ^ analogRead(PINO_BPM));
  return random(60, 116);
}

bool lerTemperaturaUmidade(float* temperatura, float* umidade) {
  *temperatura = sensorDHT.readTemperature();
  *umidade     = sensorDHT.readHumidity();

  if (isnan(*temperatura) || isnan(*umidade)) {
    Serial.println("[ERRO] Falha na leitura do DHT22");
    *temperatura = -999.0;
    *umidade     = -999.0;
    return false;
  }
  return true;
}

LeituraVital coletarDadosVitais() {
  LeituraVital leitura;
  leitura.timestamp = millis();
  leitura.bpm       = lerBPM();
  lerTemperaturaUmidade(&leitura.temperatura, &leitura.umidade);
  leitura.alertaAtivo = verificarAlertas(leitura);
  totalLeituras++;
  if (leitura.alertaAtivo) totalAlertasGerados++;
  return leitura;
}

bool verificarAlertas(LeituraVital leitura) {
  bool alerta = false;

  if (leitura.bpm > BPM_LIMITE_ALTO) {
    Serial.print("[ALERTA] Taquicardia: ");
    Serial.print(leitura.bpm);
    Serial.println(" BPM");
    alerta = true;
  }

  if (leitura.temperatura > TEMP_LIMITE_ALTO) {
    Serial.print("[ALERTA] Febre: ");
    Serial.print(leitura.temperatura, 1);
    Serial.println(" C");
    alerta = true;
  }

  if (leitura.umidade < UMIDADE_LIMITE_BAIXO || leitura.umidade > UMIDADE_LIMITE_ALTO) {
    Serial.print("[ALERTA] Umidade inadequada: ");
    Serial.print(leitura.umidade, 1);
    Serial.println("%");
    alerta = true;
  }

  return alerta;
}

void atualizarLEDs(bool alertaAtivo) {
  digitalWrite(PINO_LED_WIFI,   wifiConectado ? HIGH : LOW);
  digitalWrite(PINO_LED_ALERTA, alertaAtivo   ? HIGH : LOW);
}

void armazenarOffline(LeituraVital leitura) {
  if (totalRegistrosArmazenados < MAX_REGISTROS_OFFLINE) {
    filaOffline[totalRegistrosArmazenados] = leitura;
    totalRegistrosArmazenados++;
  } else {
    for (int i = 0; i < MAX_REGISTROS_OFFLINE - 1; i++) {
      filaOffline[i] = filaOffline[i + 1];
    }
    filaOffline[MAX_REGISTROS_OFFLINE - 1] = leitura;
    Serial.println("[EDGE] Fila cheia - registro mais antigo descartado");
  }
}

void imprimirLeitura(LeituraVital leitura, String status) {
  Serial.print("[");
  Serial.print(leitura.timestamp);
  Serial.print("ms] BPM: ");
  Serial.print(leitura.bpm);
  Serial.print(" | Temp: ");
  Serial.print(leitura.temperatura, 1);
  Serial.print("C | Umidade: ");
  Serial.print(leitura.umidade, 1);
  Serial.print("% | Status: ");
  Serial.print(status);
  if (leitura.alertaAtivo) Serial.print(" | ALERTA");
  Serial.println();
}

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
  }

  totalRegistrosArmazenados = 0;

  Serial.println("========================================");
  Serial.println("[SYNC] Concluido. Fila limpa.");
  Serial.println("========================================");
}

void simularConectividadeWiFi() {
  if (millis() - ultimoToggleWifi >= INTERVALO_WIFI) {
    ultimoToggleWifi = millis();
    wifiConectado = !wifiConectado;

    if (wifiConectado) {
      Serial.println("\n[WIFI] CONECTADO - Modo online ativado");
      Serial.println("   Iniciando sincronizacao automatica...");
      sincronizarDadosOffline();
    } else {
      Serial.println("\n[WIFI] DESCONECTADO - Modo offline ativado");
      Serial.println("   Armazenamento local (Edge Computing) ativo");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  pinMode(PINO_LED_WIFI,   OUTPUT);
  pinMode(PINO_LED_ALERTA, OUTPUT);

  sensorDHT.begin();
  delay(2000);

  Serial.println("========================================");
  Serial.println("  CardioIA - Monitoramento Cardiaco");
  Serial.println("  Fase 3: Edge Computing + IoT Medico");
  Serial.println("========================================");
  Serial.println("Hardware:");
  Serial.println("  - ESP32 DevKit v1");
  Serial.println("  - DHT22 (Temp + Umidade) - GPIO4");
  Serial.println("  - Potenciometro BPM      - GPIO34");
  Serial.println("  - LED WiFi (verde)        - GPIO2");
  Serial.println("  - LED Alerta (vermelho)   - GPIO15");
  Serial.println("Configuracoes:");
  Serial.print("  - Leitura a cada: ");
  Serial.print(INTERVALO_LEITURA / 1000);
  Serial.println("s");
  Serial.print("  - Fila offline: ");
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

  ultimoToggleWifi = millis();
  ultimaLeitura    = millis();
  wifiConectado    = false;
  atualizarLEDs(false);
}

void loop() {
  simularConectividadeWiFi();

  if (millis() - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = millis();

    LeituraVital leitura = coletarDadosVitais();
    atualizarLEDs(leitura.alertaAtivo);

    if (wifiConectado) {
      imprimirLeitura(leitura, "ONLINE");
      Serial.println("    -> Enviado para nuvem via MQTT");
    } else {
      armazenarOffline(leitura);
      imprimirLeitura(leitura, "OFFLINE");
      Serial.print("    -> Fila local: [");
      Serial.print(totalRegistrosArmazenados);
      Serial.print("/");
      Serial.print(MAX_REGISTROS_OFFLINE);
      Serial.println("]");
    }

    if (totalLeituras % 10 == 0) {
      Serial.print("\n[STATS] Leituras: ");
      Serial.print(totalLeituras);
      Serial.print(" | Alertas: ");
      Serial.print(totalAlertasGerados);
      Serial.println("\n");
    }
  }

  delay(100);
}
