
# Estación Solar con ESP32

Este proyecto mide el índice UV y la irradiancia solar utilizando un ESP32, mostrando los datos en una pantalla LCD, una página web local y publicándolos en Adafruit IO mediante MQTT.

## 🧰 Requisitos

- ESP32
- Sensor UV analógico
- Sensor TSL2591 (I2C)
- Pantalla LCD 16x2 (I2C)
- Cuenta en [Adafruit IO](https://io.adafruit.com/)

## 📦 Contenido

- `index.html`: Dashboard en vivo con los feeds de Adafruit IO
- `proyecto.html`: Explicación del hardware y conexiones
- `codigo.ino`: Código fuente para cargar al ESP32
- `style.css`: Estilo para las páginas
- `README.md`: Este archivo

## 🌐 GitHub Pages

Puedes publicar este sitio activando GitHub Pages en el repositorio (branch `main`, carpeta `/root`).

El sitio se verá en:

```
https://TUUSUARIO.github.io/esp32-solar-monitor/
```

## 📸 Vista previa

![Ejemplo del dashboard](https://via.placeholder.com/800x400.png?text=Dashboard+Solar)
