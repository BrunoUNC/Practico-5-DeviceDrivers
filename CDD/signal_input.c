/**
 * signal_input.c - Controlador de señal de entrada GPIO
 * 
 * Este controlador de Raspberry Pi permite detectar el estado de dos señales externas 
 * con un período de un segundo conectadas a dos pines GPIO.   
 * El controlador debe ser capaz de decirle al CDD cuál de las dos señales leer.
 * 
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/moduleparam.h>

#define DEVICE_NAME "signal_input"
#define GPIO_INPUT1 22
#define GPIO_INPUT2 27

static int gpio_pin1 = GPIO_INPUT1;
static int gpio_pin2 = GPIO_INPUT2;
module_param(gpio_pin1, int, 0444);
MODULE_PARM_DESC(gpio_pin1, "Numero de pin 1 GPIO 22");
module_param(gpio_pin2, int, 0444);
MODULE_PARM_DESC(gpio_pin2, "Numero de pin 2 GPIO 27");

// Definir direcciones base de GPIO
#define BCM2837_GPIO_ADDRESS 0x3F200000

// Prototipos de funciones
static void configure_gpio_as_input(unsigned int pin);
static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *file, char *buffer, size_t len, loff_t *offset);
static ssize_t device_write(struct file *file, const char *buffer, size_t len, loff_t *offset);
static int __init gpio_signal_init(void);
static void __exit gpio_signal_exit(void);

// Variables globales
static int major_number             = 0;
static unsigned int selected_pin    = 0;
static unsigned int *gpio_registers = NULL;

// Estructura de operaciones de archivo (necesaria para el dispositivo de caracteres)
static struct file_operations fops = {
	.open       = device_open,
	.release    = device_release,
	.read       = device_read,
	.write      = device_write,
};

/**
 * @brief Función de lectura del dispositivo.
 * 
 * Esta función se llama cuando se lee el archivo del dispositivo para obtener elvalor del pin seleccionado.
 * 
 * @param file Puntero a la estructura del archivo.
 * @param buffer Puntero al búfer donde se escribirá el dato.
 * @param len Longitud del búfer.
 * @param offset Puntero al offset en el archivo.
 * @return Número de bytes leídos.
*/
static ssize_t device_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
    unsigned int gpio_value;
    char value_str[3];
    size_t value_str_len;

    // Leer el valor del pin GPIO
    gpio_value = (*(gpio_registers + 13) & (1 << selected_pin)) != 0;

    // Convertir el valor del GPIO a una cadena ("0\n" o "1\n")
    snprintf(value_str, sizeof(value_str), "%d\n", gpio_value);
    value_str_len = strlen(value_str);

    // Verificar si el offset está más allá de la longitud de la cadena
    if (*offset >= value_str_len)
        return 0;

    // Ajustar len para asegurarse de que no se lea más allá del final de la cadena
    if (len > value_str_len - *offset)
        len = value_str_len - *offset;

    // Copiar los datos al búfer del usuario
    if (copy_to_user(buffer, value_str + *offset, len))
        return -EFAULT;

    // Número de bytes leídos
    *offset += len;
    
    printk(KERN_INFO "SIGNAL INPUT: Valor leido %u.\n", gpio_value);
    
    return len;
}

/**
 * @brief Función de escritura del dispositivo.
 * 
 * Esta función se llama cuando se escribe en el archivo del dispositivo para seleccionar el pin para leer la señal.
 * 
 * @param file Puntero a la estructura del archivo.
 * @param buffer Puntero al búfer desde donde se leerán los datos.
 * @param len Longitud del búfer.
 * @param offset Puntero al offset en el archivo.
 * @return Número de bytes escritos.
*/
static ssize_t device_write(struct file *file, const char *buffer, size_t len, loff_t *offset)
{
    char kbuf[2];

    // Limitar la cantidad de datos a leer a 1 byte
    if (len > 1) len = 1;

    // Copiar los datos desde el búfer del usuario (pin a configurar)
    if (copy_from_user(kbuf, buffer, len))
        return -EFAULT;

    if (kbuf[0] == '1') {
        selected_pin = gpio_pin1;
    } else if (kbuf[0] == '2') {
        selected_pin = gpio_pin2;
    } else {
        return -EINVAL;
    }

    printk(KERN_INFO "SIGNAL INPUT: Pin seleccionado %u.\n", selected_pin);

    return len;
}

/**
 * @brief Configurar el pin GPIO como entrada.
 * 
 * Los GPIO están en grupos de 10, por lo que la función calcula el índice y la posición del bit.
 * Cada pin tiene 3 bits en el registro FSEL.
 * 
 * @param pin El número de pin GPIO a configurar como entrada.
 * @return void
*/
static void configure_gpio_as_input(unsigned int pin)
{
    unsigned int fsel_index = pin / 10;
    unsigned int fsel_bitpos = pin % 10;
    unsigned int *gpio_fsel = gpio_registers + fsel_index;

    *gpio_fsel &=~ (7 << (fsel_bitpos * 3)); // Limpiar los bits para el pin
    *gpio_fsel |= (0 << (fsel_bitpos * 3));  // Configurar el pin como entrada
    
    printk(KERN_INFO "SIGNAL INPUT: Pin %d configurado como entrada.\n", pin);
}

/**
 * @brief Función de apertura del dispositivo.
 *
 * Esta función se llama cuando se abre el archivo del dispositivo.
 *
 * @param inode Puntero a la estructura del inode.
 * @param file Puntero a la estructura del archivo.
 * @return 0 en éxito.
 */
static int device_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "SIGNAL INPUT: Dispositivo abierto.\n");
	return 0;
}

/**
 * @brief Función de liberación del dispositivo.
 *
 * Esta función se llama cuando se cierra el archivo del dispositivo.
 *
 * @param inode Puntero a la estructura del inode.
 * @param file Puntero a la estructura del archivo.
 * @return 0 en éxito.
 */
static int device_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "SIGNAL INPUT: Dispositivo cerrado.\n");
	return 0;
}

/**
 * @brief Función de inicialización del módulo.
 *
 * Esta función se llama cuando se carga el módulo. Inicializa los
 * registros GPIO y registra el dispositivo de caracteres.
 *
 * @return 0 en éxito, o un código de error negativo en caso de fallo.
 */
static int __init gpio_signal_init(void)
{
	printk(KERN_INFO "SIGNAL INPUT: Inicializando.\n");

	// Mapear la memoria GPIO
	gpio_registers = (unsigned int *)ioremap(BCM2837_GPIO_ADDRESS, PAGE_SIZE);
	if (!gpio_registers)
	{
		printk(KERN_ALERT "SIGNAL INPUT: Error al mapear la memoria GPIO.\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "SIGNAL INPUT: Memoria GPIO mapeada correctamente.\n");

	configure_gpio_as_input(gpio_pin1);
	configure_gpio_as_input(gpio_pin2);
	selected_pin = gpio_pin1; // Pin seleccionado por defecto

	// Registrar dispositivo de caracteres
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if (major_number < 0)
	{
		printk(KERN_ALERT "SIGNAL INPUT: Error al registrar un major number.\n");
		iounmap(gpio_registers);
		return major_number;
	}

	printk(KERN_INFO "SIGNAL INPUT: Registrado correctamente con el major number %d.\n", major_number);
	return 0;
}

/**
 * @brief Función de salida del módulo.
 *
 * Esta función se llama cuando se descarga el módulo. Desregistra el
 * dispositivo de caracteres y desmapea la memoria GPIO.
 */
static void __exit gpio_signal_exit(void)
{
	iounmap(gpio_registers);
	unregister_chrdev(major_number, DEVICE_NAME);
	printk(KERN_INFO "SIGNAL INPUT: Modulo descargado.\n");
}

MODULE_LICENSE("MIT License");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("GPIO Controller Raspberry Pi 3B");
MODULE_VERSION("1");

module_init(gpio_signal_init);
module_exit(gpio_signal_exit);

