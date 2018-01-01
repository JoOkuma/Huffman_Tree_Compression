#include <stdio.h>
#include "bitstream.h"

/*
 * Estrutura byteNode: nó para árvore de Huffman
 * qnt: quantas vezes o byte repetiu no arquivo
 * bt: byte
 * *esq: ponteiro para o lado esquerdo da árvore
 * *dir: ponteiro para o lado direito da árvore
 *
 * Quando referirmos ao tamanho (menor, maior, etc) de um nó estaramos falando de acordo com o tamanho do ->qnt;
 */

typedef struct _byteNode{
    int qnt;
    byte bt;
    struct _byteNode *esq;
    struct _byteNode *dir;
} byteNode;


void freeArvore(byteNode* raiz);
byteNode* initByte(int qnt, byte bt);
byteNode* compInitNode(byteNode* menor1, byteNode* menor2);
int isFolha(byteNode* raiz);
void printPreOrdem(byteNode* raiz);
void gravadorArvore(byteNode* raiz, bitstreamoutput *arqBin);
void codificaByte(byteNode* raiz, bitstream *dict, bitstream bs);
void heapify(byteNode** raiz, int tam, int i);
void minHeap(byteNode** raiz, int tam);
void compMontaArvore(byteNode** raiz, int tam);
void compactador(char nomeArq[], char nomeNovo[]);
byteNode* descompInitNode(byteNode* direito, byteNode* esquerdo);
byteNode* descompMontaArvore(bitstreaminput* bsi);
byteNode* achaByte(byteNode* raiz, bitstreaminput* bsi);
void descompactador(char nomeArq[], char nomeNovo[]);

/*
 * Função main: lê o nome do arquivo e seleciona se ele será compactado ou descompactado
 */

int main(int argc, char **argv){

    if(argv[1][0] == 'c')
        compactador(argv[2], argv[3]);

    if(argv[1][0] == 'd')
        descompactador(argv[2], argv[3]);

    return 0;
}


/*
 * Função freeArvore: desaloca a memória dos nós da árvore
 * raiz: raiz da árvore
 */

void freeArvore(byteNode* raiz){
    if(raiz == NULL) return;
    freeArvore(raiz->esq);
    freeArvore(raiz->dir);
    free(raiz);
}


/*
 * Função initByte: aloca memória para um nó de uma folha da árvore de Huffman
 * qnt: quantas vezes o byte repetiu no arquivo
 * bt: byte
 * Retorna: o ponteiro deste novo nó
 */

byteNode* initByte(int qnt, byte bt){
    byteNode *novo;

    novo = calloc(1, sizeof(byteNode));
    novo->qnt = qnt;
    novo->bt = bt;

    return novo;
}


/*
 * Função compInitNode: aloca memória para um nó da árvore que ira apontar para outros dois nós
 * menor1: ponteiro para menor nó do heap
 * menor2: ponteiro para o segundo menor nó do heap
 * Retorna: o ponteiro deste novo nó
 */

byteNode* compInitNode(byteNode* menor1, byteNode* menor2){
    byteNode* node;

    node = calloc(1, sizeof(byteNode));
    node->qnt = menor1->qnt + menor2->qnt;
    node->esq = menor1;
    node->dir = menor2;

    return node;
}


/*
 * Função isFolha: checa se esse nó é uma folha return 1 e caso contrário 0
 * raiz: nó para ser checado
 */

int isFolha(byteNode* raiz){
    return raiz->esq == NULL && raiz->dir == NULL;
}


/*
 * Função printPreOrdem: imprimi a árvore em pré-ordem **FUNÇÂO APENAS PARA DEBUG**
 * raiz: ponteiro do primeiro nó da árvore
 *
 * função recursiva imprimi o valor da raiz, depois do ramo a esquerda, por último o ramo da direita
 */

void printPreOrdem(byteNode* raiz){
    if(raiz == NULL) return;
    if(isFolha(raiz))
        printf("%u  %d\n", raiz->bt, raiz->qnt);
    printPreOrdem(raiz->esq);
    printPreOrdem(raiz->dir);
}


/*
 * Função gravadorArvore: grava a árvore de Huffman no arquivo compactado
 * raiz: ponteiro para o primeiro nó da árvore
 * arqBin: vetor que guarda os valores binários que será adicionado no arquivo compactado
 */

void gravadorArvore(byteNode* raiz, bitstreamoutput *arqBin){
    bitstream bs;
    bsClean(&bs);
    if(raiz == NULL) return;
    if(!isFolha(raiz)){
        bsPushInPlace(0, &bs);
        encodedOutputWrite(arqBin, &bs);
        bsClean(&bs);
    }
    else{
        bsPushInPlace(1, &bs);
        encodedOutputWrite(arqBin, &bs);
        encodedOutputRawWrite(arqBin, raiz->bt);
        bsClean(&bs);
    }
    gravadorArvore(raiz->esq, arqBin);
    gravadorArvore(raiz->dir, arqBin);
}


/*
 * Função codificaByte: adiciona num "dicionário" o valor da árvore de Huffman para cada byte
 * raiz: raiz da árvore de Huffman
 * dict: vetor de bitstreams, é básicamente o dicionário dos bytes
 * bs: bitstream que será o valor do byte no dicionário
 */

void codificaByte(byteNode* raiz, bitstream *dict, bitstream bs){
    if(raiz == NULL) return;
    if(!isFolha(raiz)){
        codificaByte(raiz->esq, dict, bsPush(0, bs));
        codificaByte(raiz->dir, dict, bsPush(1, bs));
    }
    else{
        dict[raiz->bt] = bs;
    }
}


/*
 * Função heapify: organiza em heap o ramo da árvore selecionado **ATENÇÃO: essa função organiza o heap MÍNINO!!**
 * raiz: vetor dos heaps
 * tam: tamanho máximo do heap
 * i: posição da raiz do ramo no vetor
 */

void heapify(byteNode** raiz, int tam, int i){
    byteNode* t;
    int f = 2 * i;

    if(f < tam && raiz[f]->qnt > raiz[f + 1]->qnt) f++;
    if(f <= tam && raiz[f / 2]->qnt > raiz[f]->qnt){
        t = raiz[f / 2];
        raiz[f / 2] = raiz[f];
        raiz[f] = t;
        heapify(raiz, tam, f);
    }
}


/*
 * Função minHeap: percorre o vetor heap inteiro para gerar o heap MÌNIMO
 * raiz: ponteiro/vetor para os nós
 * tam: tamanho máximo do vetor
 */

void minHeap(byteNode** raiz, int tam) {
    for (int i = tam / 2; i > 0; i--)
        heapify(raiz, tam, i);
}

/*
 * Função compMontaAvore: utilizando o minHeap a função busca os menores nós para junta-los num novo nó e remove-los
 * esse novo nó é adicionado ao fim do vetor, isso ocorrerá até que o vetor tenha apenas um nó que será a raiz
 * raiz: ponteiro/vetor dos nós
 * tam: tamanho máximo do vetor
 */

void compMontaArvore(byteNode** raiz, int tam){
    byteNode* min1, *min2;
    while(tam > 1) {
        minHeap(raiz, tam);
        min1 = raiz[1];
        raiz[1] = raiz[tam];
        raiz[tam] = min1;
        tam--;

        minHeap(raiz, tam);
        min2 = raiz[1];
        raiz[1] = raiz[tam];
        raiz[tam] = compInitNode(min1, min2);
    }
}


/*
 * Função compactador: função principal para compactar arquivos
 * nomeArq: nome do arquivo que deve ser compactado
 *
 * Explicações no corpo do código
 */

void compactador(char nomeArq[], char nomeNovo[]){
    FILE *arquivo; // Arquivo que deve ser compactado
    FILE *arqNovo; // O novo arquivo que será gerado

    int *caracteres = calloc(256, sizeof(int)); // Vetor da quantidade que se repete cada byte no arquivo
    int tam = 1; // Sera incrementado para chegar ao tamanho do heap, deve começar no 1
    byte bt;
    uint32_t arqSize = 0; // Tamanho do arquivo, nó máximo 4gb

    byteNode *raiz; // Ponteiro para raiz da árvore de Huffman
    byteNode **lista = calloc(257, sizeof(byteNode*)); // Inicializa lista que conterá todos as folhas de cada byte

    arquivo = fopen(nomeArq, "rb");


    while (fread(&bt, 1, 1, arquivo)) {
        // Lê todos bytes do arquvio e incrementa suas quantidades
        caracteres[bt]++;
        arqSize++;
    }

    fclose(arquivo);

    for(int i = 0; i < 256; i++){
        // Inicializa todos os nós dos bytes
        if(caracteres[i] != 0)
            lista[tam++] = initByte(caracteres[i], (byte) i);
    }

    free(caracteres);
    caracteres = NULL;

    lista = realloc(lista, (tam) * sizeof(byteNode*));

    // Ordena pela primeira vez o ponteiro dos nós e monta a árvore de Huffman
    minHeap(lista, tam - 1);
    compMontaArvore(lista, tam - 1);

    raiz = lista[1]; // Ponteiro para a raiz da árvore

    arqNovo = fopen(nomeNovo, "wb");
    fwrite(&arqSize, sizeof(uint32_t), 1, arqNovo); // Escreve no arquivo no arquivo binario o tamanho do arquivo
                                                    // a ser compactado, 4 bytes, no máximo 4gb

    bitstreamoutput *bso = criaEncodedOutput(arqNovo); // Inicializa o bitstream para gravar no arquivo binário
    gravadorArvore(raiz, bso); // Grava a árvore de Huffman no arquivo binário

    bitstream *dict = calloc(256, sizeof(bitstream)); // Inicializa o "dicionário"
    bitstream bs;
    bsClean(&bs);
    codificaByte(raiz, dict, bs); // Adiciona o o valor binário para cada char desse "dicionário"

    // Abre o arquivo original novamente e grava os bytes codificados no arquivo binário
    arquivo = fopen(nomeArq, "r");
    while (fread(&bt, 1, 1, arquivo))
        encodedOutputWrite(bso, &dict[bt]);

    free(dict);
    freeArvore(raiz);
    free(lista);

    destroiEncodedOutput(bso);
    fclose(arquivo);
    fclose(arqNovo);
}


/*
 * Função descompInitNode: inicializa os nós que ligam outros nós e folhas
 * direito: nó que será ligado pelo ponteiro *dir
 * esquerdo: nó que será ligado pelo ponteiro *esq
 */

byteNode* descompInitNode(byteNode* direito, byteNode* esquerdo){
    byteNode* node;

    node = calloc(1, sizeof(byteNode));
    node->qnt = 0;
    node->esq = esquerdo;
    node->dir = direito;

    return node;
}


/*
 * Função descompMontaArvore: lê os arquivos binário e chama a função para iniciar os nós
 * bsi: bitstream que está sendo lido
 */

byteNode* descompMontaArvore(bitstreaminput* bsi){
    int node = bsiPop(bsi);
    byte bt;
    if(node == 0){
        return descompInitNode(descompMontaArvore(bsi), descompMontaArvore(bsi));
    }
    else{
        bt = bsiPopByte(bsi);
        return initByte(1, bt);
    }
}


/*
 * Função achaByte: utiliza os bits do arquivo como guia para encontrar a byte na árvore de Huffman
 * raiz: ponteiro para raiz da árvore de Huffman
 * bsi: bitstream dos bits que estão sendo lidos
 */

byteNode* achaByte(byteNode* raiz, bitstreaminput* bsi){
    int bit;
    if(raiz->qnt == 0){
        bit = bsiPop(bsi);
        if(bit == 0) return achaByte(raiz->esq, bsi);
        else return achaByte(raiz->dir, bsi);
    }
    else {
        return raiz;
    }
}


/*
 * Função descompactador: função principal para descompactar arquivos
 * nomeArq: nome do arquivo que deve ser compactado
 *
 * Explicações no corpo do código
 */

void descompactador(char nomeArq[], char nomeNovo[]){
    FILE* arquivo; // Arquivo binário compactado
    FILE* arqNovo; // Arquivo novo descompactado
    uint32_t arqSize; // Tamanho do arquivo descompactado
    int contador = 0; // Contador para limitar até o tamanho do arquivo original
    byte bt;
    byteNode* raiz;


    arquivo = fopen(nomeArq, "rb");
    fread(&arqSize, sizeof(uint32_t), 1, arquivo);  // Lẽ o tamanho do arquivo

    bitstreaminput *bsi = criaEncodedInput(arquivo);
    raiz = descompMontaArvore(bsi); // A raiz agora aponta para toda árvore de Huffman contida no arquivo


    arqNovo = fopen(nomeNovo, "w");

    while(contador < arqSize){
        // Lẽ a mensagem enquanto estiver dentro do tamanho do arquivo, assim evita o filler
        bt = achaByte(raiz, bsi)->bt;
        fwrite(&bt, sizeof(byte), 1, arqNovo);
        contador++;
    }

    destroiEncodedInput(bsi);

    fclose(arqNovo);
    fclose(arquivo);

    freeArvore(raiz);
}
