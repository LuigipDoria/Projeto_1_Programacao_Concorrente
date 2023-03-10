#include <stdlib.h>

#include "hostess.h"
#include "globals.h"
#include "args.h"


int hostess_check_for_a_free_conveyor_seat() {
    /* 
        MODIFIQUE ESSA FUNÇÃO PARA GARANTIR O COMPORTAMENTO CORRETO E EFICAZ DO HOSTESS.
        NOTAS:
        1.  O HOSTESS DEVE FICAR EM ESPERA ATÉ QUE UMA VAGA SEJA LIBERADA NA ESTEIRA.
        2.  A VAGA INICIAL (conveyor->_seats[0]) É RESERVADA PARA O SUSHI CHEF.
        3.  NÃO REMOVA OS PRINTS, NEM O `msleep()` DE DENTRO DO WHILE LOOP.
        4.  O RETORNO DESSA FUNÇÃO É O ÍNDICE DO CONVEYOR INDICANDO UM ASSENTO LIVRE.
        5.  CUIDADO COM PROBLEMAS DE SINCRONIZAÇÃO!
    */

    // PARTE NÃO ORIGINAL

    /*
        Caso o sushi shop feche, o Hostess não irá procurar um lugar livre na esteira
    */

    if (globals_get_estado_restaurante() == TRUE) { // Ve se o restaurante fechou
        return 0; // Caso já tenha fechado, o hostess não leva o cliente para o lugar livre
    }

    // PARTE NÃO ORIGINAL

    conveyor_belt_t* conveyor = globals_get_conveyor_belt();
    virtual_clock_t* virtual_clock = globals_get_virtual_clock();
    
    print_virtual_time(globals_get_virtual_clock());
    fprintf(stdout, GREEN "[INFO]" NO_COLOR " O Hostess está procurando por um assento livre...\n");
    print_conveyor_belt(conveyor);

    // PARTE NÃO ORIGINAL
    /*
        Foi colocado um mutex para garantir a leitura correta da lista de vagas do sushi shop.
        Dessa forma durante a leitura nenhuma thread podera modificar a lista.
    */
    // FIM PARTE NÃO ORIGINAL

    while (TRUE) {

        // PARTE NÃO ORIGINAL

        pthread_mutex_lock(&conveyor->_seats_mutex); // Trava o acesso de outras thread para ler o coteudo da lista e ver se tem uma vaga na esteira
        
        if (globals_get_estado_restaurante() == TRUE) { // Ve se o restaurante fechou
            pthread_mutex_unlock(&conveyor->_seats_mutex); // Destrava o mutex após fechar o restaurante
            break; // Caso já tenha fechado, o hostess não leva o cliente para o lugar livre
        }
        
        // PARTE NÃO ORIGINAL

        for (int i=1; i<conveyor->_size; i++) {
            if (conveyor->_seats[i] == -1 && i !=0) {  // Atenção à regra! (-1 = livre, 0 = sushi_chef, 1 = customer)
                print_virtual_time(globals_get_virtual_clock());
                fprintf(stdout, GREEN "[INFO]" NO_COLOR " O Hostess encontrou o assento %d livre para o próximo cliente!\n", i);
                
                // PARTE NÃO ORIGINAL
                
                pthread_mutex_unlock(&conveyor->_seats_mutex); // Destrava o mutex após fazer a leitura da lista
                
                // FIM PARTE NÃO ORIGINAL

                return i;
            }
        }

        // PARTE NÃO ORIGINAL

        /*
            Caso não ache nenhum lugar vago, destrava o mutex. Para que enquanto essa thread fique em sleep não cause
            um mal funcionamento nas outras outras.
        */

        pthread_mutex_unlock(&conveyor->_seats_mutex); // Destrava o mutex após fazer a leitura da lista

        // FIM PARTE NÃO ORIGINAL

        msleep(120000/virtual_clock->clock_speed_multiplier);  // Não remova esse sleep!
    }
    return 0;
}

void hostess_guide_first_in_line_customer_to_conveyor_seat(int seat) {
    /* 
        MODIFIQUE ESSA FUNÇÃO PARA GARANTIR O COMPORTAMENTO CORRETO E EFICAZ DO HOSTESS.
        NOTAS:
        1.  NESSA FUNÇÃO É PAPEL DO HOSTESS GARANTIR 3 ATUALIZAÇÕES:
            a. O valor _seat_position do cliente em questão deve ser atualizado
            b. O valor do assento em questão no conveyor_belt global deve ser atualizado 
               (-1 = vazio, 0 = sushi_chef, 1 = cliente)
            c. O cliente em questão deve ser removido da fila global
        2.  CUIDADO COM PROBLEMAS DE SINCRONIZAÇÃO!
        3.  NÃO REMOVA OS PRINTS!
    */
    conveyor_belt_t* conveyor = globals_get_conveyor_belt();
    queue_t* queue = globals_get_queue();

    customer_t* customer = queue_remove(queue);

    // PARTE NÃO ORIGINAL

    /*
        Foi colocado um mutex para garantir a alteração correta da lista de vagas do sushi shop.
        Dessa forma durante a alteração nenhuma thread podera ler a lista.
    */
    queue_remove(queue);
    pthread_mutex_lock(&conveyor->_seats_mutex); // Trava o mutex para mudar a lista

    /*
        Caso a thread tenha ficado parada no lock ate o sushi shop fechar, foi colocado um verificação
        para não colocar um customer em um lugar vago após o fechamento da esteira.
        E é dado unlock no mutex, para evitar qualquer tipo de mal funcionamento.
    */

    
    if (globals_get_estado_restaurante() == TRUE) { // Ve se o restaurante fechou
        pthread_mutex_unlock(&conveyor->_seats_mutex); // Destrava o mutex após fechar o restaurante
        return; // Caso já tenha fechado, o hostess não leva o cliente para o lugar livre
    }

    // FIM PARTE NÃO ORIGINAL

    conveyor->_seats[seat] = 1;

    // PARTE NÃO ORIGINAL

    pthread_mutex_unlock(&conveyor->_seats_mutex); // Destrava o mutex após atlerar a lista
    
    // FIM PARTE NÃO ORIGINAL

    customer->_seat_position=seat;

    print_virtual_time(globals_get_virtual_clock());
    fprintf(stdout, GREEN "[INFO]" NO_COLOR " O Hostess levou o cliente %d para o assento %d!\n", customer->_id, seat);
    print_conveyor_belt(conveyor);    
}

void* hostess_run() {
    /* 
        MODIFIQUE ESSA FUNÇÃO PARA GARANTIR O COMPORTAMENTO CORRETO E EFICAZ DO HOSTESS.
        NOTAS:
        1.  O HOSTESS DEVE FUNCIONAR EM LOOP, RETIRANDO CLIENTES DA FILA GLOBAL E ADICIONANDO-OS NA
            ESTEIRA GLOBAL CONFORME VAGAS SÃO LIBERADAS.
        2.  QUANDO O SUSHI SHOP FECHAR, O HOSTESS DEVE PARAR DE GUIAR NOVOS CLIENTES DA FILA PARA 
            A ESTEIRA, E ESVAZIAR A FILA GLOBAL, FINALIZANDO OS CLIENTES EM ESPERA.
        3.  CUIDADO COM PROBLEMAS DE SINCRONIZAÇÃO!
        4.  NÃO REMOVA OS PRINTS!
    */
    virtual_clock_t* virtual_clock = globals_get_virtual_clock();
    queue_t* queue = globals_get_queue();

    /*
        Foi utilizado break para evitar o sleep, e o hostess sair assim que o sushi shop fechar
    */

    while (TRUE) {  // Adicione a lógica para que o Hostess realize o fechamento do Sushi Shop!
        if (queue->_length > 0) {
            int seat = hostess_check_for_a_free_conveyor_seat();
            hostess_guide_first_in_line_customer_to_conveyor_seat(seat);
        }
        if (globals_get_estado_restaurante() == TRUE && queue->_length == 0) { // Ve se o restaurante fechou e se ja foi removido todo mundo da fila
            break; // Caso já tenha fechado, o hostess não leva o cliente para o lugar livre
        }
        msleep(3000/virtual_clock->clock_speed_multiplier);  // Não remova esse sleep!
    }   

    pthread_mutex_lock(&virtual_clock->_n_codigos_finalizados);
    globals_set_n_codigos_finalizados();
    pthread_mutex_unlock(&virtual_clock->_n_codigos_finalizados);
    pthread_exit(NULL);
}

hostess_t* hostess_init() {
    /* NÃO PRECISA ALTERAR ESSA FUNÇÃO */
    hostess_t* self = malloc(sizeof(hostess_t));
    if (self == NULL) {
        fprintf(stdout, RED "[ERROR] Bad malloc() at `hostess_t* hostess_init()`.\n" NO_COLOR);
        exit(EXIT_FAILURE);
    }
    pthread_create(&self->thread, NULL, hostess_run, NULL);
    return self;
}

void hostess_finalize(hostess_t* self) {
    /* NÃO PRECISA ALTERAR ESSA FUNÇÃO */
    pthread_join(self->thread, NULL);
    free(self);
}
