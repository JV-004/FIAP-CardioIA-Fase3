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

# Integração com MQTT e Sincronização de Dados

## Visão Geral

Nesta etapa do projeto, foi implementada a comunicação entre o dispositivo ESP32 e a nuvem utilizando o protocolo MQTT, juntamente com uma estratégia resiliente baseada em Edge Computing.

O objetivo principal foi garantir o envio contínuo dos dados de sinais vitais, mesmo em cenários de instabilidade de conexão.

---

## Comunicação com a Nuvem (MQTT)

O protocolo MQTT foi escolhido devido à sua leveza e eficiência em aplicações IoT, permitindo comunicação rápida e confiável entre dispositivos e servidores.

O ESP32 atua como **publisher**, enviando dados continuamente para um broker MQTT na nuvem.

### Configuração do Broker

| Parâmetro     | Valor            |
|--------------|------------------|
| Broker       | HiveMQ Cloud     |
| Protocolo    | MQTT v3.1.1      |
| Porta        | 8883             |
| Autenticação | Usuário e senha  |
| Tópico       | cardio/dados     |

---

## Estrutura dos Dados

Os dados são enviados no formato JSON:

```json
{
  "timestamp": 43054,
  "bpm": 93,
  "temperatura": 36.5,
  "umidade": 60.0,
  "alerta": false,
  "origem": "tempo_real"
}
```

Essa estrutura facilita a interpretação, integração com outros sistemas e escalabilidade da solução.

---

## Estratégia de Edge Computing

Para garantir o funcionamento contínuo do sistema, foi implementada uma abordagem baseada em Edge Computing.

### Comportamento do Sistema

| Situação   | Ação do Sistema                          |
|------------|------------------------------------------|
| Online     | Envio imediato via MQTT                  |
| Offline    | Armazenamento em fila local              |
| Reconexão  | Sincronização automática com a nuvem     |

---

## Fluxo de Funcionamento

O funcionamento do sistema segue as seguintes etapas:

1. Coleta dos dados (BPM, temperatura e umidade)  
2. Verificação do status da conexão  
3. Caso online: envio imediato via MQTT  
4. Caso offline: armazenamento em fila local  
5. Reconexão: sincronização automática dos dados  

---

## Gerenciamento de Conectividade

O sistema realiza continuamente:

- Verificação do status da conexão WiFi  
- Tentativas de reconexão ao broker MQTT  
- Manutenção da conexão ativa com `mqttClient.loop()`  

---

## Resultados da Implementação

Como resultado, foi possível desenvolver um sistema capaz de:

- Monitorar dados em tempo real  
- Operar mesmo sem conexão com a internet  
- Garantir integridade dos dados  
- Sincronizar automaticamente com a nuvem  

---

# Integração MQTT com Node-RED (Backend)

## Objetivo

Validar a comunicação entre o ESP32 e o backend, garantindo o recebimento e visualização dos dados em tempo real.

---

## Fluxo no Node-RED

```
MQTT IN → DEBUG
```

---

## Configuração dos Nós

| Componente | Função                                   |
|-----------|------------------------------------------|
| MQTT IN   | Recebe dados do tópico cardio/dados      |
| DEBUG     | Exibe os dados no painel do Node-RED     |

---

## Estrutura dos Dados Recebidos

```json
{
  "timestamp": 113551,
  "bpm": 68,
  "temperatura": 36.5,
  "umidade": 60,
  "alerta": false
}
```

---

## Decisão Técnica

Durante os testes, optou-se por remover o nó de conversão JSON para simplificar o fluxo e garantir maior estabilidade na recepção dos dados.

---

## Resultados Obtidos

A integração permitiu validar com sucesso:

- Conexão com o broker MQTT  
- Recebimento contínuo de dados em tempo real  
- Visualização dos dados diretamente no Node-RED  
- Comunicação funcional entre ESP32 e backend  

---

## Conclusão

A integração entre ESP32, MQTT e Node-RED foi realizada com sucesso, estabelecendo uma base sólida para futuras etapas do projeto.

## O que NAO foi implementado (outras partes do projeto)

- Dashboard Node-RED (Membro 3 - Frontend)
- Grafana Cloud (Membro 3 - opcional)
- Relatorios escritos (Membro 4 - Documentacao)
- API REST e envio de e-mail (Ir Alem 1)
- Analise de IA com series temporais (Ir Alem 2)
