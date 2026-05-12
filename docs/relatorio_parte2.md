# Relatório Técnico: CardioIA — Transmissão via MQTT e Visualização em Node-RED

**Disciplina:** Inteligência Artificial (IA) — FIAP  
**Etapa:** Fase 3, Parte 2  

---

## 1. Introdução
O presente relatório documenta a evolução da arquitetura do projeto CardioIA, com foco na Parte 2 da Fase 3 da disciplina. Enquanto a Parte 1 estabeleceu as bases de *Edge Computing* para processamento local e contingência (*offline*) através de fila circular no dispositivo embarcado (ESP32), a Parte 2 concentra-se no escoamento e na representação visual desses dados. A continuidade lógica do projeto exige que a transição do ambiente físico — ou neste caso, emulado via Wokwi — para a infraestrutura de nuvem seja realizada de forma segura, assíncrona e resiliente. Dessa forma, o escopo desta etapa engloba a adoção do protocolo de comunicação MQTT, o estabelecimento da infraestrutura de *broker* via HiveMQ Cloud e a implementação de uma interface interativa de monitoramento médico operada por meio do Node-RED. O objetivo primordial é garantir que tanto as leituras em tempo real quanto os registros sincronizados retroativamente após quedas de conectividade sejam transmitidos, consolidados e visualizados sem perda de integridade.

## 2. Protocolo MQTT e Justificativa Técnica
A escolha do protocolo de comunicação é uma decisão crítica no desenho de arquiteturas de Internet das Coisas (IoT), especialmente em contextos de telemedicina. Para o CardioIA, optou-se pela adoção do protocolo MQTT (*Message Queuing Telemetry Transport*), suplantando a alternativa clássica baseada em requisições HTTP/REST.

O protocolo MQTT opera fundamentado no paradigma *Publish/Subscribe*, o que desatrela temporal e espacialmente os produtores de dados (o embarcado ESP32) dos consumidores (Node-RED). Em contraste com o HTTP, que exige o estabelecimento contínuo de sessões TCP custosas e o tráfego de cabeçalhos (*headers*) longos, o MQTT possui um *overhead* mínimo. Esta característica torna o protocolo excepcionalmente leve e otimizado para dispositivos operados por microcontroladores com recursos escassos de processamento e energia. Além disso, a natureza assíncrona do MQTT corrobora com a estratégia de *Edge Computing* abordada na fase anterior: os pacotes oriundos do processo de *sincronização automática* podem ser publicados em rajada sem sobrecarregar o microcontrolador, mantendo o tráfego veloz e o enfileiramento administrado de forma eficaz pelo *broker*.

## 3. Configuração do Broker HiveMQ Cloud
Como intermediador central (*broker*) da troca de mensagens, a infraestrutura escolhida foi o HiveMQ Cloud, reconhecido por sua alta disponibilidade e conformidade com padrões empresariais. Considerando o caráter médico dos dados transacionados, a segurança tornou-se o pilar fundamental desta configuração. Toda a comunicação foi criptografada através da camada TLS (*Transport Layer Security*) na porta 8883, impedindo a interceptação em texto plano dos pacotes biométricos e ambientais na rede pública. O ESP32 foi configurado estritamente como *publisher* (publicador) neste contexto. 

A tabela a seguir consolida os parâmetros arquiteturais do *broker*:

| Parâmetro de Conexão | Especificação Adotada |
| :--- | :--- |
| **Broker** | HiveMQ Cloud |
| **Protocolo** | MQTT v3.1.1 |
| **Segurança / Porta** | TLS / 8883 |
| **Autenticação** | Credenciais restritas (Usuário e Senha) |
| **Tópico de Publicação** | `cardio/dados` |
| **Papel do Dispositivo** | *Publisher* (ESP32) |

## 4. Estrutura dos Dados e Rastreabilidade
A padronização das mensagens transmitidas pelo ESP32 foi feita através do formato JSON (*JavaScript Object Notation*), garantindo interoperabilidade entre o hardware e o ambiente analítico do Node-RED. A carga útil (*payload*) carrega os parâmetros clínicos em tempo real ou provenientes do banco de *Edge Computing*.

O JSON foi modelado com máxima eficiência, contendo apenas as chaves estritamente essenciais. Um diferencial técnico na modelagem do *payload* foi a inclusão do campo temporal (`timestamp`) e do campo `origem`. O campo `origem` assume papel fundamental na rastreabilidade dos dados: permite que o sistema em nuvem identifique se o pacote corresponde a um dado gerado em tempo real ou a um pacote represado durante o estado *offline* que foi recém-sincronizado.

### Exemplo de Payload JSON Publicado:
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

### Dicionário de Dados do Payload:
| Campo JSON | Tipo de Dado | Descrição Técnica |
| :--- | :--- | :--- |
| `timestamp` | Inteiro (Long) | Marcação temporal de geração do dado. |
| `bpm` | Inteiro | Frequência cardíaca simulada (Batimentos por Minuto). |
| `temperatura` | Flutuante (Float)| Leitura do sensor DHT22 em Graus Celsius (°C). |
| `umidade` | Flutuante (Float)| Leitura do sensor DHT22 em percentual (%). |
| `alerta` | Booleano | *Flag* de triagem de borda (`true` se houve anomalia local). |
| `origem` | *String* | Indica rastreabilidade (`tempo_real` ou `sincronizado`). |

## 5. Integração Node-RED
A plataforma Node-RED atua como o principal orquestrador (*backend*) e mecanismo de interface humano-computador (IHC) do sistema CardioIA. A lógica de ingestão foi estruturada a partir de um fluxo linear e otimizado: `Nó MQTT IN → Nó de Lógica e Conversão → Nós de Dashboard`.

Uma importante decisão técnica tomada durante a integração foi a reestruturação e **remoção do nó específico de conversão JSON** que atuava de forma redundante no fluxo inicial. Esta adequação teve como objetivo reduzir a complexidade e aumentar a estabilidade do fluxo de dados; observou-se que centralizar a decodificação da carga e extração dos atributos em uma lógica limpa previne exceções decorrentes de dupla serialização/deserialização de objetos. Em decorrência dessa ação de refatoração, a comunicação entre o ESP32 e o *backend* validou-se como completamente funcional, sem interrupções por erro de formatação, assegurando o recebimento contínuo do fluxo de informações gerado pelo ambiente simulado do Wokwi.

## 6. Dashboard e Alertas Automáticos
O *dashboard* operacional foi concebido (conforme imagens geradas na Parte 3: `docs/images/01_nodered_normal.png` e `docs/images/02_nodered_alerta.png`) para proporcionar máxima clareza e rápida tomada de decisão pelos profissionais de saúde. A interface visual conta com os seguintes componentes e comportamentos automáticos, cujas lógicas de fronteira são respeitadas:

- **Gráfico de Série Temporal (BPM):** Plota o histórico recente da frequência cardíaca através dos últimos 20 pontos de captura. Possui uma linha referencial subentendida no limite clínico superior, evidenciando taquicardia quando há ultrapassagem de 120 BPM.
- **Gauge de Temperatura Corporal:** Instrumento visual calibrado na escala de 35 °C a 42 °C. O espectro visual da barra alterna progressivamente, definindo uma marca verde para estado eutérmico (normal) e cor vermelha ao ultrapassar 38 °C (indicando condição febril).
- **Gauge de Umidade:** Calibrado em escala de 0% a 100%. A lógica demarca a zona de conforto humano como área verde de normalidade no intervalo de 40% a 80%. Valores aquém ou além deste limiar são sinalizados na cor vermelha.
- **Indicador Visual de Alerta (Status):** Através da observância dos dados JSON, o painel assume um modo explícito, exibindo texto estático em destaque: em circunstâncias controladas, apresenta a *badge* "SISTEMA NORMAL"; quando detecta anomalias, substitui-a imediatamente por avisos dinâmicos como "ALERTA: TAQUICARDIA".
- **Badge de Origem do Dado:** Uma *tag* visual acoplada ao painel informa de imediato se os dados plotados em tela pertencem à varredura atual (TEMPO REAL) ou se provêm de uma carga atrasada da fila circular do ESP32 (SINCRONIZADO), oferecendo contexto vital à equipe de enfermagem.

## 7. Fluxo Completo de Dados
A visão ponta-a-ponta da esteira de processamento do CardioIA descreve uma jornada coerente e coesa entre o dado cru (físico/simulado) e sua representação visual de negócio final (sendo referenciada a integração adjacente com o histórico, visualizada na imagem `docs/images/03_grafana_historico.png`). A tabela seguinte ilustra cada elo desta corrente de valor:

| Etapa | Componente Principal | Protocolo / Meio | Função no Sistema |
| :---: | :--- | :--- | :--- |
| **1** | ESP32 (Wokwi Simulador) | Físico/GPIO | Captação via sensores e computação de borda (*Edge*). |
| **2** | HiveMQ Cloud (Broker) | MQTT (TLS/8883) | Intermediação em nuvem, recebimento e direcionamento seguro. |
| **3** | Plataforma Node-RED | TCP/WebSockets | *Backend* de ingestão e lógica de apresentação em tempo real. |
| **4** | Dashboard Interativo | HTTP / Frontend | Representação em interface gráfica de usuário (IHC). |
| **5** | [Grafana Cloud] | API / Database | Repositório complementar (*time-series*) para histórico denso. |

## 8. Conclusão
A consolidação da Fase 3, Parte 2 do projeto CardioIA comprova a exequibilidade de um sistema de monitoramento biométrico e ambiental fundamentado nas mais modernas práticas de Internet das Coisas. O emprego do MQTT aliado à autenticação em TLS forneceu uma blindagem à aplicação no nível de tráfego, garantindo preceitos legais associados aos dados de saúde, mantendo o consumo de rede no patamar mínimo desejável. 

A resiliência operacional alcançada — proveniente da sincronia lógica entre a computação de borda e o sistema de mensagens enfileiradas — confere ao sistema CardioIA alta escalabilidade técnica. A integração com o Node-RED demonstra que, a partir da simples padronização JSON, é possível acoplar fluxos modulares robustos, capazes de disparar ações reativas instantâneas nos *dashboards*. Em síntese, a solução projetada mitiga efetivamente os atritos de conectividade instável e eleva a confiabilidade sistêmica a padrões propícios a implementações em ecossistemas reais e dinâmicos de telemedicina.
