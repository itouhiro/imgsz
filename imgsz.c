/*
 * imgsz  -  output imagesize-info as HTML format to stdout
 *
 * Filename:   imgsz.c
 * Version:    0.2
 * Time-stamp: <Sep 18 2001>
 */
#define PROGNAME "imgsz"
#define PROGVERSION "0.2"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SUFLEN 4 //suffix length (including dot)
                 //拡張子の文字数(dotも含める) ".jpg", ".png" など4文字
#define GIFHEAD 32 //in fact, 10 (sig(6)+width(2)+height(2)) is enough
#define PNGHEAD 32 //really, 24 (sig(8)+IHDRlen(4)+type(4)+width(4)+height(4)) is enough
#define XBMHEAD 100 //??
#define MAXLINECHAR 1024
#define errret { size.width = -1; return size; }

/**********************************************************************
 * struct
 **********************************************************************/
typedef struct{
	long width;
	long height;
} size_type;
const char *suf[] = { ".gif", ".jpg", ".png", ".xbm" };


/**********************************************************************
 * func prototype
 **********************************************************************/
int main(int argc, char *argv[]);
int getSuffix(char *fname);
size_type gifGetSize(char *fname);
size_type jpgGetSize(char *fname);
size_type pngGetSize(char *fname);
size_type xbmGetSize(char *fname);



int main(int argc, char *argv[]){
	int i;

	/*
	 * print help & exit
	 */
	if(argc<=1){
		fprintf(stderr,
			"%s version %s\n"
			"           output imagesize-info as HTML format to stdout\n"
			"usage:     %s file...\n"
			"format:    "
			, PROGNAME, PROGVERSION, PROGNAME);
		for(i=0; i<sizeof(suf)/sizeof(suf[0]); i++)
			fprintf(stderr, "%s ", suf[i]);
		fprintf(stderr, 
			"\n"
			"caution:   This software don't recognize UPPER-CASE suffix.\n"
			"           (like 'JPG', 'PNG'..)\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * argument handling
	 */
	for(i=1; i<argc; i++){
		size_type size;
		int ftype;

		/*
		 * filename check
		 */
		ftype = getSuffix(argv[i]);
		switch(ftype){
		case 0:
			size = gifGetSize(argv[i]); //gif
			break;
		case 1:
			size = jpgGetSize(argv[i]); //jpg
			break;
		case 2:
			size = pngGetSize(argv[i]); //png
			break;
		case 3:
			size = xbmGetSize(argv[i]); //xbm
			break;
		default:
			size.width = 0;
			break;
		}

		/*
		 * print to stdout
		 */
		if(size.width==0){
			; //not image-file suffix
		}else if(size.width==-1){
			fprintf(stdout, "<!-- error: %s is not an image file? -->\n", argv[i]);
		}else{
			fprintf(stdout, "<img src=\"%s\" width=\"%ld\" height=\"%ld\" alt=\" \">\n", argv[i], size.width, size.height);
		}
	}//for(i
	return EXIT_SUCCESS;
}



int getSuffix(char *fname){
	char *s, *sc; //string current
	int i, sufnum;

	//ファイル名の(複数あるdotの) last dot をpointする
	s = NULL;
	sc = fname;
	while(*sc!='\0'){
		if(*sc=='.')
			s = sc;
		sc++;
	}
	if(s==NULL)
		return -1;

	sufnum = sizeof(suf) / sizeof(suf[0]); //拡張子の種類の数
	for(i=0; i<sufnum; i++){
		if(strcmp(s, suf[i]) == 0)
			return i;
	}
	return -1;
}


size_type gifGetSize(char *fname){
	/*
	 * License:
	 *   The Graphics Interchange Format(c) is the Copyright property of
	 *   CompuServe Incorporated. GIF(sm) is a Service Mark property of
	 *   CompuServe Incorporated.
	 */
	size_type size;
	unsigned char *p, src[GIFHEAD];
	short w, h;
	FILE *fin;

	/*
	 * file open
	 */
	if((fin=fopen(fname,"rb")) == NULL)
		errret;
	if(fread(src, 1, GIFHEAD, fin)!=GIFHEAD)
		errret;

	/*
	 * validity check ファイル正当性チェック
	 *  最初の6バイトが 'GIF87a' か 'GIF89a'であればいい
	 */
	if(memcmp(src, "GIF8", 4)!=0)
		errret;

	/*
	 * get size (Logical Screen Size)
	 */
	p = src + 6; // 'GIF'(3) + version(3)
	w = *p | *(p+1)<<8; //LSBfirst(little endian 2bytes)
	h = *(p+2) | *(p+3)<<8;

	size.width = w;
	size.height = h;
	return size;
}


size_type jpgGetSize(char *fname){
	size_type size;
	unsigned char *p, *src;
	short w, h;
	FILE *fin;
	struct stat info;

	/*
	 * file open & memory allocate
	 */
	if((fin=fopen(fname,"rb")) == NULL)
		errret;
	if(stat(fname, &info) != 0)
		errret;
	if((src=malloc(info.st_size)) == NULL)
		errret;
	if(fread(src, 1, info.st_size, fin) != info.st_size){
		free(src);
		errret;
	}
	fclose(fin);


	/*
	 * validity check
	 *  最初の2バイトは SOI というmarker。
	 *  marker とは、各セグメント最初の2バイトにある識別子。
	 *  PNGでいうchunkname。
	 *
	 *  JFIFの場合は APP0 も調べたほうがいいけど調べてない。
	 */
	if(*src==0xff && *(src+1)==0xd8){
		;//SOI(start of image) exists
	}else{
		free(src);
		errret;
	}

	/*
	 * get size
	 */
	p = src + 2; //SOI(start of image)(2bytes)
	for(;;){
		short next; //次のセグメントまでの相対距離
		static unsigned char sof[] = { 0xc0, 0xc1, 0xc2, 0xc3,
					       0xc5, 0xc6, 0xc7, 0xc9,
					       0xca, 0xcb, 0xcd, 0xce,
					       0xcf, };
		//0xc4,0xc8,0xccは、SOFn以外に使用されている
		int i;

		//セグメント先頭の 'ff'をサーチ
		while(*p!=0xff){
			p++;
			if(p >= src + info.st_size){
				free(src);
				errret;
			}
		}

		//セグメント種を調べる
		for(i=0; i<sizeof(sof)/sizeof(sof[0]); i++){
			if(*(p+1) == sof[i]){
				//SOFn (start of frame) を見つけた
				p += 5; //marker(2) + segmentlength(2) + bit/sample(1)
				if(p >= src + info.st_size){
					free(src);
					errret;
				}
				h = *p<<8 | *(p+1);
				w = *(p+2)<<8 | *(p+3);
				size.width = w;
				size.height = h;
				free(src);
				return size;
			}
		}
		if(*(p+1)==0xd8 || *(p+1)==0xd3){
			//EOI(end of image), SOS(start of scan) が
			//SOFnの前にある => なんらかのエラー
			free(src);
			errret;
		}else{
			//ここで探してない種のmarkerだった
			p += 2;
			if(p >= src + info.st_size){
				free(src);
				errret;
			}
			next = *p<<8 | *(p+1);
			p = p + next; //skip this segment
			if(p >= src + info.st_size){
				free(src);
				errret;
			}
		}//if(*p
	}//for(;;
}


size_type pngGetSize(char *fname){
	size_type size;
	unsigned char *p, src[PNGHEAD];
	long w, h;
	FILE *fin;

	/*
	 * file open
	 */
	if((fin=fopen(fname,"rb")) == NULL)
		errret;
	if(fread(src, 1, PNGHEAD, fin)!=PNGHEAD)
		errret;

	/*
	 * validity check
	 *  最初の8バイトが、89 50 4e 47 0d 0a 1a 0a
	 *                  (c) P  N  G  \r \n    \n
	 */
	if(memcmp(src, "\x89PNG\r\n\x1a\x0a", 8)==0)
		;
	else
		errret;

	/*
	 * get size
	 */
	p = src + 16; //PNG signature(8) + IHDR chunk length(4)+type(4)
	w = *p<<24 | *(p+1)<<16 | *(p+2)<<8 | *(p+3); //MSBfirst(big endian 4bytes)
	h = *(p+4)<<24 | *(p+5)<<16 | *(p+6)<<8 | *(p+7);

	size.width = w;
	size.height = h;
	return size;
}


size_type xbmGetSize(char *fname){
	size_type size;
	FILE *fin;
	char *p, *top, src[MAXLINECHAR];
	long w=0, h=0;
	int flag = 0;

	/*
	 * file open
	 */
	if( (fin=fopen(fname,"r")) == NULL )
		errret;

	/*
	 * get size
	 */
	for(;;){
		if(flag>=2)
			break;

		//file read
		p = src;
		memset(p, 0x00, MAXLINECHAR);
		if(fgets(p, MAXLINECHAR-1, fin)==NULL)
			errret;

		//行末まで調べる
		while(*p!='\0'){
			if(strncmp(p, "width", strlen("width"))==0){
				top = memchr(p, ' ', XBMHEAD); //topを widthのあとの最初の空白に位置
				p = memchr(top, '\n', XBMHEAD); //pをwidthの行の改行位置に位置,\0 でうめる
				*p = '\0';
				w = atoi(top);
				flag++;
			}else if(strncmp(p, "height", strlen("height"))==0){
				top = memchr(p, ' ', XBMHEAD);
				p = memchr(top, '\n', XBMHEAD);
				*p = '\0';
				h = atoi(top);
				flag++;
			}//if(strncmp
			p++;
		}//while(*p   1行ぶんのチェックがおわった
	}
	size.width = w;
	size.height = h;
	return size;
}
//end of file
