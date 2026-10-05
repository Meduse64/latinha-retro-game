# Porte do app Retro (LatinhaColor) para o CYD

Este documento parte do arquivo do app Retro do firmware anterior (LatinhaColor), que você colou na conversa. Eu só vi **esse arquivo**: não tenho o `gb_core.c`, o `src/smsplus`, o restante do firmware (tela, botões, menu) nem o script `gravar_jogos.ps1`. Tudo abaixo é leitura do código colado e **não foi compilado nem testado**.

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
| Botões | Joystick 5D + SW1–SW4 + amarelo | PCF8574 no CN1 + botão BOOT (secção 3) |
| Rede | Telegram (`tgIdle`, `netPause`, tarefa no núcleo 0) | Some: sem Wi-Fi, sobra mais RAM |
| ROMs | Partição `spiffs` de cerca de 2 MB (esquema "No OTA" de 4 MB, a confirmar), gravada pelo `gravar_jogos.ps1` | **microSD** (secção 5) |
| Som | Nenhum | Alto-falante no GPIO26, opcional |

---

## 3. Botões: dá para usar o que você já tem

O app usa 9 teclas: cima, baixo, esquerda, direita, `KEY_OK`, `KEY_BACK`, `KEY_A`, `KEY_B` e `KEY_HOME`. O PCF8574 tem 8 pinos, e o amarelo (`KEY_HOME`) vai para o botão BOOT do CYD (GPIO0, já na placa). **Não precisa comprar o PCF8575.**

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
| GPIO0 (BOOT) | `KEY_HOME` | botão da placa | menu de pausa | menu de pausa |

Ligações do PCF8574 no CN1: SDA no IO22, SCL no IO27, VCC no 3V3, GND no GND. O COM do módulo 5D e o outro lado de cada botão vão no GND. O endereço depende dos jumpers A0/A1/A2. O sketch de teste está na secção 5.3 do `BOTOES.md`.

Dois detalhes:
- O GPIO0 é um pino de boot: o botão precisa estar solto na hora de ligar ou de resetar. Depois que o programa roda, pode ser usado como entrada normal.
- O MID (centro) do módulo 5D fica sem uso.

---

## 4. O que eu preciso de você

Só tenho o arquivo do app. Para montar o projeto do CYD preciso dos outros arquivos do firmware anterior:

1. `gb_core.c` e o `peanut_gb.h`, com os wrappers `gbw_*`.
2. A pasta `src/smsplus/` inteira, incluindo as alterações que você fez (`latinha_sms_line`, `latinha_sms_piece`, `latinha_alloc_sram`, `sms_frame`, `emu_system_init`).
3. A parte do firmware que define o que o app usa: `gfx` (e `getBuffer()`), `flush()`, `SW`/`SH`, `rgb()`, as cores `C_*`, `txt`/`txtC`/`txtR`/`statusBar`/`hint`, `keys[]`, `Key`, `BtnEvt`, `isPress`, `appOwnsScreen`.
4. O esquema de partições (arquivo CSV ou a opção da placa) e o `gravar_jogos.ps1`.
5. O `platformio.ini` ou a configuração do Arduino (placa, bibliotecas e versões).

**Como enviar:** copie tudo para uma pasta deste repositório (por exemplo `latinha-color/`) e dê push na branch `claude/retro-game-proposal-07kbyg`. **Sem as ROMs.** Deixei um `.gitignore` que bloqueia `roms/` e as extensões `.gb`, `.gbc`, `.gg`, `.sms`, `.nes`, `.sfc`, `.smc` e `.gba`, porque o repositório é público. Se o firmware anterior estiver em outro repositório do GitHub, ele não está liberado para esta sessão: dá para liberar nas configurações do app do Claude no GitHub.

---

## 5. ROMs no cartão SD

Hoje o app lê as ROMs de uma partição da flash, mapeada como memória. Para usar o microSD é preciso mudar isso, e a razão é o SMS Plus: ele lê a ROM como memória (`cart.rom`), então não dá para ler do cartão aos poucos como o CYDboy faz com o Game Boy.

| Sistema | Como ler do cartão |
|---------|--------------------|
| Game Gear e Master System | **Copiar a ROM do cartão para a partição da flash** ao escolher o jogo e mapear de lá, como hoje. Essas ROMs são pequenas (em geral de dezenas a algumas centenas de KB), então a cópia deve levar poucos segundos (estimativa minha). Se o jogo escolhido já é o que está na partição, pula a cópia |
| Game Boy | Mesma cópia no começo (reaproveita todo o código). Depois, se quiser jogos maiores que a partição, o Peanut-GB lê por função de callback, e dá para ler direto do cartão |

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
- **Saves e fotos passam a ser arquivos** em `saves/` (por exemplo `jogo.sav` e `jogo.state`). Gasta menos a flash e dá para fazer backup copiando o cartão. Isso troca o código dos `BAT3` e `STA3` que gravam na partição.
- A flash passa a servir só para o jogo que está rodando. O tamanho da partição define o maior jogo que cabe (a definir quando eu vir o tamanho do firmware).
- **Game Boy Color:** o Peanut-GB é só do Game Boy original (DMG). Jogos que são só de GBC não vão rodar. O CYDboy usa o Walnut-CGB, um derivado com suporte a GBC (MIT), que pode entrar no app depois.
- O NES e as ROMs do Anemoia ficam na raiz do cartão, como ele pede, e não atrapalham.

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

Em pé, a tela fica 240 de largura por 320 de altura, como um Game Boy original. O Game Boy e o Game Gear são mais largos que altos (160×144), então a imagem enche a **largura**, mas não a altura:

| Sistema | Escala | Tamanho na tela | Sobra | Dados por quadro |
|---------|--------|-----------------|-------|------------------|
| Game Boy e Game Gear | ×1,5 (240 ÷ 160) | **240×216** | faixa de 104 px embaixo | 104 KB |
| Master System | ×0,9375 (240 ÷ 256, reduz 1 pixel a cada 16) | 240×180 | faixa de 140 px | 86 KB |

- **Não é tela cheia:** o Game Gear ocupa toda a largura, mas só 216 dos 320 pixels de altura (cerca de 2/3).
- A imagem **não fica maior** que no modo ×1,5 na horizontal (também 240×216). Na horizontal dá para chegar a 267×240 (×1,67), mas em pé o máximo sem distorcer é 240×216.
- Em compensação o formato lembra o Game Boy, e a faixa de 104 px serve para mostrar bateria, o nome do jogo ou botões na tela. Como o firmware é nosso, dá para usar o toque nessa faixa para Start, Select e Menu, e liberar pinos do expansor.
- O Master System é um console de tela larga: em pé fica menor que na horizontal (onde enche 320×240).
- Girar a tela é só uma configuração do driver, não custa velocidade.

**A escolha de em pé ou deitado define a caixa e a posição dos botões**, então vale decidir antes de projetá-la: em pé combina com Game Boy e Game Gear, deitado combina com Master System.

---

## 7. Plano

| Fase | Entrega | Critério de sucesso |
|------|---------|---------------------|
| 1 | Esqueleto para o CYD: tela ILI9341, leitura dos botões (PCF8574 + BOOT), sem emuladores | Uma tela de teste mostra a tecla apertada |
| 2 | microSD: lista de jogos lida das pastas `roms/` e cópia da ROM para a partição | A lista mostra os jogos do cartão e a ROM escolhida aparece na partição |
| 3 | Game Boy | Um jogo roda com os botões, em velocidade correta |
| 4 | Game Gear e Master System | Os dois rodam, com os modos de tamanho da secção 6 |
| 5 | Saves e fotos em arquivos no cartão | Save de bateria e foto funcionando como no LatinhaColor |
| 6 | Extras | Som no GPIO26, Game Boy Color com o Walnut-CGB |

O NES fica de fora por enquanto: o Anemoia-ESP32 é GPLv3 e o SMS Plus é GPL v2, e misturar os dois pode dar problema de licença. Se quiser NES, o caminho é o firmware do Anemoia separado, como descrito no `BOTOES.md`.

---

## 8. Licenças

O repositório é público. O SMS Plus é **GPL v2**: o firmware que o inclui, se for distribuído ou publicado, precisa seguir a GPL v2 (e manter os avisos de copyright). O Peanut-GB é MIT. Quando o código entrar aqui, adiciono um `LICENSE` e os avisos no lugar certo.
