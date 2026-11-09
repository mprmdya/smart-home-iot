const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const mqtt = require('mqtt');
const cors = require('cors');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const PORT = process.env.PORT || 5000;
const MQTT_BROKER = process.env.MQTT_BROKER_URL || 'mqtt://broker.emqx.io:1883';

app.use(cors());
app.use(express.json());

// Hubungkan ke Broker MQTT
const mqttClient = mqtt.connect(MQTT_BROKER, {
  clientId: `SmartHomeServer_${Math.random().toString(16).substring(2, 8)}`
});

let latestTelemetry = { temperature: 0, humidity: 0, autoFan: true, uptimeSec: 0 };
let latestDevices = { light: "OFF", fan: "OFF", auto: "ON" };

mqttClient.on('connect', () => {
  console.log(`[MQTT] Terhubung ke Broker: ${MQTT_BROKER}`);
  mqttClient.subscribe('smarthome/state/#');
});

mqttClient.on('message', (topic, message) => {
  try {
    const data = JSON.parse(message.toString());
    if (topic === 'smarthome/state/telemetry') {
      latestTelemetry = { ...latestTelemetry, ...data };
    } else if (topic === 'smarthome/state/devices') {
      latestDevices = { ...latestDevices, ...data };
    }

    // Broadcast data ke semua client WebSocket yang terhubung
    const payload = JSON.stringify({ topic, data });
    wss.clients.forEach(client => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(payload);
      }
    });
  } catch (err) {
    console.error(`Error parsing message on ${topic}:`, err.message);
  }
});

// REST API Endpoints
app.get('/api/health', (req, res) => {
  res.json({ status: 'OK', mqttConnected: mqttClient.connected });
});

app.get('/api/state', (req, res) => {
  res.json({ telemetry: latestTelemetry, devices: latestDevices });
});

app.post('/api/control', (req, res) => {
  const { device, action } = req.body;
  if (!['light', 'fan', 'auto'].includes(device)) {
    return res.status(400).json({ error: 'Device tidak valid. Gunakan: light, fan, atau auto' });
  }

  const topic = `smarthome/control/${device}`;
  mqttClient.publish(topic, String(action).toUpperCase());
  res.json({ message: `Perintah '${action}' berhasil dikirim ke '${device}'`, topic });
});

// WebSocket Handler
wss.on('connection', (ws) => {
  console.log('[WebSocket] Client terhubung');
  // Kirim status terkini segera setelah client tersambung
  ws.send(JSON.stringify({ topic: 'smarthome/state/telemetry', data: latestTelemetry }));
  ws.send(JSON.stringify({ topic: 'smarthome/state/devices', data: latestDevices }));

  ws.on('close', () => console.log('[WebSocket] Client terputus'));
});

server.listen(PORT, () => {
  console.log(`[Server] Smart Home Bridge berjalan pada port ${PORT}`);
});
