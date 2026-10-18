# Servidor de voz de Midoryu (en tu compu con Windows)

Esto le da cerebro y voz a tu StackChan usando tu propia compu, sin el servidor oficial.
Tiempo estimado: 30-45 min la primera vez. Nada de esto toca tu StackChan hasta el paso 7.

## 1. Instala Docker Desktop
1. Ve a https://www.docker.com/products/docker-desktop/ y descarga **Docker Desktop for Windows**.
2. Instálalo (deja marcada la opción de WSL 2), reinicia la compu si te lo pide.
3. Ábrelo y espera a que abajo a la izquierda diga **Engine running** (en verde).

## 2. Prepara la carpeta
1. Descarga esta carpeta `midoryu-server` a tu compu (por ejemplo en `C:\midoryu-server`).
2. Dentro crea una carpeta `models`, y dentro de ella otra llamada `SenseVoiceSmall`.

## 3. Baja el modelo que entiende tu voz
Descarga el archivo `model.pt` desde
https://modelscope.cn/models/iic/SenseVoiceSmall/resolve/master/model.pt
y ponlo en `C:\midoryu-server\models\SenseVoiceSmall\model.pt` (pesa casi 1 GB).

## 4. Saca tu llave gratis de Gemini
1. Entra a https://aistudio.google.com/apikey con tu cuenta de Google.
2. Crea una API key y cópiala. Es tuya: no la compartas ni la pegues en el chat.

## 5. Averigua la IP de tu compu
1. Abre **Símbolo del sistema** (busca `cmd` en el menú Inicio).
2. Escribe `ipconfig` y busca **Dirección IPv4** del WiFi, algo como `192.168.1.50`.

## 6. Crea tu configuración
1. En `C:\midoryu-server\data` copia `.config.yaml.example` y renómbralo a `.config.yaml`.
2. Ábrelo con el Bloc de notas y cambia dos cosas:
   - `192.168.1.50` por tu IP del paso 5.
   - `PEGA_TU_KEY_AQUI` por tu llave del paso 4.
3. En el Símbolo del sistema:
   ```
   cd C:\midoryu-server
   docker compose up -d
   docker logs -f xiaozhi-esp32-server
   ```
   Cuando los logs muestren las direcciones OTA y websocket sin errores, está listo.
4. Prueba en el navegador: `http://TU_IP:8003/xiaozhi/ota/` debe decir que la interfaz OTA funciona.
   Si Windows te pregunta por el firewall, dale **Permitir** en redes privadas.

## 7. Apunta el StackChan a tu servidor
El firmware trae una página de configuración WiFi con un campo para la dirección OTA (lo vi en el código, pero no lo he probado en el aparato). Entra al modo de configuración WiFi del StackChan, abre las opciones avanzadas y pon:
`http://TU_IP:8003/xiaozhi/ota/`
Guarda y reinicia. Si tu pantalla no tiene esa opción, avísame y te genero una imagen nueva con la dirección ya metida.

## Notas
- La compu tiene que estar prendida y en el mismo WiFi que el StackChan.
- Fija la IP de tu compu en el router para que no cambie (te guío si quieres).
- El reconocimiento de voz (SenseVoice) maneja bien inglés y chino; el español no es su fuerte.
  Por eso la voz de prueba viene en inglés. Más adelante cambiamos a un reconocedor mejor para español.
