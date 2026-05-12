# CardioIA — Fase 3
## Documento de Entrega — FIAP · Curso de Inteligência Artificial

---

## 👨‍🎓 Grupo

| Nome | RM | Contribuição Principal |
|---|:---:|---|
| João | RM565999 | Hardware — ESP32 + sensores + simulação no Wokwi |
| Tayná Esteves | RM562491 | Backend — Broker MQTT (HiveMQ Cloud) + alertas + Edge Computing |
| Carlos Eduardo | RM566487 | Frontend — Dashboard Node-RED + Grafana Cloud |
| Endrew Alves | RM563646 | Documentação — Relatórios técnicos + Análise de IA |

---

## 🔗 Links do Projeto

| Recurso | URL |
|---|---|
| **Repositório GitHub** | https://github.com/JV-004/FIAP-CardioIA-Fase3 |
| **Simulação Wokwi (ESP32)** | https://wokwi.com/projects/463299672900584449 |
| **Vídeo Demonstrativo** | https://www.youtube.com/watch?v=OR99ABXvCMg |
| **Notebook IA (Google Colab)** | https://colab.research.google.com/github/JV-004/FIAP-CardioIA-Fase3/blob/main/notebooks/cardio_ia_series_temporais.ipynb |

---

## 📁 Estrutura de Entregáveis

### Parte 1 — Hardware e Edge Computing

| Artefato | Caminho no Repositório |
|---|---|
| Código C++ principal (Wokwi) | `wokwi/sketch.ino` |
| Código C++ (PlatformIO) | `wokwi/src/main.cpp` |
| Circuito virtual do ESP32 | `wokwi/diagram.json` |
| Testes unitários da lógica | `wokwi/test_sistema.py` |
| Relatório técnico (Parte 1) | `docs/relatorio_parte1.md` |

> O código C++ está **integralmente comentado em português**, descrevendo cada bloco de lógica:
> fila circular offline, sincronização automática, alertas médicos e comunicação MQTT.

### Parte 2 — Comunicação MQTT

| Artefato | Caminho no Repositório |
|---|---|
| Relatório técnico (Parte 2) | `docs/relatorio_parte2.md` |

> **Broker:** HiveMQ Cloud · **Protocolo:** MQTT v3.1.1 · **Porta:** 8883 (TLS)  
> **Tópico:** `cardio/dados` · **Payload:** JSON com BPM, temperatura, umidade, alerta e origem.

### Parte 3 — Dashboard Node-RED

| Artefato | Caminho no Repositório |
|---|---|
| Fluxo Node-RED (importável) | `node-red/flows.json` |
| Screenshot — estado normal | `docs/images/01_nodered_normal.png` |
| Screenshot — alerta ativo | `docs/images/02_nodered_alerta.png` |
| Screenshot — Grafana histórico | `docs/images/03_grafana_historico.png` |
| Diagrama de arquitetura | `docs/images/04_arquitetura_fluxo.png` |

> O arquivo `node-red/flows.json` pode ser importado diretamente no Node-RED local via
> **Menu → Import → Clipboard**. Requer o pacote `node-red-dashboard`.

### Parte 4 — Relatórios e IA (Ir Além 2)

| Artefato | Caminho no Repositório |
|---|---|
| Relatório Parte 1 — Edge Computing (≥ 1 página) | `docs/relatorio_parte1.md` |
| Relatório Parte 2 — MQTT + Dashboard (≥ 2 páginas) | `docs/relatorio_parte2.md` |
| Notebook IA — Regressão Logística vs. LIF | `notebooks/cardio_ia_series_temporais.ipynb` |
| Imagem — Comparação de métricas | `docs/images/ia_comparacao_modelos.png` |
| Imagem — Curvas ROC | `docs/images/ia_curvas_roc.png` |
| Imagem — Matrizes de confusão | `docs/images/ia_matriz_confusao.png` |

---

## 🧪 Como Executar o Projeto

### 1. Simulação do Hardware (Wokwi)
1. Acesse https://wokwi.com/projects/463299672900584449
2. Clique em **▶ Start Simulation**
3. Abra o **Monitor Serial** (baud 115200) para ver os logs

### 2. Dashboard Node-RED (local)
```bash
cd node-red
npm install
npm start
# Acessar: http://localhost:1880/ui
```
> Configure as credenciais do broker HiveMQ Cloud no nó `MQTT IN` antes de fazer o Deploy.

### 3. Notebook de IA
Acesse diretamente no Google Colab pelo link acima ou abra o arquivo `.ipynb`
no VS Code / Jupyter. Todas as dependências (`numpy`, `scikit-learn`, `matplotlib`, `seaborn`)
estão disponíveis no Colab sem instalação adicional.

---

## 📡 Fluxo Completo do Sistema

```
ESP32 (Wokwi)
    │ MQTT v3.1.1 / TLS / porta 8883
    ▼
HiveMQ Cloud (broker) — tópico: cardio/dados
    │
    ▼
Node-RED (subscriber + dashboard)
    ├── Tab 1: Sinais Vitais (gráfico BPM, gauges, cards)
    └── Tab 2: Alertas (status, mensagem, LED, histórico)
    │
    ▼
[Grafana Cloud] — painel histórico 24h
```

---

## ⚠️ Limites de Alerta Configurados

| Parâmetro | Condição Normal | Alerta |
|:---|:---:|:---|
| BPM | ≤ 120 bpm | > 120 → **Taquicardia** |
| Temperatura | ≤ 38 °C | > 38 °C → **Febre** |
| Umidade | 40% – 80% | < 40% → **Ambiente Seco** / > 80% → **Ambiente Úmido** |

---

*FIAP — Faculdade de Informática e Administração Paulista · Curso de Inteligência Artificial · Fase 3 · 2025/2026*
