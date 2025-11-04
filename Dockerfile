# Minimal image to build and run hashbrowns benchmarks
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# OS deps
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential cmake git python3 python3-pip \
      ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Workdir
WORKDIR /app

# Copy sources
COPY . /app

# Optional: Python plotting deps
RUN pip3 install --no-cache-dir -r requirements.txt || true

# Build
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --config Release -j$(nproc)

# Default command: run quick benchmarks and plots
CMD ["bash", "scripts/run_benchmarks.sh", "--runs", "3", "--size", "10000", "--max-size", "32768", "--yscale", "mid"]
