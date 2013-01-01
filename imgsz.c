/*
 * imgsz  -  print 'image size' -- print width and height pixels as XHTML format
 *
 * Filename:   imgsz.c
 * Version:    0.6
 * Author:     itouh
 * Time-stamp: <Jun 11 2009>
 * Copyright (c) 2003,2009 Itou Hiroki
 *
 * todo: JPEG file may cause error,  because of reading first 500 kilo bytes only of a JPEG fiile.
 *       JPEGファイルは（ファイル読み込みの速度のため）最大で0.5MBまでしか読み込まないので、エラーになるかも。
 *       GIF,PNGは先頭32バイトしか読み込まないので早いし問題ない。
 */
#define PROGNAME "imgsz"
#define PROGVERSION "0.6"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strrchr() */
#include <sys/stat.h>

#define GIFHEAD 32 //10 byte (sig(6)+width(2)+height(2)) is enough
#define PNGHEAD 32 //24 byte (sig(8)+IHDRlen(4)+type(4)+width(4)+height(4)) is enough
#define XBMHEAD 100 //??
#define MAXLINECHAR 1024
#define MAXFILEBYTE (1024*512)
#define errret {free(src); pxsize.width=-1; return pxsize;}

/*
 * global variable
 */
const char *g_suffix[] = { ".gif", ".GIF", ".jpg", ".jpeg",".JPG",".JPEG",".png", ".PNG",".xbm", ".XBM" };
char g_exifdate[MAXLINECHAR];
char g_exifmaker[MAXLINECHAR];
char g_exifmodel[MAXLINECHAR];

/*
 * struct
 */
typedef struct{
    long width;
    long height;
} size_type;
typedef enum {gif, GIF, jpg, jpeg, JPG, JPEG, png, PNG, xbm, XBM, notype} imagetype;


/*
 * prototype
 */
int getSuffix(char *fname);
size_type jpgGetSize(char *fname);
size_type gifGetSize(char *fname);
size_type pngGetSize(char *fname);
size_type xbmGetSize(char *fname);
void getExifInfo(unsigned char *exiftop);
void getExifData(unsigned char *dst, unsigned char *p, unsigned char *exiftop, int bigEndian);
unsigned short big2ltls(void *v);
unsigned long big2ltll(void *v);
char * getBasename(char * str);

/***********************************************************************/
int main(int argc, char *argv[]){
    int i;
    int ratio = 1;
    int exif = 0;
    int basename = 0;
    int xhtml_output = 1;
    int alt_filename = 0;

    /*
     * print help & exit
     */
    if(argc<=1){
        printf("%s version %s - print width & height sizes of images as XHTML format\n"
               "usage:     %s <option> file..\n"
               "option:    -2  --double      double size for width, height\n"
               "           -a  --altfilename 'alt' attribute with filename without suffix\n"
               "           -b  --basename    remove prefix path and show just a filename\n"
               "           -e  --exif        output Exif info if exist.  EXCLUSIVE with '-a'\n"
               "           -n  --normal      no XHTML output.  format: filename width height\n"
               "supported: ", PROGNAME, PROGVERSION, PROGNAME);
        for(i=0; i<sizeof(g_suffix)/sizeof(g_suffix[0]); i++)
            fprintf(stdout, "%s ", g_suffix[i]);
        fflush(stdout);
        printf("\n");
        exit(1);
    }

    /*
     * argument handling
     */
    for(i=1; i<argc; i++){
        size_type pxsize; //pixel size
        int ftype;
        char *img_fname, *p;

        if((strcmp(argv[i],"-2")==0) ||
           (strcmp(argv[i],"--double")==0)){
            ratio = 2;
            continue;
        }else if((strcmp(argv[i],"-a")==0) ||
                 (strcmp(argv[i],"--altfilename")==0)){
            alt_filename = 1;
            continue;
        }else if((strcmp(argv[i],"-b")==0) ||
                 (strcmp(argv[i],"--basename")==0)){
            basename = 1;
            continue;
        }else if((strcmp(argv[i],"-e")==0) ||
                 (strcmp(argv[i],"--exif")==0)){
            exif = 1;
            continue;
        }else if((strcmp(argv[i],"-n")==0) ||
                 (strcmp(argv[i],"--normal")==0)){
            xhtml_output = 0;
            continue;
        }

        /*
         * filename check
         */
        ftype = getSuffix(argv[i]);
        switch(ftype){
        case 0:
        case 1:
            pxsize = gifGetSize(argv[i]); //gif
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            pxsize = jpgGetSize(argv[i]); //jpg
            break;
        case 6:
        case 7:
            pxsize = pngGetSize(argv[i]); //png
            break;
        case 8:
        case 9:
            pxsize = xbmGetSize(argv[i]); //xbm
            break;
        default:
            pxsize.width = -1;
            break;
        }

        /*
         * output result
         */
        img_fname = (char *)malloc(MAXLINECHAR);
        memset(img_fname, 0x00, MAXLINECHAR);
        p=argv[i];
        if (basename){ p = getBasename(argv[i]); }
        strncpy(img_fname, p, MAXLINECHAR - 1);

        if(pxsize.width <= -1){
            fprintf(stdout, "<!-- error: '%s' is not an image file? or JPEG MetaInfo is over 1MB. -->\n", argv[i]);
        }else{
            if(xhtml_output){
                if(! alt_filename && exif && *g_exifdate!='\0'){
                    //fprintf(stdout, "<img src=\"%s\" width=\"%ld\" height=\"%ld\" alt=\"%s %s %s\" title=\"%s %s %s\" />\n", argv[i], pxsize.width * ratio, pxsize.height * ratio, g_exifmaker, g_exifmodel, g_exifdate, g_exifmaker, g_exifmodel, g_exifdate);
                    fprintf(stdout, "<img src=\"%s\" width=\"%ld\" height=\"%ld\" alt=\"%s %s\" title=\"%s %s\" />\n", img_fname, pxsize.width * ratio, pxsize.height * ratio, g_exifmodel, g_exifdate, g_exifmodel, g_exifdate);
                    memset(g_exifdate, 0x00, MAXLINECHAR);
                    memset(g_exifmaker, 0x00, MAXLINECHAR);
                    memset(g_exifmodel, 0x00, MAXLINECHAR);
                }else{
                    char alt_str[MAXLINECHAR];
                    char * begin_suffix;
                    if(alt_filename){
                        strncpy(alt_str, getBasename(img_fname), MAXLINECHAR - 1);
                        //delete suffix
                        begin_suffix = strrchr(alt_str, '.');
                        if(begin_suffix != NULL){ *begin_suffix = '\0'; }
                    }else{
                        * alt_str = '\0';
                    }
                    fprintf(stdout, "<img src=\"%s\" width=\"%ld\" height=\"%ld\" alt=\"%s\" />\n", img_fname, pxsize.width * ratio, pxsize.height * ratio, alt_str);
                }
            }else{ //if(xhtml_output)
                fprintf(stdout, "%s %ld %ld\n", img_fname, pxsize.width * ratio, pxsize.height * ratio);
            }
        }
    }//for(i
    exit(0);
}


int getSuffix(char *fname){
    char *suf;
    int sufnum;
    int i;

    suf = strrchr(fname, '.');
    if(suf == NULL) return notype;
    sufnum = sizeof(g_suffix) / sizeof(g_suffix[0]); //拡張子の種類の数
    for(i=0; i<sufnum; i++){
        if(strcmp(suf, g_suffix[i]) == 0)
            return i;
    }
    return -1;
}


size_type jpgGetSize(char *fname){
    size_type pxsize;
    unsigned char *p, *pmax;
    unsigned char *src = NULL;
    int flagsize = 0;

    //file open
    {
        FILE *fin;
        struct stat info;
        unsigned int filesize = 0;

        if((fin=fopen(fname,"rb")) == NULL) errret;
        if(stat(fname, &info) != 0) errret;
        filesize = info.st_size;
        if(filesize > MAXFILEBYTE){
            filesize = MAXFILEBYTE;
        }
        if((src=malloc(filesize)) == NULL) errret;
        if(fread(src, 1, filesize, fin) != filesize){
            free(src); errret;
        }
        fclose(fin);
        pmax = src + filesize;
    }

    /*
     * JPG 妥当性チェック
     *  最初の 2byte が SOI(start of image) という marker
     *  と一致すれば jpeg 画像。
     */
    if(*src==0xff && *(src+1)==0xd8){
        p = src + 2; //2 byte
    }else{
        errret;
    }


    /* SOFn セグメントを探す */
    for(;;){
        while(*p!=0xff){
            p++; if(p>=pmax) errret;
        }

        //SOS(start of scan),EOI(end of image) セグメントを見つけたのでデータ検索は終了
        if((*(p+1)==0xda || *(p+1)==0xd9) && flagsize==1){
            free(src);
            return pxsize;
        }

        //printf("ff %02x\n", *(p+1));

        // SOFn セグメントかどうかチェック
        if(flagsize==0){
            int i;
            static unsigned char sof[] = { 0xc0, 0xc1, 0xc2, 0xc3, 0xc5, 0xc6, 0xc7, 0xc9, 0xca, 0xcb, 0xcd, 0xce, 0xcf, };
            for(i=0; i<sizeof(sof)/sizeof(sof[0]); i++){
                if(*(p+1) == sof[i]){
                    /*
                     * SOFn セグメント発見
                     * 画像のピクセルサイズを得る
                     * 
                     * SOFmarker segmentlength bit/sample height width ..
                     * ff cN     NN NN         NN         NN NN  NN NN ..
                     */
                    pxsize.height = *(p+5)<<8 | *(p+6);
                    pxsize.width = *(p+7)<<8 | *(p+8);
                    //printf("width=%ld height=%ld\n", pxsize.width, pxsize.height);
                    flagsize = 1;
                    break;
                }
            }//for
        }//if

        //APPn セグメントかどうかチェック
        if(*(p+1)>=0xe0 && *(p+1)<0xef){
            if(strncmp(p+4, "Exif", strlen("Exif"))==0){
                getExifInfo(p+2+2+6); //APP1marker(2byte) + APP1size(2) + APP1str(6)
            }
        }

        //SOFn セグメントではなかった
        {
            unsigned short next = (*(p+2))<<8 | *(p+3);
            p += 2;
            p += next; //marker(2byte) + segment length を足す
            if(p>=pmax) errret;
        }
    }

    return pxsize;
}


void getExifInfo(unsigned char *exiftop){
    unsigned char *p = exiftop;
    int bigEndian = 0;
    unsigned short dentry = 0;
    unsigned char *exifsubifd = NULL;
    int i;

    //endian チェック
    if(*p=='M' && *(p+1)=='M'){
        bigEndian = 1; //Motorola(BigEndian)
    }else if(*p=='I' && *(p+1)=='I'){
        bigEndian = 0; //Intel(LittleEndian)
    }

    //get offset to first IFD(ImageFileDirectory)
    if(bigEndian){
        p += big2ltll(p+4);
    }else{
        p += *(unsigned long*)(p+4);
    }
    
    //get directory entry num
    IFDtop:
    if(bigEndian)
        dentry = big2ltls(p);
    else
        dentry = *(unsigned short *)p;
    p += 2;

    //get directory entry
    for(i=0; i<dentry; i++){
        unsigned short tag;
        if(bigEndian){
            tag = big2ltls(p);
        }else{
            tag = *(unsigned short *)p;
        }

        //必要な Exif データを取得する
        if(tag == 0x010f){
            //Make
            getExifData(g_exifmaker, p, exiftop, bigEndian);
        }else if(tag == 0x0110){
            //Model
            getExifData(g_exifmodel, p, exiftop, bigEndian);
        }else if(tag == 0x9003 || tag == 0x0132){
            //DateTimeOriginal / DateTime
            getExifData(g_exifdate, p, exiftop, bigEndian);
        }else if(tag == 0x8769){
            //ExifOffset のタグであれば SubIFDへのオフセットを記録
            unsigned long offset;
            if(bigEndian){
                offset = big2ltll(p+8);
            }else{
                offset = *(unsigned long*)(p+8);
            }
            exifsubifd = exiftop + offset;
        }

        p += 12;
    }//for

    if(exifsubifd != NULL){
        p = exifsubifd;
        exifsubifd = NULL;
        goto IFDtop;
    }
}

void getExifData(unsigned char *dst, unsigned char *p, unsigned char *exiftop, int bigEndian){
    unsigned char *src;
    unsigned long numunit;

    if(bigEndian){
        numunit =  big2ltll(p+4);
        if(numunit <= 4){
            src = p+8;
        }else{
            src = exiftop + big2ltll(p+8);
        }
    }else{
        numunit = *(unsigned long*)(p+4);
        if(numunit <= 4){
            src = p+8;
        }else{
            src = exiftop + *(unsigned long*)(p+8);
        }
    }
    strncpy(dst, src, MAXLINECHAR);

    //remove trailing spaces
    {
        unsigned char *c;
        for(c=dst+strlen(dst)-1; c>dst; c--){
            if(*c==' ' || *c=='\t' || *c=='\n' || *c=='\r'){
                *c = '\0';
            }else{
                break;
            }
        }
    }
    //printf("Exif: '%s' '%s'\n", src, dst);
}

    


size_type gifGetSize(char *fname){
    /*
     * License:
     *   The Graphics Interchange Format(c) is the Copyright property of
     *   CompuServe Incorporated. GIF(sm) is a Service Mark property of
     *   CompuServe Incorporated.
     */
    size_type pxsize;
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
    fclose(fin);

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

    pxsize.width = w;
    pxsize.height = h;
    return pxsize;
}


size_type pngGetSize(char *fname){
    size_type pxsize;
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
    fclose(fin);

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

    pxsize.width = w;
    pxsize.height = h;
    return pxsize;
}


size_type xbmGetSize(char *fname){
    size_type pxsize;
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
        if(fgets(p, MAXLINECHAR-1, fin)==NULL){
            fclose(fin);
            errret;
        }

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
    fclose(fin);

    pxsize.width = w;
    pxsize.height = h;
    return pxsize;
}
/* exchange big endian to little endina; 16 bit short */
unsigned short big2ltls(void *v){
    int i;
    unsigned char *c = v;
    unsigned char *buf = calloc(sizeof(unsigned short), sizeof(unsigned char));
    for(i=0; i<sizeof(unsigned short); i++){
        *(buf+i) = *(c + sizeof(unsigned short) - 1 - i);
    }
    return *(unsigned short*)buf;
}

/* exchange big endian to little endina; 32bit long */
unsigned long big2ltll(void *v){
    int i;
    unsigned char *c = v;
    unsigned char *buf = calloc(sizeof(unsigned short), sizeof(unsigned char));
    for(i=0; i<sizeof(unsigned long); i++){
        *(buf+i) = *(c + sizeof(unsigned long) - 1 - i);
    }
    return *(unsigned long*)buf;
}

char * getBasename(char * str){
    char * p = str;
    char * j;
    //printf("debug %p %d\n", p, strlen(str));
    for (j=str; j<str+strlen(str) - 1; j++){
        if (*j =='\\' || *j =='/'){
            p = j+1;
            //printf("debug %s (%d)\n", p, j-argv[0]);
        }
    }
    return p;
}
//end of file
