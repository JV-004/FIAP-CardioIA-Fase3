#!/usr/bin/env python3
"""
Teste da lógica do sistema CardioIA — Fase 3
Valida: mapeamento de BPM, fila offline, alertas médicos e sincronização.

Este script testa a lógica implementada no ESP32 usando Python
para validar o comportamento antes da implementação em hardware.
"""

import random
import time
from typing import List, Dict, Any

# ═══════════════════════════════════════════════════════════════════════════════
# CONSTANTES (espelham as definições do ESP32)
# ═══════════════════════════════════════════════════════════════════════════════

MAX_REGISTROS_OFFLINE = 20
INTERVALO_LEITURA = 5000  # ms
INTERVALO_WIFI = 30000    # ms

# Limites de alerta médico
BPM_LIMITE_ALTO = 120
TEMP_LIMITE_ALTO = 38.0
UMIDADE_LIMITE_BAIXO = 40.0
UMIDADE_LIMITE_ALTO = 80.0

# ═══════════════════════════════════════════════════════════════════════════════
# ESTRUTURAS DE DADOS
# ═══════════════════════════════════════════════════════════════════════════════

class LeituraVital:
    """Espelha a struct LeituraVital do ESP32"""
    def __init__(self, timestamp: int, bpm: int, temperatura: float, umidade: float):
        self.timestamp = timestamp
        self.bpm = bpm
        self.temperatura = temperatura
        self.umidade = umidade
        self.alertaAtivo = self.verificar_alertas()
    
    def verificar_alertas(self) -> bool:
        """Verifica se há condições de alerta médico"""
        if self.bpm > BPM_LIMITE_ALTO:
            return True
        if self.temperatura > TEMP_LIMITE_ALTO:
            return True
        if self.umidade < UMIDADE_LIMITE_BAIXO or self.umidade > UMIDADE_LIMITE_ALTO:
            return True
        return False

# ═══════════════════════════════════════════════════════════════════════════════
# SIMULAÇÃO DOS SENSORES
# ═══════════════════════════════════════════════════════════════════════════════

def mapear_bpm(valor_adc: int) -> int:
    """Mapeia ADC 0-4095 para BPM 40-180 (equivalente ao map() do Arduino)"""
    return int(40 + (valor_adc / 4095) * (180 - 40))

def simular_sensores(timestamp: int) -> LeituraVital:
    """Simula leitura dos sensores com valores aleatórios realistas"""
    # Simula potenciômetro (ADC 0-4095)
    adc_bpm = random.randint(0, 4095)
    bpm = mapear_bpm(adc_bpm)
    
    # Simula DHT22 com valores realistas
    temperatura = round(random.uniform(35.0, 40.0), 1)  # Temperatura corporal
    umidade = round(random.uniform(30.0, 90.0), 1)      # Umidade ambiente
    
    return LeituraVital(timestamp, bpm, temperatura, umidade)

# ═══════════════════════════════════════════════════════════════════════════════
# SISTEMA DE ARMAZENAMENTO OFFLINE (EDGE COMPUTING)
# ═══════════════════════════════════════════════════════════════════════════════

class SistemaEdgeComputing:
    """Simula o sistema de Edge Computing do ESP32"""
    
    def __init__(self):
        self.fila_offline: List[LeituraVital] = []
        self.total_leituras = 0
        self.total_alertas = 0
    
    def armazenar_offline(self, leitura: LeituraVital) -> None:
        """Adiciona leitura na fila offline (fila circular)"""
        if len(self.fila_offline) >= MAX_REGISTROS_OFFLINE:
            # Fila cheia: remove o mais antigo (FIFO)
            self.fila_offline.pop(0)
            print("    [EDGE] Fila cheia - registro mais antigo descartado")
        
        self.fila_offline.append(leitura)
    
    def sincronizar_dados_offline(self) -> List[LeituraVital]:
        """Sincroniza dados offline e limpa a fila"""
        if not self.fila_offline:
            print("[SYNC] Nenhum dado offline para sincronizar")
            return []
        
        print("=" * 40)
        print(f"[SYNC] Sincronizando {len(self.fila_offline)} registro(s) offline...")
        print("=" * 40)
        
        dados_sincronizados = self.fila_offline.copy()
        for leitura in dados_sincronizados:
            self.imprimir_leitura(leitura, "SYNC->CLOUD")
        
        self.fila_offline.clear()
        print("=" * 40)
        print("[SYNC] Concluido. Fila limpa.")
        print("=" * 40)
        
        return dados_sincronizados
    
    def imprimir_leitura(self, leitura: LeituraVital, status: str) -> None:
        """Formata e imprime leitura (espelha o formato do ESP32)"""
        alerta_str = " | ALERTA" if leitura.alertaAtivo else ""
        print(f"[{leitura.timestamp}ms] BPM: {leitura.bpm} | "
              f"Temp: {leitura.temperatura}C | "
              f"Umidade: {leitura.umidade}% | "
              f"Status: {status}{alerta_str}")
    
    def processar_leitura(self, leitura: LeituraVital, wifi_conectado: bool) -> None:
        """Processa uma leitura baseado no status da conectividade"""
        self.total_leituras += 1
        
        if leitura.alertaAtivo:
            self.total_alertas += 1
            if leitura.bpm > BPM_LIMITE_ALTO:
                print(f"[ALERTA] Taquicardia: {leitura.bpm} BPM")
            if leitura.temperatura > TEMP_LIMITE_ALTO:
                print(f"[ALERTA] Febre: {leitura.temperatura}C")
            if leitura.umidade < UMIDADE_LIMITE_BAIXO or leitura.umidade > UMIDADE_LIMITE_ALTO:
                print(f"[ALERTA] Umidade inadequada: {leitura.umidade}%")
        
        if wifi_conectado:
            self.imprimir_leitura(leitura, "ONLINE")
            print("    -> Enviado para nuvem via MQTT")
        else:
            self.armazenar_offline(leitura)
            self.imprimir_leitura(leitura, "OFFLINE")
            print(f"    -> Fila local: [{len(self.fila_offline)}/{MAX_REGISTROS_OFFLINE}]")

# ═══════════════════════════════════════════════════════════════════════════════
# TESTES UNITÁRIOS
# ═══════════════════════════════════════════════════════════════════════════════

def test_mapeamento_bpm():
    """Testa o mapeamento ADC -> BPM"""
    print("-- Teste 1: Mapeamento de BPM --")
    assert mapear_bpm(0) == 40
    assert mapear_bpm(4095) == 180
    meio = mapear_bpm(2047)
    assert 108 <= meio <= 112
    print(f"  ADC=0    -> {mapear_bpm(0)} BPM  OK")
    print(f"  ADC=2047 -> {mapear_bpm(2047)} BPM  OK")
    print(f"  ADC=4095 -> {mapear_bpm(4095)} BPM  OK\n")

def test_sistema_alertas():
    """Testa o sistema de deteccao de alertas"""
    print("-- Teste 2: Sistema de Alertas --")
    leitura_normal = LeituraVital(1000, 80, 36.5, 60.0)
    assert not leitura_normal.alertaAtivo
    print("  Valores normais: sem alerta OK")
    leitura_taquicardia = LeituraVital(2000, 130, 36.5, 60.0)
    assert leitura_taquicardia.alertaAtivo
    print("  Taquicardia (BPM > 120): alerta ativo OK")
    leitura_febre = LeituraVital(3000, 80, 39.0, 60.0)
    assert leitura_febre.alertaAtivo
    print("  Febre (Temp > 38C): alerta ativo OK")
    leitura_umidade = LeituraVital(4000, 80, 36.5, 30.0)
    assert leitura_umidade.alertaAtivo
    print("  Umidade inadequada (< 40%): alerta ativo OK\n")

def test_fila_offline():
    """Testa a fila circular offline"""
    print("-- Teste 3: Fila Offline (Edge Computing) --")
    sistema = SistemaEdgeComputing()
    for i in range(MAX_REGISTROS_OFFLINE):
        leitura = LeituraVital(i * 1000, 80, 36.5, 60.0)
        sistema.armazenar_offline(leitura)
    assert len(sistema.fila_offline) == MAX_REGISTROS_OFFLINE
    print(f"  Fila preenchida: {MAX_REGISTROS_OFFLINE} registros OK")
    primeiro_timestamp = sistema.fila_offline[0].timestamp
    leitura_extra = LeituraVital(99999, 80, 36.5, 60.0)
    sistema.armazenar_offline(leitura_extra)
    assert len(sistema.fila_offline) == MAX_REGISTROS_OFFLINE
    assert sistema.fila_offline[0].timestamp != primeiro_timestamp
    assert sistema.fila_offline[-1].timestamp == 99999
    print("  Descarte FIFO: funcionando OK\n")

def test_sincronizacao():
    """Testa a sincronizacao de dados offline"""
    print("-- Teste 4: Sincronizacao --")
    sistema = SistemaEdgeComputing()
    for i in range(5):
        leitura = LeituraVital(i * 5000, 80 + i, 36.5, 60.0)
        sistema.armazenar_offline(leitura)
    assert len(sistema.fila_offline) == 5
    dados_sync = sistema.sincronizar_dados_offline()
    assert len(sistema.fila_offline) == 0
    assert len(dados_sync) == 5
    print("  Sincronizacao e limpeza: funcionando OK\n")

def test_ciclo_completo():
    """Testa um ciclo completo offline -> online"""
    print("-- Teste 5: Ciclo Completo Offline -> Online --")
    
    sistema = SistemaEdgeComputing()
    timestamp = 0
    wifi_conectado = False
    
    print("  [Fase OFFLINE - 6 leituras]")
    for i in range(6):
        leitura = simular_sensores(timestamp)
        sistema.processar_leitura(leitura, wifi_conectado)
        timestamp += INTERVALO_LEITURA
    
    assert len(sistema.fila_offline) == 6, "Deve ter 6 registros offline"
    
    wifi_conectado = True
    print("\n  [WIFI] CONECTADO - iniciando sincronizacao...")
    sistema.sincronizar_dados_offline()
    
    assert len(sistema.fila_offline) == 0, "Fila deve estar limpa apos sync"
    
    print("\n  [Fase ONLINE - 3 leituras diretas]")
    for i in range(3):
        leitura = simular_sensores(timestamp)
        sistema.processar_leitura(leitura, wifi_conectado)
        timestamp += INTERVALO_LEITURA
    
    print("  Ciclo completo offline -> online: funcionando OK\n")

# ═══════════════════════════════════════════════════════════════════════════════
# EXECUÇÃO DOS TESTES
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    """Executa todos os testes do sistema CardioIA"""
    print("=" * 60)
    print("  CardioIA - Testes da Logica do Sistema ESP32")
    print("  Fase 3: Edge Computing + IoT Medico")
    print("=" * 60 + "\n")
    
    try:
        test_mapeamento_bpm()
        test_sistema_alertas()
        test_fila_offline()
        test_sincronizacao()
        test_ciclo_completo()
        
        print("=" * 60)
        print("  TODOS OS TESTES PASSARAM COM SUCESSO!")
        print("  Sistema pronto para implementacao no ESP32")
        print("=" * 60)
        
    except AssertionError as e:
        print(f"❌ TESTE FALHOU: {e}")
        return False
    except Exception as e:
        print(f"❌ ERRO INESPERADO: {e}")
        return False
    
    return True

if __name__ == "__main__":
    main()