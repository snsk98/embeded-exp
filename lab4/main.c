#include <stdio.h>
#include "../common/common.h"

#define COLOR_BACKGROUND	FB_COLOR(0xff,0xff,0xff)
#define BLACK FB_COLOR(0,0,0)
#define PURPLE FB_COLOR(139,0,255)
#define BLUE FB_COLOR(0,0,255)
#define CYAN FB_COLOR(0,127,255)
#define GREEN FB_COLOR(0,255,0)
//#define COLOR_BACKGROUND	FB_COLOR(0xff,0xff,0xff)
//#define BLACK 0xdddddddd
//#define PURPLE 0xbbbbbbbb
//#define BLUE 0x99999999
//#define CYAN 0x77777777
//#define GREEN 0x55555555




static int radius=50;
static int ox[5],oy[5];
static int touch_fd;
static void touch_event_cb(int fd)
{
	int type,x,y,finger;

	int color;
	
	type = touch_read(fd, &x,&y,&finger);
	switch(finger){
			case 0:color=BLACK; break;				
			case 1:color=PURPLE; break;				
			case 2:color=BLUE; break;				
			case 3:color=CYAN; break;				
			case 4:color=GREEN; break;				
			default: break;
	}
		
	switch(type){
	case TOUCH_PRESS:
		printf("TOUCH_PRESS：x=%d,y=%d,finger=%d\n",x,y,finger);
		
		fb_draw_round(x,y,radius,color);
		ox[finger]=x;
		oy[finger]=y;	
		
		break;
	case TOUCH_MOVE:
		printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n",x,y,finger);
		
		fb_draw_round(ox[finger],oy[finger],radius,COLOR_BACKGROUND);
		fb_draw_round(x,y,radius,color);
		ox[finger]=x;
		oy[finger]=y;

		break;
	case TOUCH_RELEASE:
		printf("TOUCH_RELEASE：x=%d,y=%d,finger=%d\n",x,y,finger);
		
		fb_draw_round(ox[finger],oy[finger],radius,COLOR_BACKGROUND);
		
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

int main(int argc, char *argv[])
{
	fb_init("/dev/fb0");
	fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
	fb_update();
	
	//printf("chckpoint\n");

	//打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event1");
	//添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
	task_add_file(touch_fd, touch_event_cb);
	
	//printf("checkpoint\n");
	task_loop(); //进入任务循环
	return 0;
}
