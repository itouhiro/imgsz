/*
 * imgsz  -  output imagesize-info as HTML format to stdout
 *
 * Filename:   imgsz.c
 * Version:    0.1
 * Time-stamp: <Sep 17 2001>
 * Author:     ITOU Hiroki (itouh@lycos.ne.jp)
 */

/*
 * Copyright (c) 2001 ITOU Hiroki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define PROGNAME "imgsz"
#define PROGVERSION "0.1"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SUFLEN 4 //suffix length (including dot)
                 //拡張子の文字数(dotも含める) ".jpg", ".png" など4文字
#define GIFHEAD 32 //in fact, 10(sig(6) + width(2) + height(2) is enough
#define PNGHEAD 32 //really, 24(sig(8) + IHDR len(4)+type(4)+width(4)+height(4)) is enough
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

	//print help & exit
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

	//argument handling
	for(i=1; i<argc; i++){
		size_type size;
		int ftype;

		//filename check
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

		//print to stdout
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
	//License:
	//  The Graphics Interchange Format(c) is the Copyright property of
	//  CompuServe Incorporated. GIF(sm) is a Service Mark property of
	//  CompuServe Incorporated.
	size_type size;
	unsigned char *p, src[GIFHEAD];
	short w, h;
	FILE *fin;

	//file open
	if( (fin=fopen(fname,"rb")) == NULL )
		errret;
	if(fread(src, 1, GIFHEAD, fin)!=GIFHEAD)
		errret;

	//get size (Logical Screen Size)
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

	//file open & memory allocate
	if((fin=fopen(fname,"rb")) == NULL)
		errret;
	if(stat(fname, &info) != 0)
		errret;
	if((src=malloc(info.st_size)) == NULL)
		errret;
	if(fread(src, 1, info.st_size, fin) != info.st_size)
		errret;

	//get size
	p = src + 2; //SOI(start of image)(2bytes)
	for(;;){
		short next;
		if(*p==0xff && *(p+1)==0xc0){
			//found SOF0 (start of frame 0)
			p += 5; //marker(2) + segmentlength(2) + bit/sample(1)
			h = *p<<8 | *(p+1);
			w = *(p+2)<<8 | *(p+3);
			size.width = w;
			size.height = h;
			return size;
		}
		p += 2; //marker(2bytes PNGでいう chunkname)をとばすのだが、
		//segment lengthに markerも含まれている
		next = *p<<8 | *(p+1); //このセグメントをとばす
		p = p + next;
		if(p >= src+info.st_size)
			errret;
	}
	errret;
}


size_type pngGetSize(char *fname){
	size_type size;
	unsigned char *p, src[PNGHEAD];
	long w, h;
	FILE *fin;

	//file open
	if( (fin=fopen(fname,"rb")) == NULL )
		errret;
	if(fread(src, 1, PNGHEAD, fin)!=PNGHEAD)
		errret;


	//get size
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
	long w, h;
	int flag = 0;

	//file open
	if( (fin=fopen(fname,"r")) == NULL )
		errret;

	//get size
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
