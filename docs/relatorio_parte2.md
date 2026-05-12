# CardioIA — Parte 2: Transmissão via MQTT e Visualização em Node-RED/Grafana

**Disciplina:** Inteligência Artificial — FIAP  
**Fase:** 3 · Parte 2  
**Projeto:** CardioIA — Monitoramento Contínuo IoT na Saúde  

---

## 1. Introdução

A Parte 2 do projeto CardioIA concentrou-se na implementação da camada de **transmissão de dados** e **visualização em tempo real**, consolidando a integração entre o dispositivo embarcado ESP32 e a infraestrutura de nuvem. Esta etapa representou a evolução natural da arquitetura de Edge Computing estabelecida na Parte 1: os dados coletados e processados localmente — incluindo aqueles preservados na fila circular durante períodos de desconexão — passaram a ser transmitidos de forma segura e eficiente para um broker MQTT na nuvem, de onde foram consumidos pela plataforma Node-RED e apresentados em painéis interativos de monitoramento.

O fluxo completo de dados, do sensor à tela, foi estruturado em três camadas complementares:

1. **Produção (ESP32/Wokwi):** Coleta de sinais vitais, triagem local e publicação MQTT;
2. **Transporte (HiveMQ Cloud):** Intermediação segura via protocolo MQTT com criptografia TLS;
3. **Consumo e Visualização (Node-RED + Grafana):** Ingestão, processamento de fluxo e apresentação em dashboards interativos.

Esta arquitetura em camadas promoveu separação de responsabilidades, escalabilidade horizontal e resiliência a falhas em qualquer nível da pilha, atendendo aos requisitos de sistemas críticos de saúde.

---

## 2. Protocolo MQTT

### Justificativa Técnica da Escolha

A seleção do protocolo **MQTT** (*Message Queuing Telemetry Transport*) como mecanismo de comunicação entre o ESP32 e a infraestrutura de nuvem foi fundamentada em critérios técnicos objetivos, especialmente relevantes para o contexto de IoT médico.

O MQTT opera sobre o paradigma **Publish/Subscribe**, no qual os produtores de dados (publishers) e os consumidores (subscribers) são completamente desacoplados — temporalmente, espacialmente e sincronicamente. Este desacoplamento é crítico em cenários de monitoramento médico, onde o dispositivo de campo (ESP32) deve continuar operando independentemente do estado dos sistemas de consumo (dashboards, bases de dados, sistemas de alerta).

### Comparação com HTTP/REST

| Critério | MQTT | HTTP/REST |
|:---|:---:|:---:|
| **Overhead de protocolo** | Muito baixo (cabeçalho mínimo de 2 bytes) | Alto (headers HTTP completos) |
| **Modelo de comunicação** | Pub/Sub assíncrono | Request/Response síncrono |
| **Adequação para IoT** | Nativa — projetado para dispositivos embarcados | Limitada — projetado para aplicações web |
| **Consumo de energia** | Mínimo | Elevado por sessão |
| **Latência** | Baixa | Moderada a alta |
| **Suporte a QoS** | Nativo (0, 1, 2) | Não disponível nativamente |
| **Reconexão automática** | Suportada | Deve ser implementada manualmente |

O MQTT demonstrou superioridade técnica em três dimensões fundamentais para o CardioIA: (i) **leveza do protocolo**, essencial dado o poder de processamento limitado do ESP32; (ii) **assincronicidade**, que permitiu ao dispositivo publicar dados sem aguardar resposta dos consumidores; e (iii) **suporte nativo a QoS** (*Quality of Service*), garantindo entrega confiável mesmo em redes instáveis.

---

## 3. Configuração do Broker HiveMQ Cloud

A infraestrutura de broker selecionada foi o **HiveMQ Cloud**, plataforma gerenciada de MQTT reconhecida por sua alta disponibilidade, conformidade com padrões de segurança e suporte nativo ao protocolo MQTT v3.1.1. A tabela abaixo consolida os parâmetros de configuração adotados:

| Parâmetro | Valor Configurado |
|:---|:---|
| **Provedor do Broker** | HiveMQ Cloud (managed) |
| **Versão do Protocolo** | MQTT v3.1.1 |
| **Porta de Conexão** | 8883 (TLS/SSL obrigatório) |
| **Mecanismo de Autenticação** | Credenciais (usuário e senha) |
| **Tópico de Publicação** | `cardio/dados` |
| **Papel do ESP32** | Publisher exclusivo |
| **Papel do Node-RED** | Subscriber exclusivo |
| **Keep-alive** | 60 segundos |
| **Clean Session** | Habilitado |

### Segurança TLS na Porta 8883

Toda a comunicação entre o ESP32 e o broker HiveMQ Cloud foi criptografada por meio do protocolo **TLS** (*Transport Layer Security*) na porta 8883. Esta configuração é mandatória para dados de saúde, garantindo que os sinais vitais transmitidos não possam ser interceptados em texto plano por atores maliciosos na rede pública.

A autenticação por credenciais (usuário e senha) adicionou uma segunda camada de controle de acesso, assegurando que apenas dispositivos autorizados puderam publicar no tópico `cardio/dados`.

---

## 4. Estrutura dos Dados Transmitidos

Os dados foram serializados no formato **JSON** (*JavaScript Object Notation*) antes da publicação no broker, garantindo interoperabilidade entre o firmware C++ do ESP32 e o ambiente JavaScript do Node-RED. O payload foi projetado para ser compacto, auto-descritivo e rastreável.

### Exemplo de Payload JSON

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

### Dicionário de Campos do Payload

| Campo | Tipo | Exemplo | Descrição |
|:---|:---:|:---:|:---|
| `timestamp` | `integer` | `43054` | Tempo em milissegundos desde o boot do ESP32 |
| `bpm` | `integer` | `93` | Frequência cardíaca atual (batimentos por minuto) |
| `temperatura` | `float` | `36.5` | Temperatura corporal em graus Celsius (°C), sensor DHT22 |
| `umidade` | `float` | `60.0` | Umidade relativa do ar em percentual (%), sensor DHT22 |
| `alerta` | `boolean` | `false` | `true` se qualquer parâmetro excedeu o limiar de normalidade |
| `origem` | `string` | `"tempo_real"` | Rastreabilidade: `"tempo_real"` (online) ou `"SYNC→CLOUD"` (sincronizado da fila offline) |

### Importância do Campo `origem` para Rastreabilidade

O campo `origem` desempenha papel fundamental na rastreabilidade dos dados ao longo da cadeia de processamento. Ao distinguir registros transmitidos em tempo real (`"tempo_real"`) de registros sincronizados retroativamente da fila circular (`"SYNC→CLOUD"`), o sistema permitiu que os dashboards e as análises posteriores identificassem a natureza temporal de cada medição — informação crítica para a interpretação clínica correta de séries históricas de sinais vitais.

---

## 5. Integração Node-RED

### Fluxo de Nós Implementado

O Node-RED atuou como a camada de **orquestração e backend** do sistema, conectando o broker MQTT aos componentes visuais do dashboard. O fluxo implementado seguiu a seguinte arquitetura:

```
[MQTT IN — cardio/dados]
        │
        ▼
[Validar e Enriquecer Payload]  ← Garante campos mínimos, calcula alertas individuais
        │
        ▼
[Distribuidor de Mensagens]  ← Cria 5 saídas independentes
        │
        ├──▶ [Gráfico BPM — Série Temporal]
        ├──▶ [Gauge — Temperatura Corporal]
        ├──▶ [Gauge — Umidade Relativa]
        ├──▶ [Formatar Cards] ──▶ Cards de BPM, Temp, Umidade, Origem
        └──▶ [Lógica de Alerta] ──▶ Status, Mensagem, LED, Histórico
```

### Decisão Técnica: Remoção do Nó de Conversão JSON

Durante a implementação, identificou-se que a inserção de um nó dedicado de conversão JSON entre o `MQTT IN` e os nós de dashboard introduzia redundância e instabilidade no fluxo. O nó `MQTT IN` do Node-RED já oferece suporte nativo à deserialização automática de payloads JSON quando o tipo de dado é configurado como `auto`. A **remoção do nó de conversão explícito** eliminou a dupla serialização/deserialização, reduzindo a complexidade do fluxo e aumentando sua estabilidade operacional.

### Validação da Comunicação

A integração entre o ESP32, o broker HiveMQ Cloud e o Node-RED foi validada com sucesso: a conexão com o broker foi confirmada via painel de administração do HiveMQ Cloud, e o recebimento contínuo de dados pelo Node-RED foi verificado através do painel de debug lateral, exibindo os payloads desserializados corretamente em cada ciclo de 5 segundos.

O estado de operação normal do dashboard pode ser observado na figura abaixo:

![Node-RED — Estado Normal de Operação](docs/images/01_nodered_normal.png)

---

## 6. Dashboard e Alertas Automáticos

### Componentes Visuais Implementados

O dashboard do CardioIA foi estruturado em duas abas temáticas no Node-RED Dashboard (`node-red-dashboard`):

**Aba 1 — Sinais Vitais:**

- **Gráfico de Série Temporal (BPM):** Exibiu os últimos 20 pontos de leitura de BPM em um gráfico de linha contínuo, com linha de referência implícita no limite clínico de 120 BPM;
- **Gauge de Temperatura Corporal:** Instrumento circular calibrado entre 35°C e 42°C, com faixas de cor progressivas — verde para eutermia (≤37°C), amarelo para hipertermia leve (37–38°C) e vermelho para febre (>38°C);
- **Gauge de Umidade Relativa:** Instrumento calibrado em escala de 0% a 100%, com zona de normalidade destacada entre 40% e 80%;
- **Cards de Valores Atuais:** Quatro cards com ícones dinâmicos exibindo BPM, temperatura, umidade e a origem do dado (TEMPO REAL ou SINCRONIZADO).

**Aba 2 — Alertas:**

- **Indicador de Status:** Exibiu "✅ SISTEMA NORMAL" ou "🚨 ALERTA ATIVO" com fonte ampliada para máxima legibilidade;
- **Mensagem Descritiva:** Detalhou o parâmetro e o valor responsável pelo alerta (ex.: `TAQUICARDIA — BPM: 127`);
- **LED Indicador:** Verde em situação normal, vermelho em alerta;
- **Histórico de Eventos:** Log cronológico dos eventos de alerta com hora local.

O comportamento em situação de alerta — com ativação de taquicardia detectada — está documentado na figura abaixo:

![Node-RED — Alerta Ativo de Taquicardia](docs/images/02_nodered_alerta.png)

### Painel Grafana — Histórico 24h

O Grafana Cloud foi integrado como repositório complementar de séries temporais, oferecendo visualização histórica das últimas 24 horas de dados, anotações de eventos de alerta, configuração de thresholds visuais e tabela de ocorrências com timestamps. Este painel viabilizou análises de tendência e padrões de comportamento impossíveis de observar apenas no dashboard em tempo real do Node-RED.

![Grafana — Painel Histórico 24h](docs/images/03_grafana_historico.png)

---

## 7. Fluxo Completo de Dados (End-to-End)

O diagrama textual abaixo representa a jornada completa de um dado desde o sensor físico até a representação visual final, passando por todas as camadas da arquitetura CardioIA:

![Arquitetura do Sistema CardioIA](docs/images/04_arquitetura_fluxo.png)

| Etapa | Tecnologia | Protocolo | Direção | Resultado |
|:---:|:---|:---:|:---:|:---|
| **1** | ESP32 + DHT22 + Potenciômetro (Wokwi) | GPIO/ADC | Local | Coleta e processamento de sinais vitais |
| **2** | Firmware C++ — Lógica de Edge Computing | Interno | Local → Fila | Triagem de alertas e armazenamento offline |
| **3** | HiveMQ Cloud (broker MQTT) | MQTT v3.1.1 / TLS 8883 | ESP32 → Cloud | Transporte seguro e intermediação de mensagens |
| **4** | Node-RED (subscriber + orquestrador) | MQTT / WebSocket | Cloud → Backend | Ingestão, enriquecimento e roteamento de dados |
| **5** | Node-RED Dashboard (widgets UI) | HTTP / WebSocket | Backend → Browser | Visualização em tempo real — BPM, gauges, alertas |
| **6** | [Grafana Cloud] | InfluxDB / HTTP | Backend → Cloud | Persistência histórica e análise de tendências |

---

## 8. Conclusão

A implementação da Parte 2 do CardioIA consolidou uma arquitetura de IoT médico robusta, segura e escalável. A escolha do protocolo MQTT, aliada à criptografia TLS e à autenticação por credenciais no HiveMQ Cloud, estabeleceu um canal de comunicação confiável e adequado à sensibilidade dos dados biométricos transmitidos.

A integração com o Node-RED demonstrou como plataformas de fluxo visual podem orquestrar pipelines de dados complexos com eficiência e legibilidade, facilitando a manutenção e a evolução do sistema. O campo `origem` no payload revelou-se uma decisão de design particularmente acertada, permitindo rastreabilidade completa dos dados desde a coleta até a visualização.

A arquitetura resultante demonstrou aderência às boas práticas de IoT médico: separação clara de responsabilidades entre camadas, resiliência a falhas de conectividade (herdada do Edge Computing da Parte 1), transmissão segura e dashboards com alertas automáticos de alto contraste e imediata legibilidade clínica. O sistema demonstrou potencial real de aplicação em UTIs de pequeno porte, postos de saúde remotos e monitoramento domiciliar de pacientes cardíacos, contextos nos quais a confiabilidade de dados e a velocidade de alerta podem impactar diretamente desfechos clínicos.

---

## Referências

- HiveMQ Cloud Documentation — https://www.hivemq.com/docs/hivemq-cloud/
- Node-RED Documentation — https://nodered.org/docs/
- Grafana Cloud Documentation — https://grafana.com/docs/grafana-cloud/
- MQTT Protocol Specification v3.1.1 — OASIS Standard
- Banks, A.; Gupta, R. *MQTT Version 3.1.1*. OASIS Standard, 2014.
- Documentação ESP32 — Espressif Systems: https://docs.espressif.com/
