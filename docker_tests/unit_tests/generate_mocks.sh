#!/bin/bash
# Generate CMock mocks from mock_headers/ directory
# This script is shared between standalone mock generation and full test runs

# Ensure we're in the unit_tests directory
cd "$(dirname "$0")"

mkdir -p mocks
echo 'Scanning mock_headers/ for mock_*.h files...'
for header_file in mock_headers/mock_*.h; do
  if [ -f "$header_file" ]; then
    filename=$(basename "$header_file" .h)
    echo "Generating mock for $filename.h..."
    ruby /opt/cmock/lib/cmock.rb --mock_prefix=Mock --mock_path=mocks --unity_path=/opt/unity -o cmock.yml "$header_file"
    echo "Generated mocks for $filename.h"
  fi
done
echo 'Mock generation completed!'
