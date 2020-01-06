#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "os.h"
#include "cstring.h"
#include "hi_rtc.h"

#define ASC2NUM(ch) (ch - '0')
#define HEXASC2NUM(ch) (ch - 'A' + 10)


#define RTC_DEV_NAME	("/dev/hi_rtc")
const char *dev_name = RTC_DEV_NAME;


void usage(void)
{
	printf(
			"\n"
			"Usage: ./rtc_time [options] [parameter1] ...\n"
			"Options: \n"
			"	-s    set sys time,    e.g '-s 2012/7/15/13/37/59'\n"
			"	-g    get sys time,    e.g '-g'\n"
			
			"	-w    set rtc time,    e.g '-w 2012/7/15/13/37/59'\n"	//依赖于底层硬件
			"	-r    get rtc time,    e.g '-r'\n"
			
			"	-reset rtc reset\n"
			"	-sync    sys time->rtc time\n"
			"	-anisync rtc time->sys time\n"
			"	-D open ntpd server    e.g '-D /usr/share/zoneinfo/Shanghai'\n"				//同步时区时间,成功的话，会写rtc

			"\n");
	exit(1);
}


static int _atoul(const char *str, unsigned char *pvalue)
{
	unsigned int result=0;
	while (*str)
	{
		if (isdigit((int)*str))
		{
			if ((result<429496729) || ((result==429496729) && (*str<'6')))
			{
				result = result*10 + (*str)-48;
			}
			else
			{
				*pvalue = (char)result;
				return -1;
			}
		}
		else
		{
			*pvalue=result;
			return -1;
		}
		str++;
	}
	*pvalue=result;
	return 0;
}

static int  _atoulx(const char *str, unsigned char *pvalue)
{
	unsigned int   result=0;
	unsigned char  ch;

	while (*str)
	{
		ch=toupper(*str);
		if (isdigit(ch) || ((ch >= 'A') && (ch <= 'F' )))
		{
			if (result < 0x10000000)
			{
				result = (result << 4) + ((ch<='9')?(ASC2NUM(ch)):(HEXASC2NUM(ch)));
			}
			else
			{
				*pvalue=result;
				return -1;
			}
		}
		else
		{
			*pvalue=result;
			return -1;
		}
		str++;
	}

	*pvalue=result;
	return 0;
}

static int str_to_num(const char *str, unsigned char *pvalue)
{
	if ( *str == '0' && (*(str+1) == 'x' || *(str+1) == 'X') ){
		if (*(str+2) == '\0'){
			return -1;
		}
		else{
			return _atoulx(str+2, pvalue);
		}
	}
	else {
		return _atoul(str,pvalue);
	}
}

//将时间字符串转换成rtc_time_t：2012/7/15/13/37/59	年/月/日/时/分/秒
static int parse_string(char *string, rtc_time_t *p_tm)
{
	int value[10], ret;
    match_t match = {"/////e",6};
	ret = c_parse_number(string, &match, value);
	if (ret != 6) return -1;

	p_tm->year   = value[0];
	p_tm->month  = value[1];
	p_tm->date   = value[2];
	p_tm->hour   = value[3];
	p_tm->minute = value[4];
	p_tm->second = value[5];
	p_tm->weekday = 0;

	return 0;
}

static void set_rtc_time(rtc_time_t *p_tm)
{
	int fd = -1;
	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		printf("open %s failed\n", dev_name);
		return;
	}
	
	if (ioctl(fd, HI_RTC_SET_TIME, p_tm) < 0) {
		printf("ioctl: HI_RTC_SET_TIME failed\n");
	}
	close(fd);
	return;
}

static void get_rtc_time(rtc_time_t *p_tm)
{
	int fd = -1;
	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		printf("open %s failed\n", dev_name);
		return;
	}
	if (ioctl(fd, HI_RTC_RD_TIME, p_tm) < 0){
		printf("ioctl: HI_RTC_RD_TIME failed\n");
	}
	close(fd);	
	return;
}

static void reset_rtc_time(void)
{
	int fd = -1;
	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		printf("open %s failed\n", dev_name);
		return;
	}

	if(ioctl(fd, HI_RTC_RESET)){
		printf("reset err\n");
	}
	close(fd);	
	return;
}

//2012-07-15 13:38:49
static int parse_string2(char *string, rtc_time_t *p_tm)
{
	int value[10], ret;
    match_t match = {"-- ::e",6};
	ret = c_parse_number(string, &match, value);
	if (ret != 6) return -1;

	p_tm->year   = value[0];
	p_tm->month  = value[1];
	p_tm->date   = value[2];
	p_tm->hour   = value[3];
	p_tm->minute = value[4];
	p_tm->second = value[5];
	p_tm->weekday = 0;
		
	return 0;
}

static void set_sys_time(rtc_time_t *p_tm)
{
	char data_time[48] = {0};		//date -s "2012-05-23 01:01:01"
	sprintf(data_time, "date -s \"%d-%d-%d %d:%d:%d\"", p_tm->year, p_tm->month, p_tm->date, p_tm->hour, p_tm->minute, p_tm->second);
	system(data_time);
	return;
}

static void get_sys_time(rtc_time_t *p_tm)
{
	FILE *fp = NULL;
	fp = popen("date +\"%F %T\"", "r");
	if(fp == NULL){
		printf("open date cmd failed\n");
		return;
	}
	char data_time[48] = {0};
	if (NULL != fgets(data_time, 100, fp)){					
		parse_string2(data_time, p_tm);
	}else{
		printf("get date cmd failed\n");			
	}
	pclose(fp);
	return;
}

static void print_time(rtc_time_t *p_tm)
{
	printf("%d-%d-%d %02d:%02d:%02d\r\n", p_tm->year, p_tm->month, p_tm->date, p_tm->hour, p_tm->minute, p_tm->second);
}



#if 0
typedef int (*CMD_FUNC)(char *);

typedef struct {
	const char *str;	//命令字符串
	char		argc;	//对应命令参数个数
	CMD_FUNC	func;
}cmd_table_t;

//注意数组个数
static cmd_table_t cmd[8] = {
	{"-s",		1, },
	{"-g", 		0},
	{"-w", 		1},
	{"-r", 		0},
	{"-reset", 	0},
	{"-sync", 	0},
	{"-anisync",0},
	{"-D", 		1}
};
#endif

int main(int argc, const char *argv[])
{
	rtc_time_t tm;
	reg_data_t regv;
	reg_temp_mode_t mode;
	int ret = -1;

	char string[50] = {0};
	memset(&tm, 0, sizeof(tm));

	if (argc < 2){
		usage();
		return 0;
	}

	if (!strcmp(argv[1],"-s")) {
		//设置系统命令
		if (argc < 3) {
			usage();
			goto err1;	
		}
		strncpy(string, argv[2], sizeof(string) - 1);
		ret = parse_string(string, &tm);
		if (ret < 0){
			printf("parse time param failed\n");
			goto err1;
		}	
		set_sys_time(&tm);
		print_time(&tm);

	} else if (!strcmp(argv[1],"-g")) {
		if (argc < 2) {
			usage();
			goto err1;	
		}
		//date +"%F %T"
		get_sys_time(&tm);
		print_time(&tm);
		
	} else if (!strcmp(argv[1],"-w")) {		
		//设置硬件RTC时间
		if (argc < 3) {
			usage();
			goto err1;	
		}
		
		strncpy(string, argv[2], sizeof(string) - 1);
		ret = parse_string(string, &tm);
		if (ret < 0){
			printf("parse time param failed\n");
			goto err1;
		}
		print_time(&tm);
		set_rtc_time(&tm);
		
	} else if (!strcmp(argv[1],"-r")) {
		if (argc < 2) {
			usage();
			goto err1;	
		}
		get_rtc_time(&tm);
		print_time(&tm);
		
	} else if (!strcmp(argv[1],"-reset")) {
		if (argc < 2) {
			usage();
			goto err1;	
		}		
		reset_rtc_time();
		
	} else if (!strcmp(argv[1], "-sync")) {
		if (argc < 2) {
			usage();
			goto err1;	
		}		

		get_sys_time(&tm);
		set_rtc_time(&tm);

	} else if (!strcmp(argv[1], "-anisync")) {
		if (argc < 2) {
			usage();
			goto err1;	
		}
		
		get_rtc_time(&tm);
		set_sys_time(&tm);
		
	} else if (!strcmp(argv[1], "-D")) {
		if (argc < 2) {
			usage();
			goto err1;	
		}
	
		if (OSLIB_SystemCheck("ping -c 1 ntp1.aliyun.com > /dev/null 2>&1")) goto err1;
		
		//ping成功	ntpd -p cn.ntp.org.cn
		system("killall ntpd 0>/dev/null 1>/dev/null 2>&1");
		system("rm -rf /etc/localtime");
		if (argc == 3){
			strncpy(string, argv[2], sizeof(string) - 1);	//复制时区文件
		}
		
		if (string[0]){
			char cmd_string[128] = {0};
			sprintf(cmd_string, "cp %s /etc/localtime", string);
			system(cmd_string);
		}

		if (OSLIB_SystemCheck("ntpd -p ntp1.aliyun.com > /dev/null 2>&1")) goto err1;
		//不会立马成功需要等待
		int i;
		printf("ntp sync time, please wait ");
		fflush(stdout);
		for(i = 0; i < 7; i++){
			sleep(1);
			printf("*");
			fflush(stdout);
		}
		printf(" [ok]\n");
		//同步成功后，设置硬件rtc
		get_sys_time(&tm);
		print_time(&tm);
		set_rtc_time(&tm);
	} else {
		printf("unknown download mode.\n");
		goto err1;
	}
	ret = 0;
err1:
	return ret;
}

