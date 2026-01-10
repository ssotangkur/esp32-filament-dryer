FROM espressif/idf:v5.5.1

# Install pip and testing dependencies in ESP-IDF environment
RUN apt-get update && apt-get install -y python3-pip python3-venv && \
  . /opt/esp/python_env/idf5.5_py3.12_env/bin/activate && \
  pip install pytest pytest-embedded-serial pytest-embedded-idf pytest-embedded-qemu

# Set working directory
WORKDIR /project

# Default command
CMD ["/bin/bash"]
