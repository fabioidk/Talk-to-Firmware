# Talk-to-Firmware

**Talk to Firmware - Introducción a la Ingeniería Inversa en Hardware con JTAG**

## Descripción

**TALK // UNL0CK** es un reto introductorio de ingeniería inversa en hardware
desarrollado para la plataforma **ESP32-C3**.

El participante recibe una placa previamente programada y debe analizar su
comportamiento como una **caja negra**, sin depender del código fuente. 
La interacción con el dispositivo se realiza mediante
sus interfaces físicas y las herramientas de depuración disponibles en la
plataforma.

El objetivo es investigar el firmware, identificar información relevante en
memoria y comprender cómo el estado interno del dispositivo afecta su
comportamiento, para así completar las distintas etapas del reto hasta reconstruir 
el mensaje oculto transmitido por el dispositivo.

El firmware ofrece únicamente una pista inicial:

> **Listen to the board. Inspect what changes.**  
> **Alter what remains.**

La placa será quien indique si se está siguiendo el camino correcto.

## Inicio del reto

Una vez preparada la placa, el participante parte únicamente del dispositivo
programado.

Al iniciar, el firmware presenta:
```text
+--------------------------------------------------+
|              TALK // UNL0CK                      |
|        ESP32-C3 FIRMWARE CHALLENGE               |
+--------------------------------------------------+
|  Listen to the board. Inspect what changes.      |
|  Alter what remains.                             |
+--------------------------------------------------+
```
## Pasos para iniciar el reto

Antes de comenzar el reto, prepare el entorno de trabajo y programe la
ESP32-C3 siguiendo los manuales incluidos en este repositorio.

### 1. Configurar el entorno

Consulte:

[**Manual de Configuración de Entorno - Talk to Firmware**](./Manual_Configuración_de_Entorno_Talk_to_Firmware.pdf)

Este documento explica la preparación del entorno y la instalación de las
herramientas necesarias para trabajar con el reto.

### 2. Flashear la ESP32-C3

Consulte:

[**Manual de Flasheo de ESP32-C3 - Talk to Firmware**](./Manual_de_Flasheo_de_ESP32_C3_Talk_to_Firmware.pdf)

Siga este manual para cargar el firmware del reto en la ESP32-C3 DevKitM-1.

### 3. Iniciar el reto

Consulte:

[**Write-up del reto Talk to Firmware**](./WriteUp_Reto_Talk_to_Firmware.pdf)

Una vez programada la placa, el reto debe abordarse como un escenario de
**caja negra**: a partir de este momento, la resolución parte del dispositivo
programado y de su comportamiento observable. Use el write-up para guiarse paso
a paso en la resolución del reto.


## Nota importante:

Algunos de los comandos presentados en las guías utilizan rutas que pueden variar según en dónde usted haya
descargado los archivos del repositorio, por lo que debe prestar atención a dichas variaciones y simplemente
ajustar la ruta según sea el caso en su máquina.
