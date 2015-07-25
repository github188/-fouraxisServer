#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "camera_config.h"

int malloc_image_share_buff();
int free_image_share_buff();
int realloc_image_share_buff();

int camera_conf_init()
{
	memset(&private_camera_conf, 0, sizeof(private_camera_conf));
	private_camera_conf.camera_fd = -1;
	pthread_mutex_init(&private_camera_conf.config_lock, NULL);
	pthread_mutex_init(&private_camera_conf.image_share_buff_lock, NULL);

	sem_init(&private_camera_conf.image_sem, 0, 0);

	return 0;
}

struct camera_config *cam_conf_get_instance()
{
	return &private_camera_conf;
}

int cam_conf_get_camera_fd(int *fd)
{
	if (fd != NULL)
	{
		*fd = private_camera_conf.camera_fd;
	}
	
	return private_camera_conf.camera_fd;
}

int cam_conf_set_camera_fd(int fd)
{
	pthread_mutex_lock(&private_camera_conf.config_lock);
	private_camera_conf.camera_fd = fd;
	pthread_mutex_unlock(&private_camera_conf.config_lock);

	return 0;
}

#if 0
int cam_conf_get_share_buff_nums(int *share_buff_nums)
{
	if (share_buff_nums != NULL)
	{
		*share_buff_nums = private_camera_conf.share_buff_nums;
	}
	return private_camera_conf.share_buff_nums;
}
int cam_conf_set_share_buff_nums(int share_buff_nums)
{
	
	pthread_mutex_lock(&private_camera_conf.config_lock);
	private_camera_conf.share_buff_nums = share_buff_nums;
	pthread_mutex_unlock(&private_camera_conf.config_lock);

	return 0;
}
#endif

int cam_conf_get_width(int *width)
{
	if (width != NULL)
	{
		*width = private_camera_conf.width;
	}
	return private_camera_conf.width;
}
int cam_conf_set_width(int width)
{
	
	pthread_mutex_lock(&private_camera_conf.config_lock);
	private_camera_conf.width = width;
	pthread_mutex_unlock(&private_camera_conf.config_lock);

	return 0;
}
int cam_conf_get_height(int *height)
{
	if (height != NULL)
	{
		*height = private_camera_conf.height;
	}
	return private_camera_conf.height;
}
int cam_conf_set_height(int height)
{
	
	pthread_mutex_lock(&private_camera_conf.config_lock);
	private_camera_conf.height = height;
	pthread_mutex_unlock(&private_camera_conf.config_lock);

	return 0;
}

int cam_conf_get_v_share_mem_info(struct camera_share_mem * sm)
{
	if (sm == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&private_camera_conf.config_lock);
	memcpy(sm, &private_camera_conf.v_share_mem, sizeof(struct camera_share_mem));
	pthread_mutex_unlock(&private_camera_conf.config_lock);

	return 0;
}
int cam_conf_set_v_share_mem_info(struct camera_share_mem * sm)
{
	if (sm == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&private_camera_conf.config_lock);
	memcpy(&private_camera_conf.v_share_mem, sm, sizeof(struct camera_share_mem));
	private_camera_conf.v_share_mem.mem_len = private_camera_conf.p_share_mem.mem_len;
	pthread_mutex_unlock(&private_camera_conf.config_lock);

	realloc_image_share_buff();

	return 0;
}

int cam_conf_get_p_share_mem_info(struct camera_share_mem * sm)
{
	if (sm == NULL)
	{
		return -1;
	}
	
	memcpy(sm, &private_camera_conf.p_share_mem, sizeof(struct camera_share_mem));

	return 0;
}

int cam_conf_set_p_share_mem_info(struct camera_share_mem * sm)
{
	if (sm == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&private_camera_conf.config_lock);

	memcpy(&private_camera_conf.p_share_mem, sm, sizeof(struct camera_share_mem));
	pthread_mutex_unlock(&private_camera_conf.config_lock);
	return 0;
}

int cam_conf_lock_share_buf()
{
	pthread_mutex_lock(&private_camera_conf.image_share_buff_lock);
	return 0;
}
int cam_conf_unlock_share_buf()
{
	pthread_mutex_unlock(&private_camera_conf.image_share_buff_lock);
	return 0;
}

int malloc_image_share_buff()
{	
	cam_conf_lock_share_buf();
	private_camera_conf.image_share_buff_len = private_camera_conf.v_share_mem.mem_len;
	print("private_camera_conf.v_share_mem.mem_len: %d\n", private_camera_conf.v_share_mem.mem_len);
	if (private_camera_conf.image_share_buff_len < 0)
	{
		//return -1;
		private_camera_conf.image_share_buff = NULL;
		private_camera_conf.image_share_buff_len = -1;
	}
	else
	{
		print("malloc image mem success, size: %d\n", private_camera_conf.image_share_buff_len);
		private_camera_conf.image_share_buff = (unsigned char *)malloc(private_camera_conf.image_share_buff_len);
	}
	cam_conf_unlock_share_buf();

	return private_camera_conf.image_share_buff==NULL?-1:0;
}
int free_image_share_buff()
{
	if (private_camera_conf.image_share_buff == NULL)
	{
		return -1;
	}
	cam_conf_lock_share_buf();
	free(private_camera_conf.image_share_buff);
	private_camera_conf.image_share_buff = NULL;
	private_camera_conf.image_share_buff_len = 0;
	cam_conf_unlock_share_buf();
}
int realloc_image_share_buff()
{
	int ret;
	ret = free_image_share_buff();
	
	ret = malloc_image_share_buff();
	return ret;
}

int share_image_get_mem_size()
{
	return private_camera_conf.image_share_buff_len;
}

char * share_image_get_mem_addr()
{
	return private_camera_conf.image_share_buff;
}

int copy_image_into_share_mem()
{
	int i;

	if (private_camera_conf.image_share_buff == NULL)
	{
		print("private_camera_conf.image_share_buff is NULL\n");
		if (-1 == realloc_image_share_buff())
		{
			return -1;
		}
	}
	
	cam_conf_lock_share_buf();
	memcpy(private_camera_conf.image_share_buff, private_camera_conf.v_share_mem.mem_addr, private_camera_conf.image_share_buff_len);
	cam_conf_unlock_share_buf();
	//print("copy data len: %d\n", private_camera_conf.image_share_buff_len);
	for (i=0; i< private_camera_conf.client_connect.clint_nums; i++)
	{
		sem_post(&private_camera_conf.image_sem);
	}
	return 0;
}

//参数:目标缓存和长度
int copy_image_from_share_mem(char * dst_buff, int *dst_buff_len)
{
	if (dst_buff == NULL)
	{
		return -1;
	}
	if (*dst_buff_len < private_camera_conf.image_share_buff_len)
	{
		print("dst buffer is too short\n");
		return -2;
	}
	sem_wait(&private_camera_conf.image_sem);
	cam_conf_lock_share_buf();
	memcpy(dst_buff, private_camera_conf.image_share_buff, private_camera_conf.image_share_buff_len) ;
	cam_conf_unlock_share_buf();

	*dst_buff_len = private_camera_conf.image_share_buff_len;
	return 0;
}

int add_client(int client_fd, struct sockaddr_in * client_addr)
{
	int i;
	int client_index;
	
	if (client_addr == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&private_camera_conf.config_lock);
	// 0-15
	if (private_camera_conf.client_connect.clint_nums+1 > MAX_CLIENT_NUMS-1)
	{
		print("client full\n");
		goto _err;
	}

	//
	client_index = private_camera_conf.client_connect.clint_nums;
	private_camera_conf.client_connect.client_tail[client_index].client_fd = client_fd;
	memcpy(&private_camera_conf.client_connect.client_tail[client_index].clint_addr, client_addr, sizeof(struct sockaddr_in));	
	private_camera_conf.client_connect.clint_nums++;

_err:
	pthread_mutex_unlock(&private_camera_conf.config_lock);
	return -2;
	
}

int del_client(int client_fd)
{
	int i;
	int client_index;
	
	for (i=0; i< private_camera_conf.client_connect.clint_nums; i++)
	{
		if (private_camera_conf.client_connect.client_tail[i].client_fd == client_fd)
		{
			client_index == i;
			break;
		}
	}

	if (i == private_camera_conf.client_connect.clint_nums)
	{
		print("no this connect\n");
		goto _err;
	}
	pthread_mutex_lock(&private_camera_conf.config_lock);
	for (i=client_index; i< private_camera_conf.client_connect.clint_nums-1; i++)
	{
		memcpy(&private_camera_conf.client_connect.client_tail[i], &private_camera_conf.client_connect.client_tail[i+1], sizeof(struct client_info));
	}
	memset(&private_camera_conf.client_connect.client_tail[i], 0, sizeof(struct client_info));

	private_camera_conf.client_connect.clint_nums--;
	
	pthread_mutex_unlock(&private_camera_conf.config_lock);
	return 0;
_err:
	return -1;
}


