#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <string.h>
#include "camera_config.h"

int camera_open_dev()
{
	int camera_fd = -1;

	cam_conf_set_camera_fd(-1);
	camera_fd = open("/dev/video0", O_RDWR);
	if (camera_fd < 0)
	{
		printf("open dev error\n");
		return -1;
	}

	cam_conf_set_camera_fd(camera_fd);

	return 0;
}

int camera_init()
{
	struct v4l2_capability camera_capa;
	struct v4l2_fmtdesc camera_fmtdesc;
	struct v4l2_format camera_format;
	struct v4l2_requestbuffers camera_reqbuff;
	struct v4l2_buffer camera_buffer;
	struct camera_share_mem tmp_mem;
	struct camera_share_mem tmp_v_mem;
	
	int ret;
	int i;
	int camera_fd = cam_conf_get_camera_fd(NULL);
		
	ret = ioctl(camera_fd, VIDIOC_QUERYCAP, &camera_capa);
	if (ret > 0)
	{
		print("ioctl VIDIOC_QUERYCAP error\n");
	}
	print("driver:\t%s\n", camera_capa.driver);
	print("card:\t%s\n", camera_capa.card);
	print("bus_info:\t%s\n", camera_capa.bus_info);
	print("version:\t%d\n", camera_capa.version);
	print("capabilities:\t%x\n", camera_capa.capabilities); //V4L2_CAP_VIDEO_CAPTURE 
	printf("\n");
	//支持的格式
	camera_fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	for (camera_fmtdesc.index = 0; -1 != ioctl(camera_fd, VIDIOC_ENUM_FMT, &camera_fmtdesc); camera_fmtdesc.index++)
	{
		print("camera_fmtdesc.index:\t%d\n", camera_fmtdesc.index);
		print("camera_fmtdesc.type:\t%x\n", camera_fmtdesc.type);
		print("camera_fmtdesc.flags:\t%x\n", camera_fmtdesc.flags);
		print("camera_fmtdesc.description:\t%s\n", camera_fmtdesc.description);
		print("camera_fmtdesc.pixelformat:\t%x\n", camera_fmtdesc.pixelformat);
	}
	printf("\n");

	//当前帧信息
	camera_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(camera_fd, VIDIOC_G_FMT, &camera_format)< 0)
	{
		print("ioctl VIDIOC_G_FMT error\n");
	}
	
	print("camera_format.fmt.pix.width:\t%d\n", camera_format.fmt.pix.width);
	print("camera_format.fmt.pix.height:\t%d\n", camera_format.fmt.pix.height);
	print("camera_format.fmt.pix.pixelformat:\t%d\n", camera_format.fmt.pix.pixelformat);
	print("camera_format.fmt.pix.sizeimage:\t%d\n", camera_format.fmt.pix.sizeimage);	
	printf("\n");

	//设置为指定像素
	camera_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	camera_format.fmt.pix.width = 640;
	camera_format.fmt.pix.height= 480;
	
	if(ioctl(camera_fd, VIDIOC_S_FMT, &camera_format)< 0)
	{
		print("ioctl VIDIOC_S_FMT error\n");
	}
	else
	{
		print("set pixel as %d * %d ok\n", camera_format.fmt.pix.width, camera_format.fmt.pix.height);
		cam_conf_set_width(camera_format.fmt.pix.width);
		cam_conf_set_height(camera_format.fmt.pix.height);
	}

	//申请内存映射
	camera_reqbuff.count = 1; //申请4块内存
	camera_reqbuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	camera_reqbuff.memory = V4L2_MEMORY_MMAP;
	if(ioctl(camera_fd, VIDIOC_REQBUFS, &camera_reqbuff)< 0)
	{
		print("ioctl VIDIOC_REQBUFS error\n");
		return -1;
	}
	print("VIDIOC_REQBUFS ok\n");

	camera_buffer.index = 0;
	camera_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	camera_buffer.memory = V4L2_MEMORY_MMAP;
	if(ioctl(camera_fd, VIDIOC_QUERYBUF, &camera_buffer)< 0)
	{
		print("ioctl VIDIOC_REQBUFS error\n");
		return -1;
	}
	tmp_mem.offset = camera_buffer.m.offset;
	tmp_mem.mem_len = camera_buffer.length;
	print("VIDIOC_QUERYBUF ok\n");
	cam_conf_set_p_share_mem_info(&tmp_mem);
	print("cam_conf_set_p_share_mem_info ok\n");

	ret = cam_conf_get_p_share_mem_info(&tmp_mem);
	if (ret < 0)
	{
		print("cam_conf_get_p_share_mem_info\n");
		return -1;
	}
	print("cam_conf_get_p_share_mem_info ok\n");
	tmp_v_mem.mem_addr = mmap(NULL, tmp_mem.mem_len, PROT_READ | PROT_WRITE, MAP_SHARED, camera_fd, tmp_mem.offset);
	if (tmp_v_mem.mem_addr != NULL)
	{
		print("mem mmap success, addr : %p\n", tmp_v_mem.mem_addr );
		cam_conf_set_v_share_mem_info(&tmp_v_mem);
	}
	else
	{
		print("mem mmap failed");
		cam_conf_set_v_share_mem_info(&tmp_v_mem);
	}	

	return 0;
}

void *net_work_thread(void * arg)
{
	//监听端口，建立客户端连接
	for ( ; ; )
	{
		
	}
}

void *stream_recv_thread(void *arg)
{
	int v4l2type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret;
	fd_set rfds;
	struct timeval outtime;
	int camera_fd = cam_conf_get_camera_fd(NULL);
	struct v4l2_buffer camera_qbuff;
	struct v4l2_buffer camera_dqbuff;
	
	print("start stream_recv thread\n");

	memset(&camera_qbuff, 0, sizeof(camera_qbuff));
	camera_qbuff.index = 0;
	camera_qbuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	camera_qbuff.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(camera_fd, VIDIOC_QBUF, &camera_qbuff);
	if (ret < 0)
	{
		print("VIDIOC_QBUF error\n");
		return NULL;
	}

	ret = ioctl(camera_fd, VIDIOC_STREAMON, &v4l2type);
	if(ret < 0)
	{
		print("VIDIOC_STREAMON error\n");
		return NULL;
	}
	//取流
	for ( ; ; )
	{
		FD_ZERO(&rfds); 
		FD_SET(camera_fd, &rfds); 
		outtime.tv_sec = 1;       /* Timeout. */ 
		outtime.tv_usec = 0; 

		ret = select(camera_fd+1, &rfds, NULL, NULL, &outtime); 
		if (ret == -1)
		{
			print("camera fd select error\n");
		}
		else if (ret == 0)
		{
			//time out
			print("strem recv time out\n");
		}
		else
		{
			//data is ready
			print("data is ready\n");
			memset(&camera_dqbuff, 0, sizeof(camera_dqbuff));
			camera_dqbuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			camera_dqbuff.memory = V4L2_MEMORY_MMAP;
			ret = ioctl(camera_fd, VIDIOC_DQBUF, &camera_dqbuff);
			if (ret < 0)
			{
				print("VIDIOC_QBUF error\n");
			}
			else
			{
				//取流到共享缓冲区
				copy_image_into_share_mem();
				//重新放入队列
				memset(&camera_qbuff, 0, sizeof(camera_qbuff));
				camera_qbuff.index = 0;
				camera_qbuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				camera_qbuff.memory = V4L2_MEMORY_MMAP;
				ret = ioctl(camera_fd, VIDIOC_QBUF, &camera_qbuff);
				if (ret < 0)
				{
					print("VIDIOC_QBUF error\n");
					return NULL;
				}
			}
		}
	}

	v4l2type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
	ret = ioctl(camera_fd, VIDIOC_STREAMOFF, &v4l2type);
}

int main(int argc, char * argv[])
{
	pthread_t net_thread_pid;
	pthread_t stream_pid;
	int ret;

	camera_conf_init(); 
	ret= camera_open_dev();
	if (ret < 0)
	{
		print("open camera device error\n");
		return -1;
	}

	//摄像头初始化
	camera_init();

	//启动网络线程
	pthread_create(&net_thread_pid, NULL,net_work_thread, NULL);
	//启动取流线程
	pthread_create(&stream_pid, NULL,stream_recv_thread, NULL);

	while(1)
	{
		sleep(1);
	}

	return 0;
}
