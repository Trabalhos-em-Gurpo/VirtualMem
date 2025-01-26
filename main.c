#include <stdio.h>
#include <stdlib.h>

//principal
#define PAGE_MASK 0xFF00
#define OFFSET_MASK 0x00FF

#define PAGE_TABLE_SIZE 256
#define FRAME_SIZE 256
#define TLB_SIZE 16
#define MEMORY_SIZE (PAGE_TABLE_SIZE * FRAME_SIZE) //65.536

typedef struct {
    int page_number;
    int frame_number;
} TLBEntry;

int page_table[PAGE_TABLE_SIZE];
TLBEntry tlb[TLB_SIZE];
char memory[MEMORY_SIZE];

int tlb_hits = 0;
int page_faults = 0;
int next_frame = 0;

// Inicializar tabela de páginas
void initialize_page_table() {
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i] = -1; // -1 indica que a página não está na memória
    }
}

// Inicializar TLB
void initialize_tlb() {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page_number = -1; // -1 indica que a entrada está vazia
        tlb[i].frame_number = -1;
    }
}

// Função para consultar o TLB
int search_tlb(int page_number) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page_number == page_number) {
            tlb_hits++;
            return tlb[i].frame_number;
        }
    }
    return -1; // Miss no TLB
}

// Função para atualizar o TLB usando FIFO
void update_tlb(int page_number, int frame_number) {
    static int next_tlb_entry = 0;
    tlb[next_tlb_entry].page_number = page_number;
    tlb[next_tlb_entry].frame_number = frame_number;
    next_tlb_entry = (next_tlb_entry + 1) % TLB_SIZE;
}

// Função para tratar falhas de página
int handle_page_fault(int page_number) {
    FILE *backing_store = fopen("BACKING_STORE.bin", "rb");
    if (backing_store == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo BACKING_STORE.bin\n");
        exit(1);
    }

    // Mover para a posição da página no arquivo
    fseek(backing_store, page_number * FRAME_SIZE, SEEK_SET);

    // Ler a página do arquivo
    fread(&memory[next_frame * FRAME_SIZE], sizeof(char), FRAME_SIZE, backing_store);

    fclose(backing_store);

    page_table[page_number] = next_frame;
    next_frame++;
    page_faults++;
    return page_table[page_number];
}

// Função para traduzir endereços lógicos em físicos
int translate_address(int logical_address) {
    int page_number = (logical_address & PAGE_MASK) >> 8;
    int offset = logical_address & OFFSET_MASK;

    // Consultar TLB
    int frame_number = search_tlb(page_number);
    if (frame_number == -1) {
        // Miss no TLB, consultar tabela de páginas
        frame_number = page_table[page_number];
        if (frame_number == -1) {
            // Falha de página
            frame_number = handle_page_fault(page_number);
        }
        update_tlb(page_number, frame_number);
    }

    int physical_address = (frame_number * FRAME_SIZE) + offset;
    return physical_address;
}

// Função para ler endereços lógicos a partir de um arquivo
void read_logical_addresses(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", filename);
        exit(1);
    }

    int logical_address;
    while (fscanf(file, "%d", &logical_address) != EOF) {
        int physical_address = translate_address(logical_address);
        char value = memory[physical_address];
        printf("Endereço Lógico: %d, Endereço Físico: %d, Valor: %d\n", logical_address, physical_address, value);
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo_de_endereços>\n", argv[0]);
        return 1;
    }

    initialize_page_table();
    initialize_tlb();

    read_logical_addresses(argv[1]);

    // Calcular estatísticas
    int total_addresses = 1000;
    double page_fault_rate = (double)page_faults / total_addresses * 100.0;
    double tlb_hit_rate = (double)tlb_hits / total_addresses * 100.0;

    printf("Taxa de falhas de página: %.2f%%\n", page_fault_rate);
    printf("Taxa de acertos do TLB: %.2f%%\n", tlb_hit_rate);

    return 0;
}
