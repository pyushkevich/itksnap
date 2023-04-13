FROM opensuse/leap:15.4
RUN zypper install -y -t pattern devel_basis
RUN zypper install -y git cmake wget vim gzip
RUN zypper install -y libX11-devel libxkbcommon-devel libxkbcommon-x11-devel \
                      fontconfig-devel freetype-devel libXext-devel \
                      libXfixes-devel libXi-devel libXrender-devel \
                      libxcb-cursor0 libxcb-keysyms1 libxcb-image0 libxcb-icccm4 libxcb-render-util0 xcb-util-devel \
                      xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-image-devel xcb-util-wm-devel xcb-util-cursor-devel \
                      gcc9-c++ Mesa-devel ninja \
                      libcurl-devel xvfb-run ImageMagick xwd 

# Set to use GCC 9
ENV CC=gcc-9
ENV CXX=g++-9

# Get the Qt sources
ARG QTVER=6.4.3
RUN mkdir -p /qt/src
WORKDIR /qt/src
RUN wget https://download.qt.io/official_releases/qt/6.4/$QTVER/submodules/qtbase-everywhere-src-$QTVER.tar.xz
RUN tar -xf qtbase-everywhere-src-$QTVER.tar.xz

# Critically, xcb has to be enabled
RUN cd qtbase-everywhere-src-$QTVER \
    && ./configure -xcb \
    && cmake --build . --parallel \
    && cmake --install .

# Build Qml (there is some unexplained crash when building, which is why I call build twice)
WORKDIR /qt/src
RUN wget https://download.qt.io/official_releases/qt/6.4/$QTVER/submodules/qtdeclarative-everywhere-src-$QTVER.tar.xz
RUN tar -xf qtdeclarative-everywhere-src-$QTVER.tar.xz
WORKDIR /qt/src/qtdeclarative-everywhere-src-$QTVER
RUN cmake -G Ninja -DCMAKE_PREFIX_PATH=/usr/local/Qt-${QTVER}/lib/cmake . 
RUN cmake --build . --parallel || :
RUN cmake --build . --parallel 
RUN cmake --install .

# Install ITK
# Download and build ITK
RUN git clone https://github.com/Kitware/ITK.git /tk/itk/src
RUN cd /tk/itk/src && git checkout v5.2.1
RUN mkdir /tk/itk/build
WORKDIR /tk/itk/build
RUN cmake -G Ninja  \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DModule_MorphologicalContourInterpolation=ON \
    /tk/itk/src
RUN cmake --build . --parallel

# Install VTK
RUN git clone https://gitlab.kitware.com/vtk/vtk.git /tk/vtk/src
RUN cd /tk/vtk/src && git checkout v9.1.0
RUN mkdir /tk/vtk/build
WORKDIR /tk/vtk/build
RUN cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DVTK_USE_SYSTEM_LIBRARIES=ON \
    -DVTK_RENDERING_BACKEND=OpenGL2 \
    -DVTK_REQUIRED_OBJCXX_FLAGS= \
    -DBUILD_TESTING=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DVTK_GROUP_ENABLE_Qt=YES \
    -DVTK_MODULE_ENABLE_VTK_GUISupportQtQuick=NO \
    -DVTK_MODULE_ENABLE_VTK_GUISupportQtSQL=NO \
    -DCMAKE_PREFIX_PATH=/usr/local/Qt-${QTVER}/lib/cmake \
    /tk/vtk/src
RUN cmake --build . --parallel

# Go to the tk directory
WORKDIR /tk