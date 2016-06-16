/*************************************************************************
	> File Name: logo_image_tool.c
	> Author: vonbrave
	> Mail: vonbrave@126.com 
	> Created Time: 2016年05月30日 星期一 08时31分02秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MAX_IMAGE_SIZE    (0x200000)
#define BMP_FILE_NUM_MAX  10

/* BMP file header. */
typedef struct tagBITMAPFILEHEADER {
	unsigned short pad;
	unsigned short bfType;
    	unsigned int   bfSize;
    	unsigned short bfReserved1;
    	unsigned short bfReserved2;
    	unsigned int   bfOffBits;
} BITMAPFILEHEADER;

/* BMP file map info header. */
typedef struct tagBITMAPINFOHEADER {
    	unsigned int 	biSize;
    	unsigned int 	biWidth;
    	unsigned int 	biHeight;
    	unsigned short 	biPlanes;
    	unsigned short 	biBitCount;
    	unsigned int 	biCompression;
    	unsigned int 	biSizeImage;
    	unsigned int	biXPelsPerMeter;
    	unsigned int 	biYPelsPerMeter;
    	unsigned int 	biClrUsed;
    	unsigned int 	biClrImportant;
} BITMAPINFOHEADER;

struct image_head_t
{
	unsigned short type;
	unsigned short zone_num;	
};

void usage(char *exename)
{
    printf("Usage:\n");
    printf("    %s x_width y_height bpp bmp_num bmp_file0 bmp_file1 .. bmp_file(num-1)\n", exename);    
}

void bmp_data_to_bpp16(unsigned char *bmp_data, BITMAPINFOHEADER *bmp_info_hdr, unsigned char *fb_img_data)
{
	int sj = 0, dj = 0, k = 0;
	int height  = bmp_info_hdr->biHeight;
	int width = bmp_info_hdr->biWidth;
	int dlinelen = width*2; 
	unsigned char * stemp;
	unsigned char * dtemp;
	unsigned int red,green,blue,temp;
		
	#define RED 2
	#define GREEN 1
	#define BLUE 0
			
	int mask[3];
	int offset[3];
	mask[RED]=5; mask[GREEN]=6; mask[BLUE]=5;
	offset[RED]=11; offset[GREEN]=5;offset[BLUE]=0;
		
	unsigned int bmp_linelen = ((((bmp_info_hdr->biWidth)*3 + 3) >> 2) << 2);
	
	for(sj = height-1,dj=0; dj<height; sj--,dj++)
	{
		for(k=0; k < width; k++)
		{
			stemp = bmp_data + sj*bmp_linelen+k*3;
			dtemp = fb_img_data+dj*dlinelen+k*2; 							
			
			red = (stemp[RED]>>(8-mask[RED])) & ((1<<mask[RED])-1);
			green = (stemp[GREEN]>>(8-mask[GREEN])) & ((1<<mask[GREEN])-1);
			blue = (stemp[BLUE]>>(8-mask[BLUE])) & ((1<<mask[BLUE])-1);
			
			temp = ((red<<offset[2]) | (green<<offset[1]) | (blue<<offset[0]));
			
			*(unsigned short*)dtemp = (unsigned short)temp; 
		}
	}	
}

int get_dest_data_from_bmp_file(const char *bmp_file, unsigned char *fb_img_data, int bpp)
{
	int ret = 0;
	int bmp_fd = 0;
	int bmp_head_size = 0;	
	unsigned long bmp_linelen = 0;
	unsigned char *bmp_data = NULL;	
	BITMAPFILEHEADER bmp_file_hdr;
	BITMAPINFOHEADER bmp_info_hdr;

	bmp_head_size = sizeof(bmp_file_hdr);
	bmp_head_size -= sizeof(bmp_file_hdr.pad);
	
	bmp_fd = open(bmp_file, O_RDONLY);
	if (bmp_fd < 0)
	{
		printf("Cannot open bmp file %s\n", bmp_file);
		ret = -1;
		goto x_close;
	}	

	ret = read(bmp_fd, (void *)&bmp_file_hdr, bmp_head_size);			
	if (ret != bmp_head_size)
	{
		printf("Read bmp file %s file header error\n", bmp_file);
		close(bmp_fd);
		ret = -1;
		goto x_close;
	}

	ret = read(bmp_fd, (void *)&bmp_info_hdr, sizeof(bmp_info_hdr));
	if (ret != sizeof(bmp_info_hdr))
	{
		printf("Read bmp file %s info header error\n", bmp_file);
		close(bmp_fd);
		ret = -1;
		goto x_close;
	}

	printf("Read bmp file %s biWidth = %d, biHeight = %d\n", bmp_file,bmp_info_hdr.biWidth, bmp_info_hdr.biHeight);

	bmp_linelen = ((((bmp_info_hdr.biWidth)*3 + 3) >> 2) << 2);

	bmp_data = (unsigned char *)malloc(bmp_linelen*(bmp_info_hdr.biHeight));
	if (NULL == bmp_data)
	{
		printf("malloc 	bmp_data error\n");
		close(bmp_fd);
		ret = -1;
		goto x_close;
	}

	read(bmp_fd, (void *)bmp_data, bmp_linelen*(bmp_info_hdr.biHeight));
	if (16 == bpp)
	{
		bmp_data_to_bpp16(bmp_data,&bmp_info_hdr,fb_img_data);
	}
	
	ret = 0;
	close(bmp_fd);
	free(bmp_data);	
x_close:
	return ret;	
}

int get_dcmp_data(const int x_width, const int y_height, const int bpp,const unsigned char *buf1, const unsigned char *buf2, unsigned char *dcmp_buf)
{
	int ret = 0;
	int i = 0,j =0;
	int data1 = 0,data2 = 0;
	unsigned int x_y_index = 0;
	unsigned int dcmp_index = 0;
	unsigned int *p_dcmp_buf = (unsigned int *)dcmp_buf;	
		
	for (i =0; i < y_height; i++)
	{
		for (j = 0; j < x_width; j++)
		{
			if (16 == bpp)
			{
				unsigned short *p_buf1 = (unsigned short *)buf1;
				unsigned short *p_buf2 = (unsigned short *)buf2;

				data1 = *(p_buf1 + i * x_width + j);

				data2 = *(p_buf2 + i * x_width + j);

				if (data1 != data2)
				{
					*(p_dcmp_buf + dcmp_index) = (i << 16) + (j);							
					*(p_dcmp_buf + dcmp_index + 1) = data2;
					
					dcmp_index += 2;
				}
			}	
			
		}
	}		
		 
	ret = (dcmp_index << 2);  //返回字节数			

	return ret;
}

int main(int argc, char *argv[])
{
	int ret = 0;	
	int x_width = 0,y_height = 0;
	int bpp = 0, bmp_num = 0;	
	int fd_image = 0;
	int fd_bmp1 = 0, fd_bmp2 = 0;
	unsigned char *fb_data_buf1 = NULL;
	unsigned char *fb_data_buf2 = NULL;
	unsigned char *fb_dcmp_buf = NULL;
	unsigned char *image_buf = NULL;
	int *p_buf = NULL;
	unsigned int bmp_src_data_size = 0;     //x_width * y_width * 3
	unsigned int bmp_dest_data_size = 0;    //x_width * y_width * 2
	unsigned int image_size = 0;
	struct image_head_t image_head;	
	int zone_cnt_index = 0;
	unsigned int cp_index = 0;
	int bmp_index = 0;
	unsigned int dcmp_size = 0; 	

	//检查输入参数	
	if (argc < 4)
	{
		usage(argv[0]);
		exit(0);
	}

	x_width = atoi(argv[1]);
	y_height = atoi(argv[2]);
	bpp = atoi(argv[3]);
	bmp_num = atoi(argv[4]);

	if (argc != 5 + bmp_num)
	{		
		usage(argv[0]);
		exit(1);
	}

	bmp_src_data_size = x_width * y_height * 3;
	bmp_dest_data_size = x_width * y_height * (bpp / 8);  

	//分配image buffer	
	image_buf = (unsigned char *)malloc(MAX_IMAGE_SIZE);
	if (NULL == image_buf)
	{
		printf("malloc image_buf error\n");	
		exit(1);
	}
	p_buf = (int *)image_buf;	

	fb_data_buf1 = (unsigned char *)malloc(bmp_dest_data_size + 4);
	if (NULL == fb_data_buf1)
	{
		printf("malloc fb_data_buf1 error\n");	
		free(image_buf);
		exit(1);
	}

	fb_data_buf2 = (unsigned char *)malloc(bmp_dest_data_size + 4);
	if (NULL == fb_data_buf2)
	{
		printf("malloc fb_data_buf2 error\n");	
		free(image_buf);
		free(fb_data_buf1);
		exit(1);
	}

	fb_dcmp_buf = (unsigned char *)malloc(x_width * y_height * 8);
	if (NULL == fb_dcmp_buf)
	{
		printf("malloc fb_dcmp_buf error\n");	
		free(image_buf);
		free(fb_data_buf1);
		free(fb_data_buf2);
		exit(1);
	}

	//构建镜像文件
	fd_image = open("logo.img", O_WRONLY|O_TRUNC|O_CREAT);
	
	//写镜像头  
	image_head.type = 0x6D7A;   //2字节  'ZM'
	image_head.zone_num = bmp_num;	
	
	memcpy((void *)image_buf,(void *)&image_head,sizeof(struct image_head_t));

	image_size = sizeof(struct image_head_t) + bmp_num * 4;
	
	cp_index = sizeof(struct image_head_t) + bmp_num * 4;

	//zone size and data 
	//logo1
	memset(fb_data_buf1,0,bmp_dest_data_size + 4);

	ret = get_dest_data_from_bmp_file(argv[5],fb_data_buf1,bpp);
	if (0 == ret)
	{
		memcpy((void *)(image_buf + cp_index),(void *)fb_data_buf1,bmp_dest_data_size);
		cp_index += bmp_dest_data_size;	

		image_size += bmp_dest_data_size;

		zone_cnt_index++;

		*(p_buf + zone_cnt_index) = bmp_dest_data_size;	

		bmp_index++;
				
	}
	else
	{
		printf("Error: get_dest_data_from_bmp_file %s\n",argv[5]);			
	}

	//logo2
	memset(fb_data_buf1,0,bmp_dest_data_size + 4);

	ret = get_dest_data_from_bmp_file(argv[6],fb_data_buf1,bpp);
	if (0 == ret)
	{
		memcpy((void *)(image_buf + cp_index),(void *)fb_data_buf1,bmp_dest_data_size);
		cp_index += bmp_dest_data_size;	

		image_size += bmp_dest_data_size;

		zone_cnt_index++;

		*(p_buf + zone_cnt_index) = bmp_dest_data_size;	
			
		bmp_index++;				
	}
	else
	{
		printf("Error: get_dest_data_from_bmp_file %s\n",argv[6]);			
	}
	
	//logo3--dcmp3_2
	memset(fb_data_buf2,0,bmp_dest_data_size + 4);

	while(bmp_index < bmp_num)
	{
		//获取下一张图像数据到buf2		
		ret = get_dest_data_from_bmp_file(argv[5 + bmp_index],fb_data_buf2,bpp);
		if (0 == ret)
		{
			memset(fb_dcmp_buf,0,x_width * y_height * 8);

			dcmp_size = get_dcmp_data(x_width,y_height,bpp,fb_data_buf1,fb_data_buf2,fb_dcmp_buf);
			printf("cmp %s--%s--dcmp_size = %u\n",argv[5 + bmp_index -1], argv[5 + bmp_index],dcmp_size);
			if (dcmp_size > 0)
			{
				memcpy((void *)(image_buf + cp_index),(void *)fb_dcmp_buf,dcmp_size);
				cp_index += dcmp_size;	

				image_size += dcmp_size;

				zone_cnt_index++;

				*(p_buf + zone_cnt_index) = dcmp_size;	
			
				bmp_index++;

				//保存当前数据到buf1
				memcpy((void *)fb_data_buf1,(void *)fb_data_buf2,bmp_dest_data_size);	
			}
			else
			{
				printf("Error: get_dcmp_data zero %s\n",argv[5 + bmp_index]);	
			}			
		}
		else
		{
			printf("Error: get_dest_data_from_bmp_file %s\n",argv[5 + bmp_index]);

			break;			
		}

	}

	printf("Write %u Bytes to file\n",image_size);

	write(fd_image, image_buf, image_size);
		
	free(image_buf);

	free(fb_data_buf1);
	free(fb_data_buf2);

	close(fd_image);


	return 0;		

}
