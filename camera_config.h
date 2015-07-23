#ifndef _CAMERA_CONFIG_H_
#define _CAMERA_CONFIG_H_
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

#define print(format, ...) printf("%s %s %d --> "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

struct camera_share_mem
{
	unsigned int mem_len;
	union
	{
		unsigned int offset; //mmap use
		unsigned char * mem_addr; //v addr
	};
};

struct camera_config
{
	int width;
	int height;

	pthread_mutex_t config_lock; //配置文件锁
	int camera_fd;
	
	struct camera_share_mem p_share_mem; //物理数据，最大1个
	struct camera_share_mem v_share_mem; //共享地址通过mmap后的映射

	unsigned char * image_share_buff;
	int image_share_buff_len;
	pthread_mutex_t image_share_buff_lock; //共享缓冲区锁
	sem_t image_sem; //共享图片信号量
};

static struct camera_config private_camera_conf;
struct camera_config *cam_conf_get_instance();
int cam_conf_get_camera_fd(int *fd);
int cam_conf_set_camera_fd(int fd);

int cam_conf_get_width(int *width);
int cam_conf_set_width(int width);
int cam_conf_get_height(int *height);
int cam_conf_set_height(int height);

int cam_conf_get_v_share_mem_info(struct camera_share_mem * sm);
int cam_conf_set_v_share_mem_info(struct camera_share_mem * sm);

int cam_conf_set_p_share_mem_info(struct camera_share_mem * sm);
int cam_conf_get_p_share_mem_info(struct camera_share_mem * sm);

//锁定共享内存
int cam_conf_lock_share_buf();
int cam_conf_unlock_share_buf();

//网络线程和图片线程共享缓冲区
int share_image_get_mem_size();
char * share_image_get_mem_addr();
//图形数据拷贝
int copy_image_into_share_mem();
int copy_image_from_share_mem(char * dst_buff, int dst_buff_len);
#endif

