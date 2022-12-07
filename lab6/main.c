#include "../common/common.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "audio_util.h"

#include <math.h>

#define RED FB_COLOR(255, 0, 0)
#define ORANGE FB_COLOR(255, 165, 0)
#define YELLOW FB_COLOR(255, 255, 0)
#define GREEN FB_COLOR(0, 255, 0)
#define CYAN FB_COLOR(0, 127, 255)
#define BLUE FB_COLOR(0, 0, 255)
#define PURPLE FB_COLOR(139, 0, 255)
#define WHITE FB_COLOR(255, 255, 255)
#define BLACK FB_COLOR(0, 0, 0)

#define PROMPT_X 100
#define PROMPT_Y 0

#define RECORD_X 0
#define RECORD_Y 0

#define YANG_X 0
#define YANG_Y 100

#define JGB_X 0
#define JGB_Y 200

#define RECT_W 100
#define RECT_H 100

/*语音识别要求的pcm采样频率*/
#define PCM_SAMPLE_RATE 16000 /* 16KHz */


static char *send_to_vosk_server(char *file);
extern void image_display_init(zoom_image *, fb_image *);
extern void image_move_zoom(zoom_image *, int,int,int,double);
extern void draw_image(zoom_image*);
static void touch_event_cb(int fd);
static void timer_cb(int);

static int radius = 50;	 //手势显示圆 半径 可能用不到
static int ox[5], oy[5]; //手指位置
static int touch_fd, point, type;
static char *jpgs[3] = {"./test.jpg", "./jgb.jpg"};

static fb_image *img;
static zoom_image *image_z;
static pcm_info_st pcm_info;

const double z_times[10] = {0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0};
int z_cnt = 4;

int isRecording = 0, isRecording2 = 0;

int max(int x,int y){
	return x<y?y:x;
}

int min(int x,int y){
	return x<y?x:y;
}

int main(int argc, char *argv[])
{
	// audio_record_init(NULL, PCM_SAMPLE_RATE, 1, 16); //单声道，S16采样

	// pcm_info_st pcm_info;
	// uint8_t *pcm_buf = audio_record(2000, &pcm_info); //录2秒音频

	// if(pcm_info.sampleRate != PCM_SAMPLE_RATE) { //实际录音采用率不满足要求时 resample
	// 	uint8_t *pcm_buf2 = pcm_s16_mono_resample(pcm_buf, &pcm_info, PCM_SAMPLE_RATE, &pcm_info);
	// 	pcm_free_buf(pcm_buf);
	// 	pcm_buf = pcm_buf2;
	// }

	// pcm_write_wav_file(pcm_buf, &pcm_info, "/tmp/test.wav");
	// printf("write wav end\n");

	// pcm_free_buf(pcm_buf);

	// char *rev = send_to_vosk_server("/tmp/test.wav");
	// printf("recv from server: %s\n", rev);
	// return 0;

	fb_init("/dev/fb0");
	font_init("./font.ttc");
	fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);

	image_z = (zoom_image *)malloc(sizeof(zoom_image));
	point = 0;

	img = fb_read_jpeg_image(jpgs[point]);
	fb_draw_sidebar(0, 1, 0, 0);
	image_display_init(image_z, img);
	fb_update();

	audio_record_init(NULL, PCM_SAMPLE_RATE, 1, 16); //单声道，S16采样

	//打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event2");
	//添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
	task_add_file(touch_fd, touch_event_cb);
	task_add_timer(500, timer_cb);
	task_loop(); //进入任务循环

	return 0;
}

//绘制侧边栏
void fb_draw_sidebar(int record_p, int yang_p, int jgb_p, int prompt_p)
{
	if (record_p)
	{
		fb_draw_rect(0, 0, 100, 100, YELLOW);
		fb_draw_text(2, 50, "RECORD", 24, BLACK);
	}
	else
	{
		fb_draw_rect(0, 0, 100, 100, BLACK);
		fb_draw_text(2, 50, "RECORD", 24, WHITE);
	}

	if (yang_p)
	{
		fb_draw_rect(0, 100, 100, 100, YELLOW);
		fb_draw_text(2, 150, "SHEEP", 24, BLACK);
	}
	else
	{
		fb_draw_rect(0, 100, 100, 100, BLACK);
		fb_draw_text(2, 150, "SHEEP", 24, WHITE);
	}

	if (jgb_p)
	{
		fb_draw_rect(0, 200, 100, 100, YELLOW);
		fb_draw_text(2, 250, "STICK", 24, BLACK);
	}
	else
	{
		fb_draw_rect(0, 200, 100, 100, BLACK);
		fb_draw_text(2, 250, "STICK", 24, WHITE);
	}

	// 文字提示部分 内容由计时器绘制 这边只维护边框
	// fb_draw_rect(100, 0, 100, 100, BLACK);
	// if (prompt_p)
	// {
	// 	fb_draw_text(0, 50, "loading", WHITE);
	// }

	fb_draw_border(0, 0, 200, 100, WHITE);
	fb_draw_border(0, 0, 100, 300, WHITE);
	fb_draw_border(0, 200, 100, 1, WHITE);
}






static int st = 0;
static void timer_cb(int period) /*该函数0.5秒执行一次*/
{
	char buf[100];
	if (isRecording)
	{
		sprintf(buf, "已录音%d秒……", st++);
		fb_draw_rect(101, 1, 98,98,BLACK);
		// fb_draw_border(TIME_X, TIME_Y, TIME_W, TIME_H, COLOR_TEXT);
		fb_draw_text(100 + 2, 0 + 50, buf, 24, WHITE);
		fb_update();
	}
	else
	{
		st = 0;
		fb_draw_rect(101, 1,98,98, BLACK);
		fb_update();
	}
	return;
}

// touch事件 图片切换 图片缩放 图片移动 语音控制导致的……
static void touch_event_cb(int fd)
{
	int type1, x, y, finger, color;
	type1 = touch_read(fd, &x, &y, &finger);

	switch (type1)
	{
	case TOUCH_PRESS:
		printf("TOUCH_PRESS：x=%d,y=%d,finger=%d\n", x, y, finger);

		if (x <= 100 && y <= 100)
		{
			if (isRecording && isRecording2)
			{
				//已经在录音，这个时候暂停录音
				isRecording = 0;
				isRecording2 = 0;
				fb_draw_rect(1, 1, 98,98, BLACK);
				fb_draw_text(2, 50, "RECORD", 24, WHITE);
				fb_update();
				break;
			}
			type = -1;
			printf("\n1秒后开始录制:\n");
			sleep(1);
			//进入录音状态 绘制一些提示
			fb_draw_rect(1, 1, 98,98, YELLOW);
			fb_draw_text(2, 50, "RECORD", 24, BLACK);
			fb_update();
			printf("开始！\n");

			isRecording = 1;
			uint8_t *pcm_buf = audio_record(4000, &pcm_info); //录4秒音频

			if (isRecording == 0)
			{
				printf("被提前终止录音！\n");
				break;
			}
			isRecording = 0;
			isRecording2 = 0;

			if (pcm_info.sampleRate != PCM_SAMPLE_RATE)
			{ //实际录音采用率不满足要求时 resample
				uint8_t *pcm_buf2 = pcm_s16_mono_resample(pcm_buf, &pcm_info, PCM_SAMPLE_RATE, &pcm_info);
				pcm_free_buf(pcm_buf);
				pcm_buf = pcm_buf2;
			}

			pcm_write_wav_file(pcm_buf, &pcm_info, "/tmp/test.wav");
			// printf("write wav end\n");

			pcm_free_buf(pcm_buf);

			int x_offset = 0, y_offset = 0;

			char *rev = send_to_vosk_server("/tmp/test.wav");
			printf("recv from server: %s\n", rev);
			if (strcmp(rev, "放大") == 0)
			{
				type = 0;
				if (z_cnt < 10)
					z_cnt++;
			}
			else if (strcmp(rev, "缩小") == 0)
			{
				type = 0;
				if (z_cnt > 0)
					z_cnt--;
			}
			else if (strcmp(rev, "左移") == 0)
			{
				type = 0;
				x_offset = -50;
			}
			else if (strcmp(rev, "右移") == 0)
			{
				type = 0;
				x_offset = +50;
			}
			else if (strcmp(rev, "上移") == 0)
			{
				type = 0;
				y_offset = +50;
			}
			else if (strcmp(rev, "下移") == 0)
			{
				type = 0;
				y_offset = -50;
			}
			else if (strcmp(rev, "恢复") == 0)
			{
				type = 1;
			}
			else
				type = -1;
			image_move_zoom(image_z, type, x_offset, y_offset,z_times[z_cnt]);
			fb_update();
			printf("完毕!\n");
		}
		else if (x <= 100 && y <= 200 && y > 100)
		{
			//切换到羊
			printf("now is sheep, point:%d\n",point);
			if (point == 1)
			{
				point = 0;
					
				fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
				img = fb_read_jpeg_image(jpgs[point]);
				fb_draw_sidebar(isRecording, 1, 0, 0);
				image_display_init(image_z, img);
				fb_update();
			}
		}
		else if (x <= 100 && y <= 300 && y > 200)
		{
			printf("now is 金箍棒, point:%d\n",point);
			//切换到金箍棒
			if (point == 0)
			{
				point = 1;

				fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
				img = fb_read_jpeg_image(jpgs[point]);
				fb_draw_sidebar(isRecording2, 0, 1, 0);
				image_display_init(image_z, img);
				fb_update();
			}
		}
		else
		{
			//记录手指位置
			ox[finger] = x;
			oy[finger] = y;
		}
		break;
	case TOUCH_MOVE:
		printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n", x, y, finger);

		//图片也做响应移动
		if (finger == 0)
		{
			image_z->x += x - ox[finger];
			image_z->y += y - oy[finger];
			fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
			fb_draw_sidebar(isRecording,1,0,0);
			draw_image(image_z);
			fb_update();
		}
		else
		{
			//不止一根手指，并且移动
			int dx = ox[finger] - ox[finger - 1];
			int dy = oy[finger] - oy[finger - 1];
			double od = sqrt(dx * dx + dy * dy);
			dx = x - ox[finger - 1];
			dy = y - oy[finger - 1];
			double d = sqrt(dx * dx + dy * dy);
			if (d < od)
			{
				//缩小
				printf("缩小 %lf %lf\n",od,d);
				z_cnt = max(0, z_cnt - 1);
				image_move_zoom(image_z, 0, 0, 0,z_times[z_cnt]);
			}
			else
			{
				//放大
				printf("放大 %lf %lf\n",od,d);
				z_cnt = min(9,z_cnt+1);
				image_move_zoom(image_z,0,0,0,z_times[z_cnt]);
			}
			fb_draw_sidebar(isRecording,1,0,0);
			fb_update();
		}

		ox[finger] = x;
		oy[finger] = y;
		break;
	case TOUCH_RELEASE:
		if (finger == 0 && x < 100 && y < 100)
		{
			//松开录音键
			isRecording2=1;
		}
		break;
	case TOUCH_ERROR:
		printf("close touch fd\n");
		close(fd);
		task_delete_file(fd);
		break;
	default:
		return;
	}
	fb_update();
	return;
}

/*===============================================================*/

#define IP "127.0.0.1"
#define PORT 8011

#define print_err(fmt, ...) \
	printf("%d:%s " fmt, __LINE__, strerror(errno), ##__VA_ARGS__);

static char *send_to_vosk_server(char *file)
{
	static char ret_buf[128]; //识别结果

	if ((file == NULL) || (file[0] != '/'))
	{
		print_err("file %s error\n", file);
		return NULL;
	}

	int skfd = -1, ret = -1;
	skfd = socket(AF_INET, SOCK_STREAM, 0);
	if (skfd < 0)
	{
		print_err("socket failed\n");
		return NULL;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(IP);
	ret = connect(skfd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0)
	{
		print_err("connect failed\n");
		close(skfd);
		return NULL;
	}

	printf("send wav file name: %s\n", file);
	ret = send(skfd, file, strlen(file) + 1, 0);
	if (ret < 0)
	{
		print_err("send failed\n");
		close(skfd);
		return NULL;
	}

	ret = recv(skfd, ret_buf, sizeof(ret_buf) - 1, 0);
	if (ret < 0)
	{
		print_err("recv failed\n");
		close(skfd);
		return NULL;
	}
	ret_buf[ret] = '\0';
	return ret_buf;
}
