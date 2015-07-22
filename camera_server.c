#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/videodev2.h>
#include <sys/mman.h>


#define print(format, ...) printf("%s %s %d --> "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

struct camera_share_mem
{
	unsigned int mem_len;
	union
	{
		unsigned int offset; //mmap use
		unsigned int * mem_addr; //v addr
	};
};

struct camera_config
{
	int width;
	int height;

	int share_buff_nums; //驱动共享内存个数
	struct camera_share_mem p_share_mem[16]; //物理数据，最大16个
	struct camera_share_mem v_share_mem[16]; //共享地址通过mmap后的映射
};

int camera_open_dev()
{
	int camera_fd = -1;

	camera_fd = open("/dev/video0", O_RDWR);
	if (camera_fd < 0)
	{
		printf("open dev error\n");
	}

	return camera_fd;
}

int camera_init(int camera_fd, struct camera_config *camera_conf)
{
	struct v4l2_capability camera_capa;
	struct v4l2_fmtdesc camera_fmtdesc;
	struct v4l2_format camera_format;
	struct v4l2_requestbuffers camera_reqbuff;
	struct v4l2_buffer camera_buffer;
	
	int ret;
	int i;

	if (camera_conf == NULL)
	{
		return -1;
	}
	
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
		camera_conf->width = camera_format.fmt.pix.width;
		camera_conf->height = camera_format.fmt.pix.height;
	}

	//申请内存映射
	camera_reqbuff.count = 4; //申请4块内存
	camera_reqbuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	camera_reqbuff.memory = V4L2_MEMORY_MMAP;
	if(ioctl(camera_fd, VIDIOC_REQBUFS, &camera_reqbuff)< 0)
	{
		print("ioctl VIDIOC_REQBUFS error\n");
		return -1;
	}
	print("VIDIOC_REQBUFS ok\n");
	camera_conf->share_buff_nums = camera_reqbuff.count;

	for (i=0; i< camera_reqbuff.count; i++)
	{
		camera_buffer.index = i;
		camera_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		camera_buffer.memory = V4L2_MEMORY_MMAP;
		if(ioctl(camera_fd, VIDIOC_QUERYBUF, &camera_buffer)< 0)
		{
			print("ioctl VIDIOC_REQBUFS error\n");
			return -1;
		}
		camera_conf->p_share_mem[i].offset = camera_buffer.m.offset;
		camera_conf->p_share_mem[i].mem_len = camera_buffer.length;		
	}

	for (i=0; i< camera_conf->share_buff_nums; i++)
	{
		camera_conf->v_share_mem[i].mem_addr = mmap(NULL, camera_conf->p_share_mem[i].mem_len, PROT_READ | PROT_WRITE, MAP_SHARED, camera_fd, camera_conf->p_share_mem[i].offset);
		if (camera_conf->v_share_mem[i].mem_addr != NULL)
		{
			print("mem %d mmap success, addr : %p\n", i, camera_conf->v_share_mem[i].mem_addr );
			camera_conf->v_share_mem[i].mem_len = camera_conf->p_share_mem[i].mem_len;
		}
		else
		{
			print("mem %d mmap failed", i);
			camera_conf->v_share_mem[i].mem_len = 0;
		}	
	}

	return 0;
}

int main(int argc, char * argv[])
{
	int camera_fd = -1;
	struct camera_config camera_conf;

	camera_fd = camera_open_dev();
	if (camera_fd < 0)
	{
		exit(-1);
	}

	//初始化
	camera_init(camera_fd, &camera_conf);

	//启动网络线程
	
	//启动取流线程
	
	close(camera_fd);
	return 0;
}
