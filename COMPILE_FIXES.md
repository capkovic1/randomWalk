## OPRAVY CHÝB KOMPILÁCIE

### CHYBA 1: Missing `#include <string.h>`

**Problém:**
```
simulation_handler.c: warning: implicit declaration of function 'memset'
```

**Príčina:** V `simulation_handler.c` sa používa `memset()` ale nie je includovaný `<string.h>`

**Riešenie:**
```c
// Pridaný include do simulation_handler.c
#include <string.h>
```

---

### CHYBA 2: Undefined reference to `send_command`

**Problém:**
```
ld: client/simulation_handler.o: in function `handle_interactive_mode':
    undefined reference to `send_command'
```

**Príčina:** `send_command` je definovaná v `client.c` ale `simulation_handler.c` o nej nevedel

**Riešenie:**
Pridá sa forward declaration v `simulation_handler.h`:
```c
// Forward declaration of send_command from client.c
extern StatsMessage send_command(const char* socket_path, MessageType type, int x, int y);
```

Teraz je deklarácia dostupná všetkým súborom, ktoré includujú `simulation_handler.h`.

---

### CHYBA 3: Unused parameters K a runs

**Problém:**
```
simulation_handler.c:38:23: warning: unused parameter 'K'
simulation_handler.c:38:30: warning: unused parameter 'runs'
```

**Príčina:** Funkcia `handle_interactive_mode()` dostáva parametre `K` a `runs` ale ich nepoužíva

**Riešenie:**
Označiť parametre ako intentionally unused:
```c
void handle_interactive_mode(
    ClientContext *ctx,
    StatsMessage *current_stats,
    int x, int y, int K __attribute__((unused)), int runs __attribute__((unused))
)
```

Atribút `__attribute__((unused))` hovorí gcc aby neuplatiňoval warning o nepoužitých parametroch.

---

## VÝSLEDOK

Po týchto opravách by sa projekt mal úspešne skompilovat bez chýb a warnings.

```bash
make clean
make
```

Ak budú ďalšie chyby, ozvime sa!
