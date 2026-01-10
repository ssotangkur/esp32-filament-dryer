FROM espressif/idf:v5.5.1

# Install additional tools needed for unit testing
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  ruby \
  ruby-dev \
  bundler \
  git \
  && rm -rf /var/lib/apt/lists/*

# Install Unity (required by CMock)
RUN git clone https://github.com/ThrowTheSwitch/Unity.git /opt/unity

# Install CMock from GitHub
RUN git clone https://github.com/ThrowTheSwitch/CMock.git /opt/cmock && \
  cd /opt/cmock && \
  bundle install

# Set working directory
WORKDIR /project

# Set the default command to run unit tests
CMD ["bash", "-c", "cd unit_tests && ./run_unit_tests.sh"]
