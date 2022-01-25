FROM nvcr.io/nvidia/l4t-base:r32.2.1
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y build-essential bc
RUN groupadd -g 998 jenkins
RUN useradd -u 998 -g jenkins -s /bin/bash -m jenkins
