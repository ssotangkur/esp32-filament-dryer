FROM emscripten/emsdk:latest

# Install additional build tools including ccache for faster incremental builds
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  git \
  wget \
  unzip \
  python3 \
  ccache \
  && rm -rf /var/lib/apt/lists/*

# Set up ccache configuration
ENV CC="ccache emcc" \
    CXX="ccache em++" \
    CCACHE_COMPRESS=1 \
    CCACHE_MAXSIZE=1G

# Set working directory
WORKDIR /project

# Copy managed_components for pre-building (these change infrequently)
COPY managed_components /project/managed_components

# Copy UI simulator configuration files
COPY ui_simulator/docker_root/lv_conf.h /project/ui_simulator/docker_root/
COPY ui_simulator/docker_root/CMakeLists_prebuilt.txt /project/ui_simulator/docker_root/
COPY ui_simulator/docker_root/prebuild_libs.sh /tmp/prebuild_libs.sh

# Make prebuild script executable and run it to create cached libraries
RUN chmod +x /tmp/prebuild_libs.sh && /tmp/prebuild_libs.sh

# Set the working directory to ui_simulator for the build
WORKDIR /project/ui_simulator

# Create a script that handles both full and incremental builds
COPY ui_simulator/docker_root/build_script.sh /tmp/build_script.sh
RUN chmod +x /tmp/build_script.sh

# Command to run the build when container is started
CMD ["/tmp/build_script.sh"]
