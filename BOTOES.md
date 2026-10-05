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
3. O **CYDboy** lê botões físicos: o README fala de um PCF8574 detectado sozinho, e o código (`button_input.cpp`, `hw_config.h`) o lê no endereço 0x20 com os bits 0 a 7 = cima, baixo, esquerda, direita, A, B, Start, Select, que é a mesma ordem da secção 3.1 e do `PORTE_CYD.md`. **Mas ele usa o GPIO16 como SDA e o GPIO17 como SCL**, que no CYD são o LED RGB e não saem em conector. Para usar o CN1 (IO22 e IO27) é preciso trocar duas linhas em `hw_config.h` e compilar. Li só esses arquivos e o README, não compilei nada (`PORTE_CYD.md`, secção 5.1).
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

### 7.1 Dois PCF8574 no mesmo barramento

Em I²C os módulos não ficam "em série": ficam **em paralelo** nos mesmos fios SDA e SCL (o conector J5 do teu módulo é só uma segunda saída desses fios, para ligar o módulo seguinte), e cada um tem um endereço diferente, definido pelos jumpers A0/A1/A2. Dois PCF8574 (0x20 e 0x21) dão 16 entradas, as mesmas do PCF8575.

- **Ligação:** módulo 1 (0x20) com P0–P7 nos bits 0–7 (LEFT a START), módulo 2 (0x21) com P0 em MENU e P1 em OPTION. Muda o jumper A0 de um dos módulos para ter endereços diferentes e confirma com o scanner da secção 5.1.
- **Pull-ups:** cada módulo traz resistências em SDA e SCL (R1 e R2). Pela foto parecem marcadas **102**, ou seja, 1 kΩ. Dois módulos em paralelo dão cerca de 500 Ω, o que é forte demais para o barramento. Tira (dessolda) R1 e R2 de **um** dos módulos.
- **Código:** o driver PCF857x do Retro-Go faz uma única leitura de 2 bytes num só endereço. Para dois módulos teria de ler 0x20 e 0x21 e juntar os bytes, e para o PCF8574 dizer que cada um só tem 1 porta. Não li o ficheiro completo, por isso não sei o tamanho exato do remendo. **Exige compilar o firmware**, e eu não o consigo testar.

Resumo: dois PCF8574 poupam a compra de um PCF8575 mas custam um remendo de código, a compilação e mexer em resistências SMD. Só compensa se o PCF8575 não se encontrar.

---

## 8. Alternativa mais simples para NES: Anemoia-ESP32 com um "controlo NES" próprio

Em vez do Retro-Go, o [Anemoia-ESP32](https://github.com/Shim06/Anemoia-ESP32) é um emulador de NES só para NES que suporta o CYD, no estilo do CYDboy: gravação pelo [gravador web](https://shim06.github.io/Anemoia-ESP32/), ROMs `.nes` na **raiz** da microSD, menu com **Start + Select** e save states. Segundo o README corre a ~60 fps sem PSRAM e implementa os mappers 0, 1, 2, 3, 4 e 69 (cerca de 79% dos jogos). Licença GPLv3.

**Limitação:** no CYD só aceita um **controlo NES/SNES** ou um **adaptador série**. Não menciona Bluetooth nem toque nem botões soltos.

| Sinal | GPIO do CYD | Conector |
|-------|-------------|----------|
| Clock | 22 | CN1 (ou P3) |
| Latch | 27 | CN1 |
| Data | 35 | P3 |

### 8.1 Controlo NES feito com um CD4021B

O controlo original é um registo de deslocamento 4021. Dá para o imitar com 8 botões, e o teu módulo 5D com 3 botões do kit chegam: o 5D dá cima, baixo, esquerda, direita, Select (SET) e Start (RST), e dois botões do kit dão A e B.

Ordem de leitura do protocolo NES: **A, B, Select, Start, Cima, Baixo, Esquerda, Direita**. O botão premido lê-se como nível baixo.

Ligações do **CD4021B** (DIP-16). Os números de pino abaixo são da minha memória da folha de dados: **confirma-os na folha de dados antes de ligar** e usa o teste da secção 8.2.

| Pino do CD4021B | Ligar a |
|-----------------|---------|
| 16 (VDD) | 3V3 |
| 8 (VSS) | GND |
| 9 (P/S, "latch") | GPIO27 |
| 10 (clock) | GPIO22 |
| 3 (Q8, saída série) | GPIO35 |
| 11 (entrada série) | GND |
| 1 (PI-8) | botão **A** |
| 15 (PI-7) | botão **B** |
| 14 (PI-6) | **Select** (SET do módulo 5D) |
| 13 (PI-5) | **Start** (RST do módulo 5D) |
| 4 (PI-4) | **Cima** (UP) |
| 5 (PI-3) | **Baixo** (DWN) |
| 6 (PI-2) | **Esquerda** (LFT) |
| 7 (PI-1) | **Direita** (RHT) |

- Cada entrada tem uma resistência de **10 kΩ ao 3V3** (pull-up), e o botão liga a entrada ao GND. O COM do módulo 5D vai ao GND.
- O IO35 só sai no conector **P3**, e o clock e o latch no **CN1**, por isso precisas de dois cabos JST 1,25 mm de 4 pinos.
- O CD4021B funciona de 3 V a 18 V, mas a 3,3 V é mais lento. Não sei que velocidade de clock o Anemoia usa, por isso se a leitura vier instável, o primeiro suspeito é este.
- Alternativa de dispositivo: um controlo NES ou SNES verdadeiro (o README diz que são suportados) ligado aos mesmos três sinais, a 3,3 V.

### 8.2 Testar o controlo antes de gravar o Anemoia

```cpp
const int CLK = 22, LATCH = 27, DATA = 35;
const char* NOMES[8] = {"A","B","SELECT","START","UP","DOWN","LEFT","RIGHT"};

void setup() {
  Serial.begin(115200);
  pinMode(CLK, OUTPUT);  digitalWrite(CLK, LOW);
  pinMode(LATCH, OUTPUT); digitalWrite(LATCH, LOW);
  pinMode(DATA, INPUT);                       // IO35 não tem pull-up interno
}

void loop() {
  digitalWrite(LATCH, HIGH); delayMicroseconds(12);
  digitalWrite(LATCH, LOW);  delayMicroseconds(6);
  for (int i = 0; i < 8; i++) {
    if (digitalRead(DATA) == LOW) Serial.printf("%s ", NOMES[i]);
    digitalWrite(CLK, HIGH); delayMicroseconds(6);
    digitalWrite(CLK, LOW);  delayMicroseconds(6);
  }
  Serial.println();
  delay(50);
}
```

Carrega cada botão e confirma que o nome certo aparece. Se os nomes vierem trocados, a ligação desse botão ao CD4021B está num pino diferente do que escrevi. Este sketch **não foi compilado nem testado**.

### 8.3 Controlo Bluetooth via segundo ESP32

O README do Anemoia descreve um **segundo ESP32** com o firmware *SerialGameControllerAdapter*, que lê um controlo NES, SNES, PS1, PS2 **ou Bluetooth** e envia os botões por série ao CYD (TX do adaptador para o GPIO22 do CYD, RX do adaptador para o GPIO27). Em teoria permitiria usar o Zero 2. Mas: não encontrei o repositório do adaptador, não sei se ele aceita o Zero 2 e terias de comprar mais um ESP32. Fica como possibilidade, não como plano.

---

## 9. Joystick Shield (Funduino V1.A) que você já tem

É um shield de Arduino Uno com um analógico, 6 botões grandes (A a F) e o botão K (apertar o analógico). Pela descrição das lojas ([ProtoSupplies](https://protosupplies.com/product/funduino-joystick-shield-v1-a/), [CRCibernética](https://www.crcibernetica.com/funduino-joystick-shield/)), as ligações no Arduino são:

| Componente | Pino do Arduino |
|------------|-----------------|
| Botão A | D2 |
| Botão B | D3 |
| Botão C | D4 |
| Botão D | D5 |
| Botão E | D6 |
| Botão F | D7 |
| K (apertar o analógico) | D8 |
| Eixo X | A0 |
| Eixo Y | A1 |

Os botões, segundo as mesmas fontes, costumam ser usados com o pull-up interno do Arduino, ou seja, ligam o pino ao GND ao apertar. **Confirme com o multímetro** (modo continuidade entre o pino e o GND com o botão apertado) antes de ligar, porque não consegui ver o esquema.

### O que ele ajuda e o que não ajuda

- **Ajuda como fonte de botões grandes e confortáveis.** São 7 botões digitais (A a F e K). Cada um liga o pino do shield ao GND, e o PCF8574 tem pull-up fraco interno, então dá para ligar direto nas entradas dele.
- **Não ajuda como D-pad.** O analógico precisa de duas entradas analógicas, e o CYD só deixa livre o IO35 (o IO22 e o IO27 já são do I²C). Teria que comprar um conversor I²C tipo ADS1115, e um analógico é pior que o módulo 5D para jogos de D-pad. Use o 5D para as direções.
- **É grande.** Tem o tamanho de um Arduino Uno (cerca de 69×53 mm), parecido com o do CYD. Serve bem como **controle de bancada** para testar os emuladores, mas não cabe num portátil compacto.

### Mapeamento sugerido para testes (CYD em pé, firmware próprio)

| Pino do PCF8574 | Tecla do app | Peça |
|-----------------|--------------|------|
| P0 | `KEY_UP` | módulo 5D: UP |
| P1 | `KEY_DOWN` | módulo 5D: DWN |
| P2 | `KEY_LEFT` | módulo 5D: LFT |
| P3 | `KEY_RIGHT` | módulo 5D: RHT |
| P4 | `KEY_OK` (A no Game Boy) | shield: **A** (D2) |
| P5 | `KEY_BACK` (B no Game Boy) | shield: **B** (D3) |
| P6 | `KEY_B` (Start) | shield: **C** (D4) |
| P7 | `KEY_A` (Select) | shield: **D** (D5) |
| Toque | `KEY_HOME` (pausa) | toque em qualquer lugar da tela (o BOOT do CYD, no GPIO0, é opcional) |

Os botões E, F e K ficam sobrando. Se quiser o menu de pausa num botão grande em vez do BOOT, falta um pino: aí entra o toque na faixa livre da tela (em pé) ou um segundo PCF8574.

### 9.1 Qual conector do shield usar

- **"Ext I2C" (SCL, SDA, GND, +5V): não serve para os botões.** Segundo a [ProtoSupplies](https://protosupplies.com/product/funduino-joystick-shield-v1-a/), é só uma saída extra dos fios I²C do Arduino (A4 e A5), com 5 V e GND, para ligar módulos I²C. O shield não tem nenhum chip I²C, e os botões não passam por esse barramento: ligar o CYD aí não traz informação de botão nenhuma. Além disso o pino é de **5 V**, que nunca deve ir ao CYD.
- **Conector amarelo de dupla fileira: este serve.** A mesma fonte diz que ele dá acesso a todos os botões, aos potenciômetros do analógico, a 3,3 V, 5 V e GND. Pela legenda impressa na sua foto, a fileira de cima tem `V A C E K X` e a de baixo `G B D F 3 Y`. Leitura minha, a confirmar com o multímetro: V = 5 V, G = GND, 3 = 3,3 V, A a F = botões, K = apertar o analógico, X e Y = eixos.

Ligação para o CYD: um fio do **GND (G)** do conector amarelo ao GND do CYD e do PCF8574, e um fio de cada botão (A, B, C, D) a uma entrada do PCF8574, conforme a tabela da seção 9. Não é preciso alimentar o shield: os botões só precisam ligar o pino ao GND.
