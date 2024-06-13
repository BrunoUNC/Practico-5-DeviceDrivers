import time
import matplotlib.pyplot as plt
import threading

DEVICE_PATH = '/dev/signal_input'

def select_pin(pin_number):
    """
    Selecciona el pin GPIO a leer.
    
    :param pin_number: Número del pin a seleccionar (1 o 2)
    """
    if pin_number not in [1, 2]:
        raise ValueError("El pin debe ser 1 o 2.")
    
    with open(DEVICE_PATH, 'w') as device_file:
        device_file.write(str(pin_number))
    print(f"Pin {pin_number} seleccionado.")

def leer_gpio():
    try:
        with open(DEVICE_PATH, 'r') as file:
            valor = file.read().strip()
            return float(valor)  # Asumiendo que el valor leído es un número
    except FileNotFoundError:
        print("El dispositivo /dev/signal_input no existe.")
        return None
    except ValueError:
        print("Valor no válido leído desde el dispositivo.")
        return None

def graficar():
    global pin_actual, x_data, y_data, line, ax, start_time
    
    plt.ion()  # Activar modo interactivo
    fig, ax = plt.subplots()
    x_data = []
    y_data = []

    ax.set_ylim(0, 1)  # Ajusta el rango del eje y según tus necesidades
    line, = ax.plot(x_data, y_data)

    start_time = time.time()
    ax.set_title(f"Leyendo el pin {pin_actual}")

    while True:
        valor = leer_gpio()
        current_time = time.time() - start_time

        if valor is not None:
            x_data.append(current_time)
            y_data.append(valor)

        if len(x_data) > 100:  # Mantener solo los últimos 10 segundos de datos (100 muestras)
            x_data.pop(0)
            y_data.pop(0)

        ax.set_xlim(current_time - 10, current_time)
        line.set_xdata(x_data)
        line.set_ydata(y_data)
        ax.set_title(f"Leyendo el pin {pin_actual}")

        plt.draw()
        plt.pause(0.1)  # 100 ms

def cambiar_pin():
    global pin_actual, x_data, y_data, start_time

    while True:
        try:
            pin_number = int(input("Introduce el número de pin a leer (1 o 2): "))
            select_pin(pin_number)
            pin_actual = pin_number

            # Limpiar los datos y reiniciar el gráfico
            x_data.clear()
            y_data.clear()
            start_time = time.time()
        except ValueError as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    pin_actual = 1
    select_pin(pin_actual)

    grafico_thread = threading.Thread(target=graficar)
    pin_thread = threading.Thread(target=cambiar_pin)

    grafico_thread.start()
    pin_thread.start()

    grafico_thread.join()
    pin_thread.join()
