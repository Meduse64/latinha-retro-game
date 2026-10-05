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
| **Cartão microSD (FAT32, 8–32 GB)** | Guardar as ROMs e os saves. Não o vi na foto |

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

## 5. Emulação de consolas (as tuas ROMs)

O CYD é um ESP32 clássico: 240 MHz, cerca de 320 kB de SRAM utilizável, **sem PSRAM**, 4 MB de flash. Esse limite de memória decide o que corre.

| Sistema | Viável? | Núcleo a usar | Notas |
|---------|---------|---------------|-------|
| **Game Boy (DMG)** | **Sim, já existe para o CYD** | Walnut-CGB / Peanut-GB (MIT) | O projeto **CYDboy** corre-o a 50+ fps sem PSRAM, com ROMs lidas da SD |
| **Game Boy Color** | **Sim, já existe para o CYD** | Walnut-CGB (CYDboy) | Cores CGB, VRAM e WRAM com bancos, som de 4 canais, save states |
| **NES** | **Sim, no Retro-Go para CYD** (sem PSRAM, segundo as fontes; não testei) | nofrendo (Retro-Go) | Pouca RAM (2 kB + 2 kB + framebuffer de 61 kB). **Mas o Retro-Go para CYD não tem Bluetooth** (ver "Comando") |
| **Master System** | **Sim, no Retro-Go para CYD** (idem) | smsplus (Retro-Go) | Z80 a 3,58 MHz, muito leve |
| **Game Gear** | **Sim, no Retro-Go para CYD** (idem) | smsplus | Mesmo núcleo do Master System, ecrã 160×144 |
| **SNES** | **Não é prático** | snes9x 2005 (Retro-Go) | O próprio Retro-Go marca-o como lento. Só a memória (128 kB WRAM + 64 kB VRAM + 64 kB áudio + framebuffer) já ultrapassa os ~320 kB |
| **GBA** | **Não** | — | Só a memória do sistema (32 kB + 256 kB + 96 kB de VRAM) é cerca de 384 kB, mais do que a SRAM do CYD, e as ROMs chegam a 32 MB |

Estado de confiança:
- **Game Boy e Game Boy Color**: confirmados por projetos que correm no CYD (CYDboy, cyd-gb), com Bluetooth.
- **NES, Master System e Game Gear**: o fork do Retro-Go para o CYD (branch `CYD` de DynaMight1124) existe, e as descrições que encontrei dizem que sem PSRAM corre Game Boy, Color, NES, Game Gear, Master System, PC Engine e Lynx. Não consegui abrir as páginas originais (Instructables e Thingiverse estão bloqueadas aqui) nem testei nada.
- **SNES e GBA**: conclusões minhas a partir da memória necessária. As mesmas descrições dizem que os emuladores mais pesados do Retro-Go precisam de PSRAM, que no CYD só se obtém com uma modificação de hardware delicada.

**Comando:** o Zero 2 tem D-pad, A, B, X, Y, Select e Start, o suficiente para Game Boy, Color, NES, Master System e Game Gear. Não tem L/R, que o SNES e o GBA usam. O CYDboy usa o Bluepad32, a mesma biblioteca do Zero 2 (a documentação lista "8BitDo" em geral, sem confirmar o Zero 2: testar).

**O problema do Retro-Go para CYD:** no ficheiro de configuração do alvo `cyd`, todos os botões vêm de um expansor I²C **PCF8575** ligado a GPIO22 (SDA) e GPIO27 (SCL), que são os pinos do conector CN1. **Não há Bluetooth** nem entradas por touch definidos, e o driver de bateria está desligado. Ou seja, com o Zero 2 por Bluetooth o Retro-Go não funciona tal como está.

### Arquitetura proposta

- **Um lançador + um programa por emulador**, em partições de flash separadas. O menu lê as ROMs da microSD pela extensão (`.gb`, `.gbc`, `.nes`, `.sms`, `.gg`), escolhe o emulador, guarda a escolha e reinicia para ele. Cada emulador arranca com a RAM toda livre.
- **ROMs na microSD (FAT32)**: Game Boy e Color lidas diretamente da SD (como o CYDboy); NES, SMS e GG, que são pequenas, podem ser lidas por blocos com cache ou copiadas para uma partição de flash.
- **Saves e save states** em `/saves/` na SD, ao lado de cada ROM.
- **Ecrã**: NES em 256×240 e Master System em 256×192, ambos 1:1; Game Boy e Game Gear em 160×144 (1:1 ou ajustado à altura, 240×216).
- **Som**: saída mono no DAC do GPIO26. Os emuladores têm de baixar para 16–22 kHz.
- **Licenças**: Peanut-GB, Walnut-CGB e CYDboy são MIT. Nofrendo e smsplus costumam ser GPL, por isso o código que os use e seja publicado tem de ficar GPL.

### Caminho recomendado

1. **Já**: grava o CYDboy no CYD, põe uma microSD com uma ROM Game Boy tua e emparelha o Zero 2. Isto valida o hardware, o Bluetooth e a velocidade em horas, sem escrever código.
2. Depois, ligamos o resto (bateria e boost, secção 2).
3. Para NES, Master System e Game Gear há três opções:

| Opção | O que é | Prós | Contras |
|-------|---------|------|---------|
| **A. Retro-Go com botões físicos** | Gravar o fork `CYD` do Retro-Go e ligar botões a um PCF8575 em CN1 (o cabo de 4 fios da foto serve) | Já existe e foi pensado para este ecrã; junta GB, GBC, NES, SMS, GG num só menu | Obriga a soldar cerca de 10 botões e um módulo PCF8575; o Zero 2 fica de fora; a bateria não é medida |
| **B. Retro-Go + Bluetooth** | Acrescentar o Bluepad32 ao Retro-Go para usar o Zero 2 | Um só firmware, com o comando que já tens | Trabalho de programação; a pilha Bluetooth gasta RAM e pode não caber sem PSRAM; só se sabe testando no teu hardware |
| **C. Firmware próprio** | O lançador com núcleos separados (arquitetura acima) | Controlo total | O maior trabalho |

Recomendo a **B**: é a única que usa o teu Zero 2 sem comprar nem soldar nada. O risco é a RAM do Bluetooth, e só se descobre testando no teu hardware. A opção A só serve de teste rápido se aceitares comprar um módulo PCF8575 e botões, porque o Retro-Go não é navegável sem eles.

Os jogos próprios (secção 6) passam a ser opcionais.

---

## 6. Jogos próprios (opcional)

Temática "latinha":

1. **Derruba-Latas**: estilo Breakout/Arkanoid. A bola derruba pirâmides de latas. É o melhor para validar física, colisões e som.
2. **Cobrinha**: Snake em grelha. Valida o ciclo de jogo e os recordes.
3. **Latinha Runner**: corrida infinita em que a latinha salta obstáculos. Valida sprites, scroll e animação.

Depois do MVP: shoot'em up, puzzle estilo Tetris, ou um carregador de jogos a partir da microSD.

---

## 7. Caixa

- **Impressão 3D** (recomendado): cabe CYD + LiPo + TP4056 + boost + interruptor, com recortes para USB-C, interruptor e altifalante.
- **Lata de pastilhas** (se "latinha" for isso): o CYD (~86×50 mm) e a LiPo (~60×40×9,5 mm) cabem empilhados numa lata de ~95×60×21 mm, mas **o metal bloqueia o Bluetooth**. A antena do ESP32 fica na ponta da placa e tem de ficar fora do metal (tampa de plástico ou recorte).

---

## 8. Plano em fases

| Fase | Entrega | Critério de sucesso |
|------|---------|---------------------|
| 0 | Teste de hardware | Ecrã, touch, LED, beep no altifalante e leitura da SD a funcionar |
| 1 | CYDboy + Zero 2 | Uma ROM Game Boy tua a jogar, com som e comando Bluetooth |
| 2 | Alimentação | Boost + TP4056 + interruptor, a correr a bateria e com medidor de carga |
| 3 | Lançador + Game Boy/Color | Menu que lista as ROMs da SD e arranca o emulador, com saves |
| 4 | Master System / Game Gear / NES | Cada um medido: fps e memória livre; só entra o que for jogável |
| 5 | Caixa e acabamento | Logo de arranque, aviso de bateria fraca, suspensão |
| 6 (opcional) | Jogos próprios | Derruba-Latas, Cobrinha, Latinha Runner |

---

## 9. Perguntas em aberto

1. Onde estão os projetos precedentes (repositório, código, esquemas)?
2. "Latinha" é uma lata/caixa de metal onde queres montar tudo, ou é só o nome?
3. Tens (ou compras) um módulo boost 5 V? Sem ele, só dá para o teste rápido da secção 2.
4. Preferes Arduino/PlatformIO (mais rápido) ou ESP-IDF (mais controlo)?
