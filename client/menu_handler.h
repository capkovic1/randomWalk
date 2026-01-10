#ifndef MENU_HANDLER_H
#define MENU_HANDLER_H

#include "../common/common.h"

// Funkcie pre spravovanie menu a stavov

/**
 * Handle - vytvorenie novej miestnosti a servera
 * Vráti: UI stav po vytvorení (UI_SETUP_SIM alebo UI_MENU_MODE)
 */
UIState handle_create_new_server(
    ClientContext *ctx,
    char *room_code,
    int *mode
);

/**
 * Handle - pripojenie k existujúcemu serveru
 * Vráti: UI stav po pripojení (UI_INTERACTIVE alebo UI_MENU_MODE)
 */
UIState handle_connect_to_existing(ClientContext *ctx);

/**
 * Handle - spustenie a čakanie na server
 * Vráti: 1 ak server úspešne bindol, 0 ak timeout
 */
int wait_for_server(const char *socket_path, int max_retries);

/**
 * Handle - poslanie konfigurácie serveru
 * Vráti: 1 ak úspešne, 0 ak chyba
 */
int send_config_to_server(
    ClientContext *ctx,
    int x, int y,
    int width, int height,
    int K, int runs,
    int *probs,
    const char *out_filename,
    UIState *next_state
);

/**
 * Zobrazenie chybového dialógu
 */
void show_error_dialog(const char *message);

#endif
