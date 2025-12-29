const express = require('express');
const path = require('path');
const fs = require('fs');
const os = require('os');



const app = express();
const PORT = process.env.PORT || 3001;

// Function to get local IP address
function getLocalIPAddress() {
  const interfaces = os.networkInterfaces();
  for (const interfaceName in interfaces) {
    const interface = interfaces[interfaceName];
    for (const address of interface) {
      // Look for IPv4 addresses that are not internal (not localhost)
      if (address.family === 'IPv4' && !address.internal) {
        return address.address;
      }
    }
  }
  return 'localhost'; // Fallback if no external IP found
}

// Path to ESP32 build directory (adjust if needed)
const ESP32_BUILD_DIR = path.join(__dirname, '..', 'build', 'esp32s3');
const FIRMWARE_FILE = 'firmware.bin';
const FIRMWARE_PATH = path.join(ESP32_BUILD_DIR, FIRMWARE_FILE);

// Middleware
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// CORS middleware for future React/Vite integration
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept, Authorization');
  if (req.method === 'OPTIONS') {
    res.sendStatus(200);
  } else {
    next();
  }
});

// OTA firmware endpoint
app.get('/firmware.bin', (req, res) => {
  console.log(`[${new Date().toISOString()}] OTA request for firmware.bin from ${req.ip}`);

  // Check if firmware file exists
  if (!fs.existsSync(FIRMWARE_PATH)) {
    console.error(`Firmware file not found: ${FIRMWARE_PATH}`);
    return res.status(404).json({
      error: 'Firmware file not found',
      message: `Please build the ESP32 project first. Expected at: ${FIRMWARE_PATH}`
    });
  }

  // Get file stats
  const stats = fs.statSync(FIRMWARE_PATH);
  const fileSize = stats.size;

  console.log(`Serving firmware.bin (${(fileSize / 1024 / 1024).toFixed(2)} MB)`);

  // Set appropriate headers
  res.setHeader('Content-Type', 'application/octet-stream');
  res.setHeader('Content-Length', fileSize);
  res.setHeader('Content-Disposition', `attachment; filename="${FIRMWARE_FILE}"`);
  res.setHeader('Cache-Control', 'no-cache');

  // Stream the file
  const fileStream = fs.createReadStream(FIRMWARE_PATH);
  fileStream.pipe(res);

  fileStream.on('error', (error) => {
    console.error('Error streaming firmware file:', error);
    res.status(500).json({ error: 'Error serving firmware file' });
  });

  fileStream.on('end', () => {
    console.log(`[${new Date().toISOString()}] Firmware download completed`);
  });
});

// Version endpoint
app.get('/version', (req, res) => {
  const versionFile = path.join(ESP32_BUILD_DIR, 'version.json');

  let firmwareVersion = 'unknown';
  if (fs.existsSync(versionFile)) {
    try {
      const versionData = JSON.parse(fs.readFileSync(versionFile, 'utf8'));
      firmwareVersion = versionData.version || 'unknown';
    } catch (error) {
      console.warn('Error reading version file:', error.message);
    }
  }

  const firmwareExists = fs.existsSync(FIRMWARE_PATH);
  const firmwareSize = firmwareExists ? fs.statSync(FIRMWARE_PATH).size : 0;

  res.json({
    version: firmwareVersion,
    firmware: {
      exists: firmwareExists,
      size: firmwareSize,
      sizeMB: (firmwareSize / 1024 / 1024).toFixed(2)
    }
  });
});

// Status endpoint
app.get('/status', (req, res) => {
  const firmwareExists = fs.existsSync(FIRMWARE_PATH);
  const firmwareSize = firmwareExists ? fs.statSync(FIRMWARE_PATH).size : 0;

  res.json({
    status: 'running',
    firmware: {
      exists: firmwareExists,
      path: FIRMWARE_PATH,
      size: firmwareSize,
      sizeMB: (firmwareSize / 1024 / 1024).toFixed(2)
    },
    server: {
      port: PORT,
      uptime: process.uptime()
    }
  });
});

// Root endpoint with instructions
app.get('/', (req, res) => {
  const localIP = getLocalIPAddress();
  res.send(`
    <h1>ESP32 OTA Firmware Server</h1>
    <p>Server is running on port ${PORT}</p>
    <p><strong>Detected local IP:</strong> ${localIP}</p>

    <h2>Endpoints:</h2>
    <ul>
      <li><a href="/firmware.bin">/firmware.bin</a> - Download firmware</li>
      <li><a href="/status">/status</a> - Server status (JSON)</li>
    </ul>

    <h2>Setup:</h2>
    <ol>
      <li>Build your ESP32 project: <code>esp_idf_shell.bat idf.py build</code></li>
      <li>Update your wifi_credentials.h OTA_URL to: <code>http://${localIP}:${PORT}/firmware.bin</code></li>
      <li>Call <code>ota_update_from_url(OTA_URL)</code> from your ESP32</li>
    </ol>

    <p><em>Server started at ${new Date().toISOString()}</em></p>
  `);
});

// Start server
app.listen(PORT, '0.0.0.0', () => {
  const localIP = getLocalIPAddress();

  console.log(`\n🚀 ESP32 OTA Firmware Server running on:`);
  console.log(`   http://localhost:${PORT}`);
  console.log(`   http://127.0.0.1:${PORT}`);
  console.log(`   http://${localIP}:${PORT} (local network)`);
  console.log(`\n📁 Looking for firmware at: ${FIRMWARE_PATH}`);

  // Check if firmware exists
  if (fs.existsSync(FIRMWARE_PATH)) {
    const size = fs.statSync(FIRMWARE_PATH).size;
    console.log(`✅ Firmware found: ${(size / 1024 / 1024).toFixed(2)} MB`);
  } else {
    console.log(`⚠️  Firmware not found. Build your ESP32 project first.`);
  }

  console.log(`\n📡 OTA URL for wifi_credentials.h:`);
  console.log(`   http://${localIP}:${PORT}/firmware.bin\n`);
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n🛑 Shutting down OTA server...');
  process.exit(0);
});

process.on('SIGTERM', () => {
  console.log('\n🛑 Shutting down OTA server...');
  process.exit(0);
});
