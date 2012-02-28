#include "sysbus.h"

typedef struct {
    SysBusDevice busdev;

    /* Registres GPIO (Reference Manual p119 */
    uint32_t mode; /* Mode */
    uint32_t otype; /* Output type */
    uint32_t ospeed; /* Output speed */
    uint32_t pupd; /* Pull-up/Pull-down */
    uint32_t ind; /* Input data */
    uint32_t outd; /* Output data register */
    uint32_t bsr; /* Bit set/reset */
    uint32_t lck; /* Lock */
    uint32_t afrl; /* Alternate function low */
    uint32_t afrh; /* Alternate function high */

    qemu_irq irq;
    qemu_irq out[8];
    const unsigned char id;
} stm32_gpio_state;

static const VMStateDescription vmstate_stm32_gpio = {
    .name = "stm32_gpio",
    .version_id = 2,
    .minimum_version_id = 1,
    .fields = (VMStateField[])
    {
        VMSTATE_UINT32(mode, stm32_gpio_state),
        VMSTATE_UINT32(otype, stm32_gpio_state),
        VMSTATE_UINT32(ospeed, stm32_gpio_state),
        VMSTATE_UINT32(pupd, stm32_gpio_state),
        VMSTATE_UINT32(id, stm32_gpio_state),
        VMSTATE_UINT32(od, stm32_gpio_state),
        VMSTATE_UINT32(bsr, stm32_gpio_state),
        VMSTATE_UINT32(lck, stm32_gpio_state),
        VMSTATE_UINT32(afrl, stm32_gpio_state),
        VMSTATE_UINT32(afrh, stm32_gpio_state),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32_gpio_update(stm32_gpio_state *s) {
    /* FIXME: Implement interrupts.  */
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

static void stm32_gpio_write(void *opaque, target_phys_addr_t offset,
        uint32_t value) {
    stm32_gpio_state *s = (stm32_gpio_state *) opaque;

    switch (offset) {
        case 0x00: /* Mode */
            s->mode = value;
            break;
        case 0x14: /* Output data on the 16 lower bit*/
            uint32_t tmp = value & 0x0000ffff;
            uint32_t old = s->outd & 0xffff0000;
            s->outd = tmp | old;
            break;
        default:
            hw_error("stm32_gpio_write: Bad offset %x\n", (int) offset);
    }
    stm32_gpio_update(s);
}

static void stm32_gpio_reset(stm32_gpio_state *s) {
    switch (s->id) {
        case 'B':
            s->mode = 0x00000280;
            s->outd = 0;
            break;
        default:
            s->mode = 0;
            s->outd = 0;
    }

}

static void stm32_gpio_set_irq(void * opaque, int irq, int level) {
    //stm32_gpio_state *s = (stm32_gpio_state *) opaque;

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

static int stm32_gpio_init(SysBusDevice *dev, const unsigned char id) {
    int iomemtype;
    stm32_gpio_state *s = FROM_SYSBUS(stm32_gpio_state, dev);
    s->id = id;
    iomemtype = cpu_register_io_memory(stm32_gpio_readfn,
            stm32_gpio_writefn, s,
            DEVICE_NATIVE_ENDIAN);
    sysbus_init_mmio(dev, 0x1000, iomemtype);
    sysbus_init_irq(dev, &s->irq);
    qdev_init_gpio_in(&dev->qdev, stm32_gpio_set_irq, 8);
    qdev_init_gpio_out(&dev->qdev, s->out, 8);
    stm32_gpio_reset(s);
    return 0;
}

static int stm32_gpio_init_B(SysBusDevice *dev) {
    return stm32_gpio_init(dev, 'B');
}

static SysBusDeviceInfo stm32_gpio_info = {
    .init = stm32_gpio_init_B,
    .qdev.name = "stm32_gpio_B",
    .qdev.size = sizeof (stm32_gpio_state),
    .qdev.vmsd = &vmstate_stm32_gpio,
};

static void stm32_gpio_register_devices(void) {
    sysbus_register_withprop(&stm32_gpio_info);
}

device_init(stm32_gpio_register_devices)