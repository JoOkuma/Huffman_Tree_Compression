//VERSAO 2 - 2017-04-06

#ifndef __BITSTREAM_H
#define __BITSTREAM_H

#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>



//Apenas um sinônimo para evitar escrever unsigned char em todo lugar
typedef unsigned char byte; 

typedef struct _bitstream bitstream;
//Inicializa o bitstream
void bsClean(bitstream *bs);
//Verifica se o bitstream está cheio
int bsFull(bitstream *bs);
//Escreve um bit 'bit' no bitstream bs. Bit deve ser 0 ou 1
void bsPushInPlace(int bit, bitstream *bs);
//devolve uma COPIA do bitstream bs com o bit adicionado
bitstream bsPush (int bit, bitstream bs);
//devolve o valor do bit na posição pos contido no bitstream bs
int bsAt(int pos, bitstream *bs);

//Escrita
//usado para escrever bits para um arquivo 
typedef struct _encoutput bitstreamoutput;
bitstreamoutput *criaEncodedOutput(FILE *f);
//libera a memória utilizada pelo bso
//Finaliza o bso escrevendo o que ainda estiver sobrando
//Devolve o número total de bytes escritos
uint32_t destroiEncodedOutput(bitstreamoutput *bso);
//Escreve o conteúdo do bitstream stream no bso
void encodedOutputWrite(bitstreamoutput *bso, bitstream  *vstream);
//Escreve o byte v no bso
void encodedOutputRawWrite(bitstreamoutput *bso, byte v);

//Leitura
//usado para ler bits de um arquivo
typedef struct _encinput  bitstreaminput;
//Cria um novo bsi baseado no arquivo passado como parâmetro
bitstreaminput *criaEncodedInput(FILE *f);
//Finaliza o bsi
void destroiEncodedInput(bitstreaminput *bsi);
//Faz pop em um bit do stream de entrada
int bsiPop(bitstreaminput *bsi);
//Faz pop em um byte (8 bits) do stream de entrada
byte bsiPopByte(bitstreaminput *bsi);



/**********************************************************************
 **********************************************************************
 **********************************************************************/
//Definições gerais
#define BISTREAM_MAX_BYTES 8
#define MAX_BITSTREAM_LENGTH 8 * BISTREAM_MAX_BYTES

typedef struct _bitstream {
    byte stream[BISTREAM_MAX_BYTES];
    unsigned int bits;
} bitstream;

void bsClean(bitstream *bs) {
    int i;
    bs->bits = 0;
    for (i = 0; i < BISTREAM_MAX_BYTES; i++) {
        bs->stream[i] = 0;
    }
}

int bsFull(bitstream *bs) {
    return bs->bits == MAX_BITSTREAM_LENGTH;
}

void bsPushInPlace (int bit, bitstream *bs) {
    assert(!bsFull(bs));
    bit = !!bit;
    unsigned int stream_pos = bs->bits / 8;
    assert (stream_pos >= 0 && stream_pos < BISTREAM_MAX_BYTES);
    unsigned int byte_pos = bs->bits % 8;
    assert (byte_pos >=0 && byte_pos < 8);
    bs->stream[stream_pos] ^= (-bit ^ bs->stream[stream_pos]) & (1 << byte_pos);
    bs->bits++;
    assert(bs->bits <= MAX_BITSTREAM_LENGTH);
}

//devolve uma copia do valor recebido com o bit adicionado
bitstream bsPush (int bit, bitstream bs) {
    bitstream ret = bs;
    bsPushInPlace(bit, &ret);
    return ret;
}

int bsAt(int pos, bitstream *bs) {
    assert (pos < bs->bits);
    int stream_pos = pos / 8;
    byte mask = 1 << (pos % 8);      
    return !!(bs->stream[stream_pos] & mask);
}

/*********
 * Output
 *********/
typedef struct _encoutput {
    FILE *file;
    bitstream buffer;
    uint32_t bytesEscritos;
} bitstreamoutput;

bitstreamoutput *criaEncodedOutput(FILE *file) {
    bitstreamoutput *ret = malloc(sizeof(bitstreamoutput));
    ret->file = file;
    ret->bytesEscritos = 0;
    bsClean(&ret->buffer);
    return ret;
}

void flushBSOBuffer(bitstreamoutput *bso) {
    //para fazer flush completa até o próximo multiplo de 8
    while (bso->buffer.bits % 8)
        bsPushInPlace(0, &bso->buffer);
    int written = fwrite(bso->buffer.stream, 1, bso->buffer.bits / 8, bso->file); 
    bso->bytesEscritos += written;
    bsClean(&bso->buffer);
    assert(!bsFull(&bso->buffer));
}

uint32_t destroiEncodedOutput(bitstreamoutput *bso) {
    flushBSOBuffer(bso);
    uint32_t ret = bso->bytesEscritos;
    free(bso);
    return ret;
}

void encodedOutputWrite(bitstreamoutput *bso, bitstream  *vstream) {
    int posAtual = 0;
    while (posAtual < vstream->bits) {
        if (bsFull(&bso->buffer))
            flushBSOBuffer(bso);
        bsPushInPlace(bsAt(posAtual, vstream), &bso->buffer);
        posAtual++;
    }
}

void encodedOutputRawWrite(bitstreamoutput *bso, byte v) {
    int i;
    byte mask = 1;
    for (i = 0  ; i < 8; i++) {
        assert(mask);
        int bit = !!(v & mask);
        if (bsFull(&bso->buffer))
            flushBSOBuffer(bso);
        bsPushInPlace(bit, &bso->buffer);
        mask <<= 1;
    }
}

/*********
 * Input
 *********/
typedef struct _encinput {
    FILE *file;
    bitstream buffer;
    unsigned int pos;    
} bitstreaminput;

bitstreaminput *criaEncodedInput(FILE *f) {
    bitstreaminput *ret = malloc(sizeof(bitstreaminput));
    ret->file = f;
    bsClean(&ret->buffer);
    ret->pos = 0;
    return ret;
}

void destroiEncodedInput(bitstreaminput *bsi) {
    free(bsi);
}

int bsiPop(bitstreaminput *bsi) {
    if (bsi->pos < bsi->buffer.bits) {
        return bsAt(bsi->pos++, &bsi->buffer);
    } else {
        int lidos = fread(&bsi->buffer.stream, 1, BISTREAM_MAX_BYTES, bsi->file);
        bsi->buffer.bits = lidos * 8;
        assert(bsi->buffer.bits > 0);
        bsi->pos = 1;
        return bsAt(0, &bsi->buffer);
    }    
}

byte bsiPopByte(bitstreaminput *bsi) {
    int i;
    byte ret = 0;
    for (i = 0; i < 8; i++) {
        ret |= bsiPop(bsi) << i;
    }
    return ret;
}

#endif 