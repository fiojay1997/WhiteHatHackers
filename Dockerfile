FROM ubuntu

SHELL ["/bin/bash", "-c"]

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ="America/Salt Lake City"

RUN  apt-get -y update && apt-get -y install  --no-install-recommends\
  ffmpeg \
  vim \
  autoconf \
  automake \
  build-essential \
  cmake \
  git-core \
  libx264-dev \
  libass-dev \
  libfreetype6-dev \
  libgnutls28-dev \
  libtool \
  libsdl2-dev \
  libva-dev \
  libvdpau-dev \
  libxcb1-dev \
  libxcb-shm0-dev \
  libxcb-xfixes0-dev \
  libvorbis-dev \
  libcairo2-dev \
  pkg-config \
  texinfo \
  wget \
  yasm \
  zlib1g-dev \
  python3.5 \
  python3-pip \
  && \
  apt-get clean && \
  rm -rf /var/lib/apt/lists/*

RUN mkdir -p /cs3505/ffmpeg
WORKDIR /cs3505
RUN git  clone git://source.ffmpeg.org/ffmpeg.git 
WORKDIR /cs3505/ffmpeg
RUN git reset --hard  f7fd205f11d5299d6a16b0ff0ae85fddb32772f2
RUN ./configure --prefix=/ --enable-gpl --enable-libx264
RUN make 
RUN make install
RUN pip3 install Pillow ; pip3 install awscli
RUN python3 download_img.py
