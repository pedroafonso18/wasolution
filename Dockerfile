# Use Ubuntu 22.04 as base image
FROM ubuntu:22.04

# Set environment variables to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    libboost-all-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    zlib1g-dev \
    libpq-dev \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy CMakeLists.txt first for better layer caching
COPY CMakeLists.txt .

# Copy source code
COPY src/ ./src/
COPY dependencies/ ./dependencies/

# Create build directory
RUN mkdir -p build

# Build the project
WORKDIR /app/build
RUN cmake .. \
    && make -j$(nproc)

# Create a runtime stage for a smaller final image
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libcurl4 \
    libssl3 \
    zlib1g \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

# Copy the built executable from the build stage
COPY --from=0 /app/build/wasolution /usr/local/bin/

# Set the entry point
ENTRYPOINT ["wasolution"] 