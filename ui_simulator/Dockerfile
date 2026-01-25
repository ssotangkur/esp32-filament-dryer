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
COPY shell.html ./shell.html

# Create lv_conf.h using the generate script approach from LVGL codespace
RUN touch lv_conf.h && \
  python3 managed_components/lvgl__lvgl/scripts/generate_lv_conf.py \
  --template managed_components/lvgl__lvgl/lv_conf_template.h \
  --config lv_conf.h

# Set the working directory to ui_simulator for the build
WORKDIR /project/ui_simulator

# Command to run the build when container is started
CMD ["bash", "-c", "mkdir -p build && emcmake cmake . -B build -DCMAKE_BUILD_TYPE=Release && emmake make -C build -j4 && echo 'Build completed successfully!'"]
