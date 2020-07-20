#include <linux/ioctl.h>

struct piu_cmd {
    unsigned int reg;
    unsigned int offset;
    unsigned int val;
};

#define IOC_MAGIC 'p'

#define PIU_READ  _IOWR(IOC_MAGIC, 1, struct piu_cmd)
#define PIU_WRITE _IOWR(IOC_MAGIC, 2, struct piu_cmd)
