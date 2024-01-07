
.PHONY: all clean

all: xorg_calibrator

CFLAGS += -g
CFLAGS += -I/usr/include/freetype2 -DHAVE_XFT
# CFLAGS += -DNDEBUG
CXXFLAGS += -std=c++11

LDFLAGS += -lX11
LDFLAGS += -lXft
LDFLAGS += -lXi

xorg_calibrator: xorg_calibrator.cpp touch_device.cpp transform_matrix.cpp
	$(CXX) -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

screen_x11_test: screen_x11_test.cpp
	$(CXX) -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

touch_device_test: touch_device.cpp transform_matrix.cpp
	$(CXX) -o $@ $^ \
		-DTOUCH_DEVICE_TEST \
		$(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f xorg_calibrator screen_x11_test touch_device_test
