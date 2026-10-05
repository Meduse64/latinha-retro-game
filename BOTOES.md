# Botões físicos no CYD

Guia para ligar botões ao CYD (ESP32-2432S028R) de modo a funcionarem com o **Retro-Go** (fork `CYD` de DynaMight1124), que lê os botões de um expansor I²C.

Os pinos e o mapa de botões abaixo vêm do ficheiro `components/retro-go/targets/cyd/config.h` desse fork. O resto (chips, módulos, truques de ligação) é conhecimento geral meu e **não foi testado neste hardware**.

---

## 1. Porque é preciso um expansor

O CYD só tem 3 GPIOs livres (IO35, IO22, IO27) e o Retro-Go precisa de 10 botões. A solução é um expansor I²C que usa só 2 fios (IO22 e IO27) e dá 16 entradas.

O Retro-Go para CYD está configurado para o **PCF8575** (16 pinos), no endereço **0x20**:

```c
#define RG_I2C_GPIO_DRIVER  4      // 1 = AW9523, 2 = PCF9539, 3 = MCP23017, 4 = PCF8575
#define RG_I2C_GPIO_ADDR    0x20
#define RG_GPIO_I2C_SDA     GPIO_NUM_22
#define RG_GPIO_I2C_SCL     GPIO_NUM_27
```

O código também conhece outros expansores (AW9523, PCF9539, MCP23017), mas para os usar tinhas de alterar o `config.h` e compilar o firmware tu. **Compra o PCF8575 e não precisas de compilar nada.**

Cuidado: o **PCF8574** (8 pinos, mais comum e barato) **não serve**, só dá 8 botões e o Retro-Go não está configurado para ele.

---

## 2. O que comprar

| Peça | Quantidade | Nota |
|------|-----------|------|
| Módulo **PCF8575** (16 bits, I²C) | 1 | Tem de dizer PCF8575. Verifica se os pinos de endereço A0, A1, A2 estão acessíveis |
| Módulo joystick de navegação 5D (COM, UP, DWN, LFT, RHT, MID, SET, RST) | 1 | **Já tens** |
| Botões de pressão (tactile 6×6 mm) | 10 | **Já tens** (kit de 10). Só vão ser usados 3 |
| Fio fino (28–30 AWG) | alguns metros | Cores diferentes ajudam |
| Placa perfurada ou a caixa impressa em 3D | opcional | Para fixar os botões |
| Termorretrátil, ferro de soldar, estanho | — | — |
| Multímetro | 1 | Para identificar os fios do cabo do CYD |

---

## 3. Mapa de botões (do `config.h`)

Cada botão liga entre **o pino do expansor** e **GND**. Pelo que entendo do `config.h` (`.level = 0`, `.pullup = 0`), o Retro-Go trata o nível baixo como "premido" e não liga pull-ups por software, por isso ligar ao GND é o esquema certo.

| Botão no Retro-Go | Bit no código | Pino do PCF8575 | Zero 2 equivalente |
|-------------------|--------------|-----------------|--------------------|
| LEFT (esquerda) | 0 | P00 | D-pad esquerda |
| RIGHT (direita) | 1 | P01 | D-pad direita |
| UP (cima) | 2 | P02 | D-pad cima |
| DOWN (baixo) | 3 | P03 | D-pad baixo |
| A | 4 | P04 | A |
| B | 5 | P05 | B |
| SELECT | 6 | P06 | Select |
| START | 7 | P07 | Start |
| MENU | 8 | P10 | (menu do Retro-Go) |
| OPTION | 9 | P11 | (opções do Retro-Go) |

A correspondência entre bit e pino é uma dedução minha: o código monta os botões como `(porta1 << 8) | porta0`, por isso os bits 0–7 são a porta 0 (P00–P07) e os bits 8–9 são P10 e P11. **Confirma os nomes impressos no teu módulo**, porque alguns chamam-lhes P0–P15 em vez de P00–P17.

Mínimo útil: os 4 do D-pad, A, B, Select, Start e **MENU** (para abrir o menu do jogo, guardar e sair). O OPTION é dispensável no início.

### 3.1 Com as peças que já tens

O módulo 5D tem 8 pinos: **COM** (comum) e 7 entradas (UP, DWN, LFT, RHT, MID, SET, RST). Chega para quase tudo, e só faltam 3 botões do kit (A, B e MENU):

| Retro-Go | Pino do PCF8575 | Peça |
|----------|-----------------|------|
| LEFT | P00 | módulo 5D, pino **LFT** |
| RIGHT | P01 | módulo 5D, pino **RHT** |
| UP | P02 | módulo 5D, pino **UP** |
| DOWN | P03 | módulo 5D, pino **DWN** |
| A | P04 | botão do kit nº 1 |
| B | P05 | botão do kit nº 2 |
| SELECT | P06 | módulo 5D, pino **SET** (botão pequeno do módulo) |
| START | P07 | módulo 5D, pino **RST** (botão pequeno do módulo) |
| MENU | P10 | botão do kit nº 3 |
| OPTION | P11 | módulo 5D, pino **MID** (carregar no centro) |

- O **COM** do módulo 5D vai ao **GND**. Cada botão do kit tem um terminal no pino P e o outro no GND.
- O "RST" do módulo é só o nome impresso de um botão. Não está ligado ao reset do CYD nem do ESP32.
- O MID ficou no OPTION de propósito: nestes joysticks de 5 vias é fácil carregar no centro sem querer ao empurrar uma direção, e o OPTION atrapalha menos do que o MENU.
- **Confirma com o multímetro** que SET e RST partilham o COM com as direções: modo continuidade entre COM e cada pino, com o botão correspondente premido. Não consegui verificar isto pela foto do anúncio.
- Os pinos do módulo são de 2,54 mm, por isso dá para testar tudo sem soldar, com os fios dupont (e os botões do kit numa breadboard).

---

## 4. Ligações

### 4.1 Do módulo ao CYD (conector CN1)

| PCF8575 | CYD (CN1) |
|---------|-----------|
| VCC | 3V3 |
| GND | GND |
| SDA | IO22 |
| SCL | IO27 |
| A0, A1, A2 | GND (dá o endereço 0x20) |
| INT | não ligar |

O CN1 é o conector de 4 pinos (JST 1,25 mm) que o CYD tem para expansão. Os pinos são **GND, IO22, IO27 e 3V3**, mas **a ordem no conector tem de ser confirmada lendo as letras impressas na placa**. As cores do cabo da foto (preto, amarelo, vermelho, azul) não seguem um padrão garantido: usa o multímetro em modo de continuidade entre cada fio e o pino GND do CYD, e entre o fio da 3V3 e a saída 3V3 de outro conector, antes de ligar.

Não ligues mais nada ao IO22: é partilhado com o conector P3.

### 4.2 Dos botões ao módulo

```
          PCF8575
        ┌─────────┐
 3V3 ───┤VCC   P00├──[botão LEFT ]──┐
 GND ───┤GND   P01├──[botão RIGHT]──┤
 IO22 ──┤SDA   P02├──[botão UP   ]──┤
 IO27 ──┤SCL   P03├──[botão DOWN ]──┤
 GND ───┤A0    P04├──[botão A    ]──┤
 GND ───┤A1    P05├──[botão B    ]──┤
 GND ───┤A2    P06├──[botão SELECT]─┤
        │      P07├──[botão START ]─┤
        │      P10├──[botão MENU  ]─┤
        │      P11├──[botão OPTION]─┤
        └─────────┘                 │
                                   GND (comum a todos)
```

Cada botão tem um lado no pino P e o outro lado no GND comum.

---

## 5. Antes de gravar o Retro-Go: testar

Estes testes evitam procurar erros no emulador quando o problema é uma soldadura. Os dois sketches são curtos e simples, mas **não os compilei nem testei** neste ambiente.

### 5.1 O expansor responde?

Com um sketch Arduino (placa `ESP32 Dev Module`) que percorra os endereços I²C nos pinos certos:

```cpp
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(22, 27);                      // SDA = IO22, SCL = IO27
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("Encontrado: 0x%02X\n", a);
  }
}
void loop() {}
```

Tem de aparecer **0x20**. Se não aparecer nada, verifica a soldadura, A0/A1/A2 a GND e os fios de SDA/SCL. Se o módulo não trouxer resistências de pull-up nas linhas SDA e SCL, acrescenta uma de 4,7 kΩ a 10 kΩ de cada linha para 3V3.

### 5.2 Os botões chegam ao ESP32?

```cpp
#include <Wire.h>

const char* NOMES[10] = {"LEFT","RIGHT","UP","DOWN","A","B","SELECT","START","MENU","OPTION"};

void setup() {
  Serial.begin(115200);
  Wire.begin(22, 27);
  Wire.beginTransmission(0x20);            // põe todos os pinos como entrada (nível alto)
  Wire.write(0xFF); Wire.write(0xFF);
  Wire.endTransmission();
}

void loop() {
  Wire.requestFrom((uint8_t)0x20, (uint8_t)2);
  uint8_t lo = Wire.read(), hi = Wire.read();
  uint16_t premidos = ~((hi << 8) | lo);   // 1 = premido
  for (int i = 0; i < 10; i++)
    if (premidos & (1 << i)) Serial.printf("%s ", NOMES[i]);
  Serial.println();
  delay(100);
}
```

Carrega cada botão e confirma que o nome certo aparece no monitor série. Se um nome aparecer trocado, a ligação desse botão está num pino diferente do mapa da secção 3.

### 5.3 Testar já com o módulo PCF8574 que tens

O módulo azul com a etiqueta **PCF8574** (chip PCF8574T, cabeçalho de 8 pinos P0–P7, jumpers A0/A1/A2 e resistências de pull-up já montadas) **não serve para o Retro-Go**, que espera um PCF8575 de 16 pinos. Mas serve para testar a ligação do módulo 5D e dos botões do kit, que são 8 entradas:

```cpp
#include <Wire.h>

const char* NOMES[8] = {"LEFT","RIGHT","UP","DOWN","A","B","SELECT","START"};

void setup() {
  Serial.begin(115200);
  Wire.begin(22, 27);                      // SDA = IO22, SCL = IO27
  for (uint8_t a = 0x20; a <= 0x27; a++) { // PCF8574T: 0x20 a 0x27
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("Encontrado: 0x%02X\n", a);
  }
  Wire.beginTransmission(0x20);            // ajusta se o scan mostrou outro endereço
  Wire.write(0xFF);                        // todos os pinos como entrada
  Wire.endTransmission();
}

void loop() {
  Wire.requestFrom((uint8_t)0x20, (uint8_t)1);
  uint8_t premidos = ~Wire.read();         // 1 = premido
  for (int i = 0; i < 8; i++)
    if (premidos & (1 << i)) Serial.printf("%s ", NOMES[i]);
  Serial.println();
  delay(100);
}
```

Liga P0 a P7 pela ordem da tabela da secção 3.1 (LEFT em P0, RIGHT em P1, UP em P2, DOWN em P3, A em P4, B em P5, SELECT em P6, START em P7). O endereço depende dos jumpers A0/A1/A2: o scan diz qual é. Este sketch **não foi compilado nem testado**.

---

## 6. Depois

1. Grava o Retro-Go para CYD. Há imagens prontas na release **"CYD RetroGo"** do fork (por exemplo `retro-go_1.46_cyd.img`), e as notas da release remetem para o guia do Instructables, que não consegui abrir. Segundo o `BUILDING.md` do fork, a gravação é `esptool.py write_flash --flash_size detect 0x0 retro-go_1.46_cyd.img`. Isto **apaga o CYDboy**; para voltar a ele, grava-o de novo pelo gravador web.
2. Este fork não lê o Zero 2 nem o touch. Se quiseres os botões **e** o comando Bluetooth, é a opção B da `PROPOSTA.md`.
3. Não verifiquei se o **CYDboy** lê botões físicos. O README dele só fala de Bluetooth e toque.
4. O fork `CYD` tem o driver de bateria desligado, por isso o medidor de bateria da `PROPOSTA.md` não funciona nele sem alterações.

---

## 7. Poupar entradas: toque ou GPIOs diretos

Se quiseres usar o PCF8574 de 8 pinos que já tens, ou simplesmente ter menos botões físicos, há duas ideias. **Nenhuma delas funciona com a imagem pronta do Retro-Go**: exigem alterar o código e compilar o firmware (ESP-IDF 4.4.8, `python rg_tool.py build-img`). Não as testei.

Pelo código do fork (`rg_input.c`):
- **Não há suporte a toque.** Só lê ADC, GPIOs, expansor I²C, teclado e porta série.
- **GPIOs diretos e expansor I²C podem coexistir.** Os dois mapas (`RG_GAMEPAD_GPIO_MAP` e `RG_GAMEPAD_I2C_MAP`) são somados no mesmo estado dos botões.

| Ideia | O que muda | Esforço |
|-------|-----------|---------|
| **Botões no toque** (Start, Select, Menu, Option) | Escrever um driver do XPT2046 (pinos 25, 32, 33, 36, 39), zonas de toque no ecrã e calibração, mais o driver do PCF8574 | Alto. Dois remendos novos |
| **GPIOs diretos** para Menu e Option | Só editar o `config.h`: Menu no botão **BOOT** (GPIO0, já na placa) e Option no **IO35** (precisa de uma resistência de 10 kΩ ao 3V3, porque o IO35 não tem pull-up interno). Com o PCF8574, ainda falta o driver dele | Baixo (com PCF8575) ou médio (com PCF8574) |
| **Comprar o PCF8575** | Nada | Nenhum |

Com um PCF8574 ficam 8 entradas: D-pad, A, B, Select e Start. Menu e Option iriam para GPIO0 e IO35.

Para Game Boy e Game Boy Color o **CYDboy** já tem controlos no ecrã por toque, que desaparecem quando um comando Bluetooth emparelha. Para esses dois sistemas não é preciso programar nada.
