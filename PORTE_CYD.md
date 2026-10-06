O NES fica fora deste firmware por enquanto, para não juntar mais código de terceiros com licenças diferentes (o Anemoia é GPLv3). Se quiser NES, o caminho é o firmware do Anemoia separado, como descrito no `BOTOES.md`.
# Porte do app Retro (LatinhaColor) para o CYD

Este documento parte do arquivo do app Retro do firmware anterior (LatinhaColor), que você colou na conversa. Eu só vi **esse arquivo**: não tenho o `gb_core.c`, o `src/smsplus`, o restante do firmware (tela, botões, menu) nem o script `gravar_jogos.ps1`. Tudo abaixo é leitura do código colado e **não foi compilado nem testado**.

> **Escopo (atualizado depois das medidas no CYD):** o console é de **Game Boy, Game Gear e Master System**. O Game Boy Color deixa de ser requisito: no CYD, sem PSRAM, os jogos de GBC em velocidade dupla (como o Space Invaders) rodam a uns 30 FPS, cerca de metade da velocidade real, e só um núcleo de emulador bem mais rápido resolveria isso. O Walnut-CGB continua no firmware e os jogos de GBC rodam como bônus, sem promessa de velocidade (`firmware/cydboy_fw/README.md`).
>
> **Decisão atual (opção B):** um firmware só, partindo do código do CYDboy (Game Boy, com o Walnut-CGB), com o SMS Plus do LatinhaColor acrescentado para Game Gear e Master System. O porte do app Retro inteiro, descrito nas secções 1 a 4, passa a servir sobretudo para o SMS Plus e para o mapa de botões. Veja a secção 5.1 e o plano na secção 7.

---

## 1. O que o app Retro já faz

- **Game Boy** com o Peanut-GB (MIT), **Game Gear** e **Master System** com o SMS Plus (GPL v2, versão adaptada do esp_8_bit).
- Os jogos ficam na partição `spiffs` (esquema "No OTA"), gravados pelo `gravar_jogos.ps1`, com um índice `GBR3` de até 16 jogos. A partição é mapeada como memória (`esp_partition_mmap`), então o SMS Plus lê a ROM direto.
- **Save de bateria** (`BAT3`) e **foto / save state** (`STA3`) por jogo, gravados na própria partição. O cabeçalho é gravado por último, então um save interrompido não vale.
- Menu de pausa com o botão amarelo: Continuar, Salvar foto, Carregar foto, Sair.
- Cada linha emulada vai direto para o buffer da tela (`gbw_lcd_line`, `latinha_sms_line`), e o envio para o LCD roda numa tarefa no núcleo 0 enquanto o emulador faz o quadro seguinte.
- Sem som (`emu_system_init(0)`).

Isso é a opção C da `PROPOSTA.md` (firmware próprio), mas com a parte difícil já escrita.

---

## 2. O que muda no CYD

| Item | LatinhaColor (pelo código) | CYD |
|------|----------------------------|-----|
| Tela | 160×128 (`SW` = 160) | ILI9341 **320×240** |
| Buffer | `gfx` de 16 bits, 160×128 (cerca de 40 KB) | 320×240 em 16 bits são 150 KB: evitar. Usar um buffer do tamanho do jogo (GB/GG 160×144 = 46 KB; SMS 256×192 = 98 KB) |
| Escala | GB e GG: descarta 1 linha a cada 9 (144→128). SMS: 256×192 → 160×128 (descarta linhas e colunas) | GB e GG em **160×144, 1:1** (ou 1,5× = 240×216). SMS em **256×192, 1:1**. Some o código de descarte (`gl % 9`, `line % 3`, `smsX`) |
| Taxa de quadros | SMS desenha 1 quadro sim, 1 não | Medir. A 40 MHz de SPI, 98 KB levam cerca de 20 ms e 46 KB cerca de 9 ms (conta minha, sem testar) |
| Botões | Joystick 5D + SW1–SW4 + amarelo | PCF8574 no CN1 com 8 botões, e **menu de pausa por toque** em qualquer lugar da tela (secção 3). O BOOT fica opcional |
| Rede | Telegram (`tgIdle`, `netPause`, tarefa no núcleo 0) | Some: sem Wi-Fi, sobra mais RAM |
| ROMs | Partição `spiffs` de cerca de 2 MB (esquema "No OTA" de 4 MB, a confirmar), gravada pelo `gravar_jogos.ps1` | **microSD** (secção 5) |
| Som | Nenhum | Alto-falante no GPIO26, opcional |

---

## 3. Botões: dá para usar o que você já tem

O app usa 9 teclas: cima, baixo, esquerda, direita, `KEY_OK`, `KEY_BACK`, `KEY_A`, `KEY_B` e `KEY_HOME`. O PCF8574 tem 8 pinos, que cobrem as 8 teclas de jogo. O amarelo (`KEY_HOME`, o menu de pausa) passa para o **toque**, em qualquer lugar da tela (secção 5.1). Não há botão Option: Tamanho e as outras opções ficam dentro do menu de pausa. O botão BOOT do CYD (GPIO0, já na placa) fica como alternativa opcional para o menu. **Não precisa comprar o PCF8575.**

| Pino do PCF8574 | Tecla do app | Peça | Game Boy | Game Gear / Master System |
|-----------------|--------------|------|----------|---------------------------|
| P0 | `KEY_UP` | módulo 5D: UP | cima | cima |
| P1 | `KEY_DOWN` | módulo 5D: DWN | baixo | baixo |
| P2 | `KEY_LEFT` | módulo 5D: LFT | esquerda | esquerda |
| P3 | `KEY_RIGHT` | módulo 5D: RHT | direita | direita |
| P4 | `KEY_OK` | botão do kit nº 1 | A | botão 2 |
| P5 | `KEY_BACK` | botão do kit nº 2 | B | botão 1 |
| P6 | `KEY_B` | módulo 5D: RST | Start | Start (GG) / Pause (SMS) |
| P7 | `KEY_A` | módulo 5D: SET | Select | (sem uso) |
| Toque em qualquer lugar da tela | `KEY_HOME` | tela inteira | menu de pausa | menu de pausa |
| GPIO0 (BOOT), opcional | `KEY_HOME` | botão da placa | menu de pausa | menu de pausa |

Ligações do PCF8574 no CN1: SDA no IO22, SCL no IO27, VCC no 3V3, GND no GND. O COM do módulo 5D e o outro lado de cada botão vão no GND. O endereço depende dos jumpers A0/A1/A2. O sketch de teste está na secção 5.3 do `BOTOES.md`.

Dois detalhes:
- O GPIO0 é um pino de boot: o botão precisa estar solto na hora de ligar ou de resetar. Depois que o programa roda, pode ser usado como entrada normal. Como o Menu vai por toque, o BOOT é só um extra.
- O MID (centro) do módulo 5D fica sem uso.

---

## 4. O que já recebi e o que falta

**Recebido (colado ou enviado na conversa):**

- `app_retro.h` (o app Retro inteiro) e `gb_core.c` (adaptador do Peanut-GB).
- O SMS Plus completo: `sms.c/.h`, `vdp.c/.h`, `render.c/.h`, `lut.h`, `system.c/.h`, `sn76496.c/.h`, `shared.h`, `types.h`, `z80.c/.h`, `z80daa.h`, `cpuintrf.h`, `osd_cpu.h`, `latinha_state.c` e `vrc7tone.h`.

**Verificado aqui (compilador do computador, não o do ESP32):**

- O `peanut_gb.h` oficial (MIT, branch `master` do Peanut-GB) tem todos os símbolos que o `gb_core.c` usa, e o `gb_core.c` compila sem erro com ele. O `struct gb_s` tem 16 984 bytes em 64 bits, dentro da reserva de 18 432 (`0x4800`).
- Os 8 arquivos `.c` do SMS Plus passam na verificação de sintaxe e **ligam sem nenhum símbolo faltando**. O estado do SMS (`t_vdp` + `t_sms` + `Z80_Regs` + 1 inteiro) soma 24 936 bytes em 64 bits, dentro da reserva de 26 624 (`0x6800`). No ESP32 os ponteiros são menores, então fica ainda abaixo.

**Não preciso de mais nada para começar.** O código do CYD (tela, botões, menu) eu escrevo novo. Não são necessários o `gravar_jogos.ps1` nem o esquema de partições se as ROMs forem lidas do cartão SD (seção 5).

**O que não consegui verificar:** compilar para o ESP32. Aqui o PlatformIO e a toolchain da Espressif estão bloqueados pela rede, então tudo que eu escrever para o CYD será compilado pela primeira vez no seu computador.

---

## 5. ROMs no cartão SD

Hoje o app lê as ROMs de uma partição da flash, mapeada como memória. Para usar o microSD é preciso mudar isso, e a razão é o SMS Plus: ele lê a ROM como memória (`cart.rom`), então não dá para ler do cartão aos poucos como o CYDboy faz com o Game Boy.

| Sistema | Como ler do cartão |
|---------|--------------------|
| Game Gear e Master System | **Copiar a ROM do cartão para a partição da flash** ao escolher o jogo e mapear de lá, como hoje. Essas ROMs são pequenas (em geral de dezenas a algumas centenas de KB), então a cópia deve levar poucos segundos (estimativa minha). Se o jogo escolhido já é o que está na partição, pula a cópia |
| Game Boy e Game Boy Color | **Ler direto do cartão**, como o CYDboy já faz: segundo o README dele, percorre a tabela de clusters FAT32 e usa um cache de página de 512 bytes, sem PSRAM. É o que permite as ROMs de GBC, que chegam a 8 MB e não cabem na partição de cerca de 2 MB. Não li o `sd_manager.cpp` para ver os detalhes |

Pastas no cartão, iguais às do CYDboy para o mesmo cartão servir aos dois:

```
GAMES (D:)
├── roms/
│   ├── gb/      .gb
│   ├── gbc/     .gbc (ver abaixo)
│   ├── gg/      .gg
│   └── sms/     .sms
└── saves/       criada pelo firmware
```

- A lista de jogos deixa de ter o limite de 16 (`MAXROMS`) e passa a rolar.
- **Saves e fotos passam a ser arquivos** em `saves/` (por exemplo `jogo.sav` e `jogo.state`). Gasta menos a flash e dá para fazer backup copiando o cartão. Isso troca o código dos `BAT3` e `STA3` que gravam na partição. Para Game Boy e GBC o CYDboy já faz isso (`.sav` e `.state` em `/saves/`); falta só Game Gear e Master System.
- A flash passa a servir só para o jogo que está rodando. O tamanho da partição define o maior jogo que cabe (a definir quando eu vir o tamanho do firmware).
- **Game Boy Color:** o Peanut-GB é só do Game Boy original (DMG), então jogos exclusivos de GBC não rodam e os que servem para os dois saem em preto e branco. O núcleo passa a ser o **Walnut-CGB** (derivado do Peanut-GB, com suporte a GBC, MIT), que o CYDboy já usa, e o firmware parte do código do CYDboy. Detalhes na secção 5.1.
- O NES e as ROMs do Anemoia ficam na raiz do cartão, como ele pede, e não atrapalham.

### 5.1 Game Boy e Game Boy Color: o CYDboy como base do firmware (fases 2 e 3)

**Decisão (opção B):** um firmware só, partindo do código do [CYDboy](https://github.com/Rocka84/CYDboy) (Rocka84, MIT). Ele já roda o Walnut-CGB no CYD, então Game Boy e Game Boy Color vêm prontos. O que o LatinhaColor traz para cá passa a ser o **SMS Plus** (Game Gear e Master System). O `gb_core.c` e o Peanut-GB do LatinhaColor deixam de ser usados.

O que li do CYDboy, pelo GitHub: o README e três arquivos (`include/hw_config.h`, `src/button_input.cpp`, `src/display.cpp`). **Não li** o resto (`emulator_bridge.cpp`, `ui_launcher.cpp`, `sd_manager.cpp`, `bt_controller.cpp`...), não o compilei e não o rodei no CYD.

| Item | O que o CYDboy tem (lido) | O que fazemos |
|------|---------------------------|---------------|
| Núcleo | Walnut-CGB (`walnut_cgb.h`), com CGB completo. O `include/` também traz `peanut_gb.h` | Nada |
| Tela | TFT_eSPI, rotação 2 (USB em cima). A imagem de 160×144 vai a ×1,5 (240×216) linha a linha em `display_push_gb_line()`: cada 2 pixels viram 3 e as linhas ímpares se repetem. Barra de controles de 104 px a partir de y=216 | É o que a secção 6.1 já decidiu. Falta GG e SMS nesse mesmo esquema (240×216 e 240×180) |
| ROM | Direto do cartão (secção 5). Pastas `roms/gb` e `roms/gbc` | Acrescentar `roms/gg` e `roms/sms`, com a cópia da ROM para a flash |
| Saves | `.sav` e `.state` em `/saves/` | GB e GBC prontos. GG e SMS: adaptar o `latinha_state.c` para gravar arquivo |
| Botões | PCF8574 no endereço 0x20, lido como 1 byte, nível baixo = apertado. Bits: 0 cima, 1 baixo, 2 esquerda, 3 direita, 4 A, 5 B, 6 Start, 7 Select | O mapa é **igual** ao da secção 3 para o Game Boy (P4 A, P5 B, P6 Start, P7 Select), então a ligação já planejada serve |
| Pinos do PCF8574 | **SDA = GPIO16, SCL = GPIO17** em `hw_config.h`, com o LED verde e o azul desligados (-1) | Trocar para **22 e 27** (CN1). Os GPIO16 e 17 são o LED RGB da placa e não saem em nenhum conector. Conferir se mais alguma coisa usa 22 ou 27 |
| Menu de pausa | O `hw_config.h` define zonas de toque na barra (D-pad, A, B, Start/Select e **Menu**). Não li o `touch_input.cpp` nem o código que usa a zona Menu | **Menu de pausa por toque em qualquer lugar da tela**, com Tamanho, Paleta, Pular quadros e Som dentro dele. Ver "Menu de pausa por toque", logo abaixo da tabela |
| Bluetooth e Wi-Fi | `bt_controller.cpp` (Bluepad32) e `wifi_upload.cpp` | Com botões físicos não são necessários. Deixar desligáveis por opção de compilação, para liberar RAM, e medir |
| Compilação | PlatformIO (`pio run -t upload`) | Troca a Arduino IDE do LatinhaColor. O `cyd/teste_cyd/` (Adafruit) fica só como teste de hardware; o firmware usa a TFT_eSPI do CYDboy |

**Menu de pausa por toque.** O menu abre tocando em qualquer lugar da tela. Não há botão Option: tudo fica num menu só, com estes itens:

| Item | O que faz |
|------|-----------|
| Continuar | Fecha o menu e volta ao jogo |
| Salvar foto, Carregar foto | Save state (secção 5) |
| Tamanho | Normal (1:1, bandas pretas dos lados), Ajustado ou Tela cheia (secção 6.2) |
| Paleta | Cores do modo DMG (o README do CYDboy fala de 20 paletas) |
| Pular quadros | De 0 a 4 (o README do CYDboy) |
| Som | Liga ou desliga |
| Sair | Volta à lista de jogos |

**O menu abre com um toque em qualquer lugar da tela**, não numa zona só. Isso vale também na tela cheia (secção 6.2), onde não sobra barra. Dentro do menu você navega com o D-pad e o A, então o toque só abre. O risco é abrir o menu sem querer ao segurar o aparelho com o dedo na tela. Se acontecer, as saídas são exigir um toque mais longo (por exemplo meio segundo) ou deixar uma borda sem toque, e as duas são ajustes pequenos.

Para isso o plano é esconder da tela o D-pad, o A, o B, o Start e o Select quando o PCF8574 for detectado (o `button_input.cpp` já tem a variável `pcf_detected`), como o CYDboy já faz quando um controle Bluetooth conecta. É código novo no `touch_input.cpp` e na tela, que **ainda não li**, e os itens Tamanho, Paleta, Pular quadros e Som dependem de como o CYDboy guarda essas opções (também não li). O toque é resistivo, sem tato: serve para abrir o menu de vez em quando, não para Start ou Select no meio do jogo, que ficam nos botões físicos.

**O que o SMS Plus precisa:** uma ponte para o lançador do CYDboy, parecida com a que ele tem para o Game Boy (`emulator_bridge.cpp`, ainda não lido), e a função de linha do LatinhaColor (`latinha_sms_line`) trocada pela do CYDboy, sem os descartes de linhas e colunas para 160×128. O primeiro passo da fase 4 é ler esse arquivo.

Riscos, nesta ordem:

**Medido num CYD (fork em `firmware/cydboy_fw/`):** o CYDboy original, com o Bluetooth, deixava só 116 KB de RAM, e o cache da ROM ficava com 61 páginas (30 KB). No Zelda isso dava cerca de 1 070 falhas de cache por segundo e 21 FPS. Sem o Bluetooth sobram uns 118 KB depois de alocar as 160 páginas do cache (80 KB), as falhas caem a perto de zero e, com o ritmo dos quadros corrigido, o Zelda roda a **54 FPS** em média, e os jogos de GBC de 1 MB ficam perto de 40 FPS antes da correção do ritmo. O limite agora é a CPU e o envio da imagem à tela (secção 5.1 e o `README.md` do fork). A RAM para o SMS Plus parece sobrar: 118 KB livres com o jogo de GBC rodando.

1. **Memória com dois núcleos.** O GBC já usa bastante (2 bancos de VRAM de 16 KB e 8 de WRAM de 32 KB, segundo o README), e o SMS Plus soma o estado dele (cerca de 25 KB) mais o VDP. Os dois não podem ocupar RAM ao mesmo tempo: ou se aloca só o núcleo do jogo escolhido (e se libera ao sair), ou o lançador reinicia para o emulador escolhido, como na arquitetura da `PROPOSTA.md`. Só a medição no CYD diz qual cabe.
2. **Velocidade.** O CYDboy diz mirar mais de 50 fps, com Bluetooth e toque. Sem medição minha. Tirar o Bluetooth deve ajudar.
3. **Fork difícil de mexer.** Se o código do CYDboy for apertado demais para acrescentar um segundo núcleo, o plano cai na opção A: o CYDboy puro para GB e GBC, e um firmware menor só com o SMS Plus para GG e SMS. A troca passa a ser por regravação.

---

## 6. Tela cheia

A tela do CYD na horizontal é 320×240 (4:3). Os jogos têm outros formatos:

| Sistema | Tamanho original | Modo | Tamanho na tela | Dados por quadro | Limite teórico do SPI a 40 MHz |
|---------|------------------|------|-----------------|------------------|-------------------------------|
| Master System | 256×192 (4:3) | 1:1, centralizado | 256×192 | 98 KB | ~51 quadros/s |
| Master System | 256×192 (4:3) | **tela cheia, ×1,25** | **320×240** | 154 KB | ~32 quadros/s |
| Game Boy e Game Gear | 160×144 (10:9) | 1:1, centralizado | 160×144 | 46 KB | ~108 quadros/s |
| Game Boy e Game Gear | 160×144 (10:9) | ×1,5 (altura quase cheia) | 240×216 | 104 KB | ~48 quadros/s |
| Game Boy e Game Gear | 160×144 (10:9) | ×1,67 (altura cheia) | 267×240 | 128 KB | ~39 quadros/s |
| Game Boy e Game Gear | 160×144 (10:9) | esticado | 320×240 | 154 KB | ~32 quadros/s |

Os limites vêm só da largura de banda do SPI (40 MHz = cerca de 5 MB/s), sem contar o tempo do emulador nem o overhead. Não testei nada disso.

- **Master System:** 256×192 é 4:3, o mesmo formato da tela. ×1,25 enche a tela sem distorcer. O custo é a cada 4 pixels repetir 1 (e a cada 4 linhas repetir 1), e mandar 154 KB por quadro.
- **Game Boy e Game Gear:** 160×144 não é 4:3, então **tela cheia sem distorcer não existe**. Escalas inteiras (×2 = 320×288) não cabem na altura. As opções são ×1,5 (com barras pretas nas laterais e 12 pixels em cima e embaixo), ×1,67 (altura cheia) ou esticar a imagem (fica 20% mais larga que o certo). Qualquer escala não inteira repete alguns pixels e deixa a imagem um pouco irregular.
- O LatinhaColor já desenhava 1 quadro sim e 1 não no Master System. Se o SPI não dá conta do modo escolhido, a mesma ideia funciona aqui: o emulador roda a 60 Hz e a tela atualiza a ~30 Hz.

**Proposta:** um item "Tamanho" no menu de pausa: 1:1, ajustado (padrão) e tela cheia, com a escolha guardada por sistema. Padrão: Master System em ×1,25 (tela cheia sem distorção) e Game Boy e Game Gear em ×1,5. O que cabe de verdade só se descobre medindo no CYD.

### 6.1 Com o CYD em pé (vertical, 240×320)

> **Atualização: o console passa a ficar deitado (320×240).** O Master System (4:3) enche a tela deitada sem distorcer, e o Game Gear e o SMS são consoles deitados. Os jogos que não enchem a tela ficam **centralizados, com faixas pretas dos dois lados** (secção 6.3). A interface do CYDboy, desenhada em 240×320, precisa ser refeita para 320×240, e a tela de início, o Tempo e o Ônibus já nascem deitados. O texto abaixo, sobre ficar em pé, vale como histórico e como alternativa.

**Decisão anterior: o console fica em pé.** Isso vale para o resto do plano: Game Boy e Game Gear em ×1,5 (240×216), Master System em 240×180, faixa livre embaixo da imagem (104 px no Game Boy e no Game Gear, 140 px no Master System), botões físicos na parte de baixo da caixa como no Game Boy original. A opção ×1,67 da tabela da secção 6 só vale para o CYD deitado e fica de fora. A "tela cheia" volta como um modo do menu de pausa, esticada para 240×320, junto com o Normal (1:1, bandas pretas dos lados) e o Ajustado (secção 6.2).

Em pé, a tela fica 240 de largura por 320 de altura, como um Game Boy original. O Game Boy e o Game Gear são mais largos que altos (160×144), então a imagem enche a **largura**, mas não a altura:

| Sistema | Escala | Tamanho na tela | Sobra | Dados por quadro |
|---------|--------|-----------------|-------|------------------|
| Game Boy e Game Gear | ×1,5 (240 ÷ 160) | **240×216** | faixa de 104 px embaixo | 104 KB |
| Master System | ×0,9375 (240 ÷ 256, reduz 1 pixel a cada 16) | 240×180 | faixa de 140 px | 86 KB |

- **No modo Ajustado não é tela cheia:** o Game Gear ocupa toda a largura, mas só 216 dos 320 pixels de altura (cerca de 2/3).
- A imagem **não fica maior** que no modo ×1,5 na horizontal (também 240×216). Na horizontal dá para chegar a 267×240 (×1,67), mas em pé o máximo sem distorcer é 240×216.
- Em compensação o formato lembra o Game Boy, e a faixa de 104 px serve para mostrar bateria, o nome do jogo ou botões na tela. Como o firmware é nosso, dá para usar o toque nessa faixa para Start, Select e Menu, e liberar pinos do expansor.
- O Master System é um console de tela larga: em pé fica menor que na horizontal (onde enche 320×240).
- Girar a tela é só uma configuração do driver, não custa velocidade.

### 6.2 Modos de tamanho no menu de pausa (em pé)

O item **Tamanho** do menu de pausa tem três modos, com a escolha guardada por sistema (padrão: Ajustado):

| Modo | Game Boy e Game Gear (160×144) | Master System (256×192) |
|------|-------------------------------|-------------------------|
| **Normal** (1:1, bandas pretas dos lados) | 160×144 no centro, banda preta de 40 px em cada lado. 46 KB por quadro | 256 px não cabem nos 240 da tela, então sem escala o jeito é **recortar 8 px de cada lado** (240×192). A conferir se algum jogo perde algo importante nas bordas |
| **Ajustado** (padrão) | ×1,5, 240×216, sem banda dos lados. 104 KB por quadro | 240×180 (reduz 1 pixel a cada 16). 86 KB por quadro |
| **Tela cheia** | Esticado para 240×320: ×1,5 na largura e ×2,22 na altura, então a imagem fica cerca de 48% mais alta que o certo. 154 KB por quadro | Esticado para 240×320. 154 KB por quadro |

- **Imagem esticada:** a tela cheia não existe sem distorção, porque 160×144 e 240×320 têm formatos diferentes (secção 6). É um modo para quem prefere a imagem grande.
- **Velocidade:** a 40 MHz de SPI, 154 KB levam cerca de 31 ms, ou seja ~32 quadros por segundo no limite do barramento (conta minha, sem medir). Na tela cheia pode ser preciso atualizar a tela a 30 Hz, como o LatinhaColor já fazia no Master System (um quadro sim, um não). O Normal e o Ajustado sobram: ~108 e ~48 quadros por segundo no limite.
- **Sem buffer grande:** como o CYDboy já faz no Ajustado (linha a linha, em `display_push_gb_line()`), os três modos podem repetir ou duplicar linhas e colunas enquanto enviam, sem um quadro inteiro na RAM. Os fatores novos (a altura de 144 para 320 repete as linhas em grupos de 2 e 3) são código novo.
- **Menu:** na tela cheia não sobra barra de controles, e o menu abre tocando em qualquer lugar da tela (secção 5.1). O BOOT fica como alternativa.

### 6.3 Deitado (320×240): os jogos centralizados, com faixas pretas

As faixas são pretas e **não precisam ser redesenhadas a cada quadro**: só a área do jogo é enviada ao SPI. A 7,4 MB/s (medido), cada quadro desenhado custa uns 13 ms com 98 KB, uns 14 ms com 104 KB, uns 17 ms com 128 KB e uns 21 ms com 154 KB.

| Modo | Game Boy e Game Gear (160×144) | Master System (256×192) |
|------|-------------------------------|-------------------------|
| **Normal** (1:1) | 160×144 no centro, faixas de 80 px dos lados e 48 px em cima e embaixo. 46 KB | **256×192 no centro, faixas de 32 px dos lados e 24 px em cima e embaixo. 98 KB** (padrão) |
| **Ajustado** | **×1,5: 240×216, faixas de 40 px dos lados e 12 px em cima e embaixo. 104 KB** (padrão) | ×1,25 menos um pouco: 288×216, faixas de 16 px. 124 KB |
| **Tela cheia** | ×1,67: 267×240, faixas de 26 px dos lados, **sem distorcer**. 128 KB. Esticado para 320×240 distorce uns 20%. 154 KB | **×1,25: 320×240 cheio, sem distorcer.** 154 KB |

- As faixas dos lados também podem mostrar informação: bateria, nome do jogo e a hora.
- O toque em qualquer lugar abre a pausa, como já decidido.
- A rotação do painel é só o valor do `TFT_MADCTL` (secção 10 e `config_cyd.h`). O toque precisa de calibração nova.

**A escolha de em pé ou deitado define a caixa e a posição dos botões**, então vale decidir antes de projetá-la: em pé combina com Game Boy e Game Gear, deitado combina com Master System.

---

## 7. Plano

| Fase | Entrega | Critério de sucesso |
|------|---------|---------------------|
| 1 | Teste de hardware (`cyd/teste_cyd/`, já escrito) e o **CYDboy pronto** gravado no CYD | Tela, botões, SD e memória confirmados, e o CYDboy joga GB e GBC com o toque ou o Zero 2 |
| 2 | **Fork do CYDboy** neste repositório: PCF8574 em IO22/IO27, menu de pausa por toque, com Tamanho, Paleta, Pular quadros e Som dentro dele (e os controles na tela escondidos), Bluetooth e Wi-Fi desligáveis (secção 5.1) | Compila no PlatformIO e joga um jogo de GB e um de GBC com os seus botões físicos. Memória livre medida |
| 3 | **Game Boy e Game Boy Color com o Walnut-CGB**, que já vem no CYDboy: menu de pausa por toque, pastas e velocidade | GB jogável com os seus botões e quadros por segundo medidos (hoje 54 FPS no Zelda), nos três tamanhos do menu (Normal, Ajustado e Tela cheia; secção 6.2). GBC como bônus, sem meta de velocidade |
| 4 | **Game Gear e Master System**: SMS Plus acrescentado ao fork (ponte do emulador, pastas `roms/gg` e `roms/sms`, cópia da ROM para a partição, tamanhos da secção 6) | Os dois rodam, e a memória livre foi medida com os dois núcleos no firmware |
| 5 | Saves e fotos de GG e SMS em arquivos no cartão (GB e GBC já vêm prontos) | Save de bateria e foto de GG e SMS em `saves/` |
| 6 | Extras | Som de GG e SMS no GPIO26 (ver o que o CYDboy já tem de áudio) e, se ainda interessar, os apps do LatinhaColor (secção 9) |

Se o fork do CYDboy não aguentar o segundo núcleo (risco 3 da secção 5.1), as fases 2 e 3 valem do mesmo jeito e a 4 vira um firmware separado só com GG e SMS.

O NES fica de fora por enquanto: o Anemoia-ESP32 é GPLv3 e o SMS Plus é GPL v2, e misturar os dois pode dar problema de licença. Se quiser NES, o caminho é o firmware do Anemoia separado, como descrito no `BOTOES.md`.

---

## 8. Licenças

O repositório é público, e os arquivos que você enviou têm licenças diferentes. Não sou advogado; isto é leitura dos cabeçalhos.

| Código | O que o cabeçalho diz |
|--------|-----------------------|
| **Peanut-GB** (Mahyar Koshkouei) | MIT. Pode ficar no repositório, com o aviso de copyright. |
| **CYDboy** (Rocka84) | MIT, segundo o README. Não li o arquivo `LICENSE`: conferir antes de copiar o código. |
| **Walnut-CGB** | MIT, segundo o README do CYDboy. **Não vi o cabeçalho do `walnut_cgb.h`**: confirmar o texto da licença e os autores antes de colocar o código no repositório. |
| **Bluepad32** (usado pelo CYDboy) | Não li a licença. Se o Bluetooth ficar no firmware, conferir os termos dele e da pilha Bluetooth que ele usa. |
| **SMS Plus** (Charles Mac Donald), por exemplo `system.c` | GPL v2 "ou qualquer versão posterior". |
| **Z80** (`z80.c`, Juergen Buchmueller) | "Freeware para fins **não comerciais**". Pede crédito ao autor, um aviso em cada arquivo modificado e contato para uso comercial, e reserva o direito de mudar os termos a qualquer momento, inclusive retroativamente. **Isso não é GPL.** |
| `cpuintrf.h` e `osd_cpu.h` | Vêm do MAME, que antes tinha uma licença parecida (não comercial). Esses dois arquivos não têm cabeçalho de licença no que você enviou. |

Consequências:

1. **Uso pessoal** no seu console: sem problema.
2. **Este repositório público não vai receber os arquivos do SMS Plus, do Z80 nem do MAME.** Eles ficam numa pasta que o git ignora (`third_party/`), e o projeto do CYD os pega da sua cópia do LatinhaColor (um script copia). O repositório terá só código novo, o fork do CYDboy e o Walnut-CGB (MIT, com os avisos de copyright deles) e a documentação.
3. **Vender ou distribuir** o console com esse firmware seria uso comercial e redistribuição. Aí é preciso resolver o Z80: pedir autorização ao autor ou trocar o núcleo por um livre.

---

## 9. Os outros apps do LatinhaColor (Jogos, Bichinho, Internet)

Também recebi `app_jogos.h`, `app_console.h` (Snake, Flappy, Dino), `app_gw.h` (Game & Watch: Paraquedas, Fogo, Ovos), `app_bichinho.h` e `app_internet.h`. Eles não são o foco do porte, mas mostram uma coisa útil.

**Todos desenham numa tela lógica de 160×128**, com coordenadas fixas no código (`SW` = 160, `SH` = 128). Como 160 × 1,5 = **240**, essa tela lógica ampliada ×1,5 cabe exatamente na largura do CYD em pé: **240×192**, sobrando uma faixa de 128 px embaixo. Daria para rodar esses apps **sem mudar as coordenadas**, só ampliando a imagem no final.

Isso sugere dois caminhos de desenho no firmware do CYD:

| Caminho | Quando | Como |
|---------|--------|------|
| **Tela lógica 160×128, ampliada ×1,5** | Menus, Snake, Flappy, Dino, Game & Watch, Bichinho, Internet | Mesmo código dos apps. Buffer de 16 bits de 40 KB, empurrado à tela em 240×192 |
| **Direto, resolução própria** | Game Boy, Game Gear e Master System | As linhas do emulador vão à tela em 240×216 (GB e GG) ou 240×180 (SMS), sem buffer completo |

**Para portar esses apps** eu precisaria do arquivo do firmware que define o que eles usam: `gfx` (e `getBuffer()`), `flush()`, `txt`/`txtC`/`txtR`, `statusBar`, `hint`, `bar`, `showPopup`, `rgb()`, as cores `C_*`, `keys[]`/`Key`/`BtnEvt`/`EV_CLICK`/`EV_DOWN`/`isPress`, `prefs`, `wifiOk()`, `clockText()` e `timeValid()`. Se não quiser portá-los, não preciso desse arquivo: para o Retro eu escrevo o menu novo.

Observações:
- O Bichinho e o monitor de Internet dependem de Wi-Fi e de hora (NTP). O Retro pausa a rede (`netPause`) para liberar RAM, e no CYD seria igual.
- Os recordes usam `Preferences`, que existe no ESP32 e funciona igual no CYD.
- Os apps que tratam as teclas `KEY_A`, `KEY_B`, `KEY_OK`, `KEY_BACK` e as direções cabem no mesmo mapa de botões da seção 3.


---

## 10. O hardware do LatinhaColor e o que isso muda no plano

Pelo `common.h`, `config.h` e `LatinhaColor.ino`, o aparelho anterior é: **ESP32 DevKit V1** (sem PSRAM, como o CYD), tela **ST7735 de 1,8" (160×128)**, **ADKeyboard** (5 botões num pino só, por resistores) e **joystick analógico**, compilado na **Arduino IDE** com o núcleo ESP32 3.x e as bibliotecas Adafruit GFX, Adafruit ST7735 e ArduinoJson. O `secrets.h` (Wi-Fi e token do Telegram) não foi enviado e não é necessário: nunca o envie.

Consequências:

1. **O porte fica menor do que eu pensava.** Dá para manter o framework (`common.h`, `.ino`, os 7 apps) e trocar só a tela, as teclas e o envio de imagem. A tela passa de `Adafruit_ST7735` para `Adafruit_ILI9341`, e o `flush()` amplia a tela lógica de 160×128 para 240×192.
2. **Fica a pilha da Adafruit**, em vez do LovyanGFX que eu tinha proposto. Menos mudança, e você já a conhece.
3. **O joystick analógico não vem.** Ele usa dois pinos analógicos (`JOY_V_PIN` e `JOY_H_PIN`), e o CYD só tem o IO35 livre. O D-pad passa a ser o módulo 5D digital (pelo PCF8574).
4. **O ADKeyboard pode vir**, no IO35: ele dá OK, VOLTAR, A, B e MENU com um fio só, e dispensa o hack do botão BOOT. Os limites de tensão ficam em `config_cyd.h` e devem ser recalibrados.
5. **A memória é a mesma** do aparelho anterior, e o app já era feito para esse limite (por isso o `netPause` no Retro).

**Atualização (opção B):** os itens 1 e 2 deixam de valer para o firmware final. Ele parte do CYDboy, que usa TFT_eSPI e PlatformIO, e não do framework do LatinhaColor com a pilha da Adafruit. Os 7 apps (secção 9) ficam fora do caminho principal, e portá-los exigiria uma camada `gfx` sobre a TFT_eSPI. Os itens 3 a 5 continuam valendo.

### Fase 1 já escrita: `cyd/teste_cyd/`

Um sketch de teste de hardware (tela ×1,5 em pé, as 9 teclas, PCF8574 com detecção de endereço, ADKeyboard, cartão SD e memória). Instruções em `cyd/teste_cyd/LEIAME.md`. Também lê o toque (XPT2046, por software) e mostra uma cruz que segue o dedo. Já foi **compilado para o ESP32 e gravado num CYD** (arduino-cli, núcleo ESP32 3.3.11), e ligou: pela serial, o envio de um quadro de 240×192 levou **27,4 ms** (a conta teórica da secção 6 dava cerca de 18 ms) e sobraram **244 704 bytes** de RAM. Depois foram conferidos no CYD: a tela (com o `TFT_MADCTL` 0x20, que corrigiu a orientação e as cores), o toque, o PCF8574 em 0x20 (uma ligação errada do cabo foi achada pelo diagnóstico de I²C do próprio sketch) e o cartão SD. A medida de 27,4 ms é com a biblioteca Adafruit, escrevendo linha a linha, e não vale necessariamente para o CYDboy (TFT_eSPI).

---

## 11. Apps e controles (decisão atual)

**Apps: Retro, Tempo, Ônibus (vêm do LatinhaColor) e Agenda (nova).** O Bicho, os Jogos, as Mensagens (Telegram) e o monitor de Net saem do plano. A tela de início tem quatro blocos (2×2), tudo deitado em 320×240. Cada bloco tem o ícone à esquerda, um texto grande ao lado e uma linha de detalhe embaixo, que desfila quando não cabe:

| Bloco | Texto grande | Embaixo |
|-------|--------------|---------|
| Retro | `Retro` | `GB  GG  SMS` |
| Tempo | temperatura (o ícone muda com o clima) | `Poucas nuvens` |
| Ônibus | a linha que sai primeiro (`Metro` ou `S.All`) | `em 10 min` |
| Agenda | data do próximo evento (`10 Nov`) | `14:30 Fisio` |

**Agenda:** mostra os eventos do Google Calendar pelo "endereço secreto no formato iCal" (o mesmo do projeto Meteo, com eventos repetidos por RRULE). O endereço fica no `secrets.h`.

**Protetor de tela e sono:** depois de 5 min sem usar (nos menus e apps, nunca nos jogos) aparecem uns olhos animados; aos 10 min a placa entra em deep sleep e **acorda com um toque na tela** (o pino de interrupção do toque, IO36). Tocar em "Latinha" na tela inicial liga os olhos na hora e eles ficam até alguém tocar, sem dormir.

**Game Gear e Master System com "Pular quadros":** o VDP só marca a colisão entre sprites (bit 0x20 do status, que o Fantasy Zone usa para acertar os tiros) quando a linha é desenhada. Com quadros pulados a flag nunca subia e o tiro atravessava o inimigo. Desenhar todos os quadros resolvia, mas custava metade da velocidade (33 FPS), então os quadros pulados agora só fazem a conta da colisão (`render_obj_collide`, em `smsplus/cyd_collide.c`), sem desenhar. Medido no Fantasy Zone: 50 a 61 FPS, e os tiros acertam.

**Enviar jogos pelo Wi-Fi:** o botão **ENVIAR** da lista do Retro (e a linha "Enviar jogos pelo Wi-Fi" nas Opções) liga o Wi-Fi e mostra um endereço (`http://192.168.x.x`). No navegador do PC ou do celular, na mesma rede, a página deixa escolher as ROMs (`.gb`, `.gbc`, `.gg`, `.sms`); cada uma vai para a pasta certa do cartão SD (`/roms/gb`, `/roms/gbc`, `/roms/gg`, `/roms/sms`). A página também lista e apaga jogos. **Não tem senha:** só use na rede de casa. A calibração do toque fica na tela inicial (SELECT, ou toque no rodapé).

| Ação | Toque | Botões físicos |
|------|-------|----------------|
| Abrir um app ou um jogo | Toca no bloco ou na linha | D-pad escolhe, **A** abre |
| Voltar uma tela | Toca no `‹` do canto de cima | **B** |
| Ir ao início, fora dos jogos | Toca no título | **Start** |
| Rolar uma lista (jogos, linhas de ônibus) | Setas ▲ ▼ na tela, ou toca na linha | D-pad |
| Tempo: trocar de aba (Agora, Horas, 7 dias) | Toca na aba | ◀ ▶ |
| Ônibus: ver as próximas saídas | Toca na linha | A |
| Dentro de um jogo: abrir a pausa | Toque em **qualquer lugar** da tela | Start e Select juntos |

O toque é resistivo e não é bom para arrastar o dedo, por isso as listas rolam por setas e não por deslize. Os alvos têm cerca de 40 px de altura. Dentro do jogo, Start e Select são botões do jogo, então o início só vale fora dele.

**Menu de pausa, com 5 itens:** Continuar, Salvar foto, Carregar foto, Opções e Sair. Em Opções ficam o Tamanho (Normal, Ajustado, Cheia, Esticada), a Paleta, o Pular quadros e o Brilho. Os textos não têm acento, porque a fonte da tela não os tem.

**O que o Tempo e o Ônibus precisam:**
- **Hora certa:** Wi-Fi e NTP. O Ônibus usa só a hora, porque as tabelas de horário ficam no código. O Tempo busca a previsão do Rio no Open-Meteo, sem chave de acesso.
- **Dados do Wi-Fi e da agenda:** num `secrets.h` local, **fora do git** (o modelo é o `secrets.example.h`).
- **Partição maior:** com o Wi-Fi e a conexão segura o programa passa de 1,3 MB, então o firmware é compilado com o esquema "No OTA" (2 MB para o programa, 2 MB para os dados): `PartitionScheme=no_ota` no fqbn. As ROMs de Game Gear e Master System, copiadas para a flash, são recopiadas do cartão na primeira vez.
- **RAM:** a conexão segura pede uns 40 KB livres. Fora dos jogos sobram uns 215 KB, e durante um jogo de SMS uns 180 KB.
