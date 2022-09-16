#include<stdio.h>
#include"camera.h"

#define DEVICE_NAME "/dev/video0"
#define FILE_NAME "./out.yuv"

int width_arg = 0;
int height_arg = 0;
char *user_device_path = NULL;
char *user_fmt = NULL;

int main(int argc, char **argv)
{
	int result = 0;
	int ret = 0;
	int i;
    while((result = getopt(argc, argv, "d:w:h:f:")) != -1 )
    {
           switch(result)
          {
              case 'd':
                   user_device_path = optarg;
                   break;
              case 'w':
                   width_arg = atoi(optarg);
                   break;
              case 'h':
                   height_arg = atoi(optarg);
                   break;
              case 'f':
                   user_fmt = optarg;
                   //printf("%c %c %c %c\n", user_fmt[0], user_fmt[1], user_fmt[2], user_fmt[3]);
                   break;  
              default:
                   printf("Use the default device = %s\n", DEVICE_NAME);
                   user_device_path = DEVICE_NAME;
                   break;
           }
        //printf("argv[%d]=%s\n", optind, argv[optind]);
    }
    printf("The device parameter you passed in is = %s\n", user_device_path);
	
	ret = open_device(user_device_path);
	if (ret == -1) 
		exit(EXIT_FAILURE);
	open_file(FILE_NAME);
	ret = init_device();
	if (ret == -1) 
		exit(EXIT_FAILURE);
	init_mmap();
	start_stream();

	for (i = 0 ; i < 1; i++)
	{
		process_frame();	
		printf("frame:%d\n",i);
	}

	end_stream();
	close_mmap();
	close_device();
	
	
	return 0;
}