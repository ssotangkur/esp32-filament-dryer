FROM espressif/idf:v5.5.1

# Install additional tools needed for unit testing
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /project

# Copy the unit tests
COPY unit_tests/ /project/unit_tests/

# Set the default command to run unit tests
CMD ["bash", "-c", "cd unit_tests && ./run_unit_tests.sh"]
