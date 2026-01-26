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
COPY managed_components/ ./managed_components/
COPY main/ ./main/
COPY include/ ./include/
COPY ui_simulator/ ./ui_simulator/
COPY ui_simulator/lv_conf.h ./lv_conf.h
COPY ui_simulator/lv_conf.h ./ui_simulator/lv_conf.h
COPY sdkconfig.h ./sdkconfig.h
COPY ui_simulator/shell.html ./shell.html

# Copy the existing lv_conf.h file to the project root
# The lv_conf.h file has already been configured for the project

# Set the working directory to ui_simulator for the build
WORKDIR /project/ui_simulator

# Command to run the build when container is started
CMD ["bash", "-c", "cd /emsdk && . ./emsdk_env.sh && cd /project/ui_simulator && mkdir -p build && emcmake cmake . -B build && emmake make -C build -j4 && echo 'Build completed successfully! Find the output in ui_simulator/build/'"]
