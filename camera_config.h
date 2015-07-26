#ifndef _CAMERA_CONFIG_H_
#define _CAMERA_CONFIG_H_
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <netinet/in.h>

#define print(format, ...) printf("%s %s %d --> "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LISTEN_PORT 8090
#define MAX_CLIENT_NUMS 16

struct image_head
{
	unsigned int head_size;
	unsigned int head_version;
	unsigned int image_size; //image size
    //time_t image_time;
    unsigned int image_format;	//yuyv:0 jpg:1
    unsigned int image_width;
    unsigned int image_height;
    int res[3];
};

//���紫���
typedef struct image_info
{
	struct image_head head;
    unsigned char data[0];	//data
}image_info;

struct camera_share_mem
{
	unsigned int mem_len;
	union
	{
		unsigned int offset; //mmap use
		unsigned char * mem_addr; //v addr
	};
};

struct client_info
{
	int client_fd; //socket
	struct sockaddr_in clint_addr; //�������
};

struct link_info
{
	int clint_nums; //�ͻ�����������
	struct client_info client_tail[MAX_CLIENT_NUMS];
};

struct camera_config
{
	int width;
	int height;

	pthread_mutex_t config_lock; //�����ļ���
	int camera_fd;
	
	struct camera_share_mem p_share_mem; //�������ݣ����1��
	struct camera_share_mem v_share_mem; //�����ַͨ��mmap���ӳ��

	unsigned char * image_share_buff;
	int image_share_buff_len;
	pthread_mutex_t image_share_buff_lock; //����������
	sem_t image_sem; //����ͼƬ�ź���

	//�������
	struct link_info client_connect; //�ͻ���������Ϣ
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

//���������ڴ�
int cam_conf_lock_share_buf();
int cam_conf_unlock_share_buf();

//�����̺߳�ͼƬ�̹߳�������
int share_image_get_mem_size();
char * share_image_get_mem_addr();
//ͼ�����ݿ���
int copy_image_into_share_mem();
int copy_image_from_share_mem(char * dst_buff, int *dst_buff_len);
//�������
int add_client(int client_fd, struct sockaddr_in * client_addr);
int del_client(int client_fd);

#endif

