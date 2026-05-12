# CardioIA — Parte 1: Armazenamento e Processamento Local com Edge Computing

**Disciplina:** Inteligência Artificial — FIAP  
**Fase:** 3 · Parte 1  
**Projeto:** CardioIA — Monitoramento Contínuo IoT na Saúde  

---

## 1. Introdução

O projeto CardioIA foi concebido no âmbito da Fase 3 da disciplina de Inteligência Artificial do curso de IA da FIAP, com o propósito de demonstrar a aplicação prática de tecnologias emergentes no domínio da saúde digital. O sistema foi estruturado para monitorar sinais vitais de forma contínua, combinando hardware embarcado, protocolos de comunicação eficientes e análise inteligente de dados.

A Parte 1 do projeto compreendeu a implementação da camada de **Edge Computing**, responsável por garantir a integridade e a disponibilidade dos dados mesmo diante de instabilidades de conectividade — cenário frequente em ambientes hospitalares, domiciliares e de telemedicina. Em sistemas críticos de monitoramento médico, a perda de dados durante falhas de rede pode comprometer a continuidade do cuidado e dificultar diagnósticos retroativos. Diante dessa realidade, a arquitetura foi desenhada para operar de forma autônoma no dispositivo, armazenando localmente os registros durante o período de desconexão e sincronizando-os automaticamente com a nuvem após o restabelecimento da conectividade.

O Edge Computing representa um paradigma no qual a inteligência e o processamento de dados são deslocados para a borda da rede — neste caso, para o próprio microcontrolador ESP32 —, reduzindo a latência, o consumo de banda e a dependência de infraestrutura externa. Esta abordagem demonstrou-se especialmente adequada para o contexto do CardioIA, onde a confiabilidade é requisito não negociável.

---

## 2. Arquitetura de Hardware

O sistema físico foi implementado no ambiente de simulação **Wokwi** ([wokwi.com/projects/463299672900584449](https://wokwi.com/projects/463299672900584449)), utilizando o microcontrolador **ESP32** como unidade central de processamento e controle. O ESP32 foi selecionado por sua capacidade de processamento, suporte nativo a WiFi e Bluetooth, e pela disponibilidade de ADC (*Analog-to-Digital Converter*) necessário para a leitura do potenciômetro.

| Componente | Pino / GPIO | Função no Sistema |
|:---|:---:|:---|
| **DHT22** | GPIO 4 | Leitura de temperatura corporal (°C) e umidade relativa do ambiente (%) |
| **Potenciômetro** | GPIO 32 | Simulação analógica da frequência cardíaca — BPM (60 a 115 BPM) |
| **LED Verde** | GPIO 2 | Indicador visual do status de conectividade WiFi |
| **LED Vermelho** | GPIO 15 | Indicador visual de alerta médico ativo (taquicardia, febre ou umidade) |

O sensor **DHT22** foi responsável pela coleta das variáveis ambientais e de temperatura corporal, realizando leituras a cada ciclo de 5 segundos. O **potenciômetro**, conectado ao pino analógico GPIO32, emulou a variação da frequência cardíaca (BPM) de forma contínua e ajustável durante a simulação, permitindo a representação de estados normais e de alerta. Os **LEDs** compuseram o sistema de sinalização local, proporcionando feedback imediato sobre o estado do dispositivo sem necessidade de interface de usuário.

O laço principal (*loop*) foi estruturado para operar de forma ininterrupta, executando ciclos completos de coleta, processamento e transmissão a cada 5 segundos, assegurando uma granularidade temporal adequada ao monitoramento contínuo de sinais vitais.

---

## 3. Lógica de Edge Computing Implementada

A lógica de Edge Computing foi implementada diretamente no firmware do ESP32, em linguagem C++, e compreendeu três etapas fundamentais: **coleta de dados**, **verificação de conectividade** e **decisão de armazenamento ou transmissão**.

### Fluxo de Operação

| Situação de Rede | Ação do Sistema | Resultado |
|:---|:---|:---|
| **WiFi conectado** (modo online) | Serializa os dados em JSON e publica no broker MQTT via tópico `cardio/dados` | Dado transmitido em tempo real com `origem: "tempo_real"` |
| **WiFi desconectado** (modo offline) | Adiciona o registro na fila circular local com timestamp e todos os campos do payload | Dado preservado em memória com `origem: "SYNC→CLOUD"` |
| **Reconexão WiFi detectada** | Itera sobre todos os registros pendentes na fila e os publica sequencialmente no MQTT | Sincronização completa; fila esvaziada e limpa |

A **fila circular** foi implementada com capacidade máxima de **20 registros**, o que corresponde a aproximadamente **100 segundos de dados ininterruptos** (20 amostras × 5 segundos/amostra). Esta capacidade foi estabelecida como **decisão de negócio** fundamentada em dois critérios:

1. **Restrição de hardware:** O ESP32 dispõe de memória RAM limitada (aproximadamente 520 KB de SRAM), tornando inviável o armazenamento ilimitado de registros em memória volátil;
2. **Cobertura temporal suficiente:** Uma janela de 100 segundos cobre a maioria das interrupções transitórias de rede (quedas momentâneas, reconexões automáticas), sem comprometer a continuidade do monitoramento.

Ao atingir a capacidade máxima, o algoritmo de fila circular sobrescreve o registro mais antigo, priorizando a retenção dos dados mais recentes — decisão clinicamente relevante, pois o estado atual do paciente é sempre mais crítico para a triagem do que registros históricos distantes.

Adicionalmente, a alternância do estado WiFi foi simulada por uma **variável booleana** que alterna automaticamente a cada 30 segundos, permitindo a validação completa do ciclo offline → sincronização → online durante os testes no ambiente Wokwi.

---

## 4. Sistema de Alertas Médicos

A capacidade de processamento local do ESP32 foi aproveitada para implementar uma camada de **triagem heurística imediata**, sem dependência de resposta do servidor em nuvem. Os parâmetros fisiológicos e ambientais foram submetidos a avaliação a cada ciclo de leitura, de acordo com os limiares clínicos definidos pelo grupo:

| Parâmetro | Condição Normal | Limiar de Alerta | Indicador Local |
|:---|:---:|:---|:---:|
| **BPM** | ≤ 120 bpm | > 120 bpm → Taquicardia | LED Vermelho (GPIO15) |
| **Temperatura** | ≤ 38°C | > 38°C → Febre | LED Vermelho (GPIO15) |
| **Umidade** | 40% – 80% | < 40% → Ambiente Seco | LED Vermelho (GPIO15) |
| **Umidade** | 40% – 80% | > 80% → Ambiente Úmido | LED Vermelho (GPIO15) |

Quando qualquer um dos limiares foi ultrapassado, o campo `"alerta": true` foi adicionado ao payload JSON, e o **LED vermelho no GPIO15** foi acionado imediatamente, independentemente do estado de conectividade. Este comportamento garantiu alerta situacional instantâneo, mesmo em cenários de operação completamente offline.

O **LED verde no GPIO2**, por sua vez, sinalizou o estado de conectividade WiFi: aceso durante conexão ativa e apagado durante o modo offline.

---

## 5. Resultados Observados

A saída serial registrada durante a execução do sistema no simulador Wokwi confirmou o funcionamento pleno da lógica de Edge Computing implementada:

```
[WIFI] DESCONECTADO - Modo offline ativado
[10951ms] BPM: 73 | Temp: 36.5C | Umidade: 60.0% | OFFLINE → Fila: [1/20]
[15959ms] BPM: 91 | Temp: 36.5C | Umidade: 60.0% | OFFLINE → Fila: [2/20]
[WIFI] CONECTADO - Iniciando sincronização automática...
[SYNC] Sincronizando 5 registro(s) offline...
[SYNC] Concluído. Fila limpa.
[36049ms] BPM: 88 | Temp: 36.5C | Umidade: 60.0% | ONLINE → MQTT
```

A análise da saída serial evidenciou as seguintes ocorrências:

- **Detecção de desconexão:** O firmware identificou a perda de conectividade e ativou imediatamente o modo offline, registrando a primeira leitura na fila (`[1/20]`);
- **Acumulação de registros:** As leituras subsequentes foram armazenadas sequencialmente na fila circular, com incremento do contador confirme exibido no log (`[2/20]`);
- **Reconexão e sincronização:** Ao detectar o restabelecimento do WiFi, o sistema iniciou automaticamente a sincronização dos 5 registros acumulados, publicando-os no broker MQTT com o campo `origem: "SYNC→CLOUD"`;
- **Retomada do modo online:** Após o esvaziamento da fila, o sistema retornou à operação normal, transmitindo dados em tempo real via MQTT.

Os valores observados — BPM entre 73 e 91, temperatura de 36,5°C e umidade de 60% — encontravam-se integralmente dentro dos limites de normalidade, não acionando o sistema de alertas durante este trecho de operação.

---

## 6. Conclusão

A implementação da Parte 1 do CardioIA demonstrou que o paradigma de Edge Computing constitui um alicerce indispensável para sistemas de monitoramento médico baseados em IoT. A capacidade de processar, avaliar e armazenar dados diretamente no dispositivo embarcado eliminou o ponto único de falha representado pela dependência exclusiva de conectividade de rede.

A fila circular de 20 registros provou-se eficaz como estratégia de contingência para interrupções transitórias, garantindo zero perda de dados durante os testes realizados. A sincronização automática, por sua vez, assegurou a integridade histórica das séries temporais dos sinais vitais — essencial para análise clínica retroativa.

As lições aprendidas nesta etapa orientaram o design das partes subsequentes: a rastreabilidade do campo `origem` no payload JSON, a escolha do protocolo MQTT pela sua leveza e resiliência, e a estrutura de alertas baseada em limiares fisiológicos estabelecidos foram todos fundamentos construídos sobre a arquitetura de borda implementada nesta fase.

---

## Referências

- MQTT Protocol Specification v3.1.1 — OASIS Standard
- Documentação ESP32 — Espressif Systems: https://docs.espressif.com/
- Wokwi Simulator — https://wokwi.com
- DHT22 Sensor Datasheet — Aosong Electronics
- Figueiredo, B. et al. *IoT em Saúde: Desafios e Oportunidades*. FIAP Press, 2024.
