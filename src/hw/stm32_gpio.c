#include "sysbus.h"

#define NB_IRQ_NVIC 4

typedef struct {
    SysBusDevice busdev;

    /* Registres GPIO (Reference Manual p119 */
    uint32_t mode; /* Mode */
    uint32_t otype; /* Output type */
    uint32_t ospeed; /* Output speed */
    uint32_t pupd; /* Pull-up/Pull-down */
    uint32_t ind; /* Input data */
    uint32_t outd; /* Output data register */
    uint32_t outd_old;
    uint32_t bsr; /* Bit set/reset */
    uint32_t lck; /* Lock */
    uint32_t afrl; /* Alternate function low */
    uint32_t afrh; /* Alternate function high */

    qemu_irq irq[NB_IRQ_NVIC];
    //qemu_irq in[8]; //Créé implicitement lors de l'appel à la fonction qdev_init_gpio_in(...)
    qemu_irq out[8];
    unsigned char id;
} stm32_gpio_state;


static const VMStateDescription vmstate_stm32_gpio = {
    .name = "stm32_gpio",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[])
    {
        VMSTATE_UINT32(mode, stm32_gpio_state),
        VMSTATE_UINT32(otype, stm32_gpio_state),
        VMSTATE_UINT32(ospeed, stm32_gpio_state),
        VMSTATE_UINT32(pupd, stm32_gpio_state),
        VMSTATE_UINT32(ind, stm32_gpio_state),
        VMSTATE_UINT32(outd, stm32_gpio_state),
        VMSTATE_UINT32(bsr, stm32_gpio_state),
        VMSTATE_UINT32(lck, stm32_gpio_state),
        VMSTATE_UINT32(afrl, stm32_gpio_state),
        VMSTATE_UINT32(afrh, stm32_gpio_state),
        VMSTATE_END_OF_LIST()
    }
};


static void stm32_gpio_update(stm32_gpio_state *s) {
    uint32_t changed;
    uint32_t mask;
    int i;

    changed = s->outd_old ^ s->outd; //XOR: Tous les bits à 1 seront les bits changés
    if (!changed)
        return;

    s->outd_old = s->outd;
    for (i = 0; i < 8; i++) {
        mask = 1 << i;
        if (changed & mask) {
            //Envoie une IRQ vers le periphérique branché sur la pin du GPIO
            qemu_set_irq(s->out[i], (s->outd & mask) != 0);
        }
    }
    /*
    if (s->chr) {
        uint8_t buffer;
        buffer = (uint8_t)s->outd & 0x000000FF;
        //printf("Changement GPIO %d\n", buffer);
        qemu_chr_fe_write(s->chr, &buffer, 1);
    }
     */
}


static uint32_t stm32_gpio_read(void *opaque, target_phys_addr_t offset) {
    stm32_gpio_state *s = (stm32_gpio_state *) opaque;

    switch (offset) {
        case 0x00: /* Mode */
            return s->mode;
        case 0x14: /* Output data */
            return s->outd;
        default:
            hw_error("stm32_gpio_read: Bad offset %x\n", (int) offset);
            return 0;
    }
}


static void stm32_gpio_write(void *opaque, target_phys_addr_t offset, uint32_t value) {
    stm32_gpio_state *s = (stm32_gpio_state *) opaque;

    uint32_t tmp, old;
    switch (offset) {
        case 0x00 : /* Mode */
            s->mode = value;
            break;
        case 0x14: /* Output data on the 16 lower bit*/
            tmp = value & 0x0000ffff;
            old = s->outd & 0xffff0000;
            s->outd = tmp | old;
            break;
        default:
            hw_error("stm32_gpio_write: Bad offset %x\n", (int) offset);
    }
    stm32_gpio_update(s);
}


static void stm32_gpio_reset(stm32_gpio_state *s) {
    switch (s->id) {
        case 'A':
            s->mode = 0xA8000000;
            s->outd = 0;
        case 'B':
            s->mode = 0x00000280;
            s->outd = 0;
            break;
        default:
            s->mode = 0;
            s->outd = 0;
    }

}

/*
 * Appelé quand une entrée de GPIO reçois une IT
 */
static void stm32_gpio_in_recv(void * opaque, int numPin, int level) {
    if(numPin < NB_IRQ_NVIC) {
        printf("Set irq[%d]->%d\n", numPin, level);
        stm32_gpio_state *s = (stm32_gpio_state *) opaque;
        qemu_set_irq(s->out[numPin], level);
    }
}

static CPUReadMemoryFunc * const stm32_gpio_readfn[] = {
    stm32_gpio_read,
    stm32_gpio_read,
    stm32_gpio_read
};

static CPUWriteMemoryFunc * const stm32_gpio_writefn[] = {
    stm32_gpio_write,
    stm32_gpio_write,
    stm32_gpio_write
};


/*
static int stm32_can_receive(void *opaque)
{
    return 1;
    //stm32_gpio_state *s = (stm32_gpio_state *)opaque;
    //if (s->lcr & 0x10)
    //    return s->read_count < 16;
    //else
    //    return s->read_count < 1;
}


static void stm32_receive(void *opaque, const uint8_t* buf, int size)
{
    int i;
    for(i=0; i<size; i++) {
        uint8_t var = buf[i];
        int numBit;
        for(numBit=0; numBit<8; numBit++) {
            uint8_t mask = 1;
            mask = mask << numBit;
            if((var & mask) != 0) {
                stm32_gpio_state *s = (stm32_gpio_state *)opaque;
                qemu_set_irq(s->out[i], (var & mask)!=0);
                //stm32_gpio_set_irq(opaque, numBit, (var & mask)!=0);
            }
        }
    }
}

static void stm32_event(void *opaque, int event)
{
    //if (event == CHR_EVENT_BREAK)
    //    pl011_put_fifo(opaque, 0x400);
}
*/


static int stm32_gpio_init(SysBusDevice *dev, const unsigned char id) {
    int iomemtype;
    stm32_gpio_state *s = FROM_SYSBUS(stm32_gpio_state, dev);
    s->id = id;
    iomemtype = cpu_register_io_memory(stm32_gpio_readfn, stm32_gpio_writefn, s, DEVICE_NATIVE_ENDIAN);
    sysbus_init_mmio(dev, 0x1000, iomemtype);
    
    //Initialisation des IRQ
    int i;
    for (i=0;i<NB_IRQ_NVIC; i++){
        sysbus_init_irq(dev, &s->irq[i]);
    }
    
    //Initialisation des pins d'entrées
    qdev_init_gpio_in(&dev->qdev, stm32_gpio_in_recv, 8);
    
    //Initialisation des pins de sorties
    qdev_init_gpio_out(&dev->qdev, s->out, 8);

    /*
    s->chr = qdev_init_chardev(&dev->qdev);
    s->chr = qemu_chr_find("B");
    if (s->chr) {
        qemu_chr_add_handlers(s->chr, stm32_can_receive, stm32_receive, stm32_event, s);
    }
    */
    stm32_gpio_reset(s);

    vmstate_register(&dev->qdev, -1, &vmstate_stm32_gpio, s);

    return 0;
}


static int stm32_gpio_init_B(SysBusDevice *dev) {
    return stm32_gpio_init(dev, 'B');
}

static SysBusDeviceInfo stm32_gpioB_info = {
    .init = stm32_gpio_init_B,
    .qdev.name = "stm32_gpio_B",
    .qdev.size = sizeof (stm32_gpio_state),
    .qdev.vmsd = &vmstate_stm32_gpio,
};

static int stm32_gpio_init_A(SysBusDevice *dev) {
    return stm32_gpio_init(dev, 'A');
}

static SysBusDeviceInfo stm32_gpioA_info = {
    .init = stm32_gpio_init_A,
    .qdev.name = "stm32_gpio_A",
    .qdev.size = sizeof (stm32_gpio_state),
    .qdev.vmsd = &vmstate_stm32_gpio,
};

static void stm32_gpio_register_devices(void) {
    sysbus_register_withprop(&stm32_gpioA_info);
    sysbus_register_withprop(&stm32_gpioB_info);
}

device_init(stm32_gpio_register_devices)