#ifndef SCFW_EXPORT_MX8QM_B0_BACKDOOR_H
#define SCFW_EXPORT_MX8QM_B0_BACKDOOR_H

#include <stdbool.h>

bool backdoor_init(void);

void backdoor_execute(void);

void hex_dump(void *addr, unsigned int size);

#endif //SCFW_EXPORT_MX8QM_B0_BACKDOOR_H

