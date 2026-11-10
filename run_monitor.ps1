cd "D:\Projects\esp32\esp32_filament_dryer"
.\esp_idf_shell.bat idf.py monitor 2>&1 | Out-File -FilePath "monitor_log.txt" -Encoding utf8
