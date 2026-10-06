#pragma GCC optimize ("Os")   /* o -O3 do firmware infla o codigo e o cache de instrucoes da ESP32 nao da conta */

/* CYDboy: colisao entre sprites nos quadros que nao sao desenhados.
 *
 * Licenca: GNU General Public License v2 ou posterior. Este arquivo e derivado de render_obj()
 * (render.c) do SMS Plus, de Charles Mac Donald, que e GPL v2 ou posterior; por isso ele segue a
 * mesma licenca, diferente do resto do firmware (MIT).
 *
 * O VDP so marca a flag de colisao (bit 0x20 do status) quando duas linhas de sprite se
 * sobrepoem, e o SMS Plus so descobre isso ao desenhar a linha. Jogos como Fantasy Zone usam
 * essa flag para acertar os tiros nos inimigos. Nos quadros pulados (opcao "Pular quadros") a
 * linha nao e desenhada, a flag nunca sobe e o tiro atravessa o inimigo.
 *
 * Esta funcao faz so a conta da colisao, sem desenhar nem enviar nada a tela: a mesma regra de
 * render_obj(), mas contra um mapa de ocupacao em vez do buffer da linha.
 *
 * Onde fica: este e o original, em firmware/cydboy_fw/smsplus_extra/. O script
 * scripts/copiar_smsplus.ps1 o copia para src/smsplus/ (pasta ignorada pelo git, que recebe o
 * SMS Plus), e la ele e compilado junto com o resto. Fora de src/ de proposito: sem o SMS Plus
 * o arquivo nao compila, e o firmware tem que compilar sem Game Gear e Master System.
 */
#include "shared.h"

extern uint8 *getCache(int tile, int attr);
extern int vp_vstart, vp_vend;

void render_obj_collide(int line)
{
    static uint8 occ[256];
    int idx[64], ys[64];
    int n = 0, count = 0, i, k;

    /* Mesmas condicoes de render_line(): linha visivel e video ligado */
    if((line < vp_vstart) || (line >= vp_vend)) return;
    if((!(vdp.reg[1] & 0x40)) || (((vdp.reg[2] & 1) == 0) && (IS_SMS))) return;

    /* A flag so sobe (e fica) ate o jogo ler o status: ja subiu, nao precisa calcular */
    if(vdp.status & 0x20) return;

    int width = 8;
    int height = (vdp.reg[1] & 0x02) ? 16 : 8;
    uint8 *st = (uint8 *)&vdp.vram[vdp.satb];

    if(vdp.reg[1] & 0x01)
    {
        width *= 2;
        height *= 2;
    }

    /* Sprites que cruzam esta linha, na mesma ordem e com o mesmo limite de 8 do render_obj() */
    for(i = 0; i < 64; i += 1)
    {
        int yp = st[i];
        if(yp == 208) break;
        yp += 1;
        if(yp > 240) yp -= 256;
        if((line >= yp) && (line < (yp + height)))
        {
            count += 1;
            if((vdp.limit) && (count == 9)) break;
            idx[n] = i;
            ys[n] = yp;
            n += 1;
        }
    }

    /* Com menos de dois sprites na linha nao tem como colidir */
    if(n < 2) return;

    memset(occ, 0, sizeof(occ));

    for(k = 0; k < n; k += 1)
    {
        int yp = ys[k];
        int xp = st[0x80 + (idx[k] << 1)];
        int pn = st[0x81 + (idx[k] << 1)];
        int start = 0;
        int end = width;
        int x;
        uint8 *ctp;
        uint8 *cache_ptr;

        if(vdp.reg[0] & 0x08) xp -= 8;
        if(vdp.reg[6] & 0x04) pn |= 0x0100;
        if(vdp.reg[1] & 0x02) pn &= 0x01FE;

        if(xp < 0) start = 0 - xp;
        if((xp + width) > 256) end = 256 - xp;

        if(vdp.reg[1] & 0x01)
        {
            /* Sprite de tamanho dobrado */
            ctp = getCache((pn & 0x1ff) + ((line - yp) >> 3), (pn >> 9) & 3);
            cache_ptr = &ctp[(((line - yp) >> 1) << 3)];
            for(x = start; x < end; x += 1)
            {
                if(cache_ptr[x >> 1])
                {
                    if(occ[xp + x]) { vdp.status |= 0x20; return; }
                    occ[xp + x] = 1;
                }
            }
        }
        else
        {
            ctp = getCache((pn & 0x1ff) + ((line - yp) >> 3), (pn >> 9) & 3);
            cache_ptr = &ctp[((line - yp) << 3) & 0x38];
            for(x = start; x < end; x += 1)
            {
                if(cache_ptr[x])
                {
                    if(occ[xp + x]) { vdp.status |= 0x20; return; }
                    occ[xp + x] = 1;
                }
            }
        }
    }
}
