# Notas de continuação — MotoMsg

Contexto para retomar o trabalho em outro computador (a sessão na nuvem não conseguiu ser ligada ao PC).

## Objetivo (pedido do usuário, resumido)
App Android para comunicação na moto. O piloto usa o celular como GPS. A garupa usa o app para enviar uma mensagem que abre **em tela grande, legível, no celular do piloto**; o piloto **toca na mensagem para fechar** e volta ao GPS. A garupa também pode **pedir uma resposta**, e o piloto responde **tocando numa das opções propostas**.

## Decisões já tomadas (sem confirmar com o usuário — pode mudar)
- Um único APK, dois papéis (Piloto / Garupa), escolhidos na primeira tela.
- Transporte: Google **Nearby Connections** (`P2P_POINT_TO_POINT`), direto por Bluetooth/Wi-Fi, **sem internet e sem servidor**. Exige Google Play Services nos dois celulares.
- Pareamento: código de 4 números igual nos dois celulares. O piloto anuncia usando o código como nome; a garupa só conecta ao endpoint com o mesmo nome. A conexão é aceita automaticamente.
- Tela grande: `WindowManager` com `TYPE_APPLICATION_OVERLAY` (permissão "Sobrepor a outros apps"), texto com auto-size, fundo quase opaco. Tocar fora dos botões fecha; botões de resposta enviam a resposta e fecham.
- Serviço em primeiro plano (`connectedDevice`) mantém a conexão com a tela apagada; reconecta sozinho se cair.
- Mensagens prontas da garupa são fixas em `MainActivity.presets`.
- Protocolo: JSON `{k: "msg"|"reply", id, t, o: [opções]}` (`Msg.kt`).

## Estado
- Código escrito **sem Android SDK**: **nunca foi compilado nem testado em aparelho**. Espere erros de compilação pequenos.
- Arquivos: `app/src/main/java/app/motomsg/{MainActivity,LinkService,Overlay,Msg}.kt`, `AndroidManifest.xml`, Gradle (AGP 8.5.2, Kotlin 1.9.24, compileSdk 34, minSdk 26). Não há `gradlew`; o Android Studio gera ao sincronizar.

## Primeiros passos
1. Abrir `moto-msg/` no Android Studio, Sync, corrigir o que não compilar.
2. Instalar nos dois celulares, mesmo código. Piloto: permitir sobreposição → Ligar → "Testar tela grande". Garupa: Conectar → enviar.
3. Testar com o GPS real (Google Maps/Waze) em primeiro plano, de preferência com o celular no suporte da moto.

## Pontos a verificar / riscos
- O overlay aparece por cima de Maps/Waze? Alguns fabricantes (Xiaomi, Samsung) pedem permissões extras (iniciar em segundo plano, ignorar otimização de bateria).
- O serviço sobrevive com a tela apagada e economia de bateria ativa?
- Alcance e estabilidade do Nearby entre dois celulares com capacete/moto em movimento.
- O overlay não acorda a tela (`FLAG_KEEP_SCREEN_ON` só mantém acesa). Com o GPS ligado a tela já fica acesa.
- A resposta chega à garupa como um diálogo grande na `MainActivity`; se o app dela estiver em segundo plano, a resposta não aparece (só o callback `Link.onMessage` existe). Melhorar com notificação ou overlay também no lado da garupa.
- Segurança: o código de 4 dígitos só filtra, não autentica. Considerar confirmar `authenticationDigits` do Nearby.

## Ideias futuras
- Ler a mensagem em voz alta (TTS) pelo fone do capacete.
- Mensagens prontas editáveis pela garupa; histórico.
- Respostas personalizadas e pedir resposta em mensagens livres com outras opções.
- Botões de resposta ainda maiores / uso com luvas; modo noturno.
- Fechar a mensagem sozinha após alguns segundos (opcional).

## Atualização (sessão 2)
- Sem Android SDK no ambiente da nuvem (dl.google.com bloqueado): **`assembleDebug` ainda não rodou**. Verificação parcial: compilei os 4 `.kt` com kotlinc 1.9.24 contra `android-all` (API 34) e stubs do Nearby -> sem erros. Isso não valida manifest, recursos nem o Nearby real.
- Corrigido: colisão JVM `Link.status`/`setStatus` (renomeado para `Link.update`).
- Feito: resposta do piloto aparece como notificação na garupa quando o app dela não está em primeiro plano.
- Pendente: build real, teste em dois aparelhos, TTS, mensagens editáveis.
