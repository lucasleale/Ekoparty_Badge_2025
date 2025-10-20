# Ekoparty Badge 2025
Sampler y sintetizador portátil con interfaz táctil resistiva

<img width="400" height="400" alt="top" src="https://github.com/user-attachments/assets/4f4eef39-ad58-40c7-97ec-2b52d8b22233" />

## ✨ Características principales
- **Microcontrolador RP2040:** [Core de Earle Philhower](https://github.com/earlephilhower/arduino-pico) para Arduino
- **Libreria de audio:** [AMY](https://github.com/shorepine/amy)
- **DAC I2S:** PCM5100 – 24 bits / 96 kHz, alta fidelidad
- **Amps:**
  - TPA6138 – salida de auriculares
  - TPA2028 (control I2C) – salida de parlante integrado
- **Batería recargable por USB-C**
- **10 LEDs NeoPixel** para backlight del PCB
- **4 sensores táctiles resistivos** integrados en el PCB para controlar/modular sonidos
- **Secuenciador integrado**
- **Reproductor de samples, sintetizador y efectos**

## Cómo utilizar
<img width="1371" height="706" alt="Badge (1)" src="https://github.com/user-attachments/assets/4f605918-3534-4945-80dd-0dc6b43e8bf5" />

Antes que nada, *precaución!*, a diferencia de un aparato normal adentro de un gabinete, el badge tiene la electrónica al descubierto. Por lo cual hay que tener cuidado donde apoyarlo, ya que si lo apoyamos sobre algo metálico/conductor, hará cortocircuito en los cientos de componentes que tiene en la cara trasera. Incluso con el badge apagado, ya que la bateria conserva voltaje y alimenta el sector de gestión de carga. *Cuidado!*

Con la pila cargada o el badge conectado por USB-C, prendelo con el mini switch que se encuentra del lado derecho de la PCB. Una vez encendido los LEDs de backlight harán una secuencia de luces indicando modo standby.

El badge cuenta con 4 sensores táctiles analógicos, es decir, que al tocarlos con los dedos, generarán una caida de voltaje correspondiente a la resistencia de la piel más la resistencia del circuito. Estas variaciones de voltaje son leídas por los 4 ADC de la RP2040 para controlar distintos parámetros. Básicamente estos sensores funcionan cuando el dedo cierra el circuito entre el nodo del ADC y GND, a mayor presión/superficie del dedo, mayor resistencia.

Los 4 sensores son: el número 21, el diamante, y las dos orejas inferiores. En el caso las orejas, la cara superior es el nodo del ADC, y la cara trasera es GND, por lo tanto hay que tocar ambas caras de la PCB para obtener una lectura correcta y estable.

El firmware que viene cargado por defecto en tu badge es una demo que tiene cargados samples del tema Still de Dr Dre. En este firmware, al presionar el diamante una vez (sin mantener) se activará el secuenciador, que tiene la información de reproducción de cada sample individual del tema. Los LEDs comenzarán a parpadear al ritmo de la música. El sensor del número 21 dispara distintas voces del tema de forma aleatoria. 

Los dos sensores de las orejas, a diferencia de los anteriores que funcionan como botón,  funcionan como un control continuo. La oreja izquierda (mirando el badge de frente), funciona como un pitch shifter (efecto que cambia la altura tonal de los sonidos), presionando con distintas intensidades escucharás que el sonido se hace mas grave o mas agudo *tonalmente*.
La oreja derecha funciona como un filtro pasa altos, a mayor presión, filtrará mas frecuencias graves, por lo cual el sonido sonará mas agudo *timbricamente*. Probá jugar con distintas velocidades y presiones!
Para frenar la reproducción, volve a tocar el diamante. También podes tocar el 21 y que suenen las voces (y aplicarle los efectos de las orejas) sin la música. 

El parlante suena bajito, si querés más potencia, conecta tus auris a la salida de audio del badge, o conectalo a un parlante mas grande!

## Cómo compilar

1. Instalar [**Arduino IDE**](https://www.arduino.cc/en/software/)
2. Añadir soporte para [**Raspberry Pi Pico / RP2040**](https://github.com/earlephilhower/arduino-pico)
3. Clonar este repositorio, copiar los contenidos de la carpeta Libraries en la de Arduino de tu sistema. La libreria AMY de este proyecto esta modificada para funcionar con menor latencia.
4. Con el badge apagado, mantener el botón Bootsel de la cara trasera de la PCB, conectar por USB a la computadora y encender. En la computadora aparecerá como unidad FLASH RPI-RP2. 
5. En Arduino IDE subir uno de los códigos seleccionando el puerto UF2 BOARD. Normalmente este procedimiento es necesario sólo una vez. A partir de ahí, el Arduino IDE reconocerá el puerto USB.

Los firmware se encuentran en [Firmware/Ekoparty_DrDre](https://github.com/lucasleale/Ekoparty_Badge_2025/tree/main/Firmware/Ekoparty_DrDre) (firmware por defecto), y un firmware extra [Firmware/Ekoparty_SynthDrum](https://github.com/lucasleale/Ekoparty_Badge_2025/tree/main/Firmware/Ekoparty_SynthDrum) . Dentro de cada carpeta hay una Build donde se encuentran los UF2. Estos binarios permiten cargar el firmware sin necesidad de compilar. Por ejemplo: [Firmware/Ekoparty_SynthDrum/build/rp2040.rp2040.rpipico/Ekoparty_SynthDrum.ino.uf2](https://github.com/lucasleale/Ekoparty_Badge_2025/tree/main/Firmware/Ekoparty_SynthDrum/build/rp2040.rp2040.rpipico). 
Para cargar un .uf2, de nuevo mantener bootsel antes de prender/conectar badge a la computadora, una vez conectado aparece como RPI-RP2 FLASH DRIVE, arrastrar .uf2 a la unidad.


## Créditos
Jorge Crowe (aka monstruoMIDI):
Concepto y coordinación de proyecto

Lucas Leal:
Desarrollo Hardware & Software

Nico Restbergs (aka Fábrica Marciana):
Diseño gráfico orientado a PCB
