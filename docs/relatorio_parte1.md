# Relatório Técnico: CardioIA — Armazenamento e Processamento Local com Edge Computing

**Disciplina:** Inteligência Artificial (IA) — FIAP  
**Etapa:** Fase 3, Parte 1  

---

## 1. Introdução
O projeto CardioIA foi desenvolvido com o objetivo de prover um sistema robusto de monitoramento cardíaco e ambiental, endereçando a necessidade de alta disponibilidade e resiliência em aplicações de Internet das Coisas (IoT) voltadas para a área da saúde. A Parte 1 da Fase 3 da disciplina tem como foco a implementação de armazenamento e processamento local por meio da tecnologia de *Edge Computing*. Em cenários de telemedicina, a intermitência ou a perda temporária de conectividade não pode resultar em perda de dados vitais do paciente. Dessa forma, o presente projeto implementou uma arquitetura capaz de registrar informações no estado *offline* e garantir a sincronização automatizada com a nuvem mediante o restabelecimento da conexão, assegurando integridade e continuidade na observação médica.

## 2. Arquitetura de Hardware
O sistema físico foi abstraído e implementado utilizando o simulador online Wokwi ([acesso ao projeto](https://wokwi.com/projects/463299672900584449)), tendo como unidade central de processamento o microcontrolador ESP32. A coleta de dados ambientais foi realizada pelo sensor DHT22, enquanto a atividade cardíaca foi emulada por um potenciômetro, permitindo a variação manual dos Batimentos por Minuto (BPM) dentro de uma faixa de repouso típica de 60 a 115 BPM. O laço de repetição (*loop*) foi estruturado para efetuar coletas e ciclos de processamento a cada 5 segundos ininterruptamente. O sistema incorpora também LEDs de sinalização, conforme detalhado na tabela abaixo:

| Componente | Pino / GPIO | Função no Sistema |
| :--- | :--- | :--- |
| ESP32 | - | Microcontrolador central e processamento na borda (*Edge*) |
| DHT22 | GPIO4 | Leitura de temperatura (°C) e umidade relativa (%) |
| Potenciômetro | GPIO32 | Simulação da frequência cardíaca (BPM) |
| LED Verde | GPIO2 | Indicação de status de conectividade WiFi |
| LED Vermelho | GPIO15 | Indicação visual de alerta médico ativo |

## 3. Lógica de Edge Computing
Para mitigar os impactos da oscilação de rede, foi desenvolvida uma lógica de *Edge Computing* que retém e gerencia as leituras localmente no ESP32. Uma fila circular em memória local foi estruturada para armazenar os dados capturados sempre que o dispositivo detecta a ausência de conectividade (estado *offline*). Visando simular um cenário prático de instabilidade, uma variável booleana alterna o estado do WiFi artificialmente a cada 30 segundos.

Como decisão de negócio vinculada às restrições inerentes do *hardware*, a capacidade da fila circular foi delimitada a um máximo de 20 registros. Considerando a cadência de 5 segundos por leitura, o sistema é capaz de reter o correspondente a 100 segundos de dados ininterruptos. Ao atingir o esgotamento do *buffer*, o algoritmo adota a política circular de substituição, sobrescrevendo os registros cronologicamente mais antigos, priorizando assim o diagnóstico do estado mais recente do paciente. Uma vez que o dispositivo retoma a conexão, o processador inicia uma rotina de sincronização automática, na qual todos os registros pendentes são despachados e a fila é devidamente limpa.

## 4. Sistema de Alertas Médicos
A inteligência descentralizada aplicada na borda da rede permite que as amostras passem por validação heurística instantânea localmente, mitigando a dependência do tempo de resposta do servidor em nuvem. Os limites fisiológicos e ambientais foram estabelecidos conforme a tabela a seguir:

| Parâmetro | Limite de Normalidade | Condição de Alerta |
| :--- | :--- | :--- |
| BPM | ≤ 120 | Taquicardia (> 120 BPM) |
| Temperatura | ≤ 38 °C | Febre (> 38 °C) |
| Umidade | 40% a 80% | Ambiente impróprio (< 40% ou > 80%) |

Na ocorrência da violação de qualquer um dos limiares listados, a placa ativa de imediato o LED vermelho no GPIO15, provendo alerta situacional no local em que o paciente e os cuidadores se encontram.

## 5. Resultados Observados
Os testes empíricos efetuados no simulador validaram os requisitos de consistência e recuperação de dados. A transcrição do log de saída serial exposta abaixo comprova o exato comportamento da solução durante uma transição de estado de rede:

```text
[WIFI] DESCONECTADO - Modo offline ativado
[10951ms] BPM: 73 | Temp: 36.5C | Umidade: 60.0% | Status: OFFLINE → Fila: [1/20]
[WIFI] CONECTADO - Iniciando sincronização automática...
[SYNC] Sincronizando 5 registro(s) offline...
[SYNC] Concluído. Fila limpa.
[36049ms] BPM: 88 | Temp: 36.5C | Umidade: 60.0% | Status: ONLINE → MQTT
```

A análise do extrato evidencia que o ESP32 operou de maneira resiliente: diagnosticou o isolamento e enfileirou a amostra `[1/20]`, represou com sucesso `5` registros gerados durante o hiato e, ao retomar a conexão, executou o descarregamento (*SYNC*) do bloco de dados pendente, prosseguindo com a transmissão online em tempo real.

## 6. Conclusão
A concepção da arquitetura documentada neste relatório atendeu integralmente aos requisitos de confiabilidade para o monitoramento contínuo em telemedicina. A incorporação explícita de *Edge Computing* atenuou vulnerabilidades associadas à dependência de infraestrutura, repassando regras essenciais de negócio, como validação de risco e garantia de entrega, para o embarcado. Em aplicações IoT voltadas à área da saúde, este *design* é imprescindível, visto que reduz significativamente o descarte prematuro de sinais vitais e suprime pontos únicos de falha na emissão de alarmes clínicos de urgência.
