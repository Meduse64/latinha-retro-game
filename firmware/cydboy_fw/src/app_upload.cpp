#include "app_upload.h"
#include "ui_kit.h"
#include "ui_saver.h"
#include "net.h"
#include "sd_manager.h"
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <WebServer.h>

namespace {

const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="pt"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Latinha - Enviar jogos</title>
<style>
body{font-family:system-ui,sans-serif;background:#0b1620;color:#dce6f0;max-width:560px;margin:0 auto;padding:16px}
h1{font-size:22px;margin:0 0 4px}h2{font-size:16px;margin:24px 0 8px;color:#7fd}
p{color:#8aa;margin:4px 0 12px;font-size:14px}
#d{border:2px dashed #35a;border-radius:12px;padding:28px 12px;text-align:center;cursor:pointer}
#d.on{background:#123;border-color:#7fd}
.r{display:flex;align-items:center;gap:8px;padding:8px 0;border-bottom:1px solid #223}
.r span{flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.r small{color:#8aa;min-width:64px;text-align:right}
progress{width:90px}
button{background:#1b2a3a;color:#dce6f0;border:1px solid #35a;border-radius:6px;padding:4px 10px}
</style></head><body>
<h1>Enviar jogos</h1>
<p>Escolha as ROMs: .gb .gbc .gg .sms. Elas vao para a pasta certa do cartao.</p>
<div id="d">Arraste os arquivos aqui ou toque para escolher<input id="f" type="file" multiple accept=".gb,.gbc,.gg,.sms" hidden></div>
<div id="q"></div>
<h2>No cartao</h2>
<div id="l"></div>
<script>
const $=s=>document.querySelector(s);
const fmt=n=>n>1048576?(n/1048576).toFixed(1)+' MB':Math.round(n/1024)+' KB';
async function lst(){
 const a=await (await fetch('/list')).json(),l=$('#l');l.textContent='';
 if(!a.length){l.textContent='Nenhum jogo no cartao.';return}
 for(const x of a){
  const r=document.createElement('div');r.className='r';
  const n=document.createElement('span');n.textContent=x.n;
  const s=document.createElement('small');s.textContent=fmt(x.s);
  const b=document.createElement('button');b.textContent='Apagar';
  b.onclick=async()=>{if(confirm('Apagar '+x.n+'?')){await fetch('/del?d='+encodeURIComponent(x.d)+'&n='+encodeURIComponent(x.n),{method:'POST'});lst()}};
  r.append(n,s,b);l.append(r)}
}
function up(f){return new Promise(ok=>{
 const r=document.createElement('div');r.className='r';
 const n=document.createElement('span');n.textContent=f.name;
 const p=document.createElement('progress');p.max=100;p.value=0;
 const s=document.createElement('small');s.textContent='enviando';
 r.append(n,p,s);$('#q').append(r);
 const x=new XMLHttpRequest();x.open('POST','/up');
 x.upload.onprogress=e=>{if(e.lengthComputable)p.value=e.loaded*100/e.total};
 x.onload=()=>{s.textContent=x.status==200?'pronto':x.responseText;ok()};
 x.onerror=()=>{s.textContent='erro';ok()};
 const d=new FormData();d.append('f',f,f.name);x.send(d)})}
async function go(fs){for(const f of fs)await up(f);lst()}
const d=$('#d'),f=$('#f');
d.onclick=()=>f.click();
f.onchange=()=>{go([...f.files]);f.value=''};
d.ondragover=e=>{e.preventDefault();d.classList.add('on')};
d.ondragleave=()=>d.classList.remove('on');
d.ondrop=e=>{e.preventDefault();d.classList.remove('on');go([...e.dataTransfer.files])};
lst();
</script></body></html>)HTML";

const char* const DIRS[4] = { ROM_PATH_GB, ROM_PATH_GBC, ROM_PATH_GG, ROM_PATH_SMS };

WebServer* srv = nullptr;
File upFile;
String upPath;
char upName[48] = "";
char lastErr[48] = "";
uint32_t upBytes = 0, lastReq = 0, lastDraw = 0;
bool busy = false;
int nOk = 0;

// Pasta do cartao de acordo com a extensao; nullptr se nao for ROM
const char* dirFor(const String& name) {
    String n = name;
    n.toLowerCase();
    if (n.endsWith(".gbc")) return ROM_PATH_GBC;
    if (n.endsWith(".gb")) return ROM_PATH_GB;
    if (n.endsWith(".gg")) return ROM_PATH_GG;
    if (n.endsWith(".sms")) return ROM_PATH_SMS;
    return nullptr;
}

// So o nome (sem pasta), com letras, numeros e poucos simbolos
String cleanName(const String& raw) {
    int cut = max(raw.lastIndexOf('/'), raw.lastIndexOf('\\'));
    String n = cut >= 0 ? raw.substring(cut + 1) : raw;
    String out;
    for (size_t i = 0; i < n.length() && out.length() < 80; i++) {
        char c = n[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || strchr(" ._-()[]!&',+", c);
        out += ok ? c : '_';
    }
    while (out.startsWith(".")) out.remove(0, 1);
    return out;
}

String jsonEsc(const String& s) {
    String o;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if ((uint8_t)c >= 0x20) o += c;
    }
    return o;
}

// ---- tela da CYD ----
void drawStatus(bool force) {
    if (!force && millis() - lastDraw < 250) return;
    lastDraw = millis();
    const int y0 = 128;
    tft.fillRect(0, y0, SCREEN_W, SCREEN_H - UI_FOOTER_H - y0 - 1, K_BG);
    if (busy) {
        ui_fit_text(12, y0 + 14, SCREEN_W - 24, upName, K_WHITE, K_BG, ML_DATUM);
        uint32_t total = srv ? (uint32_t)srv->clientContentLength() : 0;
        int pct = total > 0 ? (int)(((uint64_t)upBytes * 100) / total) : 0;
        if (pct > 100) pct = 100;
        tft.drawRoundRect(12, y0 + 32, SCREEN_W - 24, 16, 4, K_CYAN);
        tft.fillRoundRect(14, y0 + 34, (SCREEN_W - 28) * pct / 100, 12, 3, K_CYAN);
        char b[24]; snprintf(b, sizeof(b), "%d%%  %u KB", pct, (unsigned)(upBytes / 1024));
        ui_text(SCREEN_W / 2, y0 + 64, b, K_ICE, K_BG, 2, MC_DATUM);
    } else {
        ui_text(SCREEN_W / 2, y0 + 20, lastErr[0] ? lastErr : "Aguardando...", lastErr[0] ? K_RED : K_AMBER, K_BG, 2, MC_DATUM);
        if (nOk) {
            char b[40]; snprintf(b, sizeof(b), "Recebidos agora: %d", nOk);
            ui_text(SCREEN_W / 2, y0 + 50, b, K_GREEN, K_BG, 2, MC_DATUM);
        }
    }
}

void drawStatic(const char* url) {
    tft.fillScreen(K_BG);
    ui_header("Enviar jogos", "Wi-Fi");
    ui_text(SCREEN_W / 2, 48, "No PC ou celular, na mesma rede, abra:", K_ICE, K_BG, 2, MC_DATUM);
    uint8_t font = tft.textWidth(url, 4) <= SCREEN_W - 16 ? 4 : 2;
    ui_text(SCREEN_W / 2, 78, url, K_CYAN, K_BG, font, MC_DATUM);
    ui_text(SCREEN_W / 2, 104, "e escolha as ROMs: .gb .gbc .gg .sms", K_GRAY, K_BG, 2, MC_DATUM);
    ui_footer("B ou toque no titulo: sair");
}

// ---- servidor ----
void onList() {
    lastReq = millis();
    String out = "[";
    bool first = true;
    for (int i = 0; i < 4; i++) {
        File d = SD.open(DIRS[i]);
        if (!d || !d.isDirectory()) continue;
        for (File e = d.openNextFile(); e; e = d.openNextFile()) {
            if (e.isDirectory()) continue;
            String n = e.name();
            if (!dirFor(n)) continue;
            if (!first) out += ',';
            first = false;
            out += "{\"n\":\"" + jsonEsc(n) + "\",\"s\":" + String((uint32_t)e.size()) + ",\"d\":\"" + DIRS[i] + "\"}";
        }
    }
    out += "]";
    srv->send(200, "application/json", out);
}

void onDelete() {
    lastReq = millis();
    saver_activity();
    String d = srv->arg("d"), n = srv->arg("n");
    const char* want = dirFor(n);
    bool dirOk = false;
    for (int i = 0; i < 4; i++) if (d == DIRS[i]) dirOk = true;
    if (!dirOk || !want || d != want || cleanName(n) != n) { srv->send(400, "text/plain", "pedido invalido"); return; }
    String path = d + "/" + n;
    bool ok = SD.exists(path) && SD.remove(path);
    Serial.printf("[UPLOAD] apagar %s: %s\n", path.c_str(), ok ? "ok" : "falhou");
    srv->send(ok ? 200 : 404, "text/plain", ok ? "ok" : "nao achei o arquivo");
}

void onUploadData() {
    HTTPUpload& u = srv->upload();
    lastReq = millis();
    saver_activity();
    if (u.status == UPLOAD_FILE_START) {
        lastErr[0] = 0; upBytes = 0; busy = true;
        String nm = cleanName(u.filename);
        snprintf(upName, sizeof(upName), "%s", nm.c_str());
        const char* dir = dirFor(nm);
        if (!dir) { snprintf(lastErr, sizeof(lastErr), "Tipo nao aceito"); busy = false; drawStatus(true); return; }
        upPath = String(dir) + "/" + nm;
        if (SD.exists(upPath)) SD.remove(upPath);            // mesmo nome: troca o jogo
        upFile = SD.open(upPath, FILE_WRITE);
        if (!upFile) { snprintf(lastErr, sizeof(lastErr), "Nao deu para criar o arquivo"); busy = false; }
        drawStatus(true);
    } else if (u.status == UPLOAD_FILE_WRITE) {
        if (upFile) {
            if (upFile.write(u.buf, u.currentSize) != u.currentSize) {
                snprintf(lastErr, sizeof(lastErr), "Cartao cheio ou com erro");
                upFile.close();
                SD.remove(upPath);
                busy = false;
            }
            upBytes += u.currentSize;
        }
        if (busy) drawStatus(false);
    } else if (u.status == UPLOAD_FILE_END) {
        if (upFile) {
            upFile.close();
            Serial.printf("[UPLOAD] %s: %u bytes\n", upPath.c_str(), (unsigned)upBytes);
            nOk++;
        }
        busy = false;
        drawStatus(true);
    } else if (u.status == UPLOAD_FILE_ABORTED) {
        if (upFile) { upFile.close(); SD.remove(upPath); }
        snprintf(lastErr, sizeof(lastErr), "Envio interrompido");
        busy = false;
        drawStatus(true);
    }
}

void onUploadDone() {
    if (lastErr[0]) srv->send(400, "text/plain; charset=utf-8", lastErr);
    else srv->send(200, "text/plain", "ok");
}

}  // namespace

void upload_run() {
    ui_wait_release();
    tft.fillScreen(K_BG);
    ui_header("Enviar jogos", "Wi-Fi");
    ui_msg("Ligando o Wi-Fi...", "rede de secrets.h");
    if (!net_connect(15000)) {
        ui_msg("Sem Wi-Fi", "confira o secrets.h", K_RED);
        delay(2500);
        net_off();
        return;
    }
    WiFi.setSleep(false);                         // mais rapido para receber arquivos

    char url[40];
    snprintf(url, sizeof(url), "http://%s", WiFi.localIP().toString().c_str());
    Serial.printf("[UPLOAD] pagina em %s\n", url);

    nOk = 0; busy = false; lastErr[0] = 0; upName[0] = 0;
    srv = new WebServer(80);
    srv->on("/", HTTP_GET, []() { lastReq = millis(); srv->send_P(200, "text/html; charset=utf-8", PAGE); });
    srv->on("/list", HTTP_GET, onList);
    srv->on("/del", HTTP_POST, onDelete);
    srv->on("/up", HTTP_POST, onUploadDone, onUploadData);
    srv->begin();

    drawStatic(url);
    drawStatus(true);
    lastReq = millis();

    UiIn in;
    while (true) {
        srv->handleClient();
        ui_poll(in);
        saver_activity();                          // a tela fica ligada enquanto a pagina esta aberta
        if (in.woke) { drawStatic(url); drawStatus(true); }
        if ((in.pressed & (GB_BTN_B | GB_BTN_START)) || (in.tap && in.ty < UI_HEADER_H)) break;
        if (millis() - lastReq > 10UL * 60UL * 1000UL) break;   // 10 min sem ninguem: desliga o Wi-Fi
        drawStatus(false);
        delay(2);
    }
    srv->stop();
    delete srv;
    srv = nullptr;
    net_off();
    ui_wait_release();
}
