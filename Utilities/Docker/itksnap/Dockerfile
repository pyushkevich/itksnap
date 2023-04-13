# Initialize from the base image
FROM pyushkevich/itksnap-base:4.0

# Copy the sources
COPY . /tk/itksnap/src
RUN mkdir /tk/itksnap/build
WORKDIR /tk/itksnap/build

# Configure
ARG QTVER=6.4.3
RUN cmake \
    -G Ninja \
    -DCMAKE_PREFIX_PATH=/usr/local/Qt-${QTVER}/lib/cmake \
    -DITK_DIR=/tk/itk/build \
    -DVTK_DIR=/tk/vtk/build \
    /tk/itksnap/src
RUN cmake --build . --parallel

# Set up for testing
RUN zypper install -y xvfb-run