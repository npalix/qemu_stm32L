
#include "sysbus.h"
#include "exec-memory.h"
#include "arm-misc.h"
#include "boards.h"
#include "devices.h"

#define NB_NVIC_IRQ 5
#define NB_GPIO 6

#define GPIO_A 0 //0x4002 0000 - 0x4002 03FF
#define GPIO_B 1 //0x4002 0400 - 0x4002 07FF
#define GPIO_C 2 //0x4002 0800 - 0x4002 0BFF
#define GPIO_D 3 //0x4002 0C00 - 0x4002 0FFF
#define GPIO_E 4 //0x4002 1000 - 0x4002 13FF
#define GPIO_H 5 //0x4002 1400 - 0x4002 17FF

typedef const struct { 
    const char *name;
    uint16_t f_size;
} stm32_board_info;

static stm32_board_info stm32_board = {
    "stm32l152rbt6",
    0x0080 //128kb
};


typedef struct {
    uint32_t int_status;
    uint32_t int_mask;
    uint32_t user0;
    uint32_t user1;
    qemu_irq irq;
    stm32_board_info* board;
} ssys_state;


static void ssys_update(ssys_state *s)
{
  qemu_set_irq(s->irq, (s->int_status & s->int_mask) != 0);
}

static void ssys_reset(void *opaque)
{
    //ssys_state *s = (ssys_state *)opaque;
}

static uint32_t ssys_read(void *opaque, target_phys_addr_t offset)
{
    ssys_state *s = (ssys_state *)opaque;

    switch (offset) {
    case 0x1e0: /* USER0 */
        return s->user0;
    case 0x1e4: /* USER1 */
        return s->user1;
    default:
        //hw_error("ssys_read: Bad offset 0x%x\n", (int)offset);
        return 0;
    }
}

static void ssys_write(void *opaque, target_phys_addr_t offset, uint32_t value)
{
    ssys_state *s = (ssys_state *)opaque;

    switch (offset) {
  //  default:
        //hw_error("ssys_write: Bad offset 0x%x\n", (int)offset);
    }
    ssys_update(s);
}


static CPUReadMemoryFunc * const ssys_readfn[] = {
   ssys_read,
   ssys_read,
   ssys_read
};

static CPUWriteMemoryFunc * const ssys_writefn[] = {
   ssys_write,
   ssys_write,
   ssys_write
};


static int stm32_sys_post_load(void *opaque, int version_id)
{
    //ssys_state *s = opaque;

    //Nothing to do
    
    return 0;
}

static const VMStateDescription vmstate_stm32_sys = {
    .name = "stm32_sys",
    .version_id = 2,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .post_load = stm32_sys_post_load,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32(int_mask, ssys_state),
        VMSTATE_UINT32(int_status, ssys_state),
        VMSTATE_END_OF_LIST()
    }
};


static int stm32l_sys_init(uint32_t base, qemu_irq irq,
                           stm32_board_info * board)
{
    int iomemtype;
    ssys_state *s;

    s = (ssys_state *)g_malloc0(sizeof(ssys_state));
    s->irq = irq;
    s->board = board;

    iomemtype = cpu_register_io_memory(ssys_readfn,
                                       ssys_writefn, s,
                                       DEVICE_NATIVE_ENDIAN);
    cpu_register_physical_memory(base, 0x00001000, iomemtype);
    ssys_reset(s);
    vmstate_register(NULL, -1, &vmstate_stm32_sys, s);
    return 0;
}



static void stm32l152rbt6_init(ram_addr_t ram_size, const char *boot_device, const char *kernel_filename, const char *kernel_cmdline, const char *initrd_filename, const char *cpu_model) {
    int i,j;

    //Préparation de la mémoire
    MemoryRegion *address_space_mem = get_system_memory();
    uint16_t flash_size = stm32_board.f_size; //128KBits
    uint16_t sram_size = 0x0010; //16 KBits
    
    
    //Initialisation du processeur (+ mémoire)
    qemu_irq* pic = armv7m_init(address_space_mem, flash_size, sram_size, kernel_filename, cpu_model);
    stm32l_sys_init(0x1FF00000, pic[28], &stm32_board); //TODO: Vérifier pic[28]
    
    
    //Structures GPIO
    static const uint32_t gpio_addr[NB_GPIO] = {
        0x40020000, //GPIO_A
        0x40020400, //GPIO_B
        0x40020800, //GPIO_C
        0x40020C00, //GPIO_D
        0x40021000, //GPIO_E
        0x40021400  //GPIO_H
    };
    static const int gpio_idIrqNVIC[NB_NVIC_IRQ] = {6,7,8,9,10};
    DeviceState* gpio_dev[NB_GPIO];
    qemu_irq gpio_in[NB_GPIO][NB_NVIC_IRQ];
    qemu_irq gpio_out[NB_GPIO][8];
    
    
    //Structures des LED
    DeviceState* led_dev6, led_dev7;
    
    
    
    //Initialisation du GPIO_A
    gpio_dev[GPIO_A] = sysbus_create_varargs("stm32_gpio_A", gpio_addr[GPIO_A], pic[gpio_idIrqNVIC[0]], pic[gpio_idIrqNVIC[1]], pic[gpio_idIrqNVIC[2]], pic[gpio_idIrqNVIC[3]], pic[gpio_idIrqNVIC[4]], NULL);
    for(j=0; j<8; j++) {
        qdev_connect_gpio_out(gpio_dev[GPIO_A], j, gpio_out[GPIO_A][j]); //Connecte la sortie du GPIO à des "fils vides"
        //TODO: connexion au bouton PA0
        //qemu_irq boutonIRQ = qdev_get_gpio_in(gpio_dev[GPIO_A], 0);
    }
    
    //Initialisation du GPIO_B
    gpio_dev[GPIO_B] = sysbus_create_varargs("stm32_gpio_B", gpio_addr[GPIO_B], pic[gpio_idIrqNVIC[0]], pic[gpio_idIrqNVIC[1]], pic[gpio_idIrqNVIC[2]], pic[gpio_idIrqNVIC[3]], pic[gpio_idIrqNVIC[4]], NULL);
    for(j=0; j<8; j++) {
        //TODO: Reprendre le contenu de cette boucle
        qdev_connect_gpio_out(gpio_dev[GPIO_B], j, gpio_out[GPIO_B][j]); //Connecte la sortie du GPIO à des "fils vides"
        if(j == 5) {
            //LED 6
            led_dev6 = sysbus_create_varargs("stm32_led", 0x00000000);
            qdev_connect_gpio_out()
        } else if(j ==6) {
            //LED 7
            
        }
    }
    

    /*
     for (i = 0; i < NB_GPIO; i++) {
        //gpio_dev[i] = sysbus_create_simple("pl061", gpio_addr[i], pic[gpio_irq[i]]);
        //gpio_dev[i] = sysbus_create_simple("pl061", gpio_addr[i], NULL);
        for (j = 0; j < 8; j++) {
            //gpio_in[i][j] = qdev_get_gpio_in(gpio_dev[i], j);
            gpio_out[i][j] = NULL;
        }
    }

    for (i = 0; i < NB_GPIO; i++) {
        for (j = 0; j < 8; j++) {
            if (gpio_out[i][j]) {
                qdev_connect_gpio_out(gpio_dev[i], j, gpio_out[i][j]);
            }
        }
    }
    */
    
}

static QEMUMachine stm32l152rbt6_machine = {
    .name = "stm32l152rbt6",
    .desc = "STM32L Discovery",
    .init = stm32l152rbt6_init,
};

static void stm32l_machine_init(void) {
    qemu_register_machine(&stm32l152rbt6_machine);
}

machine_init(stm32l_machine_init);

static void stm32l_register_devices(void) {

}

device_init(stm32l_register_devices)

