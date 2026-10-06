# CYDboy sem Bluetooth (fork de trabalho)

Código base: [Rocka84/CYDboy](https://github.com/Rocka84/CYDboy), licença MIT (o aviso está em `LICENSE-CYDboy`). Esta pasta é o ponto de partida da fase 2 do plano (`PORTE_CYD.md`, secção 5.1).

## O que mudou em relação ao original

| Mudança | Para quê |
|---------|----------|
| `bt_controller.cpp` (Bluepad32) trocado por `src/bt_controller_stub.cpp`, que não faz nada | Libera cerca de 120 KB de RAM. No original sobravam 116 KB depois do boot e o cache da ROM ficava com 61 páginas. Aqui ele fica com as 160 páginas previstas |
| `-DENABLE_SOUND=0` em `build_opt.h` | Sem som (o alto-falante ainda não está ligado). Corta o código de áudio |
| `src/display.cpp`: quatro funções de compatibilidade do LEDC | O backlight usa a API antiga do LEDC, que saiu do Arduino-ESP32 3.x |
| `wifi_upload.cpp` fora do projeto | O `platformio.ini` original já o excluía |
| Compila com o `arduino-cli` e o núcleo `esp32:esp32` 3.3.11, em vez de PlatformIO | Não precisa de Python nem do pacote Bluepad32 |

Os pinos dos botões físicos **ainda não foram mudados**: o `hw_config.h` continua com o PCF8574 nos GPIO16 e 17.

## Compilar e gravar

As flags do `platformio.ini` original estão em `build_opt.h` (o núcleo ESP32 as lê sozinho). São necessárias as bibliotecas **TFT_eSPI** (2.5.43) e **XPT2046_Touchscreen**, e a placa `esp32:esp32:esp32` com modo de flash QIO a 80 MHz.

```bash
arduino-cli compile --fqbn "esp32:esp32:esp32:FlashMode=qio,FlashFreq=80" --build-path build firmware/cydboy_fw
arduino-cli upload -p COM6 --fqbn "esp32:esp32:esp32:FlashMode=qio,FlashFreq=80,UploadSpeed=230400" --input-dir build firmware/cydboy_fw
```

Isso apaga o firmware que estiver na placa. O CYDboy original volta a ser gravado pelo gravador web dele.

## Medidas num CYD (primeira versão, sem Bluetooth e sem som)

O CYDboy imprime uma linha `[PERF]` por segundo na serial (115200). Medido com frame skip 1.

| Jogo | Quadros por segundo | Emulação por quadro | Falhas de cache por segundo |
|------|--------------------|---------------------|-----------------------------|
| Super Mario Bros. Deluxe (GBC, 1 MB) | 41,7 em média | 20 ms | de 0 a 52 |
| Boulder Dash (GB, 64 KB) | 37,7 em média | 23,9 ms | perto de 0 |
| Prince of Persia (GBC, 1 MB) | cerca de 40 | 22 ms | perto de 0 |

Para comparar, no CYDboy original o Zelda (GB, 512 KB) teve uma sessão a **21 a 22 quadros por segundo**, com cerca de **1 070 falhas de cache por segundo** e 47 ms de emulação por quadro. Em outra sessão, com o cache aquecido e quase nenhuma falha, chegou a 47 a 48. O tempo de emulação inclui o desenho da tela.

Nesta versão o cartão deixou de ser o gargalo. O que limita agora é a CPU e o envio da imagem à tela.

### Correção do ritmo dos quadros

Sem áudio, o original esperava 16,742 ms desde o início do quadro anterior. Com frame skip 1, os quadros alternam entre um barato (sem desenho, perto de 10 ms) e um caro (com desenho, perto de 24 ms). O barato era esticado até 16,7 ms e o tempo economizado era jogado fora. Agora o prazo avança 16,742 ms por quadro, e um quadro barato deixa crédito para o próximo (`emu_run_frame()` em `emulator_bridge.cpp`).

| Zelda (GB, 512 KB), frame skip 1 | FPS | Emulação por quadro | Falhas de cache por segundo |
|----------------------------------|-----|---------------------|-----------------------------|
| Original, sessão com o cache pequeno (62 páginas) | 21 a 22 | 47 ms | cerca de 1 070 |
| Original, cache aquecido | 47 a 48 | 16 ms | perto de 0 |
| Sem Bluetooth, ritmo original | 43,6 em média | 17,5 ms | 1,1 |
| Sem Bluetooth, **ritmo corrigido** | **54,4 em média (47 a 56)** | 17,8 ms | 0,1 |

### Peanut-GB no lugar do Walnut-CGB (jogos de Game Boy)

O Peanut-GB (MIT, só Game Boy clássico) é o núcleo que o LatinhaColor usava. A chave `USE_PEANUT` em `build_opt.h` escolhe entre os dois. Com `USE_PEANUT=1` a lista esconde os jogos `.gbc`. Medido no mesmo Zelda, deitado, frame skip 1, 75 s:

| Núcleo | FPS médio | Emulação por quadro | Falhas de cache por segundo |
|--------|-----------|---------------------|-----------------------------|
| Walnut-CGB (dualfetch, IRAM) | 54,4 | 17,8 ms | 0,1 |
| **Peanut-GB (IRAM)** | **58,5** (34 a 60) | **14,8 ms** | 2,4 |

O Peanut-GB também deixa mais RAM: a estrutura do emulador é bem menor, e a RAM livre durante o jogo foi de 118 KB para 136 KB. Por isso é o padrão neste fork. O GBC volta se `USE_PEANUT` for 0.

### Deitado e o tamanho da imagem

`LANDSCAPE=1` (padrão) põe a tela em 320×240 e centraliza o jogo, com faixas pretas. O item **Tamanho** do menu de opções escolhe entre Normal 1:1 (160×144), Ajustado ×1,5 (240×216), Cheia ×1,67 (267×240, sem distorcer) e Esticada (320×240). A escolha fica gravada. A calibração do toque usa outro espaço do NVS (`touch_v2l`), então a primeira gravação deitada pede uma calibração nova.

### Game Gear e Master System (SMS Plus)

O SMS Plus vem do LatinhaColor (`scripts/copiar_smsplus.ps1`, GPL v2; o Z80 é "só uso não comercial"). A pasta `src/smsplus/` é ignorada pelo git, e sem ela o firmware compila só com o Game Boy (`HAS_SMS=0`).

- A ROM de `roms/gg` e `roms/sms` é **copiada do cartão para a partição de dados da flash** ao escolher o jogo, com uma barra de progresso, e mapeada na memória, porque o SMS Plus lê a ROM como um bloco. Escolhendo o mesmo jogo de novo, a cópia é pulada (o cabeçalho guarda o caminho, o tamanho e a data).
- Botões: B é o botão 1, A é o botão 2, Start é Start (GG) ou Pause (SMS). Sem som.
- **Os arquivos do SMS Plus precisam ser compilados com `-Os`**, e não com o `-O3` do resto do firmware. Medido no Columns (Game Gear):

| Compilação do SMS Plus | FPS | Tempo por quadro |
|------------------------|-----|------------------|
| `-O3` (igual ao resto) | 4,9 | 237 ms |
| **`-Os`** | **60** | 16 ms (10 ms é desenho) |

  O script de cópia põe `#pragma GCC optimize ("Os")` no topo de `z80.c`, `sms.c`, `render.c`, `vdp.c`, `system.c`, `sn76496.c` e `latinha_state.c`. Provavelmente o `-O3` aumenta o código a ponto de não caber no cache de instruções (32 KB) da ESP32, mas eu não medi o tamanho antes.

### O que limita agora

- A emulação leva uns 10,6 ms por quadro e o desenho uns 14 ms por quadro desenhado (104 KB no SPI). Com frame skip 1 só metade dos quadros é desenhada, então a imagem se atualiza a uns 27 Hz mesmo com a emulação a 54 FPS.
- Para desenhar todos os quadros a 60 Hz sem parar a emulação, o desenho teria de ir para o outro núcleo (livre sem o Bluetooth) ou para DMA. O tamanho 1:1 (46 KB por quadro) também cabe no prazo.
