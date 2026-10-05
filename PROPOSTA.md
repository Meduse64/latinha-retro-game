# Latinha Retro Game — Proposta

Consola portátil retro feita com o material da foto: ecrã ESP32 "CYD", comando 8BitDo Zero 2, bateria LiPo, carregador USB-C e altifalante.

> Nota: o repositório estava vazio e não tive acesso aos projetos anteriores, por isso esta proposta parte só do material da foto. Quando indicares os projetos (código, bibliotecas, esquemas), adapto o que for reaproveitável.

---

## 1. Material identificado na foto

| # | Peça | O que é / para que serve |
|---|------|--------------------------|
| 1 | **ESP32-2432S028R ("CYD", Cheap Yellow Display)** | ESP32-WROOM-32 (2 núcleos a 240 MHz, Wi-Fi + Bluetooth Classic/BLE), ecrã 2,8" ILI9341 **240×320**, touch resistivo XPT2046, slot microSD, saída de áudio mono (DAC GPIO26 → amplificador), LED RGB, sensor de luz. É o cérebro e o ecrã. |
| 2 | **8BitDo Zero 2** | Comando Bluetooth: D-pad, A/B/X/Y, Select, Start. Suportado pela biblioteca **Bluepad32** (Bluetooth BR/EDR, funciona só no ESP32 clássico, que é o do CYD). |
| 3 | Altifalante pequeno com espuma | Liga ao conector do altifalante do CYD (JST 1,25 mm, 2 pinos). Confirmar o conector. |
| 4 | **LiPo 3,7 V, 3000 mAh** (etiqueta; formato ~9,5×40×60 mm) | Alimentação. |
| 5 | **Módulo TP4056 USB-C (HW-373 V1.2.1)** com proteção da bateria | Carrega a LiPo por USB-C (2 LEDs: carga / completo). Tem os dois circuitos integrados de proteção (DW01 + MOSFETs duplos 8205), por isso corta em subtensão, sobretensão e curto-circuito. Bateria em **B+/B−**, consumo em **OUT+/OUT−**. |
| 6 | **Módulo regulador AMS1117 (3,3 V)**: entrada VIN+/GND, saída VOUT+/GND, LED de presença | Baixa 4,5–7 V para 3,3 V. **Não serve para ligar à LiPo** (ver secção 2), e o CYD já traz um AMS1117 igual na placa. |
| 7 | Cabo JST 1,25 mm de 4 pinos + fios dupont | Para os conectores P3/CN1 do CYD (expansão). |

### Pinagem do CYD que interessa

| Função | GPIO |
|--------|------|
| Ecrã (SPI) | DC 2, MISO 12, MOSI 13, SCK 14, CS 15, backlight 21 |
| Touch (XPT2046) | CLK 25, MOSI 32, CS 33, IRQ 36, MISO 39 |
| microSD (SPI próprio) | CS 5, SCK 18, MISO 19, MOSI 23 |
| Áudio (DAC) | 26 |
| LED RGB (ativo a 0) | R 4, G 16, B 17 |
| Sensor de luz | 34 |
| Botão BOOT (usável como entrada) | 0 |
| **GPIOs livres** | **35** (só entrada), **22**, **27** |

O cabo de 4 fios da foto liga a CN1 (GND, IO22, IO27, 3V3), que serve para I²C.

---

## 2. O ponto crítico: a alimentação

A LiPo dá 3,0–4,2 V. O CYD tem um regulador AMS1117-3,3 V que precisa de cerca de 4,5–5 V à entrada (queda de ~1,1 V). Ligar a bateria diretamente ao VIN dá uma tensão instável ou insuficiente, e ligar 4,2 V ao pino 3V3 ultrapassa o máximo do ESP32.

**Solução recomendada**

```
USB-C ─► TP4056 (HW-373) ── B+/B− ──► LiPo
              │
            OUT+ ──► interruptor ──► boost 5 V ──► VIN (conector P1)
            OUT− ───────────────────────────────► GND (P1)

  Medição da bateria:  OUT+ ─ 100 kΩ ─┬─ 100 kΩ ─ GND
                                      └──► IO35 (P3)
```

- **Boost 5 V**: um módulo MT3608 ou equivalente. **É a única peça de eletrónica que falta.** Regula o trimmer para **5,0 V com um multímetro antes de ligar ao CYD**: estes módulos costumam vir de fábrica com a saída ao máximo (~28 V) e queimam a placa.
- **Interruptor deslizante** em série com OUT+.
- **Divisor 2×100 kΩ → IO35**: dá a percentagem de bateria. O IO35 está no ADC1, que funciona com o Bluetooth ligado.
- **Carregar e jogar ao mesmo tempo**: o TP4056 pode não detetar o fim de carga se houver consumo. Carregar com o interruptor desligado é mais seguro. Os 1 A típicos do HW-373 são aceitáveis para 3000 mAh (~0,33 C).
- **Autonomia**: estimativa teórica de 6–10 h (ecrã e Bluetooth ligados). Medir na prática.

### Porque é que o módulo AMS1117 que tens não resolve

O AMS1117 precisa de a entrada estar ~1 V acima da saída. Com a LiPo a 3,7 V a saída cai para ~2,6 V, e com 4,2 V (cheia) fica em ~3,1 V, sempre abaixo dos 3,3 V. Só regula bem com 4,5 V ou mais à entrada, ou seja, **depois** do boost de 5 V. Mas aí o CYD já tem o seu próprio AMS1117 na placa e o módulo ficaria redundante. Guarda-o para outro projeto.

**Teste rápido, sem comprar nada (só para a fase 0, não para uso final):** ligar a LiPo (via TP4056, OUT+/OUT−) diretamente ao VIN do CYD pode arrancar com a bateria cheia, mas a tensão interna cai para ~2,8 V e o ESP32 vai reiniciar ou perder o Bluetooth quando a bateria baixar. Não ligues os 4,2 V ao pino 3V3: excede o máximo do ESP32 (3,6 V).

### Lista do que falta

| Peça | Para quê |
|------|----------|
| Módulo boost MT3608 (ou equivalente) | 3,7 V → 5 V para o VIN do CYD |
| Interruptor deslizante | Ligar/desligar em série com OUT+ |
| 2 resistências de 100 kΩ | Divisor do medidor de bateria (IO35) |
| Termorretrátil e fio fino | Isolar e ligar tudo |

### Cuidados com a bateria

Na primeira foto os fios da LiPo parecem ter as pontas descobertas. Isola cada fio antes de mexer e solda-os um de cada vez em B+ (vermelho) e B− (preto), sem nunca deixar as pontas a tocar uma na outra. Um curto-circuito numa LiPo de 3000 mAh pode causar fogo.

---

## 3. Controlo: duas variantes

**V1 (recomendada): Zero 2 por Bluetooth.** Sem fios nem soldaduras. O touch e o botão BOOT ficam como alternativa, para testar sem o comando.

**V2 (opcional): botões próprios** num expansor I²C (MCP23017 ou PCF8574) ligado a CN1 (IO22 = SCL, IO27 = SDA). Dá 8–16 botões com só 2 GPIO, porque só há 3 livres. O cabo de 4 fios da foto serve para isto.

---

## 4. Software

| Camada | Escolha | Porquê |
|--------|---------|--------|
| Build | **PlatformIO** + Arduino-ESP32 | Rápido de arrancar, boa gestão de bibliotecas |
| Gráficos | **LovyanGFX** (alternativa: TFT_eSPI) | DMA, sprites, rotação, mais rápido |
| Comando | **Bluepad32** | Suporta o 8BitDo Zero 2 |
| Áudio | Sintetizador próprio no DAC (GPIO26) | 2 ondas quadradas + triangular + ruído, estilo NES; mono |
| Dados | LittleFS (flash) para recordes; microSD opcional para assets | A SD tem SPI próprio, sem conflito com o ecrã |

**Risco a validar logo de início:** o Bluepad32 para Arduino usa um pacote de placas próprio (`esp32-bluepad32`), que pode atrasar-se em relação ao Arduino-ESP32 oficial. Se der problemas, o plano B é ESP-IDF puro.

### Motor de jogo (mínimo)

- Ciclo fixo a 60 Hz: `input → update → render → áudio`. O jogo corre no núcleo 1 e o áudio no núcleo 0.
- Resolução lógica **160×120** em 8 bits com paleta (19,2 kB), ampliada ×2 para 320×240 no ecrã em paisagem. Cabe na RAM com o Bluetooth ativo (um framebuffer de 16 bits a 320×240 gasta 150 kB, demasiado com o stack BT).
- Orçamento do SPI: com 40 MHz, um ecrã completo (153,6 kB) demora ~31 ms, ou seja ~30 fps. Para 60 fps, atualizar só as zonas que mudaram.
- Sprites 8×8 / 16×16 e tilemap com scroll.
- Camada de entrada única (`dpad`, `A`, `B`, `X`, `Y`, `start`, `select`) alimentada pelo Bluepad32, pelo touch ou pelo botão BOOT.
- Gestor de cenas: boot logo → menu → jogo → pausa.

---

## 5. Jogos propostos

Temática "latinha":

1. **Derruba-Latas**: estilo Breakout/Arkanoid. A bola derruba pirâmides de latas. É o melhor para validar física, colisões e som.
2. **Cobrinha**: Snake em grelha. Valida o ciclo de jogo e os recordes.
3. **Latinha Runner**: corrida infinita em que a latinha salta obstáculos. Valida sprites, scroll e animação.

Depois do MVP: shoot'em up, puzzle estilo Tetris, ou um carregador de jogos a partir da microSD.

---

## 6. Caixa

- **Impressão 3D** (recomendado): cabe CYD + LiPo + TP4056 + boost + interruptor, com recortes para USB-C, interruptor e altifalante.
- **Lata de pastilhas** (se "latinha" for isso): o CYD (~86×50 mm) e a LiPo (~60×40×9,5 mm) cabem empilhados numa lata de ~95×60×21 mm, mas **o metal bloqueia o Bluetooth**. A antena do ESP32 fica na ponta da placa e tem de ficar fora do metal (tampa de plástico ou recorte).

---

## 7. Plano em fases

| Fase | Entrega | Critério de sucesso |
|------|---------|---------------------|
| 0 | Teste de hardware | Ecrã, touch, LED, beep no altifalante e leitura da SD a funcionar |
| 1 | Emparelhar o Zero 2 | Ecrã de teste mostra todos os botões em tempo real |
| 2 | Alimentação | Boost + TP4056 + interruptor, a correr a bateria e com medidor de carga |
| 3 | Motor | 60 Hz estáveis, sprites, tilemap, som |
| 4 | Jogos | Menu + 3 jogos + recordes |
| 5 | Caixa e acabamento | Logo de arranque, aviso de bateria fraca, suspensão |

---

## 8. Perguntas em aberto

1. Onde estão os projetos precedentes (repositório, código, esquemas)?
2. "Latinha" é uma lata/caixa de metal onde queres montar tudo, ou é só o nome?
3. Tens (ou compras) um módulo boost 5 V? Sem ele, só dá para o teste rápido da secção 2.
4. Preferes Arduino/PlatformIO (mais rápido) ou ESP-IDF (mais controlo)?
