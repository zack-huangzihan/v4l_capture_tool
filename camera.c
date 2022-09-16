#include"camera.h"
#include <linux/i2c.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

int fd;
int file_fd;
int frame_size;
static Video_Buffer * buffer = NULL;
FILE * file = NULL;

extern int width_arg;
extern int height_arg;
extern char *user_fmt;

int open_device(const char * device_name)
{
	struct stat st;
    if( -1 == stat( device_name, &st ) )
    {
        printf( "Cannot identify '%s'\n" , device_name );
        return -1;
    }

    if ( !S_ISCHR( st.st_mode ) )
    {
        printf( "%s is no device\n" , device_name );
        return -1;
    }

    fd = v4l2_open(device_name, O_RDWR | O_NONBLOCK , 0);
    if ( -1 == fd )
    {
        printf( "Cannot open '%s'\n" , device_name );
        return -1;
    }
    return 0;	
}



int init_device(void)
{
	//查询设备信息
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmt_query;
	if (v4l2_ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
	{
		perror("VIDIOC_QUERYCAP");
		return -1;
	}
	printf("DriverName:%s\nCard Name:%s\nBus info:%s\nDriverVersion:%u.%u.%u\n",
		cap.driver,cap.card,cap.bus_info,(cap.version>>16)&0xFF,(cap.version>>8)&0xFF,(cap.version)&0xFF);
	printf("Capabilities:\t%x\n", cap.capabilities);	
	if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) && !(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
		printf("Device %s: supports capture.\n",cap.card);
	}else if ((cap.capabilities & V4L2_CAP_STREAMING) && !(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
		printf("Device %s: supports streaming.\n",cap.card);
	} else {
		printf("Device %s: not supports capture and streaming.\n",cap.card);
		return -1;
	}

	printf("Device support format:\n");
	fmt_query.index = 0;
 	fmt_query.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	for(; v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &fmt_query) == 0; ++fmt_query.index) {
		printf("\t%d.%s \n",fmt_query.index+1, fmt_query.description);
	}

	// set fmt
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_format fmt;
	struct v4l2_format out_fmt;
	int index = 0;
	/* Lists the pixel formats supported by the camera */
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (user_fmt != NULL) {

        printf("try format:  %s \n", user_fmt);
       	CLEAN(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (height_arg == 0 || width_arg == 0) {
        	fmt.fmt.pix.width = 640;
        	fmt.fmt.pix.height = 320;	
        }else {
        	fmt.fmt.pix.width = height_arg;
        	fmt.fmt.pix.height = width_arg;
        } 
        fmt.fmt.pix.pixelformat = v4l2_fourcc(user_fmt[0], user_fmt[1], user_fmt[2], user_fmt[3]);
        if (v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		    printf("VIDIOC_S_FMT IS ERROR! LINE:%d\n",__LINE__);
		    return -1;
		}
		//查看帧格式
		out_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if ( v4l2_ioctl(fd, VIDIOC_G_FMT, &out_fmt) < 0){
			printf("VIDIOC_G_FMT IS ERROR! LINE:%d\n", __LINE__);
			return -1;
		}
		printf("width:%d\nheight:%d\npixelformat:%c%c%c%c\n",
            	out_fmt.fmt.pix.width, out_fmt.fmt.pix.height,
            	out_fmt.fmt.pix.pixelformat &  0xFF,
            	(out_fmt.fmt.pix.pixelformat >> 8) & 0xFF,
            	(out_fmt.fmt.pix.pixelformat >> 16) & 0xFF,
            	(out_fmt.fmt.pix.pixelformat >> 24) & 0xFF
        );
        printf("frame_size = %d\n", fmt.fmt.pix.sizeimage);
        frame_size = fmt.fmt.pix.sizeimage;
        return 0;
    }
    for(; v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0; ++fmtdesc.index) {
        //printf("\t%d.%s  %x\n",fmtdesc.index+1, fmtdesc.description, fmtdesc.pixelformat);
        v4l2_frmsizeenum framesize = {};
        framesize.pixel_format = fmtdesc.pixelformat;
        for (; v4l2_ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &framesize) == 0;  ++framesize.index) {
        	printf("frame_size type: %x Range of resolution width: %d height: %d\n", framesize.type, framesize.discrete.width, framesize.discrete.height);
        	if (framesize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
        		printf("try format: %s %x width: %d height: %d \n", fmtdesc.description, fmtdesc.pixelformat, framesize.discrete.width, framesize.discrete.height);
        		CLEAN(fmt);
        		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		fmt.fmt.pix.width = framesize.discrete.width;
        		fmt.fmt.pix.height = framesize.discrete.height;
        		fmt.fmt.pix.pixelformat = fmtdesc.pixelformat;

        		if (v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		    		printf("VIDIOC_S_FMT IS ERROR! LINE:%d\n",__LINE__);
		    		return -1;
				}
				//查看帧格式
				out_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				if ( v4l2_ioctl(fd, VIDIOC_G_FMT, &out_fmt) < 0){
					printf("VIDIOC_G_FMT IS ERROR! LINE:%d\n", __LINE__);
					return -1;
				}
				printf("width:%d\nheight:%d\npixelformat:%c%c%c%c\n",
            			out_fmt.fmt.pix.width, out_fmt.fmt.pix.height,
            			out_fmt.fmt.pix.pixelformat &  0xFF,
            			(out_fmt.fmt.pix.pixelformat >> 8) & 0xFF,
            			(out_fmt.fmt.pix.pixelformat >> 16) & 0xFF,
            			(out_fmt.fmt.pix.pixelformat >> 24) & 0xFF
            	);
            	printf("frame_size = %d\n", fmt.fmt.pix.sizeimage);
            	frame_size = fmt.fmt.pix.sizeimage;
				printf("find support fmt\n");
				return 0;
        	}else if (framesize.type == V4L2_FRMSIZE_TYPE_STEPWISE || framesize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
        		printf("not support V4L2_FRMSIZE_TYPE_STEPWISE and V4L2_FRMSIZE_TYPE_CONTINUOUS, Use user set resolution\n");
       			CLEAN(fmt);
        		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		if (height_arg == 0 || width_arg == 0) {
        			fmt.fmt.pix.width = WIDTH;
        			fmt.fmt.pix.height = HEIGHT;	
        		}else {
        			fmt.fmt.pix.width = height_arg;
        			fmt.fmt.pix.height = width_arg;
        		}
        		fmt.fmt.pix.pixelformat = fmtdesc.pixelformat;
        		if (v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		    		printf("VIDIOC_S_FMT IS ERROR! LINE:%d\n",__LINE__);
		    		return -1;
				}
				//查看帧格式
				out_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				if ( v4l2_ioctl(fd, VIDIOC_G_FMT, &out_fmt) < 0){
					printf("VIDIOC_G_FMT IS ERROR! LINE:%d\n", __LINE__);
					return -1;
				}
				printf("width:%d\nheight:%d\npixelformat:%c%c%c%c\n",
            			out_fmt.fmt.pix.width, out_fmt.fmt.pix.height,
            			out_fmt.fmt.pix.pixelformat &  0xFF,
            			(out_fmt.fmt.pix.pixelformat >> 8) & 0xFF,
            			(out_fmt.fmt.pix.pixelformat >> 16) & 0xFF,
            			(out_fmt.fmt.pix.pixelformat >> 24) & 0xFF
            	);
            	printf("frame_size = %d\n", fmt.fmt.pix.sizeimage);
            	frame_size = fmt.fmt.pix.sizeimage;
            	return 0;
        	}
        }
    }
    printf("init_device is fail\n");
	return -1;

}

 int init_mmap()
{
	//申请帧缓冲区
	struct v4l2_requestbuffers req;
	CLEAN(req);
	req.count = 1;
	req.memory = V4L2_MEMORY_MMAP;  //使用内存映射缓冲区
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	//申请4个帧缓冲区，在内核空间中
	if ( v4l2_ioctl(fd, VIDIOC_REQBUFS, &req) < 0 ) 
	{
		printf("VIDIOC_REQBUFS IS ERROR! LINE:%d\n",__LINE__);
		return -1;
	}
	//获取每个帧信息，并映射到用户空间
	buffer = (Video_Buffer *)calloc(req.count, sizeof(Video_Buffer));
	if (buffer == NULL){
		printf("calloc is error! LINE:%d\n",__LINE__);
		return -1;
	}
	
	struct v4l2_buffer buf;
	int buf_index = 0;
	for (buf_index = 0; buf_index < req.count; buf_index ++)
	{
		CLEAN(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.index = buf_index;
		buf.memory = V4L2_MEMORY_MMAP;
		if (v4l2_ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) //获取每个帧缓冲区的信息 如length和offset
		{
			printf("VIDIOC_QUERYBUF IS ERROR! LINE:%d\n",__LINE__);
			return -1;
		}
		//将内核空间中的帧缓冲区映射到用户空间
		buffer[buf_index].length = buf.length;
		buffer[buf_index].start = v4l2_mmap(NULL, //由内核分配映射的起始地址
									   buf.length,//长度
									   PROT_READ | PROT_WRITE, //可读写
									   MAP_SHARED,//可共享
									   fd,
									   buf.m.offset);
		if (buffer[buf_index].start == MAP_FAILED){
			printf("MAP_FAILED LINE:%d\n",__LINE__);
			return -1;
		}
		//将帧缓冲区放入视频输入队列
		if (v4l2_ioctl(fd, VIDIOC_QBUF, &buf) < 0)
		{
			printf("VIDIOC_QBUF IS ERROR! LINE:%d\n", __LINE__);
			return -1;
		}		
		printf("Frame buffer :%d   address :0x%x    length:%d\n",buf_index, (unsigned int*)buffer[buf_index].start, buffer[buf_index].length);
	}	
}

void start_stream()
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (v4l2_ioctl(fd, VIDIOC_STREAMON, &type) < 0){
		printf("VIDIOC_STREAMON IS ERROR! LINE:%d\n", __LINE__);
		exit(EXIT_FAILURE);
	}
}
void end_stream()
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (v4l2_ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
		printf("VIDIOC_STREAMOFF IS ERROR! LINE:%d\n", __LINE__);
		exit(EXIT_FAILURE);
	}
}

static int read_frame()
{
	struct v4l2_buffer buf;
	int ret = 0;
	CLEAN(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (v4l2_ioctl(fd, VIDIOC_DQBUF, &buf) < 0){
		printf("VIDIOC_DQBUF! LINEL:%d\n", __LINE__);
		return -1;
	}
	ret = write(file_fd, buffer[buf.index].start ,frame_size);
	if (ret == -1)
	{
		printf("write is error !\n");
		return -1;
	}
	if (v4l2_ioctl(fd, VIDIOC_QBUF, &buf) < 0){
		printf("VIDIOC_QBUF! LINE:%d\n", __LINE__);
		return -1;
	}
	return 0;
}


int open_file(const char * file_name)
{
	
	file_fd = open(file_name, O_RDWR | O_CREAT, 0777);
	if (file_fd == -1)
	{
		printf("open file is error! LINE:%d\n", __LINE__);
		return -1;
	}
		
//	file = fopen(file_name, "wr+");
}

void close_mmap()
{
	int i = 0;
	for (i = 0; i < 4 ; i++)
	{
		v4l2_munmap(buffer[i].start, buffer[i].length);
	}
	free(buffer);
}
void close_device()
{
	v4l2_close(fd);
	close(file_fd);
}
int process_frame()
{
	struct timeval tvptr;
    int ret;
	tvptr.tv_usec = 0;  //等待50 us
    tvptr.tv_sec = 2;
    fd_set fdread;
    FD_ZERO(&fdread);
    FD_SET(fd, &fdread);
    ret = select(fd + 1, &fdread, NULL, NULL, &tvptr);
    if (ret == -1){
        perror("EXIT_FAILURE");
        exit(EXIT_FAILURE);
    }
	if (ret == 0){
		printf("timeout! \n");
	}
	read_frame();
}