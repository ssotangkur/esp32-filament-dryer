import React, { useState, useEffect } from 'react';
import { ThemeProvider, createTheme } from '@mui/material/styles';
import { Container, Typography, Paper, Box } from '@mui/material';
import { LineChart } from '@mui/x-charts/LineChart';

const theme = createTheme();

function App() {
  const [version, setVersion] = useState('Loading...');
  const [airData, setAirData] = useState([]);
  const [heaterData, setHeaterData] = useState([]);

  useEffect(() => {
    // Fetch firmware version
    fetch('/api/version')
      .then(res => res.text())
      .then(setVersion)
      .catch(err => setVersion('Error loading version'));

    // WebSocket for sensor data
    const ws = new WebSocket(`ws://${window.location.host}/ws/sensor-data`);
    ws.onopen = () => {
      // Send a message to request data
      ws.send('get_data');
    };
    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      data.forEach(point => {
        if (point.sensor === 'air') {
          setAirData(prev => [...prev, { x: point.timestamp, y: point.temperature }]);
        } else if (point.sensor === 'heater') {
          setHeaterData(prev => [...prev, { x: point.timestamp, y: point.temperature }]);
        }
      });
    };

    return () => ws.close();
  }, []);

  return (
    <ThemeProvider theme={theme}>
      <Container maxWidth="md" sx={{ mt: 4 }}>
        <Typography variant="h4" component="h1" gutterBottom>
          ESP32 Filament Dryer
        </Typography>

        <Paper sx={{ p: 2, mb: 2 }}>
          <Typography variant="h6">Firmware Version</Typography>
          <Typography>{version}</Typography>
        </Paper>

        <Paper sx={{ p: 2, mb: 2 }}>
          <Typography variant="h6" gutterBottom>Air Temperature</Typography>
          {airData.length > 0 ? (
            <LineChart
              xAxis={[{ dataKey: 'x', type: 'number' }]}
              series={[{ dataKey: 'y', label: 'Air (°C)' }]}
              dataset={airData}
              width={500}
              height={300}
            />
          ) : (
            <Typography>Waiting for air sensor data...</Typography>
          )}
        </Paper>

        <Paper sx={{ p: 2 }}>
          <Typography variant="h6" gutterBottom>Heater Temperature</Typography>
          {heaterData.length > 0 ? (
            <LineChart
              xAxis={[{ dataKey: 'x', type: 'number' }]}
              series={[{ dataKey: 'y', label: 'Heater (°C)' }]}
              dataset={heaterData}
              width={500}
              height={300}
            />
          ) : (
            <Typography>Waiting for heater sensor data...</Typography>
          )}
        </Paper>
      </Container>
    </ThemeProvider>
  );
}

export default App;
