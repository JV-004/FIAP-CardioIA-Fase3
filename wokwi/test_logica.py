"""
Teste da lógica do sketch.ino — CardioIA Fase 3
Valida: mapeamento de BPM, fila offline, sincronização e formato de saída.
"""

import random
import time

# ─── Parâmetros (espelham o sketch.ino) ──────────────────────────────────────
MAX_REGISTROS = 20

# ─── Estrutura de leitura ─────────────────────────────────────────────────────


class Leitura:
    def __init__(self, timestamp, bpm, temperatura, umidade):
        self.timestamp = timestamp
        self.bpm = bpm
        self.temperatura = temperatura
        self.umidade = umidade

# ─── Funções espelhadas do C++ ────────────────────────────────────────────────


def mapear_bpm(valor_adc):
    """Mapeia ADC 0–4095 para BPM 40–180 (equivalente ao map() do Arduino)."""
    return int(40 + (valor_adc / 4095) * (180 - 40))


def ler_sensores(ts):
    """Simula leitura dos sensores com valores aleatórios realistas."""
    adc = random.randint(0, 4095)
    bpm = mapear_bpm(adc)
    temp = round(random.uniform(35.0, 39.5), 1)
    umid = round(random.uniform(40.0, 80.0), 1)
    return Leitura(ts, bpm, temp, umid)


fila_offline = []


def armazenar_offline(l):
    """Adiciona na fila; descarta o mais antigo se cheia."""
    if len(fila_offline) >= MAX_REGISTROS:
        fila_offline.pop(0)
        print("    [AVISO] Fila cheia — registro mais antigo descartado.")
    fila_offline.append(l)


def imprimir_leitura(l, status):
    """Formata saída igual ao Serial do sketch."""
    print(f"[{l.timestamp}ms] BPM: {l.bpm} | Temp: {l.temperatura}C | Umidade: {l.umidade}% | Status: {status}")


def sincronizar_fila():
    """Imprime e limpa a fila (simula envio para nuvem)."""
    if not fila_offline:
        return
    print("=" * 50)
    print(f"[SYNC] Sincronizando {len(fila_offline)} registro(s) offline...")
    print("=" * 50)
    for l in fila_offline:
        imprimir_leitura(l, "SYNC→ONLINE")
    fila_offline.clear()
    print("=" * 50)
    print("[SYNC] Fila limpa.\n")

# ─── Testes ───────────────────────────────────────────────────────────────────


def test_mapeamento_bpm():
    print("── Teste 1: Mapeamento de BPM ──")
    assert mapear_bpm(0) == 40,  "ADC 0 deve mapear para 40 BPM"
    assert mapear_bpm(4095) == 180, "ADC 4095 deve mapear para 180 BPM"
    meio = mapear_bpm(2047)
    assert 108 <= meio <= 112, f"ADC ~meio deve mapear para ~110 BPM, obteve {meio}"
    print(f"  ADC=0     → {mapear_bpm(0)} BPM  ✓")
    print(f"  ADC=2047  → {mapear_bpm(2047)} BPM  ✓")
    print(f"  ADC=4095  → {mapear_bpm(4095)} BPM  ✓\n")


def test_fila_offline():
    print("── Teste 2: Fila offline (capacidade e descarte) ──")
    fila_offline.clear()

    # Preenche exatamente até o limite
    for i in range(MAX_REGISTROS):
        armazenar_offline(ler_sensores(i * 5000))
    assert len(
        fila_offline) == MAX_REGISTROS, "Fila deve ter exatamente 20 registros"
    print(f"  Fila com {MAX_REGISTROS} registros: ✓")

    primeiro_ts_antes = fila_offline[0].timestamp

    # Adiciona mais um — deve descartar o mais antigo
    armazenar_offline(ler_sensores(999999))
    assert len(fila_offline) == MAX_REGISTROS, "Fila não deve ultrapassar 20"
    assert fila_offline[0].timestamp != primeiro_ts_antes, "Registro mais antigo deve ter sido descartado"
    assert fila_offline[-1].timestamp == 999999, "Novo registro deve estar no final"
    print(f"  Descarte do mais antigo ao exceder limite: ✓\n")


def test_sincronizacao():
    print("── Teste 3: Sincronização e limpeza da fila ──")
    fila_offline.clear()
    for i in range(5):
        armazenar_offline(ler_sensores(i * 5000))
    assert len(fila_offline) == 5

    sincronizar_fila()
    assert len(fila_offline) == 0, "Fila deve estar vazia após sincronização"
    print("  Fila limpa após sync: ✓\n")


def test_formato_saida():
    print("── Teste 4: Formato de saída Serial ──")
    l = Leitura(12500, 75, 36.8, 62.3)
    print("  Saída esperada:")
    imprimir_leitura(l, "OFFLINE")
    imprimir_leitura(l, "ONLINE")
    print("  Formato validado visualmente ✓\n")


def test_ciclo_completo():
    print("── Teste 5: Ciclo completo offline → online ──")
    fila_offline.clear()
    wifi = False
    ts = 0

    # Simula 6 leituras offline
    print("  [Fase OFFLINE — 6 leituras]")
    for _ in range(6):
        l = ler_sensores(ts)
        armazenar_offline(l)
        imprimir_leitura(l, "OFFLINE")
        print(
            f"    [Fila local: {len(fila_offline)}/{MAX_REGISTROS} registros]")
        ts += 5000

    assert len(fila_offline) == 6

    # Wi-Fi volta
    wifi = True
    print("\n  >>> Wi-Fi CONECTADO — iniciando sincronização...")
    sincronizar_fila()
    assert len(fila_offline) == 0

    # Simula 3 leituras online
    print("  [Fase ONLINE — 3 leituras diretas]")
    for _ in range(3):
        l = ler_sensores(ts)
        imprimir_leitura(l, "ONLINE")
        ts += 5000

    print("  Ciclo completo offline → online: ✓\n")

# ─── Execução ─────────────────────────────────────────────────────────────────


if __name__ == "__main__":
    print("=" * 50)
    print("  CardioIA — Testes da lógica do sketch.ino")
    print("=" * 50 + "\n")

    test_mapeamento_bpm()
    test_fila_offline()
    test_sincronizacao()
    test_formato_saida()
    test_ciclo_completo()

    print("=" * 50)
    print("  Todos os testes passaram com sucesso.")
    print("=" * 50)
