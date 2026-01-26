FROM emscripten/emsdk:latest

# Install additional build tools
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  git \
  wget \
  unzip \
  python3 \
  && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /project

# Copy only the essential project files needed for the build
# COPY sdkconfig.h ./sdkconfig.h

# Set the working directory to ui_simulator for the build
WORKDIR /project/ui_simulator

# Command to run the build when container is started
CMD ["bash", "-c", "cd /emsdk && . ./emsdk_env.sh && cd /project/ui_simulator && mkdir -p /project/build && emcmake cmake . -B /project/build && emmake make -C /project/build -j4 && echo 'Build completed successfully! Find the output in ui_simulator/build/'"]
