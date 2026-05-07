# CardioIA - Fase 3

Projeto FIAP - Monitoramento Continuo IoT na Saude.

**Link do projeto Wokwi:** https://wokwi.com/projects/463299672900584449

---

## O que foi implementado

**Apenas a Parte 1 — Membro 1 (Hardware): ESP32 + sensores no Wokwi**

Esta entrega cobre exclusivamente o circuito e firmware do ESP32 com Edge Computing.
MQTT, Node-RED, dashboard, Grafana e relatórios são responsabilidade dos demais membros.

---

## Checklist da Parte 1

- [x] Circuito montado no Wokwi (ESP32 + DHT22 + potenciometro + LEDs)
- [x] Leitura do DHT22 (temperatura e umidade)
- [x] Leitura de BPM simulada (60-115 BPM, faixa normal de repouso)
- [x] Loop de leitura a cada 5 segundos
- [x] Fila offline circular com ate 20 registros (Edge Computing)
- [x] Sincronizacao automatica ao reconectar
- [x] Simulacao de WiFi alternando a cada 30 segundos
- [x] Sistema de alertas medicos (taquicardia, febre, umidade)
- [x] LEDs indicadores (WiFi e alertas)
- [x] Codigo C++ comentado em portugues
- [x] Testes unitarios da logica (test_sistema.py)

---

## Hardware

| Componente    | Pino   | Funcao                        |
| ------------- | ------ | ----------------------------- |
| DHT22         | GPIO4  | Temperatura (C) e umidade (%) |
| Potenciometro | GPIO32 | Referencia ADC para BPM       |
| LED verde     | GPIO2  | Status WiFi                   |
| LED vermelho  | GPIO15 | Alerta medico ativo           |

---

## Limites de alerta

| Parametro   | Condicao normal | Alerta         |
| ----------- | --------------- | -------------- |
| BPM         | 60 - 120        | > 120 BPM      |
| Temperatura | < 38 C          | > 38 C (febre) |
| Umidade     | 40% - 80%       | < 40% ou > 80% |

---

## Como usar no Wokwi

1. Acesse wokwi.com e crie um novo projeto ESP32
2. Copie o conteudo de `wokwi/sketch.ino`
3. Importe o circuito de `wokwi/diagram.json`
4. Inicie a simulacao
5. Abra o Monitor Serial (aba OUTPUT, baud 115200)
6. Clique no DHT22 durante a simulacao para alterar temperatura e umidade

---

## Saida esperada no Monitor Serial

```
========================================
  CardioIA - Monitoramento Cardiaco
  Fase 3: Edge Computing + IoT Medico
========================================
[TESTE] Verificando sensores...
  - BPM inicial: 87 BPM
  - Temperatura: 36.5 C
  - Umidade: 60.0%
[OK] Sistema pronto. WiFi inicia DESCONECTADO.
========================================

[WIFI] DESCONECTADO - Modo offline ativado

[10951ms] BPM: 73 | Temp: 36.5C | Umidade: 60.0% | Status: OFFLINE
    -> Fila local: [1/20]
[15959ms] BPM: 91 | Temp: 36.5C | Umidade: 60.0% | Status: OFFLINE
    -> Fila local: [2/20]

[WIFI] CONECTADO - Modo online ativado
   Iniciando sincronizacao automatica...
========================================
[SYNC] Sincronizando 5 registro(s) offline...
========================================
[10951ms] BPM: 73 | Temp: 36.5C | Umidade: 60.0% | Status: SYNC->CLOUD
...
========================================
[SYNC] Concluido. Fila limpa.
========================================

[36049ms] BPM: 88 | Temp: 36.5C | Umidade: 60.0% | Status: ONLINE
    -> Enviado para nuvem via MQTT
```

---

## Estrutura de arquivos

```
wokwi/
├── sketch.ino        # Codigo principal (usar no Wokwi)
├── src/main.cpp      # Mesmo codigo para PlatformIO
├── diagram.json      # Circuito virtual
├── platformio.ini    # Configuracao PlatformIO
├── libraries.txt     # Bibliotecas necessarias
├── wokwi.toml        # Config do simulador
└── test_sistema.py   # Testes unitarios da logica
```

---

### Integração com MQTT e Sincronização de Dados

Nesta etapa do projeto, foi implementada a comunicação entre o dispositivo ESP32 e a nuvem utilizando o protocolo MQTT, além de uma estratégia de funcionamento resiliente baseada em Edge Computing.

  - Comunicação com a Nuvem (MQTT)

A transmissão dos dados foi realizada através do protocolo MQTT, escolhido por sua leveza e eficiência em aplicações IoT.

O ESP32 atua como um publisher, enviando os dados coletados para o broker MQTT hospedado no HiveMQ Cloud.

Broker: HiveMQ Cloud
Protocolo: MQTT
Tópico utilizado: cardio/dados

Os dados são enviados no formato JSON, contendo informações como:

{
  "timestamp": 43054,
  "bpm": 93,
  "temperatura": 36.5,
  "umidade": 60.0,
  "alerta": false,
  "origem": "tempo_real"
}

Essa estrutura permite fácil interpretação, escalabilidade e integração com outros sistemas.

  - Estratégia de Edge Computing

Para garantir a continuidade do sistema mesmo em situações de falha de conectividade, foi implementada uma abordagem baseada em Edge Computing.

O funcionamento ocorre da seguinte forma:

Quando o dispositivo está offline:
Os dados coletados são armazenados em uma fila local
Nenhuma informação é perdida
Quando a conexão é restabelecida:
O sistema inicia automaticamente a sincronização
Os dados armazenados são enviados para a nuvem

Essa lógica garante:

Maior confiabilidade
Resiliência do sistema
Integridade dos dados

  - Fluxo de Funcionamento
Coleta dos dados (BPM, temperatura e umidade)
Verificação do status da conexão
Se online:
Envio imediato via MQTT
Se offline:
Armazenamento em fila local
Reconexão:
Sincronização automática com a nuvem

  - Gerenciamento da Conexão

O sistema realiza continuamente:

Verificação da conexão WiFi
Tentativa de reconexão ao broker MQTT
Manutenção da conexão ativa com mqttClient.loop()

Essa abordagem garante que o dispositivo permaneça conectado e apto a enviar dados sempre que possível.

  - Resultado

Como resultado, foi possível desenvolver um sistema capaz de:

Monitorar dados em tempo real
Operar de forma independente da conectividade
Sincronizar dados automaticamente com a nuvem
Utilizar comunicação eficiente via MQTT

## Integração MQTT com Node-RED (Backend)

Nesta etapa, foi implementada a integração entre o ESP32 (simulado no Wokwi) e o ambiente de backend utilizando o protocolo MQTT e o Node-RED para recepção e monitoramento dos dados em tempo real.

  - Objetivo

Receber os dados de sinais vitais enviados pelo ESP32 via MQTT e validar a comunicação com o backend.

  - Configuração do Broker MQTT (HiveMQ Cloud)

Foi utilizado o HiveMQ Cloud como broker MQTT.

Protocolo: MQTT v3.1.1
Porta: 8883
Autenticação: usuário e senha configurados
Tópico utilizado:
cardio/dados

O ESP32 publica continuamente os dados nesse tópico.

  - Fluxo no Node-RED

O fluxo implementado foi simplificado para garantir estabilidade na conexão:

MQTT IN → DEBUG
📥 MQTT IN
Inscrito no tópico cardio/dados
Responsável por receber os dados enviados pelo ESP32
Conectado ao broker HiveMQ
🐞 DEBUG
Exibe os dados recebidos no painel lateral do Node-RED
Utilizado para validação da comunicação
📊 Estrutura dos Dados Recebidos

Os dados são recebidos no formato JSON:

{
  "timestamp": 113551,
  "bpm": 68,
  "temperatura": 36.5,
  "umidade": 60,
  "alerta": false
}

Mesmo sem o nó de conversão JSON, o Node-RED já exibe corretamente os dados no painel de debug.

  - Resultados Obtidos
Conexão estabelecida com sucesso com o broker MQTT
Recebimento contínuo de dados em tempo real
Visualização dos dados diretamente no Node-RED
Validação da comunicação entre dispositivo (ESP32) e backend

  - Decisão Técnica

Durante os testes, o nó JSON foi removido para simplificar o fluxo e garantir o funcionamento correto da recepção de dados.

Essa abordagem mostrou-se suficiente para validação da comunicação MQTT nesta fase do projeto.

## O que NAO foi implementado (outras partes do projeto)

- Dashboard Node-RED (Membro 3 - Frontend)
- Grafana Cloud (Membro 3 - opcional)
- Relatorios escritos (Membro 4 - Documentacao)
- API REST e envio de e-mail (Ir Alem 1)
- Analise de IA com series temporais (Ir Alem 2)
