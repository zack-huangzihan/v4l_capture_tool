CFLAGS += -g -Wall
LD_FLAGS += -lpthread -lm -ldl -lv4l2 -lv4l1
CC := g++
all:
	@$(CC) $(CFLAGS) main.c camera.c -o v4l_capture_tool  $(LD_FLAGS)        

install:
	@mv -f v4l_capture_tool  /usr/bin

clean:
	@rm -f v4l_capture_tool
